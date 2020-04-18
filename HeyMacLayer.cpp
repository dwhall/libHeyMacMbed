/* Copyright 2020 Dean Hall.  See LICENSE for details. */

#include <stdint.h>

#include "mbed.h"
#include "LoRaRadio.h"

#include "HeyMacIdent.h"
#include "HeyMacLayer.h"


HeyMacLayer::HeyMacLayer
    (
    LoRaRadio *const radio,
    char const * cred_fn
    )
    :
    _radio(radio),
    _hm_ident(cred_fn)
{
    radio_events_t mac_clbks;

    // Init the radio with these callbacks
    mac_clbks.tx_done = mbed::callback(this, &HeyMacLayer::tx_done_mac_clbk);
    mac_clbks.rx_done = mbed::callback(this, &HeyMacLayer::rx_done_mac_clbk);
    mac_clbks.rx_error = mbed::callback(this, &HeyMacLayer::rx_error_mac_clbk);
    mac_clbks.tx_timeout = mbed::callback(this, &HeyMacLayer::tx_timeout_mac_clbk);
    mac_clbks.rx_timeout = mbed::callback(this, &HeyMacLayer::rx_timeout_mac_clbk);
    mac_clbks.fhss_change_channel = mbed::callback(this, &HeyMacLayer::fhss_change_channel_mac_clbk);
    mac_clbks.cad_done = mbed::callback(this, &HeyMacLayer::cad_done_mac_clbk);
    _radio->init_radio(&mac_clbks);
}

HeyMacLayer::~HeyMacLayer()
{
}

// Callbacks for LoRaRadio
void HeyMacLayer::tx_done_mac_clbk(void) {}
void HeyMacLayer::tx_timeout_mac_clbk(void) {}
void HeyMacLayer::rx_done_mac_clbk(uint8_t const *payload, uint16_t size, int16_t rssi, int8_t snr) {}
void HeyMacLayer::rx_timeout_mac_clbk(void) {}
void HeyMacLayer::rx_error_mac_clbk(void) {}
void HeyMacLayer::fhss_change_channel_mac_clbk(uint8_t current_channel) {}
void HeyMacLayer::cad_done_mac_clbk(bool channel_busy) {}
