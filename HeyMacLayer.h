/* Copyright 2020 Dean Hall.  See LICENSE for details. */

#ifndef HEYMACLAYER_H_
#define HEYMACLAYER_H_

#include <stdint.h>
#include "mbed.h"
#include "AThread.h"

#include "SX1276_LoRaRadio.h"

#include "HeyMacIdent.h"
#include "HeyMacFrame.h"


class HeyMacLayer: public AThread
{
public:
    HeyMacLayer(char const *cred_fn);
    ~HeyMacLayer();
    void dump_regs(void);
    void evt_txt_callsign(void);

private:
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

    void _tx_bcn(void);

    void _dump_buf(uint8_t const *const buf, uint8_t const sz);
};

#endif /* HEYMACLAYER_H_ */
