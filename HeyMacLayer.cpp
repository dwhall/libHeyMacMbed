/* Copyright 2020 Dean Hall.  See LICENSE for details. */

#include <stdint.h>

#include "mbed.h"
#include "SX1276_LoRaRadio.h"

#include "HeyMacIdent.h"
#include "HeyMacLayer.h"
#include "HeyMacFrame.h"
#include "HeyMacCmd.h"


static int const STACK_SZ = 6 * 1024;


HeyMacLayer::HeyMacLayer
    (
    char const * cred_fn
    )
{
    _thread = new Thread(osPriorityNormal, STACK_SZ);
    _hm_ident = new HeyMacIdent(cred_fn);
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

void HeyMacLayer::thread_start(void)
{
    _thread->start(callback(this, &HeyMacLayer::_main));
}


// Callbacks for LoRaRadio
void HeyMacLayer::_tx_done_mac_clbk(void) {}
void HeyMacLayer::_tx_timeout_mac_clbk(void) {}
void HeyMacLayer::_rx_done_mac_clbk(uint8_t const *payload, uint16_t size, int16_t rssi, int8_t snr) {}
void HeyMacLayer::_rx_timeout_mac_clbk(void) {}
void HeyMacLayer::_rx_error_mac_clbk(void) {}
void HeyMacLayer::_fhss_change_channel_mac_clbk(uint8_t current_channel) {}
void HeyMacLayer::_cad_done_mac_clbk(bool channel_busy) {}


void HeyMacLayer::_main(void)
{
    static uint8_t msg[5] = "ping";
    HeyMacFrame frame;
    HeyMacCmd cmd;

    // Parse in this thread with the large stack space
    _hm_ident->parse_cred_file();

    // Init the radio with these callbacks
    radio_events_t mac_clbks;
    mac_clbks.tx_done = mbed::callback(this, &HeyMacLayer::_tx_done_mac_clbk);
    mac_clbks.rx_done = mbed::callback(this, &HeyMacLayer::_rx_done_mac_clbk);
    mac_clbks.rx_error = mbed::callback(this, &HeyMacLayer::_rx_error_mac_clbk);
    mac_clbks.tx_timeout = mbed::callback(this, &HeyMacLayer::_tx_timeout_mac_clbk);
    mac_clbks.rx_timeout = mbed::callback(this, &HeyMacLayer::_rx_timeout_mac_clbk);
    mac_clbks.fhss_change_channel = mbed::callback(this, &HeyMacLayer::_fhss_change_channel_mac_clbk);
    mac_clbks.cad_done = mbed::callback(this, &HeyMacLayer::_cad_done_mac_clbk);
    _radio->init_radio(&mac_clbks);

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
    ThisThread::sleep_for(10);

    _radio->set_syncword(0x12);
    ThisThread::sleep_for(10);

    _radio->set_channel(432550000);
    ThisThread::sleep_for(10);

    cmd.cmd_txt(msg, sizeof(msg));

    frame.set_protocol(HM_PIDFLD_CSMA_V0);
    frame.set_src_addr(0xCA01020304050607); // TODO: use crypto addr from ident on SD card
    frame.set_payld(cmd);

    for (;;)
    {
        _radio->send(frame.get_ref(), frame.get_sz());
        ThisThread::sleep_for(1000);
    }
}

void HeyMacLayer::dump_regs(void)
{
    _radio->dump_regs();
}
