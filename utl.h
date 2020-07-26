/* C/C++ language utilities */

/* https://stackoverflow.com/questions/4415524/common-array-length-macro-for-c*/
#define CNT_OF(x) ((sizeof(x)/sizeof(0[x])) / ((size_t)(!(sizeof(x) % sizeof(0[x])))))

/* Bit utilities */

/* https://stackoverflow.com/questions/8774567/c-macro-to-create-a-bit-mask-possible-and-have-i-found-a-gcc-bug */
#define BIT_FLD(ls1, nbits) (((unsigned) -1 >> (31 - ((ls1)+(nbits) - 1))) & ~((1U << (ls1)) - 1))
