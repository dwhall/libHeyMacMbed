/* Copyright 2020 Dean Hall.  See LICENSE for details. */
/*
Reference:

https://github.com/dwhall/HeyMac/blob/master/docs/HeyMacProtocol.md
*/

#ifndef HEYMACFRAME_H_
#define HEYMACFRAME_H_

#include <stdint.h>

#include "HeyMac.h"


/* Protocol ID (PID) Field */
typedef enum
{
    // TODO: HM_PIDFLD_TDMA_V0 = 0xE0,
    HM_PIDFLD_CSMA_V0 = 0xE4,
    // TODO: HM_PIDFLD_FLOOD = 0xE8,
} hm_pidfld_t8;


class HeyMacFrame
{
public:
    HeyMacFrame();      // Init with a newly alloc'd frame
    HeyMacFrame(uint8_t *buf, uint8_t sz);  // Init with a received frame
    ~HeyMacFrame();         // dealloc the frame

    uint8_t *get_ref(void);
    uint8_t get_sz(void);

    // When building a frame, perform calls in this order:
    void set_protocol(hm_pidfld_t8 pidfld);
    void set_net_id(uint16_t net_id);
    void set_dst_addr(uint16_t dst_addr);
    void set_dst_addr(uint64_t dst_addr);
    void set_src_addr(uint16_t src_addr);
    void set_src_addr(uint64_t src_addr);
    bool set_payld(uint8_t *payld, uint8_t sz);
    void set_payld_sz(uint8_t sz);
    // TODO: set_mic()
    bool set_multihop(uint8_t hops, uint16_t tx_addr);
    bool set_multihop(uint8_t hops, uint64_t tx_addr);

    // After receiving a frame, call parse() on it
    bool parse(void);
    // TODO: update_multihop()

private:
    HeyMacFrame *_frm;
    uint8_t *_buf;
    uint8_t _payld_offset;
    uint8_t _payld_sz;
    uint8_t _mic_sz;
    uint8_t _rxd_sz;

    uint8_t _get_ie_sz(uint8_t ie_offset);
    uint8_t _get_mic_sz(uint8_t ie_offset);
    bool _validate_fields(void); // used by parse()
};

#endif /* HEYMACFRAME_H_ */
