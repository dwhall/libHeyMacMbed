/* Copyright 2020 Dean Hall.  See LICENSE for details. */

/**
 * HeyMacLayer
 *
 * A thread that manages the DLL/MAC networking layer.
 * The core operations are implemented as a state machine.
 * The radio operation can be described very easily:
 * The radio is normally listening on one frequency.
 * If a valid header is heard and a frame is received,
 * the frame is processed and the radio returns to listening.
 * If a frame appears in the tx_queue, the radio exits listening,
 * transmits the frame(s) and then returns to listening.
 * After transmitting or receiving, any outstanding settings are applied
 * to the radio.
 *
 * ===============  ==================  ==========================================
 * State            Event               Action
 * ===============  ==================  ==========================================
 * Initing          EVT_THRD_INIT       Initializes this thread and the radio object.
 *                                      Transitions to Lstning.
 * Setting          *                   Applies outstanding settings with the radio
 *                                      in standby mode and possibly sleep mode.
 *                                      If the tx_queue is non-empty, transitions to Txing;
 *                                      otherwise transitions to Lstning.
 * Lstning          EVT_TX_RDY          Sets the radio to standby mode and
 *                                      transitions to the Setting state.
 *                  EVT_THRD_PRDC       Updates RX channel meta-data,
 *                                      then remains in Lstning.
 *                  EVT_DIO_VALID_HDR   Transitions to Rxing so frame reception
 *                                      is not disturbed by other events.
 * Rxing            EVT_DIO_RX_DONE     Processes the received frame,
 *                                      then transitions to Setting.
 * Txing            *                   Transmits the frame from the front of the tx_queue,
 *                                      then transitions to Setting.
 * ===============  ==================  ==========================================
 */

#include <stdint.h>
#include <deque>

#include "mbed.h"

#include "HeyMacIdent.h"
#include "HeyMacLayer.h"
#include "HeyMacFrame.h"
#include "HeyMacCmd.h"
#include "SX127xRadio.h"

using namespace std;


static int const THRD_STACK_SZ = 6 * 1024;
static uint32_t const THRD_PRDC_MS = 100;

#define SM_HANDLED() retval = SM_RET_HANDLED
#define SM_TRAN(next_st_clbk) this->_st_handler = next_st_clbk; retval = SM_RET_TRAN

/** HeyMacLayer event flags */
enum
{
    EVT_NONE                = 0,

    EVT_THRD_INIT           = 1 << 0,   /** Thread init */
    EVT_THRD_TERM           = 1 << 1,   /** Thread terminate */
    EVT_THRD_PRDC           = 1 << 2,   /** Thread periodic */

    EVT_SM_ENTER            = 1 << 3,   /** State machine entry */
    EVT_SM_NEXT             = 1 << 4,   /** Reminder or iterator patterns */

    EVT_DIO_MODE_RDY        = 1 << 5,   /** LoRa radio DIO ModeReady */
    EVT_DIO_CAD_DETECTED    = 1 << 6,   /** LoRa radio DIO CadDetected */
    EVT_DIO_CAD_DONE        = 1 << 7,   /** LoRa radio DIO CadDone */
    EVT_DIO_FHSS_CHG_CHNL   = 1 << 8,   /** LoRa radio DIO FhssChangeChannel */
    EVT_DIO_RX_TMOUT        = 1 << 9,   /** LoRa radio DIO RxTimeout */
    EVT_DIO_RX_DONE         = 1 << 10,  /** LoRa radio DIO RxDone */
    EVT_DIO_CLK_OUT         = 1 << 11,  /** LoRa radio DIO ClockOut */
    EVT_DIO_PLL_LOCK        = 1 << 12,  /** LoRa radio DIO PllLock */
    EVT_DIO_VALID_HDR       = 1 << 13,  /** LoRa radio DIO ValidHeader */
    EVT_DIO_TX_DONE         = 1 << 14,  /** LoRa radio DIO TxDone */
    EVT_DIO_PAYLD_CRC_ERR   = 1 << 15,  /** LoRa radio DIO PayloadCrcError */

    /**
     * When a frame is added to the transmit queue,
     * this signal is dispatched to cause the Listening state
     * to stop and transfer through Setting to Transmitting.
     * If the state machine is in the Receiving or Transmitting
     * states when this signal is dispatched, this signal is ignored,
     * and the designed progression from Receiving and Transmitting
     * through Setting will detect a non-empty tansmit queue
     * and transition to the Transmitting state then.
     */
    EVT_TX_RDY              = 1 << 16,

