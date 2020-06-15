/* Copyright 2020 Dean Hall.  See LICENSE for details. */

#ifndef HEYMACCMD_H_
#define HEYMACCMD_H_

#include <stdint.h>
#include "HeyMacFrame.h"


static int const CMD_SZ_MAX = 256;


class HeyMacCmd
{
public:
    HeyMacCmd();
    ~HeyMacCmd();

    /**
     * Keeps the reference to the frame
     * so it may be used by the cmd_*() methods
     */
    void cmd_init(HeyMacFrame *frm);

    /**
     * Returns true if the command will fit within the frame's payload
     * and fills the frame's payload with the command.
     * Return false if there is no room
     * and leaves the payload unchanged.
     */
    bool cmd_txt(uint8_t const *const txt, uint8_t const sz);
    bool cmd_cbcn(uint16_t const caps, uint16_t const status); // TODO: nets,ngbrs needs outside data

private:
    HeyMacFrame *_frm;
};

#endif /* HEYMACCMD_H_ */
