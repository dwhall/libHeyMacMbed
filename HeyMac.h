#ifndef HEYMAC_H_
#define HEYMAC_H_

enum
{   // all sizes in octets
    HM_FILENAME_SZ = 64,
    HM_FILEBUF_SZ = 2048,
    HM_SECP384R1_KEY_SZ = 2 * 384 / 8, // Two 384-bit integers define the key
    HM_LONG_ADDR_SZ = 128 / 8,
    HM_SHORT_ADDR_SZ = 16 / 8
};


typedef enum
{
HM_RET_OK       = 0,
HM_RET_ERR      = 1,
HM_RET_NOT_IMPL = 2,

} hm_retval_t;


#define stringify(n) #n

#endif /* HEYMAC_H_ */
