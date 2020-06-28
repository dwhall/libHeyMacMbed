/* Copyright 2020 Dean Hall.  See LICENSE for details. */

#include <string.h>

#include "MemoryPool.h"

#include "HeyMac.h"
#include "HeyMacFrame.h"


/* Only the first three fields have a fixed position */
enum
{
    FRM_IDX_PID = 0,
    FRM_IDX_FCTL = 1,
    FRM_IDX_NETID = 2,
};

enum
{
    FCTL_BIT_P = 1 << 0,    /* Pending frame follows */
    FCTL_BIT_M = 1 << 1,    /* Multihop (Hops and TxAddr fields present) */
    FCTL_BIT_S = 1 << 2,    /* SrcAddr present */
    FCTL_BIT_I = 1 << 3,    /* IEs present */
    FCTL_BIT_D = 1 << 4,    /* DstAddr present */
    FCTL_BIT_N = 1 << 5,    /* NetId present */
    FCTL_BIT_L = 1 << 6,    /* Long Addressing */
    FCTL_BIT_X = 1 << 7,    /* Extended frame */
};

/**
 * We reserve one byte at the start of the buffer for the SPI command
 * and start the radio frame after that.
 */
static uint8_t const s_frame_start = 1;

/** A static pool of buffers for radio frames */
static rtos::MemoryPool<uint8_t, HM_FRAME_SZ * HM_FRMBUF_POOL_CNT> s_frmbuf_pool;


HeyMacFrame::HeyMacFrame()
    :
    /* Call the other constructor with memory from the pool */
    HeyMacFrame(s_frmbuf_pool.alloc(), 0)
{
}

HeyMacFrame::HeyMacFrame(uint8_t * buf, uint8_t sz)
{
    MBED_ASSERT(buf != nullptr);

    _buf = buf;
    _frm = &buf[s_frame_start];
    if(sz == 0)
        {
        /* Initialize header to an invalid value */
        _frm[FRM_IDX_PID] = HM_PIDFLD_INVALID;
        _frm[FRM_IDX_FCTL] = 0;
        }
    _payld_offset = 0;
    _payld_sz = 0;
    _mic_sz = 0;
    _rxd_sz = sz;
}


HeyMacFrame::~HeyMacFrame()
{
    s_frmbuf_pool.free(_buf);
}


uint8_t *HeyMacFrame::get_buf(void)
{
    return _buf;
}

uint16_t HeyMacFrame::get_buf_sz(void)
{
    return s_frame_start + get_frm_sz();
}

uint8_t *HeyMacFrame::get_frm(void)
{
    return &_frm[s_frame_start];
}

uint16_t HeyMacFrame::get_frm_sz(void)
{
    uint16_t sz;
    uint8_t mhop_sz = 0;

    if( _rxd_sz )
    {
        sz = _rxd_sz;
    }
    else
    {
        if (_frm[FRM_IDX_FCTL] & FCTL_BIT_M)
        {
            mhop_sz = (_frm[FRM_IDX_FCTL] & FCTL_BIT_L) ? 1 + 8 : 1 + 2;
        }
        sz = _payld_offset + _payld_sz + _mic_sz + mhop_sz;
    }
    return sz;
}

void HeyMacFrame::set_protocol(hm_pidfld_t8 pidfld)
{
    _frm[FRM_IDX_PID] = pidfld;
}

void HeyMacFrame::set_net_id(uint16_t net_id)
{
    /* MSB first (big endian) */
    _frm[FRM_IDX_NETID] = net_id >> 8;
    _frm[FRM_IDX_NETID + 1] = net_id & 0xFF;

    _frm[FRM_IDX_FCTL] |= FCTL_BIT_N;
}

