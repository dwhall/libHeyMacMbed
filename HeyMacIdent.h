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

    void copy_tac_id_into(char tac_id[HM_IDENT_TAC_ID_SZ]);
    uint64_t get_long_addr(void);
    void parse_cred_file(void);

private:
    char _cred_fn[HM_FILENAME_SZ];
    char _name[HM_IDENT_NAME_SZ];
    char _tac_id[HM_IDENT_TAC_ID_SZ];
    uint8_t _long_addr[HM_LONG_ADDR_SZ];

    void hash_key_to_addr(uint8_t const * const pub_key, uint8_t r_addr[HM_LONG_ADDR_SZ]);
    void hex_to_bin(string const & hex_data, uint8_t *const r_bin, size_t sz);

    void _spoof(void);
};

#endif /* HEYMACIDENT_H_ */
