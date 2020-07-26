/* Copyright 2020 Dean Hall.  See LICENSE for details. */

#include <stdint.h>

#include "mbed.h"

#include "SX127xRadio.h"
#include "utl.h"


uint16_t const SPI_CMD_SZ = 4;
uint8_t const DIO_VAL_MAX = 4;

/** Radio settings constant information lookup table */
SX127xRadio::stngs_info_t const SX127xRadio::_stngs_info_lut[FLD_CNT] =
{   /*                                  lora    reg                     reg     bit     bit     val                 val                 reset               */
    /* Field                            mode    start                   cnt     start   cnt     min                 max                 val                 */
    /* FLD_RDO_LF_MODE          */  {   false,  REG_RDO_OPMODE,         1,      3,      1,      0,                  1,                  1                   },
    /* FLD_RDO_LORA_MODE        */  {   false,  REG_RDO_OPMODE,         1,      7,      1,      0,                  1,                  0                   },
    /* FLD_RDO_FREQ_HZ          */  {   false,  REG_RDO_FREQ_HZ,        3,      0,      8,      FREQ_MIN,           FREQ_MAX,           0x6C                },
    /* FLD_RDO_OUT_PWR          */  {   false,  REG_RDO_PA_CFG,         1,      0,      4,      0,                  15,                 0x0F                },
    /* FLD_RDO_MAX_PWR          */  {   false,  REG_RDO_PA_CFG,         1,      4,      3,      0,                  7,                  0x04                },
    /* FLD_RDO_PA_BOOST         */  {   false,  REG_RDO_PA_CFG,         1,      7,      1,      0,                  1,                  0                   },
    /* FLD_RDO_LNA_BOOST_HF     */  {   false,  REG_RDO_LNA,            1,      0,      2,      0,                  3,                  0                   },
    /* FLD_RDO_LNA_GAIN         */  {   false,  REG_RDO_LNA,            1,      5,      3,      1,                  6,                  0x01                },
    /* FLD_RDO_DIO0             */  {   false,  REG_RDO_DIOMAP1,        1,      6,      2,      0,                  2,                  0                   },
    /* FLD_RDO_DIO1             */  {   false,  REG_RDO_DIOMAP1,        1,      4,      2,      0,                  2,                  0                   },
    /* FLD_RDO_DIO2             */  {   false,  REG_RDO_DIOMAP1,        1,      2,      2,      0,                  2,                  0                   },
    /* FLD_RDO_DIO3             */  {   false,  REG_RDO_DIOMAP1,        1,      0,      2,      0,                  2,                  0                   },
    /* FLD_RDO_DIO4             */  {   false,  REG_RDO_DIOMAP2,        1,      6,      2,      0,                  2,                  0                   },
    /* FLD_RDO_DIO5             */  {   false,  REG_RDO_DIOMAP2,        1,      4,      2,      0,                  2,                  0                   },

    /* FLD_LORA_IMPLCT_HDR_MODE */  {   true,   REG_LORA_CFG1,          1,      0,      1,      0,                  1,                  0                   },
    /* FLD_LORA_CR              */  {   true,   REG_LORA_CFG1,          1,      1,      3,      STNG_LORA_CR_MIN,   STNG_LORA_CR_MAX,   STNG_LORA_CR_4TO5   },
    /* FLD_LORA_BW              */  {   true,   REG_LORA_CFG1,          1,      4,      4,      STNG_LORA_BW_MIN,   STNG_LORA_BW_MAX,   STNG_LORA_BW_125K   },
    /* FLD_LORA_CRC_EN          */  {   true,   REG_LORA_CFG2,          1,      2,      1,      0,                  1,                  0                   },
    /* FLD_LORA_SF              */  {   true,   REG_LORA_CFG2,          1,      4,      4,      STNG_LORA_SF_MIN,   STNG_LORA_SF_MAX,   STNG_LORA_SF_128_CPS},
    /* FLD_LORA_RX_TMOUT        */  {   true,   REG_LORA_CFG2,          2,      0,      2,      0,                  (1<<10)-1,          0x00                },
    /* _FLD_LORA_RX_TMOUT_2     */  {   0,      REG_LORA_RX_SYM_TMOUT,  0,      0,      0,      0,                  0,                  0x64                },
    /* FLD_LORA_PREAMBLE_LEN    */  {   true,   REG_LORA_PREAMBLE_LEN,  2,      0,      16,     0,                  (1<<16)-1,          0x00                },
    /* _FLD_LORA_PREAMBLE_LEN_2 */  {   0,      REG_LORA_PREAMBLE_LEN_LSB, 0,   0,      0,      0,                  0,                  0x08                },
    /* FLD_LORA_AGC_ON          */  {   true,   REG_LORA_CFG3,          1,      2,      1,      0,                  1,                  0                   },
    /* FLD_LORA_SYNC_WORD       */  {   true,   REG_LORA_SYNC_WORD,     1,      0,      8,      0,                  (1<<8)-1,           0x12                },
};
//FIXME: MBED_STATIC_ASSERT(CNT_OF(SX127xRadio::_stngs_info_lut) == SX127xRadio::FLD_CNT, "_stngs_info_lut[]: incorrect table entry count");

