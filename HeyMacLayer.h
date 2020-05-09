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
    void thread_init(void);
    void thread_start(void);

private:
    Thread _thread;
    SX1276_LoRaRadio *_radio; // TODO: make type LoRaRadio
    HeyMacIdent *_hm_ident;

    void thread_main(void);

    // Callbacks for LoRaRadio
    void tx_done_mac_clbk(void);
    void tx_timeout_mac_clbk(void);
    void rx_done_mac_clbk(uint8_t const *payload, uint16_t size, int16_t rssi, int8_t snr);
    void rx_timeout_mac_clbk(void);
    void rx_error_mac_clbk(void);
    void fhss_change_channel_mac_clbk(uint8_t current_channel);
    void cad_done_mac_clbk(bool channel_busy);
};

#endif /* HEYMACLAYER_H_ */
