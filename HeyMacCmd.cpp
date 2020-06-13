/* Copyright 2020 Dean Hall.  See LICENSE for details. */

#include <stdint.h>
#include <string.h>

#include "HeyMac.h"
#include "HeyMacCmd.h"
#include "HeyMacFrame.h"


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
    _frm = nullptr;
}

HeyMacCmd::~HeyMacCmd()
{
}

void HeyMacCmd::cmd_init(HeyMacFrame *frm)
{
    _frm = frm;
}

void HeyMacCmd::cmd_txt(uint8_t *txt, uint8_t sz)
{
    uint8_t *payld;
    uint8_t offset;

    offset = _frm->get_sz();

    if (offset + 1 + sz <= HM_FRAME_SZ)
    {
        payld = _frm->get_ref() + offset;
        payld[CMD_IDX] = CMD_PREFIX | HM_CID_TXT;
        memcpy(&payld[CMD_IDX + 1], txt, sz);
        _frm->set_payld_sz(1 + sz);
    }
}
