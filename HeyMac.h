#ifndef HEYMAC_H_
#define HEYMAC_H_

enum
{   // all sizes in octets
    FILENAME_SZ = 64,
    FILEBUF_SZ = 2048,
    SECP384R1_KEY_SZ = 2 * 384 / 8, // Two 384-bit integers define the key
    LONG_ADDR_SZ = 128 / 8,
    SHORT_ADDR_SZ = 16 / 8
};


typedef enum
{
HM_RET_OK       = 0,
HM_RET_ERR      = 1,
HM_RET_NOT_IMPL = 2,

} hm_retval_t;


#define stringify(n) #n

#endif /* HEYMAC_H_ */