void HeyMacFrame::set_dst_addr(uint16_t dst_addr)
{
    /* Calc the offset of where to put the dst_addr */
    uint8_t offset = FRM_IDX_NETID;
    if (_frm[FRM_IDX_FCTL] & FCTL_BIT_N)
        offset += 2;

    /* MSB first (big endian) */
    _frm[offset + 0] = (dst_addr >> 8) & 0xFF;
    _frm[offset + 1] = (dst_addr >> 0) & 0xFF;

    _frm[FRM_IDX_FCTL] |= FCTL_BIT_D;
    _frm[FRM_IDX_FCTL] &= ~FCTL_BIT_L;
}

void HeyMacFrame::set_dst_addr(uint64_t dst_addr)
{
    /* Calc the offset of where to put the dst_addr */
    uint8_t offset = FRM_IDX_NETID;
    if (_frm[FRM_IDX_FCTL] & FCTL_BIT_N)
    {
        offset += 2;
    }

    /* MSB first (big endian) */
    _frm[offset + 0] = (dst_addr >> 56) & 0xFF;
    _frm[offset + 1] = (dst_addr >> 48) & 0xFF;
    _frm[offset + 2] = (dst_addr >> 40) & 0xFF;
    _frm[offset + 3] = (dst_addr >> 32) & 0xFF;
    _frm[offset + 4] = (dst_addr >> 24) & 0xFF;
    _frm[offset + 5] = (dst_addr >> 16) & 0xFF;
    _frm[offset + 6] = (dst_addr >>  8) & 0xFF;
    _frm[offset + 7] = (dst_addr >>  0) & 0xFF;

    _frm[FRM_IDX_FCTL] |= FCTL_BIT_D;
    _frm[FRM_IDX_FCTL] |= FCTL_BIT_L;
}

void HeyMacFrame::set_src_addr(uint16_t src_addr)
{
//    assert((_frm[FRM_IDX_FCTL] & FCTL_BIT_L) == 0);

    /* Calc the offset of where to put the src_addr */
    uint8_t offset = FRM_IDX_NETID;
    if (_frm[FRM_IDX_FCTL] & FCTL_BIT_N)
    {
        offset += 2;
    }
    if (_frm[FRM_IDX_FCTL] & FCTL_BIT_D)
    {
        // TODO: shouldn't support Long addr here since this method sets the Short addr
        offset += (_frm[FRM_IDX_FCTL] & FCTL_BIT_L) ? 8 : 2;
    }
    if (_frm[FRM_IDX_FCTL] & FCTL_BIT_I)
    {
        offset += _get_ie_sz(offset);
    }

    /* MSB first (big endian) */
    _frm[offset + 0] = (src_addr >> 8) & 0xFF;
    _frm[offset + 1] = (src_addr >> 0) & 0xFF;

    _frm[FRM_IDX_FCTL] |= FCTL_BIT_S;
    _frm[FRM_IDX_FCTL] &= ~FCTL_BIT_L;
}

void HeyMacFrame::set_src_addr(uint64_t src_addr)
{
//    assert((_frm[FRM_IDX_FCTL] & FCTL_BIT_L) != 0);

    /* Calc the offset of where to put the src_addr */
    uint8_t offset = FRM_IDX_NETID;
    if (_frm[FRM_IDX_FCTL] & FCTL_BIT_N)
    {
        offset += 2;
    }
    if (_frm[FRM_IDX_FCTL] & FCTL_BIT_D)
    {
        offset += (_frm[FRM_IDX_FCTL] & FCTL_BIT_L) ? 8 : 2;
    }
    if (_frm[FRM_IDX_FCTL] & FCTL_BIT_I)
    {
        offset += _get_ie_sz(offset);
    }

    /* MSB first (big endian) */
    _frm[offset + 0] = (src_addr >> 56) & 0xFF;
    _frm[offset + 1] = (src_addr >> 48) & 0xFF;
    _frm[offset + 2] = (src_addr >> 40) & 0xFF;
    _frm[offset + 3] = (src_addr >> 32) & 0xFF;
    _frm[offset + 4] = (src_addr >> 24) & 0xFF;
    _frm[offset + 5] = (src_addr >> 16) & 0xFF;
    _frm[offset + 6] = (src_addr >>  8) & 0xFF;
    _frm[offset + 7] = (src_addr >>  0) & 0xFF;

    _frm[FRM_IDX_FCTL] |= FCTL_BIT_S;
    _frm[FRM_IDX_FCTL] |= FCTL_BIT_L;
}

