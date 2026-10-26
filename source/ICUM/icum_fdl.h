#ifndef ICUM_FDL_H
#define ICUM_FDL_H

#include "r_typedefs.h"
#include "r_fdl.h"

#define ICUM_FDL_DFLASH_BASE       0xFF200000uL
#define ICUM_FDL_REGION_BASE       0xFF218000uL
#define ICUM_FDL_REGION_SIZE       0x00008000uL

#define EEL_VIRTUALBLOCKSIZE    (64u)

#define FDL_POOL_SIZE           (16u * EEL_VIRTUALBLOCKSIZE)    /**< Number of Data Flash blocks, accessible by
                                                                         the FDL. Typically it is the complete
                                                                         no. of available Data Flash blocks */

#define ICUM_FDL_BLOCK_SIZE        0x00000040uL
#define ICUM_FDL_REGION_OFFSET     (ICUM_FDL_REGION_BASE - ICUM_FDL_DFLASH_BASE)
#define ICUM_FDL_FIRST_BLOCK       (ICUM_FDL_REGION_OFFSET / ICUM_FDL_BLOCK_SIZE)
#define ICUM_FDL_TEST_BLOCK_INDEX  ICUM_FDL_FIRST_BLOCK
#define ICUM_FDL_END_OFFSET        (ICUM_FDL_REGION_OFFSET + ICUM_FDL_REGION_SIZE)
#define ICUM_FDL_REGION_BLOCK_COUNT (ICUM_FDL_REGION_SIZE / ICUM_FDL_BLOCK_SIZE)
#define ICUM_FDL_POOL_BLOCK_COUNT  (ICUM_FDL_FIRST_BLOCK + ICUM_FDL_REGION_BLOCK_COUNT)

r_fdl_status_t icum_fdl_init(void);
r_fdl_status_t icum_fdl_prepare(void);
r_fdl_status_t icum_fdl_erase(unsigned long first_block,
                              unsigned int block_count);
r_fdl_status_t icum_fdl_blank_check(unsigned long byte_index,
                                    unsigned int word_count);
r_fdl_status_t icum_fdl_write(unsigned long byte_index,
                              const unsigned int *data,
                              unsigned int word_count);
r_fdl_status_t icum_fdl_read(unsigned long byte_index,
                             unsigned int *data,
                             unsigned int word_count);

#endif