    /** Hardware User button event */
    EVT_BTN                 = 1 << 17,

    EVT_ALL = (EVT_BTN << 1) - 1
};


HeyMacLayer::HeyMacLayer(char const *cred_fn)
    :
    _tx_queue(deque<tx_data_t>(0))
{
    /* Thread stuff */
    _thread = new Thread(osPriorityNormal, THRD_STACK_SZ, nullptr, "HMLayer");
    _period_ms = THRD_PRDC_MS;
    _ticker = nullptr;

    /* App stuff */
    _hm_ident = new HeyMacIdent(cred_fn);
    _spi = new SPI
        (
        HM_PIN_LORA_MOSI,
        HM_PIN_LORA_MISO,
        HM_PIN_LORA_SCK,
        HM_PIN_LORA_NSS,
        use_gpio_ssel
        );
    _radio = new SX127xRadio
        (
        _spi,
        HM_PIN_LORA_RESET,
        HM_PIN_LORA_DIO0,
        HM_PIN_LORA_DIO1,
        HM_PIN_LORA_DIO2,
        HM_PIN_LORA_DIO3,
        HM_PIN_LORA_DIO4,
        HM_PIN_LORA_DIO5
        );
}

HeyMacLayer::~HeyMacLayer()
{
}


void HeyMacLayer::enq_tx_frame(HeyMacFrame *frm, uint32_t tx_time)
{
    tx_data_t tx_data;

    tx_data.frm = frm;
    tx_data.at_time_ms = tx_time;
    // TODO: Wrap _tx_queue access with smphr?
    _tx_queue.push_back(tx_data);

    if (0/*ASAP*/ == tx_time) // TODO if (tx_time < now + 100ms prdc)
    {
        _thread->flags_set(EVT_TX_RDY);
    }
}

void HeyMacLayer::evt_btn(void)
{
    _thread->flags_set(EVT_BTN);
}


void HeyMacLayer::thread_join(void)
{
    _thread->join();
}


void HeyMacLayer::thread_start(void)
{
    _thread->start(callback(this, &HeyMacLayer::_main));
    _thread->flags_set(EVT_THRD_INIT);
}


void HeyMacLayer::_main(void)
{
    uint32_t evt_flags;
    uint32_t evt_flag_enter = EVT_NONE;
    sm_ret_t retval = SM_RET_IGNORED;

    /* Set the initial state */
    _st_handler = &HeyMacLayer::_st_initing;

    /* Run the event loop */
    for (;;)
    {
        /*
        If the previous state handler caused a state transition,
        process it now; otherwise Thread sleep until any events arrive.
        */
        if (SM_RET_TRAN != retval)
            {
            evt_flags = ThisThread::flags_wait_any(EVT_ALL, true);
            }

        /* Process the Thread init event (not meant for state machines) */
        if (evt_flags & EVT_THRD_INIT)
        {
            if (_period_ms > 0)
            {
                _ticker = new LowPowerTicker();
                _ticker->attach_us(callback(this, &HeyMacLayer::_ticker_clbk), _period_ms * 1000);
            }
        }

        /* Call the state handler with the events */
        retval = (this->*_st_handler)(evt_flags | evt_flag_enter);

        /* Events have been processed */
        evt_flags = 0;

        /* Set the ENTER event flag if we transitioned states */
        evt_flag_enter = (SM_RET_TRAN == retval) ? EVT_SM_ENTER : 0;
    }
}


void HeyMacLayer::_ticker_clbk(void)
{
    /* Post the periodic event flag */
    _thread->flags_set(EVT_THRD_PRDC);
}


HeyMacLayer::sm_ret_t HeyMacLayer::_st_initing(uint32_t const evt_flags)
{
    sm_ret_t retval = SM_RET_IGNORED;

    if (evt_flags & EVT_THRD_INIT)
    {
        /* Parse in this thread with the large stack space */
        _hm_ident->parse_cred_file();

        _radio->init_radio(callback(this, &HeyMacLayer::_evt_dio));

        /* Settings that differ from hwreset */
        _radio->set(SX127xRadio::FLD_RDO_LORA_MODE, 1);
        _radio->set(SX127xRadio::FLD_RDO_FREQ_HZ, HM_LAYER_RF_FREQ_HZ);
        _radio->set(SX127xRadio::FLD_RDO_MAX_PWR, 7);
        _radio->set(SX127xRadio::FLD_RDO_PA_BOOST, 1);

        _radio->set(SX127xRadio::FLD_LORA_BW, SX127xRadio::STNG_LORA_BW_250K);
        _radio->set(SX127xRadio::FLD_LORA_SF, SX127xRadio::STNG_LORA_SF_128_CPS);
        _radio->set(SX127xRadio::FLD_LORA_CR, SX127xRadio::STNG_LORA_CR_4TO6);
        _radio->set(SX127xRadio::FLD_LORA_CRC_EN, 1);
        _radio->set(SX127xRadio::FLD_LORA_SYNC_WORD, 0x48);

        SM_TRAN(&HeyMacLayer::_st_setting);
    }

    return retval;
}


