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

    void hash_key_to_addr(uint8_t const * const pub_key, uint8_t * const r_addr);
    void hex_to_bin_512b(string const pub_key, uint8_t *const r_bin);
};

#endif /* HEYMACIDENT_H_ */