SX127xRadio::SX127xRadio
    (
    SPI *spi,
    PinName reset,
    PinName dio0,
    PinName dio1,
    PinName dio2,
    PinName dio3,
    PinName dio4,
    PinName dio5
    )
    :
    _reset(reset, PIN_INPUT, PullUp, 1),
    _dio0(dio0),
    _dio1(dio1),
    _dio2(dio2),
    _dio3(dio3),
    _dio4(dio4),
    _dio5(dio5)
{
    _spi = spi;
    _spi->format(8, 0);
    _spi->set_default_write_value(0);
    _spi->frequency(HM_LAYER_SPI_FREQ_HZ);

    _sig_dio_clbk = nullptr;
    _rng_raw = 0;
}

SX127xRadio::~SX127xRadio()
{
}


void SX127xRadio::init_radio(Callback<void(sig_dio_t const)> sig_dio_clbk)
{
    /* Store the DIO callback */
    _sig_dio_clbk = sig_dio_clbk;

    /* Issue pin reset to the radio (regs to init values) */
    _reset.output();
    _reset = 0;
    ThisThread::sleep_for(1ms);
    _reset = 1;
    ThisThread::sleep_for(6ms);

    _reset_rdo_stngs();

    _validate_chip();

    /*
    Put the radio in LoRa mode so DIO5 outputs ModeReady (instead of ClkOut).
    This is needed so the state machine receives the ModeReady event.
    */
    write_op_mode(SX127xRadio::OP_MODE_SLEEP);
    set(SX127xRadio::FLD_RDO_LORA_MODE, 1);
    write_sleep_stngs();
    write_op_mode(SX127xRadio::OP_MODE_STBY);
    _rdo_stngs_applied[SX127xRadio::FLD_RDO_LORA_MODE] = 1;
}

SX127xRadio::op_mode_t SX127xRadio::read_op_mode(void)
{
    uint8_t reg_val;

    _read(REG_RDO_OPMODE, &reg_val);

    return ((op_mode_t)(reg_val & 0x07));
}


void SX127xRadio::set(fld_t const fld, uint32_t const val)
{
    /* Bounds check the arguments */
    if((fld >= FLD_CNT) || !((_stngs_info_lut[fld].val_min <= val) && (val <= _stngs_info_lut[fld].val_max)))
    {
        MBED_ASSERT(false);
        return;
    }

    switch(fld)
    {
        /* For typical settings, store the value */
        default:
            /* If we hit this assert, the _stngs_info_lut[].reg_cnt is wrong for this fld */
            MBED_ASSERT(_stngs_info_lut[fld].reg_cnt == 1);

            /* Put the bit-limited value in the stngs holding array.  We'll bit shift when writing to the register. */
            _rdo_stngs[fld] = val & BIT_FLD(0, _stngs_info_lut[fld].bit_cnt);
            break;

        /* Below this are special cases for settings that span a register boundary */

        case FLD_RDO_FREQ_HZ:
            MBED_ASSERT(_stngs_info_lut[fld].reg_cnt == 3);

            /*
            Store in a special variable so we can apply Errata 2.3
            rejection offset easily in write_stngs()
            */
            _rdo_stngs_freq = val;
            break;

        case FLD_LORA_RX_TMOUT:
            MBED_ASSERT(_stngs_info_lut[fld].reg_cnt == 2);
            _rdo_stngs[fld + 0] = (val >> 8) & 0xFF;    /* MSB */
            _rdo_stngs[fld + 1] = (val >> 0) & 0xFF;    /* LSB */
            break;

        case FLD_LORA_PREAMBLE_LEN:
            MBED_ASSERT(_stngs_info_lut[fld].reg_cnt == 2);
            _rdo_stngs[fld + 0] = (val >> 8) & 0xFF;    /* MSB */
            _rdo_stngs[fld + 1] = (val >> 0) & 0xFF;    /* LSB */
            break;
    }
}


