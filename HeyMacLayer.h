/* Copyright 2020 Dean Hall.  See LICENSE for details. */

#ifndef HEYMACLAYER_H_
#define HEYMACLAYER_H_

#include <stdint.h>
#include "mbed.h"
#include "Thread.h"

#include "SX1276_LoRaRadio.h"

#include "HeyMacIdent.h"


class HeyMacLayer
{
public:
    HeyMacLayer(char const *cred_fn);
    ~HeyMacLayer();
    void thread_start(void);
    void dump_regs(void);

private:
    Thread *_thread;
    SX1276_LoRaRadio *_radio; // TODO: make type LoRaRadio
    HeyMacIdent *_hm_ident;

    void _main(void);

    // Callbacks for LoRaRadio
    void _tx_done_mac_clbk(void);
    void _tx_timeout_mac_clbk(void);
    void _rx_done_mac_clbk(uint8_t const *payload, uint16_t size, int16_t rssi, int8_t snr);
    void _rx_timeout_mac_clbk(void);
    void _rx_error_mac_clbk(void);
    void _fhss_change_channel_mac_clbk(uint8_t current_channel);
    void _cad_done_mac_clbk(bool channel_busy);
};

#endif /* HEYMACLAYER_H_ */
