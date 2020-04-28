/* Copyright 2020 Dean Hall.  See LICENSE for details. */

#include <stdio.h>
#include <stdint.h>

#include <string>

#include "mbed.h"
#include "SDBlockDevice.h"
#include "FATFileSystem.h"
#include "MbedJSONValue.h"

#include "HeyMac.h"
#include "HeyMacIdent.h"


HeyMacIdent::HeyMacIdent(char const * const cred_fn)
{
    parse_cred_file(cred_fn);
}


HeyMacIdent::~HeyMacIdent()
{}


hm_retval_t HeyMacIdent::get_long_addr(uint8_t* const r_addr)
{
    return HM_RET_NOT_IMPL;
}


void HeyMacIdent::hash_key_to_addr(uint8_t const * const pub_key, uint8_t r_addr[HM_LONG_ADDR_SZ])
{
    enum
    {
        HASH_SZ = 64
    };
    mbedtls_sha512_context sha_ctx;
    mbedtls_sha512_context sha_ctx2;
    unsigned char hash[HASH_SZ];

    // Perform SHA512 hash twice on the public key
    mbedtls_sha512_init(&sha_ctx);
    mbedtls_sha512_starts_ret(&sha_ctx, 0);
    mbedtls_sha512_update_ret(&sha_ctx, pub_key, HM_SECP384R1_KEY_SZ); // once
    mbedtls_sha512_clone(&sha_ctx2, &sha_ctx);
    mbedtls_sha512_finish_ret(&sha_ctx, hash);

    mbedtls_sha512_update_ret(&sha_ctx2, hash, HASH_SZ); // twice
    mbedtls_sha512_finish_ret(&sha_ctx2, hash);
    mbedtls_sha512_free(&sha_ctx);
    mbedtls_sha512_free(&sha_ctx2);

    // Copy the first 128-bits of the 512-bit hash
    memcpy(r_addr, hash, HM_LONG_ADDR_SZ);
}


void HeyMacIdent::hex_to_bin(string const & hex_data, uint8_t *const r_bin, size_t sz)
{
    char c;
    uint8_t b;
    size_t i;

    for (i = 0; i < sz; i++)
    {
        // Upper nibble
        c = hex_data[2 * i];
        if ((c >= '0') && (c <= '9'))
            c -= '0';
        else if ((c >= 'a') && (c <= 'f'))
            c = c - 'a' + 10;
        else if ((c >= 'A') && (c <= 'F'))
            c = c - 'A' + 10;
        else
            break;
        b = c << 4;

        // Lower nibble
        c = hex_data[2 * i + 1];
        if ((c >= '0') && (c <= '9'))
            c -= '0';
        else if ((c >= 'a') && (c <= 'f'))
            c = c - 'a' + 10;
        else if ((c >= 'A') && (c <= 'F'))
            c = c - 'A' + 10;
        else
            break;
        b |= c;

        r_bin[i] = b;
    }

    // Zero any remaining bytes (due to "break" from above)
    for ( ; i < sz; i++)
    {
        r_bin[i] = 0;
    }
}

void HeyMacIdent::parse_cred_file(char const * const cred_fn)
{
    #define FS_MOUNT_NAME "fs"

    SDBlockDevice bd(MBED_CONF_SD_SPI_MOSI, MBED_CONF_SD_SPI_MISO, MBED_CONF_SD_SPI_CLK, MBED_CONF_SD_SPI_CS);
    FATFileSystem fs(FS_MOUNT_NAME);

    bd.frequency(8000000); // 8 MHz SPI clock // TODO: make this a build parameter

    if (0 == fs.mount(&bd))
    {
        char fn[HM_FILENAME_SZ];
        FILE *fp;

        // Open the credential json file
        snprintf(fn, sizeof(fn), "/%s/%s", FS_MOUNT_NAME, cred_fn);
        fp = fopen(fn, "r");
        if (fp)
        {
            // Read in and parse the json file
            char buf[HM_FILEBUF_SZ];
            fread(buf, sizeof(buf), 1, fp);
            MbedJSONValue cred;
            parse(cred, buf);

            // Get "name" item from json obj
            string json_item = cred["name"].get<std::string>();
            strncpy(_name, json_item.c_str(), NAME_SZ);

            // Get "tac_id" item from json obj
            json_item = cred["tac_id"].get<std::string>();
            strncpy(_tac_id, json_item.c_str(), TAC_ID_SZ);

            // Get "pub_key" item from json obj
            json_item = cred["pub_key"].get<std::string>();

            // Convert pub key from asciihex key to binary
            uint8_t pub_key[HM_SECP384R1_KEY_SZ];
            hex_to_bin(json_item, pub_key, HM_SECP384R1_KEY_SZ);

            // Hash pub_key into long_addr
            hash_key_to_addr(pub_key, _long_addr);
        }
    }
}