bool SX127xRadio::stngs_require_sleep(void)
{
    /* The LoRa mode requires being in sleep mode */
    return (_rdo_stngs[FLD_RDO_LORA_MODE] != _rdo_stngs_applied[FLD_RDO_LORA_MODE]);
}

void SX127xRadio::updt_rng(void)
{
    uint8_t reg = 0;

    /* Use the least-significant bit of RSSI as noise */
    _read(SX127xRadio::REG_LORA_RSSI_WB, &reg);
    _rng_raw = (_rng_raw << 1) | (reg & 1);
}

/** The caller MUST leave data[0] available for the SPI command; FIFO data should occupy data[1:] */
void SX127xRadio::write_fifo(uint8_t * const data, uint16_t const sz)
{
    uint8_t const SPI_WRITE_CMD = 0x80;

    MBED_ASSERT(sz > 0);

    data[0] = REG_RDO_FIFO | SPI_WRITE_CMD;

    _spi->write((char*)data, sz, nullptr, 0);
}

void SX127xRadio::write_fifo_ptr(uint8_t const offset)
{
    uint8_t regs[3];

    regs[0] = regs[1] = regs[2] = offset;
    _write(REG_LORA_FIFO_ADDR_PTR, regs, sizeof(regs));
}

void SX127xRadio::write_lora_irq_mask(irq_bitf_t const disable_these, irq_bitf_t const enable_these)
{
    uint8_t reg;

    /* rmw the register. Setting a bit (to 1) masks/disables the IRQ.  Clearing enables. */
    _read(REG_LORA_IRQ_MASK, &reg);
    reg |= disable_these;
    reg &= ~enable_these;
    _write(REG_LORA_IRQ_MASK, &reg);
}

void SX127xRadio::write_lora_irq_flags(irq_bitf_t const clear_these)
{
    /* Write a 1 to ack/clear the interrupt flag */
    _write(REG_LORA_IRQ_FLAGS, (uint8_t *)&clear_these);
}

void SX127xRadio::write_op_mode(op_mode_t const op_mode)
{
    uint8_t reg;

    /* rmw the setting into the register */
    _read(REG_RDO_OPMODE, &reg);
    reg &= ~0x7;
    reg |= (0x7 & op_mode);
    _write(REG_RDO_OPMODE, &reg);
}