bool HeyMacFrame::set_payld(uint8_t * payld, uint8_t sz)
{
    bool success = false;
    uint8_t offset = FRM_IDX_NETID;

    if (_frm[FRM_IDX_FCTL] & FCTL_BIT_N)
    {
        offset += 2;
    }
    if (_frm[FRM_IDX_FCTL] & FCTL_BIT_D)
    {
        offset += (_frm[FRM_IDX_FCTL] & FCTL_BIT_L) ? 8 : 2;
    }
    if (_frm[FRM_IDX_FCTL] & FCTL_BIT_I)
    {
        offset += _get_ie_sz(offset);
    }
    if (_frm[FRM_IDX_FCTL] & FCTL_BIT_S)
    {
        offset += (_frm[FRM_IDX_FCTL] & FCTL_BIT_L) ? 8 : 2;
    }

    if ((offset + sz) < HM_FRAME_SZ)
    {
        memcpy(&_frm[offset], payld, sz);
        _payld_offset = offset;
        _payld_sz = sz;
        success = true;
    }

    return success;
}

void HeyMacFrame::set_payld_sz(uint8_t sz)
{
    _payld_sz = sz;
}

// TODO: set_mic()
// MIC requires setting an IE field
// NOTE: MIC is distinct from LoRa's CRC

/*
Returns true if there is room in the buffer for multihop fields
and the frame's multihop fields are set properly.
Returns false if there is no room in the buffer for new multihop data
or the frame is already multihop
or long addressing is in use (this overload is for short addrs)
*/
bool HeyMacFrame::set_mhop(uint8_t hops, uint16_t tx_addr)
{
    uint8_t mhop_sz;
    uint8_t offset;
    bool success = false;

    /* If the frame is not yet multihop and long addressing is not in use */
    if (((_frm[FRM_IDX_FCTL] & FCTL_BIT_M) == 0)
     &&((_frm[FRM_IDX_FCTL] & FCTL_BIT_L) == 0))
    {
        /* Check for available space */
        mhop_sz = (_frm[FRM_IDX_FCTL] & FCTL_BIT_L) ? 1 + 8 : 1 + 2;
        if ((_payld_offset + _payld_sz + _mic_sz + mhop_sz) <= HM_FRAME_SZ)
        {
            /* Append Hops, TxAddr fields */
            offset = _payld_offset + _payld_sz + _mic_sz;
            _frm[offset + 0] = hops;

            /* MSB first (big endian) */
            _frm[offset + 1] = (tx_addr >> 8) & 0xFF;
            _frm[offset + 2] = (tx_addr >> 0) & 0xFF;

            _frm[FRM_IDX_FCTL] |= FCTL_BIT_M;
            success = true;
        }
    }
    return success;
}

