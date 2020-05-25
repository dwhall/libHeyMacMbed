/* Copyright 2020 Dean Hall.  See LICENSE for details. */

#include <stdint.h>
#include <string.h>

#include "HeyMacCmd.h"


typedef uint8_t hm_cid_t8;
enum
{
    HM_CID_INVALID = 0,
    HM_CID_SBCN = 1,
    HM_CID_EBCN = 2,
    HM_CID_TXT = 3,
    HM_CID_CBCN = 4,
    HM_CID_JOIN = 5,
};

static uint8_t const CMD_IDX = 0;
static uint8_t const CMD_PREFIX = 0x80;
static uint8_t const CMD_PREFIX_MASK = 0xC0;
static uint8_t const CMD_MASK = 0x3F;

HeyMacCmd::HeyMacCmd()
{
    _buf[CMD_IDX] = 0;
    _cmd_sz = 0;
}

void HeyMacCmd::cmd_txt(uint8_t *txt, uint8_t sz)
{
    _buf[CMD_IDX] = CMD_PREFIX | HM_CID_TXT;
    memcpy(&_buf[CMD_IDX + 1], txt, sz);

    _cmd_sz = 1 + sz;
}

uint8_t *HeyMacCmd::get_ref(void)
{
    return _buf;
}

uint8_t HeyMacCmd::get_sz(void)
{
    return _cmd_sz;
}
