/* Copyright 2020 Dean Hall.  See LICENSE for details. */

#ifndef HEYMACLAYER_H_
#define HEYMACLAYER_H_

#include <stdint.h>
#include <list>           // std::list
#include <queue>          // std::queue
#include "mbed.h"

#include "AThread.h"
#include "SX127xRadio.h"
#include "HeyMacIdent.h"
#include "HeyMacFrame.h"


class HeyMacLayer: public AThread
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

private:
    /** State machine return values */
    typedef enum
    {
        SM_RET_HANDLED = 0,
        SM_RET_IGNORED,
        SM_RET_TRAN
    } sm_ret_t;

    typedef struct
    {
        HeyMacFrame *frm;
        uint32_t at_time_ms;
        // TODO: tx_stngs;
    } tx_data_t;

    SPI *_spi;
    SX127xRadio *_radio;
    HeyMacIdent *_hm_ident;
    sm_ret_t (HeyMacLayer::*_st_handler)(uint32_t const evt_flags);
    std::list<tx_data_t> _tx_list;
    std::queue<tx_data_t, std::list<tx_data_t>> _tx_queue;

    /** Runs this thread's main loop */
    void _main(void);

    /**
     * Posts an event to this thread indicating a radio DIOx pin rising edge.
     * This is given to the radio instance as a callback.
     */
    void _evt_dio(sig_dio_t const dio);

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

    /**
     * Transmit beacon
     * Prepares a HeyMac Beacon Command
     * and inserts it into the transmit queue.
     */
    void _tx_bcn(void);

    void _dump_buf(uint8_t const *const buf, uint8_t const sz);
};

#endif /* HEYMACLAYER_H_ */
