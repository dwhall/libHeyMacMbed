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

private:
    char const * const _cred_fn;
    uint8_t _long_addr[LONG_ADDR_SZ];

    void gen_long_addr(void);
    void hash_key_to_addr(uint8_t const * const pub_key, uint8_t * const r_addr);
    void hex_to_bin(string const & hex_data, uint8_t *const r_bin, size_t sz);
};

#endif /* HEYMACIDENT_H_ */