/*
Returns true if there is room in the buffer for multihop fields
and the frame's multihop fields are set properly.
Returns false if there is no room in the buffer for new multihop data
or the frame is already multihop
or short addressing is in use (this overload is for long addrs)
*/
bool HeyMacFrame::set_mhop(uint8_t hops, uint64_t tx_addr)
{
    uint8_t mhop_sz;
    uint8_t offset;
    bool success = false;

    // FIXME: FCTL.L bit might not be set in rare case of no src/dst addr is set (multihop to root, anonymous)

    /* If the frame is not yet multihop and long addressing is in use */
    if (((_frm[FRM_IDX_FCTL] & FCTL_BIT_M) == 0)
     &&((_frm[FRM_IDX_FCTL] & FCTL_BIT_L) != 0))
    {
        /* Check for available space */
        mhop_sz = (_frm[FRM_IDX_FCTL] & FCTL_BIT_L) ? 1 + 8 : 1 + 2;
        if ((_payld_offset + _payld_sz + _mic_sz + mhop_sz) <= HM_FRAME_SZ)
        {
            /* Append Hops, TxAddr fields */
            offset = _payld_offset + _payld_sz + _mic_sz;
            _frm[offset + 0] = hops;

            /* MSB first (big endian) */
            _frm[offset + 1] = (tx_addr >> 56) & 0xFF;
            _frm[offset + 2] = (tx_addr >> 48) & 0xFF;
            _frm[offset + 3] = (tx_addr >> 40) & 0xFF;
            _frm[offset + 4] = (tx_addr >> 32) & 0xFF;
            _frm[offset + 5] = (tx_addr >> 24) & 0xFF;
            _frm[offset + 6] = (tx_addr >> 16) & 0xFF;
            _frm[offset + 7] = (tx_addr >>  8) & 0xFF;
            _frm[offset + 8] = (tx_addr >>  0) & 0xFF;

            _frm[FRM_IDX_FCTL] |= FCTL_BIT_M;
            success = true;
        }
    }
    return success;
}

/*
Returns true/false if the data in the buffer is a valid/invalid HeyMac frame.

This method should be called immediately after instantiating a frame
with received data.
*/
bool HeyMacFrame::parse(void)
{
    bool success = false;
    uint8_t offset = FRM_IDX_NETID;

    if (_frm[FRM_IDX_FCTL] & FCTL_BIT_N)
        offset += 2;
    if (_frm[FRM_IDX_FCTL] & FCTL_BIT_D)
    {
        offset += (_frm[FRM_IDX_FCTL] & FCTL_BIT_L) ? 8 : 2;
    }
    if (_frm[FRM_IDX_FCTL] & FCTL_BIT_I)
    {
        offset += _get_ie_sz(offset);
        _mic_sz = _get_mic_sz(offset);
    }
    if (_frm[FRM_IDX_FCTL] & FCTL_BIT_S)
    {
        offset += (_frm[FRM_IDX_FCTL] & FCTL_BIT_L) ? 8 : 2;
    }

    if (offset < _rxd_sz)
    {
        uint8_t mhop_sz = 0;
        int16_t payld_sz;

        if (_frm[FRM_IDX_FCTL] & FCTL_BIT_M)
        {
            mhop_sz = (_frm[FRM_IDX_FCTL] & FCTL_BIT_L) ? 1 + 8 : 1 + 2;
        }

        _payld_offset = offset;
        payld_sz = (_rxd_sz - mhop_sz - _mic_sz) - _payld_offset;
        if (payld_sz > 0)
        {
            _payld_sz = payld_sz;
            success = _validate_fields();
        }
    }
    return success;
}


// PRIVATE

/* Returns the size of all of the IEs */
uint8_t HeyMacFrame::_get_ie_sz(uint8_t ie_offset)
{
    uint8_t ie_sz = 0;

    // TODO: implement

    return ie_sz;
}

/* Returns the size of the MIC as determined from an IE */
uint8_t HeyMacFrame::_get_mic_sz(uint8_t ie_offset)
{
    uint8_t mic_sz = 0;

    // TODO: implement

    return mic_sz;
}

/*
Returns true if no fields are invalid.
Assumes the _frm contents' size has been validated by caller
*/
bool HeyMacFrame::_validate_fields(void)
{
    bool success = true;

    /* Only CSMA_V0 is supported at this time */
    success = success && (_frm[FRM_IDX_PID] == HM_PIDFLD_CSMA_V0);

    /*
    Any eXtended frame is valid because remaining contents are undefined
    now check non-eXtended frames
    */
    if ((_frm[FRM_IDX_FCTL] & FCTL_BIT_X) == 0)
    {
        // TODO: validate NetId
        // TODO: validate DstAddr (HONR)
        // TODO: validate IEs
        // TODO: validate SrcAddr (HONR)
        // TODO: validate Payld has MAC or APv6 header
    }
    return success;
}
