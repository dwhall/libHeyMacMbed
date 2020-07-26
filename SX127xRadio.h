/* Copyright 2020 Dean Hall.  See LICENSE for details. */

#ifndef PHYSX127XSPI_H_
#define PHYSX127XSPI_H_

/**
 * SX127xRadio
 *
 * A low-level device driver for the Semtec SX127X family
 * of radio modems written in C++ with the mbed framework.
 * Only the modem features needed by the HeyMac project are implemented.
 *
 * DOES NOT PROVIDE:
 * - FHSS in LoRa mode.
 * - Anything related to LoRaMAC or LoRaWAN.
 * - Anything related to the FSK half of the radio modem.
 *
 * REFERENCES:
 * - www.semtech.com/products/wireless-rf/lora-transceivers/sx1276
 */

#include <stdint.h>

#include "mbed.h"
#include "Callback.h"


class SX127xRadio
{
    public:
        SX127xRadio
            (
            SPI *spi,
            PinName reset,
            PinName dio0,
            PinName dio1,
            PinName dio2,
            PinName dio3,
            PinName dio4 = NC,
            PinName dio5 = NC
            );
        ~SX127xRadio();

        /** Frequency range of the SX127X family [Hz] */
        static uint32_t const FREQ_MIN =  137000000;
        static uint32_t const FREQ_MAX = 1020000000;

        /**
         * The LoRa radio hardware has DIO0:5 pin interrupts
         * which have different meanings depending on DIO settings.
         * This SX127xRadio library handles the DIO pin interrupt
         * and uses current DIO settings to translate the interrupt
         * to one of these signals.  This library calls the provided
         * callback with one of these signals.
         */
        typedef enum : uint8_t
        {
            SIG_DIO_MODE_RDY = 0,
            SIG_DIO_CAD_DETECTED,
            SIG_DIO_CAD_DONE,
            SIG_DIO_FHSS_CHG_CHNL,
            SIG_DIO_RX_TMOUT,
            SIG_DIO_RX_DONE,
            SIG_DIO_CLK_OUT,
            SIG_DIO_PLL_LOCK,
            SIG_DIO_VALID_HDR,
            SIG_DIO_TX_DONE,
            SIG_DIO_PAYLD_CRC_ERR,

            SIG_DIO_CNT
        } sig_dio_t;

        /** Callback type for SX127xRadio to notify its caller of a DIO Signal */
        typedef void (* sig_dio_clbk_t)(sig_dio_t const);

        /** LoRa mode IRQ bit flags */
        typedef enum : uint8_t
        {
            LORA_IRQ_NONE            = 0x00,
            LORA_IRQ_CAD_DETECTED    = 0x01,
            LORA_IRQ_FHSS_DHGD_CHNL  = 0x02,
            LORA_IRQ_CAD_DONE        = 0x04,
            LORA_IRQ_TX_DONE         = 0x08,
            LORA_IRQ_VALID_HEADER    = 0x10,
            LORA_IRQ_PAYLD_CRC_ERR   = 0x20,
            LORA_IRQ_RX_DONE         = 0x40,
            LORA_IRQ_RX_TIMEOUT      = 0x80,
            LORA_IRQ_ALL             = 0xFF
        } irq_bitf_t;

