/* Copyright 2020 Dean Hall.  See LICENSE for details. */

#include <stdio.h>
#include <stdint.h>

#include <string>
#include <fstream>
#include <streambuf>

#include "mbed.h"
#include "BlockDevice.h"
#include "LittleFileSystem.h"
#include "MbedJSONValue.h"

#include "HeyMac.h"
#include "HeyMacIdent.h"


HeyMacIdent::HeyMacIdent(char const * const cred_fn)
    :
    _cred_fn(cred_fn)
{
}


HeyMacIdent::~HeyMacIdent()
{}


hm_retval_t HeyMacIdent::get_long_addr(uint8_t* const r_addr)
{
    #define LFS_MOUNT_NAME "fs"

    BlockDevice *bd = BlockDevice::get_default_instance();
    LittleFileSystem fs(LFS_MOUNT_NAME);
    hm_retval_t retval = HM_RET_ERR;

    if (0 == fs.mount(bd))
        {
        char fn[FILENAME_SZ];
        FILE *fp;

        // Read credential file into buffer
        snprintf(fn, sizeof(fn), "/%s/%s", LFS_MOUNT_NAME, _cred_fn);
        fp = fopen(fn, "r");
        if (fp)
        {
            char buf[FILEBUF_SZ];
            fread(buf, sizeof(buf), 1, fp);

            // Get "pub_key" item from json obj in buffer
            MbedJSONValue cred;
            parse(cred, buf);
            string pub_key_hex = cred["pub_key"].get<std::string>();

            // Convert asciihex key to binary key
            uint8_t pub_key[BIN_KEY_SZ];
            hex_to_bin_512b(pub_key_hex, pub_key);

            // Hash pub_key into long_addr
            hash_key_to_addr(pub_key, r_addr);
            retval = HM_RET_OK;
            }
        }

    return retval;
}


void HeyMacIdent::hash_key_to_addr(uint8_t const * const pub_key, uint8_t * const r_addr)
{
    mbedtls_sha512_context sha_ctx;


    mbedtls_sha512_init(&sha_ctx);

}


void HeyMacIdent::hex_to_bin_512b(string const pub_key, uint8_t* const r_bin)
{
    char c;
    uint8_t b;

    for (uint8_t i = 0; i < BIN_KEY_SZ; i++)
    {
        // Upper nibble
        c = pub_key[2 * i];
        if ((c >= '0') && (c <= '9'))
            c -= '0';
        else if ((c >= 'a') && (c <= 'f'))
            c -= 'a';
        else if ((c >= 'A') && (c <= 'F'))
            c -= 'A';
        else
            c = 0;
        b = c << 4;

        // Lower nibble
        c = pub_key[2 * i + 1];
        if ((c >= '0') && (c <= '9'))
            c -= '0';
        else if ((c >= 'a') && (c <= 'f'))
            c -= 'a';
        else if ((c >= 'A') && (c <= 'F'))
            c -= 'A';
        else
            c = 0;
        b |= c;

        r_bin[i] = b;
    }
}
