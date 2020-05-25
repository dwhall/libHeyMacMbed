/* Copyright 2020 Dean Hall.  See LICENSE for details. */

#ifndef HEYMACCMD_H_
#define HEYMACCMD_H_

#include <stdint.h>

static int const CMD_SZ_MAX = 256;


class HeyMacCmd
{
public:
    HeyMacCmd();
    ~HeyMacCmd();

    void cmd_txt(uint8_t *txt, uint8_t sz);
//    void cmd_cbcn(uint16_t caps, uint16_t status); // TODO: nets,ngbrs needs outside data

    uint8_t *get_ref(void);
    uint8_t get_sz(void);

private:
    uint8_t _buf[CMD_SZ_MAX];
    uint8_t _cmd_sz;
};

#endif /* HEYMACCMD_H_ */