        /** Field indices used to select radio settings */
        typedef enum
        {
            FLD_RDO_LF_MODE = 0,
            FLD_RDO_LORA_MODE,
            FLD_RDO_FREQ_HZ,
            FLD_RDO_OUT_PWR,
            FLD_RDO_MAX_PWR,
            FLD_RDO_PA_BOOST,
            FLD_RDO_LNA_BOOST_HF,
            FLD_RDO_LNA_GAIN,
            FLD_RDO_DIO0,
            FLD_RDO_DIO1,
            FLD_RDO_DIO2,
            FLD_RDO_DIO3,
            FLD_RDO_DIO4,
            FLD_RDO_DIO5,

            FLD_LORA_IMPLCT_HDR_MODE,
            FLD_LORA_CR,
            FLD_LORA_BW,
            FLD_LORA_CRC_EN,
            FLD_LORA_SF,
            FLD_LORA_RX_TMOUT,
            _FLD_LORA_RX_TMOUT_2,       /* DO NOT USE as an argument to set(), use FLD_LORA_RX_TMOUT */
            FLD_LORA_PREAMBLE_LEN,
            _FLD_LORA_PREAMBLE_LEN_2,   /* DO NOT USE as an argument to set(), use FLD_LORA_PREAMBLE_LEN */
            FLD_LORA_AGC_ON,
            FLD_LORA_SYNC_WORD,

            FLD_CNT
        } fld_t;

        /**
         * Radio operation modes.
         * Not used in radio settings because the opmode
         * must be set before other radio/lora settings
         * may be applied
         */
        typedef enum : uint8_t
        {
            OP_MODE_SLEEP = 0,
            OP_MODE_STBY,
            OP_MODE_FSTX,
            OP_MODE_TX,
            OP_MODE_FSRX,
            OP_MODE_RXCONT,
            OP_MODE_RXONCE,
            OP_MODE_CAD,

            OP_MODE_CNT
        } op_mode_t;

        /**
         * lora Coding Rate (CR)
         * The ratio of extra bits produced by the radio's FEC algorithm.
         * Greater value creates greater coding gain which increases the chances
         * of a successful decoding by the receiver, but results in a lower datarate
         * and costs extra transmission time which consumes power.
         */
        typedef enum
        {
            STNG_LORA_CR_4TO5 = 1,
            STNG_LORA_CR_4TO6 = 2,
            STNG_LORA_CR_4TO7 = 3,
            STNG_LORA_CR_4TO8 = 4,

            STNG_LORA_CR_MIN = 1,
            STNG_LORA_CR_MAX = 4
        } lora_cr_t;

        /**
         * lora Bandwidth (BW)
         * The width of radio frequencies around the carrier center frequency
         * used to convey the modem signal.  Greater values allow a higher
         * data rate, but lesser values achieve greater signal range.
         */
        typedef enum
        {
            STNG_LORA_BW_7K8 = 0,
            STNG_LORA_BW_10K4 = 1,
            STNG_LORA_BW_15K6 = 2,
            STNG_LORA_BW_20K8 = 3,
            STNG_LORA_BW_31K25 = 4,
            STNG_LORA_BW_41K7 = 5,
            STNG_LORA_BW_62K5 = 6,
            STNG_LORA_BW_125K = 7,
            STNG_LORA_BW_250K = 8,
            STNG_LORA_BW_500K = 9,

            STNG_LORA_BW_CNT,

            STNG_LORA_BW_MIN = 0,
            STNG_LORA_BW_MAX = 9
        } lora_bw_t;

        /**
         * lora Spread Factor (SF)
         * The number of chips-per-second used to represent a bit.
         * Greater values increase the SNR (better chance of reception),
         * but result in a lower datarate and costs extra transmission
         * time which consumes power.
         */
        typedef enum
        {
            STNG_LORA_SF_64_CPS = 6,
            STNG_LORA_SF_128_CPS = 7,
            STNG_LORA_SF_256_CPS = 8,
            STNG_LORA_SF_512_CPS = 9,
            STNG_LORA_SF_1024_CPS = 10,
            STNG_LORA_SF_2048_CPS = 11,
            STNG_LORA_SF_4096_CPS = 12,

            STNG_LORA_SF_MIN = 6,
            STNG_LORA_SF_MAX = 12
        } lora_sf_t;

        /**
         * Initializes the SX127X radio.
         * Performs pin reset to put all regs in known state.
         * Stores callback function to call upon a DIOx pin signal.
         */
        void init_radio(Callback<void(sig_dio_t)> sig_dio_clbk);

