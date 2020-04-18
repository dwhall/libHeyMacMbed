#ifndef HEYMAC_H_
#define HEYMAC_H_

enum
{   // all sizes in octets
    FILENAME_SZ = 64,
    FILEBUF_SZ = 2048,
    BIN_KEY_SZ = 512 / 8,
    LONG_ADDR_SZ = 128 / 8,
    SHORT_ADDR_SZ = 16 / 8
};


typedef enum
{
HM_RET_OK       = 0,
HM_RET_ERR      = 1,

} hm_retval_t;


#define stringify(n) #n

#endif /* HEYMAC_H_ */
