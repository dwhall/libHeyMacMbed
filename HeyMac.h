#ifndef HEYMAC_H_
#define HEYMAC_H_

#include "mbed.h"

enum
{   // all sizes in octets
    HM_FILENAME_SZ = 64,
    HM_FILEBUF_SZ = 2048,
    HM_FRAME_SZ = 256,
    HM_SECP384R1_KEY_SZ = 2 * 384 / 8, // Two 384-bit integers define the key
    HM_LONG_ADDR_SZ = 128 / 8,
    HM_SHORT_ADDR_SZ = 16 / 8,

    // item counts (not size)
    HM_FRMBUF_POOL_CNT = 4,
};


typedef enum
{
HM_RET_OK       = 0,
HM_RET_ERR      = 1,
HM_RET_NOT_IMPL = 2,

} hm_retval_t;

// typedef uint8_t hm_frame_buf_t[HM_FRAME_SZ];

#define stringify(n) #n


extern MemoryPool<uint8_t, HM_FRAME_SZ * HM_FRMBUF_POOL_CNT> g_frmbuf_pool;

#endif /* HEYMAC_H_ */
