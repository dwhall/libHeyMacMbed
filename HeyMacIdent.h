/* Copyright 2020 Dean Hall.  See LICENSE for details. */

#ifndef HEYMACIDENT_H_
#define HEYMACIDENT_H_

#include <stdint.h>

#include "HeyMac.h"


class HeyMacIdent
{
public:
    HeyMacIdent(char const * const cred_fn);
    ~HeyMacIdent();

    hm_retval_t get_long_addr(uint8_t* const r_addr);
    void parse_cred_file(void);

private:
    static uint8_t const NAME_SZ = 64;
    static uint8_t const TAC_ID_SZ = 16;

    char _cred_fn[HM_FILENAME_SZ];
    char _name[NAME_SZ];
    char _tac_id[TAC_ID_SZ];
    uint8_t _long_addr[HM_LONG_ADDR_SZ];

    void hash_key_to_addr(uint8_t const * const pub_key, uint8_t r_addr[HM_LONG_ADDR_SZ]);
    void hex_to_bin(string const & hex_data, uint8_t *const r_bin, size_t sz);

    void _spoof(void);
};

#endif /* HEYMACIDENT_H_ */