void SX127xRadio::write_stngs(bool for_rx)
{
    bool auto_if_on;
    uint8_t bitf;
    uint8_t fld;
    uint32_t freq;
    uint8_t reg;
    uint8_t reg_freq[3];
    uint8_t reg_if_freq2;

    freq = _rdo_stngs_freq;
    auto_if_on = false; /* errata-recommended value after reset */
    reg_if_freq2 = 0x20; /* reset value */

    /* Apply errata 2.3 for LoRa mode receiving */
    if (for_rx && _rdo_stngs[FLD_RDO_LORA_MODE])
    {
        /* Parameters for improved RX packet rejection (Errata 2.3) */
        static struct
        {
            uint32_t rejection_offset_hz;
            uint8_t if_freq2;
        }
        const freq_info_lut[] =
        {
            /* STNG_LORA_BW_7K8   */ { 7810, 0x48},
            /* STNG_LORA_BW_10K4  */ {10420, 0x44},
            /* STNG_LORA_BW_15K6  */ {15620, 0x44},
            /* STNG_LORA_BW_20K8  */ {20830, 0x44},
            /* STNG_LORA_BW_31K25 */ {31250, 0x44},
            /* STNG_LORA_BW_41K7  */ {41670, 0x44},
            /* STNG_LORA_BW_62K5  */ {    0, 0x40},
            /* STNG_LORA_BW_125K  */ {    0, 0x40},
            /* STNG_LORA_BW_250K  */ {    0, 0x40},
        };
//FIXME:    MBED_STATIC_ASSERT(CNT_OF(freq_info_lut) == STNG_LORA_BW_CNT - 1);

        if (_rdo_stngs[FLD_LORA_BW] >= STNG_LORA_BW_500K)
        {
            auto_if_on = true;
        }
        else
        {
            auto_if_on = false;

            /* Adjust the intermediate freq per errata */
            reg_if_freq2 = freq_info_lut[_rdo_stngs[FLD_LORA_BW]].if_freq2;

            /* Add the offset to the carrier freq and fill the stngs holding array with that */
            freq += freq_info_lut[_rdo_stngs[FLD_LORA_BW]].rejection_offset_hz;
        }
    }

    /* If LoRa mode or LoRa BW has changed, apply the errata values to their regs */
    if ((_rdo_stngs[FLD_RDO_LORA_MODE] != _rdo_stngs_applied[FLD_RDO_LORA_MODE])
     || (_rdo_stngs[FLD_LORA_BW]       != _rdo_stngs_applied[FLD_LORA_BW]      ))
    {
        _write(REG_LORA_IF_FREQ_2, &reg_if_freq2);

        _read(REG_LORA_DTCT_OPTMZ, &reg);
        reg &= ~0x80;
        reg |= (auto_if_on) ? 0x80 : 0;
        _write(REG_LORA_DTCT_OPTMZ, &reg);
    }

    /* Write outstanding carrier freq to the regs */
    if (freq != _rdo_stngs_freq_applied)
    {
        reg_freq[0] = (freq >> 16) & 0xFF;   /* MSB */
        reg_freq[1] = (freq >>  8) & 0xFF;   /* MID */
        reg_freq[2] = (freq >>  0) & 0xFF;   /* LSB */
        _write(REG_RDO_FREQ_HZ, reg_freq, sizeof(reg_freq));
        _rdo_stngs_freq_applied = freq;
    }

    /* rmw typical settings if they've changed */
    for (fld = 0; fld < FLD_CNT; fld++)
    {
        if (_rdo_stngs[fld] != _rdo_stngs_applied[fld])
        {
            _read(_stngs_info_lut[fld].reg_start, &reg);
            bitf = BIT_FLD(_stngs_info_lut[fld].bit_start, _stngs_info_lut[fld].bit_cnt);
            reg &= ~bitf;
            reg |= (bitf & (_rdo_stngs[fld] << _stngs_info_lut[fld].bit_start));
            _write(_stngs_info_lut[fld].reg_start, &reg);

            /* Record what we have written */
            _rdo_stngs_applied[fld] = _rdo_stngs[fld];
        }
    }
}

void SX127xRadio::write_sleep_stngs(void)
{
    uint8_t reg;

    /* If the setting hasn't changed, skip it */
    if (_rdo_stngs[FLD_RDO_LORA_MODE] != _rdo_stngs_applied[FLD_RDO_LORA_MODE])
    {
        /* Read/mod/write the LoRa Mode bit in the OpMode reg */
        _read(REG_RDO_OPMODE, &reg);
        if (_rdo_stngs[FLD_RDO_LORA_MODE])
        {
            reg |= 0x80;
        }
        else
        {
            reg &= ~0x80;
        }
        _write(REG_RDO_OPMODE, &reg);

        /* Record what we have written */
        _rdo_stngs_applied[FLD_RDO_LORA_MODE] = _rdo_stngs[FLD_RDO_LORA_MODE];
    }
}

void SX127xRadio::_dio0_isr(void)
{
    static sig_dio_t const dio0_to_sig_lut[] =
    {
        SX127xRadio::SIG_DIO_RX_DONE,
        SX127xRadio::SIG_DIO_TX_DONE,
        SX127xRadio::SIG_DIO_CAD_DONE
    };

    MBED_ASSERT(_rdo_stngs_applied[FLD_RDO_DIO0] < DIO_VAL_MAX);

    if(_sig_dio_clbk)
    {
        _sig_dio_clbk(dio0_to_sig_lut[_rdo_stngs_applied[FLD_RDO_DIO0]]);
    }
}

void SX127xRadio::_dio1_isr(void)
{
    static sig_dio_t const dio1_to_sig_lut[] =
    {
        SX127xRadio::SIG_DIO_RX_TMOUT,
        SX127xRadio::SIG_DIO_FHSS_CHG_CHNL,
        SX127xRadio::SIG_DIO_CAD_DETECTED
    };

    MBED_ASSERT(_rdo_stngs_applied[FLD_RDO_DIO1] < DIO_VAL_MAX);

    if(_sig_dio_clbk)
    {
        _sig_dio_clbk(dio1_to_sig_lut[_rdo_stngs_applied[FLD_RDO_DIO1]]);
    }
}

