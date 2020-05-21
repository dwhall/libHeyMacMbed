/* Copyright 2020 Dean Hall.  See LICENSE for details. */

#include <stdint.h>

#include "mbed.h"
#include "SX1276_LoRaRadio.h"

#include "HeyMacIdent.h"
#include "HeyMacLayer.h"


HeyMacLayer::HeyMacLayer
    (
    char const * cred_fn
    )
{
// FIXME: the following code works on its own, but uncommenting it will cause the SX1276_LoRaRadio constructor to fail.
//    _hm_ident = new HeyMacIdent(cred_fn);
    _radio = new SX1276_LoRaRadio(
        HM_PIN_LORA_MOSI,
        HM_PIN_LORA_MISO,
        HM_PIN_LORA_SCK,
        HM_PIN_LORA_NSS,
        HM_PIN_LORA_RESET,
        HM_PIN_LORA_DIO0,
        HM_PIN_LORA_DIO1,
        HM_PIN_LORA_DIO2,
        HM_PIN_LORA_DIO3,
        HM_PIN_LORA_DIO4,
        HM_PIN_LORA_DIO5);
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


void HeyMacLayer::thread_init(void)
{
    // Init the radio with these callbacks
    radio_events_t mac_clbks;
    mac_clbks.tx_done = mbed::callback(this, &HeyMacLayer::tx_done_mac_clbk);
    mac_clbks.rx_done = mbed::callback(this, &HeyMacLayer::rx_done_mac_clbk);
    mac_clbks.rx_error = mbed::callback(this, &HeyMacLayer::rx_error_mac_clbk);
    mac_clbks.tx_timeout = mbed::callback(this, &HeyMacLayer::tx_timeout_mac_clbk);
    mac_clbks.rx_timeout = mbed::callback(this, &HeyMacLayer::rx_timeout_mac_clbk);
    mac_clbks.fhss_change_channel = mbed::callback(this, &HeyMacLayer::fhss_change_channel_mac_clbk);
    mac_clbks.cad_done = mbed::callback(this, &HeyMacLayer::cad_done_mac_clbk);
    _radio->init_radio(&mac_clbks);
}


void HeyMacLayer::thread_start(void)
{
    _thread.start(callback(this, &HeyMacLayer::thread_main));
}


void HeyMacLayer::thread_main(void)
{
    static uint8_t msg[5] = "ping";

    _radio->set_tx_config(
        MODEM_LORA,
        12 /*pwr*/,
        0 /*fdev (unused)*/,
        1 /*BW*/,   // 1 +7 ==> 250 kHz
        7 /*SF 128*/,
        2 /*CR 4/6*/,
        8 /*preamble*/,
        false /*fixlen*/,
        true /*crc*/,
        false /*freqhop*/,
        0 /*hop prd*/,
        false /*iq inverted*/,
        100 /*ms tx timeout*/);
    ThisThread::sleep_for(100);

    _radio->set_syncword(0x12);
    ThisThread::sleep_for(100);

    _radio->set_channel(432550000);
    ThisThread::sleep_for(100);

    for (;;)
        {
        _radio->send(msg, sizeof(msg));
        ThisThread::sleep_for(1000);
        }
}

void HeyMacLayer::dump_regs(void)
{
    _radio->dump_regs();
}
