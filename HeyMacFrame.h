/* Copyright 2020 Dean Hall.  See LICENSE for details. */
/*
HeyMac is a flexible frame definition and communication protocol
designed to carry Data Link (Layer 2) and Network (Layer 3) frames
between small payload radio modems such as the Semtech SX127x.
HeyMac is distilled from and incompatible with IEEE 802.15.4.

HeyMac offers 16- and 64-bit addressing, multi-network and multi-hop capabilities.
Extensions for cryptographic authentication and encryption are possible.

The HeyMac frame is very dynamic.  Only the first two fields, Protocol ID
and Frame Control, are required.  The Frame Control field defines which
of the other fields are present.  The length of a HeyMac frame
MUST be conveyed by the physical layer.
HeyMac uses LoRa's Explicit Header mode to convey the physical frame length.

Frame authentication and encryption are each optional.
If both signing and encryption take place, signing is done first
and then the signature is encrypted.  The following diagram shows the order
of the HeyMac frame fields and which fields are covered by
authentication and which fields are encrypted.
The topmost field in the diagram is transmitted first.

    +----+----+----+----+----+----+----+----+---+---+
    |  Protocol ID                (1 octet) | C |   |
    +----+----+----+----+----+----+----+----+ l + A +
    |  Frame Control              (1 octet) | e | u |
    +----+----+----+----+----+----+----+----+ a + t +
    |  Network ID           (0 or 2 octets) | r | h |
    +----+----+----+----+----+----+----+----+ t + e +
    |  Destination Address (0, 2, 8 octets) | e | n |
    +----+----+----+----+----+----+----+----+ x + t +
    |  Hdr Information Elements  (variable) | t | i |
    +----+----+----+----+----+----+----+----+---+ c +
    |  Bdy Information Elements  (variable) |   | a |
    +----+----+----+----+----+----+----+----+ C + t +
    |  Source Address    (0, 2 or 8 octets) | r | e |
    +----+----+----+----+----+----+----+----+ y + d +
    |  Payload                   (variable) | p |   |
    +----+----+----+----+----+----+----+----+ t +---+
    |  Msg Integrity Code   (0 or N octets) |   |
    +----+----+----+----+----+----+----+----+---+
    |  Hops                  (0 or 1 octet) |
    +----+----+----+----+----+----+----+----+
    |  Transmitter Addr  (0, 2 or 8 octets) |
    +----+----+----+----+----+----+----+----+
*/

#ifndef HEYMACFRAME_H_
#define HEYMACFRAME_H_

#include <stdint.h>
//#include "mbed.h"
//#include "Thread.h"

#include "HeyMacCmd.h"


static int const FRAME_SZ_MAX = 256;

/*
Protocol ID (PID) Field

=========== =================================
Bit pattern Protocol
=========== =================================
1110 00vv   HeyMac TDMA, major (vv)ersion
1110 01vv   HeyMac CSMA, major (vv)ersion
1110 1xxx   HeyMac (RFU)
=========== =================================
*/
typedef enum
{
    // TODO: HM_PIDFLD_TDMA_V0 = 0xE0,
    HM_PIDFLD_CSMA_V0 = 0xE4,
    // TODO: HM_PIDFLD_FLOOD = 0xE8,
} hm_pidfld_t8;

class HeyMacFrame
{
public:
    HeyMacFrame();
    HeyMacFrame(uint8_t * buf, uint8_t sz);
    ~HeyMacFrame();

    uint8_t *get_ref(void);
    uint8_t get_sz(void);

    // When building a frame, perform calls in this order:
    void set_protocol(hm_pidfld_t8 pidfld);
    void set_net_id(uint16_t net_id);
    void set_dst_addr(uint16_t dst_addr);
    void set_dst_addr(uint64_t dst_addr);
    void set_src_addr(uint16_t src_addr);
    void set_src_addr(uint64_t src_addr);
    bool set_payld(uint8_t * payld, uint8_t sz);
    bool set_payld(HeyMacCmd &cmd);
    // TODO: set_mic()
    bool set_multihop(uint8_t hops, uint16_t tx_addr);
    bool set_multihop(uint8_t hops, uint64_t tx_addr);

    // After receiving a frame, call parse() on it
    bool parse(void);
    // TODO: update_multihop()

private:
    uint8_t _buf[FRAME_SZ_MAX];
    uint8_t _payld_offset;
    uint8_t _payld_sz;
    uint8_t _mic_sz;
    uint8_t _rxd_sz;

    uint8_t _get_ie_sz(uint8_t ie_offset);
    uint8_t _get_mic_sz(uint8_t ie_offset);
    bool _validate_fields(void); // used by parse()
};

#endif /* HEYMACFRAME_H_ */