HeyMacLayer::sm_ret_t HeyMacLayer::_st_setting(uint32_t const evt_flags)
{
    sm_ret_t retval = SM_RET_IGNORED;

    if (evt_flags & EVT_SM_ENTER)
    {
        if (_radio->stngs_require_sleep())
        {
            _radio->write_op_mode(SX127xRadio::OP_MODE_SLEEP);
            /* Radio will give us the DIO ModeReady signal when it reaches sleep mode */
            SM_HANDLED();
        }
        else
        {
            _thread->flags_set(EVT_SM_NEXT);
            SM_HANDLED();
        }
    }

    /*
    Radio has reached sleep mode.
    Write outstanding settings that require sleep mode
    and command to standby mode
    */
    else if (evt_flags & EVT_DIO_MODE_RDY)
    {
        _radio->write_sleep_stngs();
        _thread->flags_set(EVT_SM_NEXT);
        SM_HANDLED();
    }

    else if (evt_flags & EVT_SM_NEXT)
    {
        _radio->write_op_mode(SX127xRadio::OP_MODE_STBY);
        // TODO: await mode ready?

        /* If there are frames to be transmitted */
        if (!_tx_queue.empty())
        {
            /* Set DIO to allow TX_DONE interrupt */
            _radio->set(SX127xRadio::FLD_RDO_DIO0, 1);
            _radio->write_stngs(false);

            SM_TRAN(&HeyMacLayer::_st_txing);
        }
        else
        {
            /* Set DIO to allow RxDone, RxTimeout, ValidHeader interrupts */
            _radio->set(SX127xRadio::FLD_RDO_DIO0, 0);
            _radio->set(SX127xRadio::FLD_RDO_DIO1, 0);
            _radio->set(SX127xRadio::FLD_RDO_DIO3, 1);
            _radio->write_stngs(true);

            SM_TRAN(&HeyMacLayer::_st_lstning);
        }
    }

    return retval;
}

HeyMacLayer::sm_ret_t HeyMacLayer::_st_lstning(uint32_t const evt_flags)
{
    sm_ret_t retval = SM_RET_IGNORED;

    if (evt_flags & EVT_SM_ENTER)
    {
        _radio->write_lora_irq_mask(
            /* disable_these */ SX127xRadio::LORA_IRQ_ALL,
            /* enable_these */ (SX127xRadio::irq_bitf_t)
                            ( SX127xRadio::LORA_IRQ_RX_DONE
                            | SX127xRadio::LORA_IRQ_PAYLD_CRC_ERR
                            | SX127xRadio::LORA_IRQ_VALID_HEADER));
        _radio->write_lora_irq_flags((SX127xRadio::irq_bitf_t)
                            ( SX127xRadio::LORA_IRQ_RX_DONE
                            | SX127xRadio::LORA_IRQ_PAYLD_CRC_ERR
                            | SX127xRadio::LORA_IRQ_VALID_HEADER));
        _radio->write_fifo_ptr(0x00);
        _radio->write_op_mode(SX127xRadio::OP_MODE_RXCONT);
        SM_HANDLED();
    }

    else if (evt_flags & EVT_THRD_PRDC)
    {
        // TODO: update status, rx meta-data
        _radio->updt_rng();
        SM_HANDLED();
    }

    else if (evt_flags & EVT_TX_RDY)
    {
        _radio->write_op_mode(SX127xRadio::OP_MODE_STBY);
        SM_TRAN(&HeyMacLayer::_st_setting);
    }

    // TEMPORARY: when the button is pressed, emit a HeyMac Txt Command with they HMIdentity's callsign as the payload
    else if (evt_flags & EVT_BTN)
    {
        char tac_id[HM_IDENT_TAC_ID_SZ];
        HeyMacFrame *frm;
        HeyMacCmd cmd;

        _hm_ident->copy_tac_id_into(tac_id);
        frm = new HeyMacFrame();
        frm->set_protocol(HM_PIDFLD_CSMA_V0);
        frm->set_src_addr(_hm_ident->get_long_addr());
        cmd.cmd_init(frm);
        cmd.cmd_txt(tac_id, strlen(tac_id));
        enq_tx_frame(frm);
        SM_HANDLED();
    }

    else if (evt_flags & EVT_DIO_VALID_HDR)
    {
        // TODO: Store timestamp for the incoming frame

        SM_TRAN(&HeyMacLayer::_st_rxing);
    }

    return retval;
}

