/* Copyright 2020 Dean Hall.  See LICENSE for details. */

#ifndef HEYMACLAYER_H_
#define HEYMACLAYER_H_

#include <stdint.h>
#include <list>
#include <queue>
#include "mbed.h"

#include "SX127xRadio.h"
#include "HeyMacIdent.h"
#include "HeyMacFrame.h"

using namespace std;


class HeyMacLayer
{
public:
    HeyMacLayer(char const *cred_fn);
    ~HeyMacLayer();

    /**
     * Enqueue a frame into the transmit queue
     * TODO: to transmit at the given time (tx_time == 0 means ASAP)
     * and signal the state machine.
     */
    void enq_tx_frame(HeyMacFrame *frm, uint32_t tx_time = 0/*ASAP*/ /*TODO: ,tx_stngs*/);

    /**
     * Posts an event to this thread indicating a button press.
     * The main app uses this method as a callback.
     */
    void evt_btn(void);

    /** Thread join */
    void thread_join(void);

    /** Starts the thread and sends the init event */
    void thread_start(void);


private:
    /** State machine return values */
    typedef enum
    {
        SM_RET_HANDLED = 0,
        SM_RET_IGNORED,
        SM_RET_TRAN
    } sm_ret_t;

    /** Transmit data struct that is stored in the _tx_queue */
    typedef struct
    {
        HeyMacFrame *frm;
        uint32_t at_time_ms;
        // TODO: tx_stngs;
    } tx_data_t;

    /* Thread stuff */
    Thread *_thread;
    LowPowerTicker *_ticker;
    uint32_t _period_ms;

    /* State machine stuff */
    sm_ret_t (HeyMacLayer::*_st_handler)(uint32_t const evt_flags);

    /* App stuff */
    SPI *_spi;
    SX127xRadio *_radio;
    HeyMacIdent *_hm_ident;
    deque<tx_data_t> _tx_queue;

    /** Runs this thread's main loop */
    void _main(void);

    /**
     * Posts an event to this thread indicating a radio DIOx pin rising edge.
     * This is given to the radio instance as a callback.
     */
    void _evt_dio(SX127xRadio::sig_dio_t const dio);

    /* State handlers */
    /**
     * Initing state
     * Handles thread-init event.
     * Parses credential file to get HeyMacIdentity data.
     * Inits the radio and requests default settings.
     * Always transitions to _st_setting()
     */
    sm_ret_t _st_initing(uint32_t const evt_flags);

    /**
     * Setting state
     * Commands the radio to sleep if there are
     * outstanding settings that need sleep mode.
     * Enters Standby mode and transitions to
     * Listening or Transmitting if the TX queue is not empty.
     */
    sm_ret_t _st_setting(uint32_t const evt_flags);

    /**
     * Listening state
     * Prepares the radio to receive.
     * Commands the radio to receive-continuous mode.
     * Handles the thread-periodic event
     * and updates radio state info.
     * If the TX queue is not empty, transitions to Setting.
     * Handles the radio-valid-header event
     * and transitions to Receiving.
     */
    sm_ret_t _st_lstning(uint32_t const evt_flags);

    /**
     * Receiving state
     * Handles the radio-receive-done event,
     * reads radio frame and reception meta-data,
     * processes the frame and transitions to Setting.
     */
    sm_ret_t _st_rxing(uint32_t const evt_flags);

    /**
     * Transmit state
     * Prepares the radio to transmit.
     * Commands the radio to transmit mode.
     * Handles the radio-transmit-done event
     * and transitions to Setting.
     */
    sm_ret_t _st_txing(uint32_t const evt_flags);

    /** Ticker callback.  Posts the periodic event to thread */
    void _ticker_clbk(void);

    /**
     * Transmit beacon
     * Prepares a HeyMac Beacon Command
     * and inserts it into the transmit queue.
     */
    void _tx_bcn(void);
};

#endif /* HEYMACLAYER_H_ */