        /**
         * Reads and returns the current op_mode from the radio
         */
        op_mode_t read_op_mode(void);

        /**
         * Sets a field in the logical LoRa settings to the given value.
         * The settings are NOT written to the regs in this procedure.
         */
        void set(fld_t const fld, uint32_t const val);

        /** Returns true if there are any outstanding settings that require Sleep op_mode */
        bool stngs_require_sleep(void);

        /**
         * Writes the given data[1:] into the FIFO reg.
         * data MUST have its first byte open to fill with the spi command.
         * sz should include the entire length of data.
         */
        void write_fifo(uint8_t * const data, uint16_t const sz);

        /** Writes the given offset to the FIFO pointer and RX base pointer */
        void write_fifo_ptr(uint8_t const offset=0x00);

        /**
         * Reads, modifies, writes the IRQ flags register (0x11)
         * Disables the IRQ for each flag set in disable_these.
         * Enables the IRQ for each flag set in enable_these.
         * All other IRQ flags are left as-is.
         */
        void write_lora_irq_mask(irq_bitf_t const disable_these = LORA_IRQ_ALL, irq_bitf_t const enable_these = LORA_IRQ_NONE);

        void write_lora_irq_flags(irq_bitf_t const clear_these = LORA_IRQ_ALL);

        /** Writes the given op_mode to the register immediately */
        void write_op_mode(op_mode_t const op_mode);

        /** Writes all outstanding settings with the radio in Standby mode */
        void write_stngs(bool for_rx);

        /** Writes the few setting(s) that require the radio to be in Sleep mode */
        void write_sleep_stngs(void);


    private:
        /** SX127X radio register addresses */
        typedef enum
        {
            /* General radio registers */
            REG_RDO_FIFO = 0x00,
            REG_RDO_OPMODE = 0x01,
            REG_RDO_FREQ_HZ = 0x06, /* MSB first. [3] */
            REG_RDO_FREQ_HZ_MID = 0x07,
            REG_RDO_FREQ_HZ_LSB = 0x08,
            REG_RDO_PA_CFG = 0x09,
            REG_RDO_LNA = 0x0C,

            REG_RDO_DIOMAP1 = 0x40,
            REG_RDO_DIOMAP2 = 0x41,
            REG_RDO_CHIP_VRSN = 0x42,

            /* LoRa specific registers */
            REG_LORA_FIFO_ADDR_PTR = 0x0D,
            REG_LORA_FIFO_TX_BASE = 0x0E,
            REG_LORA_FIFO_RX_BASE = 0x0F,
            REG_LORA_FIFO_CURR_ADDR = 0x10,
            REG_LORA_IRQ_MASK = 0x11,
            REG_LORA_IRQ_FLAGS = 0x12,
            REG_LORA_RX_CNT = 0x13,
            REG_LORA_RX_HDR_CNT = 0x14, /* MSB first. [2] */
            REG_LORA_RX_HDR_CNT_LSB = 0x15,
            REG_LORA_RX_PKT_CNT = 0x16, /* MSB first. [2] */
            REG_LORA_RX_PKT_CNT_LSB = 0x17,
            REG_LORA_MODEM_STAT = 0x18,
            REG_LORA_PKT_SNR = 0x19,
            REG_LORA_PKT_RSSI = 0x1A,
            REG_LORA_CFG1 = 0x1D,
            REG_LORA_CFG2 = 0x1E,
            REG_LORA_RX_SYM_TMOUT = 0x1F,
            REG_LORA_PREAMBLE_LEN = 0x20,
            REG_LORA_PREAMBLE_LEN_LSB = 0x21,
            REG_LORA_CFG3 = 0x26,
            REG_LORA_IF_FREQ_2 = 0x2F,
            REG_LORA_DTCT_OPTMZ = 0x31,
            REG_LORA_SYNC_WORD = 0x39
        } reg_addr_t;