HeyMacLayer::sm_ret_t HeyMacLayer::_st_rxing(uint32_t const evt_flags)
{
    sm_ret_t retval = SM_RET_IGNORED;

    if (evt_flags & EVT_SM_ENTER)
    {
    }

    else if (evt_flags & EVT_DIO_RX_DONE)
    {
        // TODO: get frame and meta-data from radio

        SM_TRAN(&HeyMacLayer::_st_setting);
    }

    return retval;
}

HeyMacLayer::sm_ret_t HeyMacLayer::_st_txing(uint32_t const evt_flags)
{
    sm_ret_t retval = SM_RET_IGNORED;

    if (evt_flags & EVT_SM_ENTER)
    {
        _radio->write_lora_irq_mask(
            /* disable_these */ SX127xRadio::LORA_IRQ_ALL,
            /* enable_these */  SX127xRadio::LORA_IRQ_TX_DONE);
        _radio->write_lora_irq_flags(SX127xRadio::LORA_IRQ_TX_DONE);
        _radio->write_fifo_ptr(0x00);

        // TODO: Wrap _tx_queue access with smphr?
        tx_data_t tx_data = _tx_queue.front();
        _tx_queue.pop_front();
        HeyMacFrame *frm = tx_data.frm;
        _radio->write_fifo(frm->get_buf(), frm->get_buf_sz());

        _radio->write_op_mode(SX127xRadio::OP_MODE_TX);
        SM_HANDLED();
    }

    else if (evt_flags & EVT_DIO_TX_DONE)
    {
        SM_TRAN(&HeyMacLayer::_st_setting);
    }

    return retval;
}


/* Handler for the SX127xRadio callback for DIO signals */
void HeyMacLayer::_evt_dio(SX127xRadio::sig_dio_t const sig_dio)
{
    static uint32_t const sig_to_evt_lut[SX127xRadio::SIG_DIO_CNT] =
    {
        /* SIG_DIO_MODE_RDY */      EVT_DIO_MODE_RDY,
        /* SIG_DIO_CAD_DETECTED */  EVT_DIO_CAD_DETECTED,
        /* SIG_DIO_CAD_DONE */      EVT_DIO_CAD_DONE,
        /* SIG_DIO_FHSS_CHG_CHNL */ EVT_DIO_FHSS_CHG_CHNL,
        /* SIG_DIO_RX_TMOUT */      EVT_DIO_RX_TMOUT,
        /* SIG_DIO_RX_DONE */       EVT_DIO_RX_DONE,
        /* SIG_DIO_CLK_OUT */       EVT_DIO_CLK_OUT,
        /* SIG_DIO_PLL_LOCK */      EVT_DIO_PLL_LOCK,
        /* SIG_DIO_VALID_HDR */     EVT_DIO_VALID_HDR,
        /* SIG_DIO_TX_DONE */       EVT_DIO_TX_DONE,
        /* SIG_DIO_PAYLD_CRC_ERR */ EVT_DIO_PAYLD_CRC_ERR
    };

    MBED_ASSERT(sig_dio < SX127xRadio::SIG_DIO_CNT);

    /*
    Convert a DIO signal to an application event flag
    and post the flag to this thread
    */
    _thread->flags_set(sig_to_evt_lut[sig_dio]);
}


void HeyMacLayer::_tx_bcn(void)
{
    HeyMacFrame *frm;
    HeyMacCmd cmd;

    frm = new HeyMacFrame();
    frm->set_protocol(HM_PIDFLD_CSMA_V0);
    frm->set_src_addr(_hm_ident->get_long_addr());
    cmd.cmd_init(frm);
    uint16_t const caps = 0xCA; // TODO: impl:
    uint16_t const status = 0x00; // status = red flags = (1==fault)
    cmd.cmd_cbcn(caps, status);   //TODO: , nets, ngbrs);
    enq_tx_frame(frm);
}