void SX127xRadio::_dio2_isr(void)
{
    static sig_dio_t const dio2_to_sig_lut[] =
    {
        SX127xRadio::SIG_DIO_FHSS_CHG_CHNL,
        SX127xRadio::SIG_DIO_FHSS_CHG_CHNL,
        SX127xRadio::SIG_DIO_FHSS_CHG_CHNL
    };

    MBED_ASSERT(_rdo_stngs_applied[FLD_RDO_DIO2] < DIO_VAL_MAX);

    if(_sig_dio_clbk)
    {
        _sig_dio_clbk(dio2_to_sig_lut[_rdo_stngs_applied[FLD_RDO_DIO2]]);
    }
}

void SX127xRadio::_dio3_isr(void)
{
    static sig_dio_t const dio3_to_sig_lut[] =
    {
        SX127xRadio::SIG_DIO_CAD_DONE,
        SX127xRadio::SIG_DIO_VALID_HDR,
        SX127xRadio::SIG_DIO_PAYLD_CRC_ERR
    };

    MBED_ASSERT(_rdo_stngs_applied[FLD_RDO_DIO3] < DIO_VAL_MAX);

    if(_sig_dio_clbk)
    {
        _sig_dio_clbk(dio3_to_sig_lut[_rdo_stngs_applied[FLD_RDO_DIO3]]);
    }
}

void SX127xRadio::_dio4_isr(void)
{
    static sig_dio_t const dio4_to_sig_lut[] =
    {
        SX127xRadio::SIG_DIO_CAD_DETECTED,
        SX127xRadio::SIG_DIO_PLL_LOCK,
        SX127xRadio::SIG_DIO_PLL_LOCK
    };

    MBED_ASSERT(_rdo_stngs_applied[FLD_RDO_DIO4] < DIO_VAL_MAX);

    if(_sig_dio_clbk)
    {
        _sig_dio_clbk(dio4_to_sig_lut[_rdo_stngs_applied[FLD_RDO_DIO4]]);
    }
}

void SX127xRadio::_dio5_isr(void)
{
    static sig_dio_t const dio5_to_sig_lut[] =
    {
        SX127xRadio::SIG_DIO_MODE_RDY,
        SX127xRadio::SIG_DIO_CLK_OUT,
        SX127xRadio::SIG_DIO_CLK_OUT
    };

    MBED_ASSERT(_rdo_stngs_applied[FLD_RDO_DIO5] < DIO_VAL_MAX);

    if(_sig_dio_clbk)
    {
        _sig_dio_clbk(dio5_to_sig_lut[_rdo_stngs_applied[FLD_RDO_DIO5]]);
    }
}


void SX127xRadio::_read(reg_addr_t const addr, uint8_t * data, uint16_t const sz)
{
    uint8_t const SPI_READ_MASK = (uint8_t)~0x80;
    uint8_t rxbuf[SPI_CMD_SZ];
    uint8_t txbuf[SPI_CMD_SZ];

    MBED_ASSERT(sz < SPI_CMD_SZ);

    txbuf[0] = addr & SPI_READ_MASK;
    memset(&txbuf[1], 0, sz);

    _spi->write((char*)txbuf, 1 + sz, (char*)rxbuf, 1 + sz);

    memcpy(data, &rxbuf[1], sz);
}


void SX127xRadio::_reset_rdo_stngs(void)
{
    for (uint8_t fld = 0; fld < FLD_CNT; fld++)
    {
        _rdo_stngs[fld]         = _stngs_info_lut[fld].reset_val;
        _rdo_stngs_applied[fld] = _stngs_info_lut[fld].reset_val;
    }
}


void SX127xRadio::_validate_chip(void)
{
    static uint8_t const SEMTECH_SX127X_SI_REV_ID = 0x12;
    uint8_t reg;

    _read(REG_RDO_CHIP_VRSN, &reg);

    /* If we hit this this assert, the SPI bus or the chip is not working */
    MBED_ASSERT(SEMTECH_SX127X_SI_REV_ID == reg);
}


void SX127xRadio::_write(reg_addr_t const addr, uint8_t * const data, uint16_t const sz)
{
    uint8_t const SPI_WRITE_CMD = 0x80;
    uint8_t txbuf[SPI_CMD_SZ];

    /* If we hit this assert, increase SPI_CMD_SZ */
    MBED_ASSERT(sz <= (SPI_CMD_SZ + 1));

    txbuf[0] = addr | SPI_WRITE_CMD;
    memcpy(&txbuf[1], data, sz);

    _spi->write((char*)txbuf, 1 + sz, nullptr, 0);
}