        /** LoRa IRQ flags used in REG_LORA_IRQ_MASK, REG_LORA_IRQ_FLAGS */
        typedef enum
        {
            IRQ_BITI_CAD_DETECTED = 0,
            IRQ_BITI_FHSS_CHG_CHNL,
            IRQ_BITI_CAD_DONE,
            IRQ_BITI_TX_DONE,
            IRQ_BITI_VALID_HDR,
            IRQ_BITI_CRC_ERR,
            IRQ_BITI_RX_DONE,
            IRQ_BITI_RX_TMOUT,

            IRQ_BITF_CAD_DETECTED = 1 << IRQ_BITI_CAD_DETECTED,
            IRQ_BITF_FHSS_CHG_CHNL = 1 << IRQ_BITI_FHSS_CHG_CHNL,
            IRQ_BITF_CAD_DONE = 1 << IRQ_BITI_CAD_DONE,
            IRQ_BITF_TX_DONE = 1 << IRQ_BITI_TX_DONE,
            IRQ_BITF_VALID_HDR = 1 << IRQ_BITI_VALID_HDR,
            IRQ_BITF_CRC_ERR = 1 << IRQ_BITI_CRC_ERR,
            IRQ_BITF_RX_DONE = 1 << IRQ_BITI_RX_DONE,
            IRQ_BITF_RX_TMOUT = 1 << IRQ_BITI_RX_TMOUT,
        };

        typedef struct
        {
            bool        lora_mode;  /* setting applies to reg when in LoRa mode     */
            reg_addr_t  reg_start;
            uint8_t     reg_cnt;
            uint8_t     bit_start;
            uint8_t     bit_cnt;
            uint32_t    val_min;
            uint32_t    val_max;
            uint32_t    reset_val;  /* hardware reset value of the field */
        } stngs_info_t;

        static stngs_info_t const _stngs_info_lut[FLD_CNT];

        SPI *_spi;
        DigitalInOut _reset;

        /** DIO pins are output from the radio and interrupt this MCU */
        InterruptIn _dio0;
        InterruptIn _dio1;
        InterruptIn _dio2;
        InterruptIn _dio3;
        InterruptIn _dio4;
        InterruptIn _dio5;

        /** The procedure called by a DIO pin ISR */
        Callback<void(sig_dio_t const)> _sig_dio_clbk;

        /**
         * The settings holding array.
         * Logical radio settings are kept here until they are written to the registers
         */
        uint8_t _rdo_stngs[FLD_CNT];
        uint32_t _rdo_stngs_freq; /* special case */

        /**
         * Logical radio settings that have been written to the registers.
         * This is used so we can determine which fields in _rdo_stngs are outstanding.
         */
        uint8_t _rdo_stngs_applied[FLD_CNT];
        uint32_t _rdo_stngs_freq_applied; /* special case */

        /**
         * Applies RX Spurious Reception countermeasures to settings.
         * Should be applied after all settings are set,
         * but before they are written to the regs
         * */
        void _apply_rx_errata_2_3(void);

        /**
         * DIOx radio pin Interrupt Service Routines
         * Triggered by the rising edge of the configured pin.
         * Uses the current DIO mapping to translate the pin signal
         * into a sig_dio_t value and gives that value to the _sig_dio_clbk
         */
        void _dio0_isr(void);
        void _dio1_isr(void);
        void _dio2_isr(void);
        void _dio3_isr(void);
        void _dio4_isr(void);
        void _dio5_isr(void);

        /** Sets radio settings to match hardware reset values */
        void _reset_rdo_stngs(void);

        /** Reads content over SPI from one or more radio registers */
        void _read(reg_addr_t const addr, uint8_t * data, uint16_t const sz = 1);

        /** Asserts if the chip fails to read the proper value from the silicon rev ID register */
        void _validate_chip(void);

        /** Writes content over SPI to one or more radio registers */
        void _write(reg_addr_t const addr, uint8_t * const data, uint16_t const sz = 1);
};

#endif /* PHYSX127XSPI_H_ */
