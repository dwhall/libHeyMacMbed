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

using namespace std;


HeyMacIdent::HeyMacIdent(char const * const cred_fn)
{
    strncpy(_cred_fn, cred_fn, sizeof(_cred_fn));
}


HeyMacIdent::~HeyMacIdent()
{}


void HeyMacIdent::copy_tac_id_into(char tac_id[HM_IDENT_TAC_ID_SZ])
{
    strncpy(tac_id, _tac_id, sizeof(tac_id));
}


uint64_t HeyMacIdent::get_long_addr(void)
{
    uint64_t addr;
    memcpy(&addr, _long_addr, sizeof(addr)); // FIXME: endianness
    return addr;
}


void HeyMacIdent::_hash_key_to_addr(uint8_t const * const pub_key, uint8_t r_addr[HM_LONG_ADDR_SZ])
{
    enum
    {
        HASH_SZ = 64
    };
    mbedtls_sha512_context sha_ctx;
    mbedtls_sha512_context sha_ctx2;
    unsigned char hash[HASH_SZ];

    /* Perform SHA512 hash twice on the public key */
    mbedtls_sha512_init(&sha_ctx);
    mbedtls_sha512_starts_ret(&sha_ctx, 0);
    mbedtls_sha512_update_ret(&sha_ctx, pub_key, HM_SECP384R1_KEY_SZ); /* once */
    mbedtls_sha512_clone(&sha_ctx2, &sha_ctx);
    mbedtls_sha512_finish_ret(&sha_ctx, hash);

    mbedtls_sha512_update_ret(&sha_ctx2, hash, HASH_SZ); /* twice */
    mbedtls_sha512_finish_ret(&sha_ctx2, hash);
    mbedtls_sha512_free(&sha_ctx);
    mbedtls_sha512_free(&sha_ctx2);

    /* Copy the first 128-bits of the 512-bit hash */
    memcpy(r_addr, hash, HM_LONG_ADDR_SZ);
}


void HeyMacIdent::_hex_to_bin(string const & hex_data, uint8_t *const r_bin, size_t sz)
{
    char c;
    uint8_t b;
    size_t i;

    for (i = 0; i < sz; i++)
    {
        /* Upper nibble */
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

        /* Lower nibble */
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

    /* Null any remaining bytes (due to "break" from above) */
    for ( ; i < sz; i++)
    {
        r_bin[i] = 0;
    }
}


void HeyMacIdent::parse_cred_file(void)
{
#define FS_MOUNT_NAME "fs"

    SDBlockDevice bd(MBED_CONF_SD_SPI_MOSI, MBED_CONF_SD_SPI_MISO, MBED_CONF_SD_SPI_CLK, MBED_CONF_SD_SPI_CS);
    FATFileSystem fs(FS_MOUNT_NAME);
    char fn[HM_FILENAME_SZ];

    bd.frequency(HM_IDENT_SD_SPI_CLK_HZ);

    // If no SD/FAT FS is available, use spoof credentials
    // TODO: Find way to randomize values.  Warn user?
    if (0 != fs.mount(&bd))
    {
        _spoof();
    }
    else
    {
        FILE *fp;

        /* Open the credential json file */
        snprintf(fn, sizeof(fn), "/" FS_MOUNT_NAME "/%s", _cred_fn);
        fp = fopen(fn, "r");
        if (fp)
        {
            /* Read in and parse the json file */
            char buf[HM_FILEBUF_SZ];
            fread(buf, sizeof(buf), 1, fp);
            MbedJSONValue cred;
            parse(cred, buf);

            /* Get "name" item from json obj */
            string json_item = cred["name"].get<string>();
            strncpy(_name, json_item.c_str(), HM_IDENT_NAME_SZ);

            /* Get "tac_id" item from json obj */
            json_item = cred["tac_id"].get<string>();
            strncpy(_tac_id, json_item.c_str(), HM_IDENT_TAC_ID_SZ);

            /* Get "pub_key" item from json obj */
            json_item = cred["pub_key"].get<string>();

            /* Convert pub key from asciihex key to binary */
            uint8_t pub_key[HM_SECP384R1_KEY_SZ];
            _hex_to_bin(json_item, pub_key, HM_SECP384R1_KEY_SZ);

            /* Hash pub_key into long_addr */
            _hash_key_to_addr(pub_key, _long_addr);
        }
    }
}

/**
 * Spoof identity values
 *
 * This is handy during testing if a cred.json file is not available
 */
void HeyMacIdent::_spoof(void)
{
    static uint8_t const spoof_addr[HM_LONG_ADDR_SZ] = {0xFD, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F};

    strcpy(_name, "Dean Hall");
    strcpy(_tac_id, "KC4KSU-491");
    memcpy(_long_addr, spoof_addr, sizeof(spoof_addr));
}
