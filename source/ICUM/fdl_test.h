#ifndef FDL_TEST_H
#define FDL_TEST_H

#include "icum_fdl.h"

#define ICUM_FDL_TEST_WORD_COUNT 16u

extern volatile r_fdl_status_t g_icum_fdl_init_status;
extern volatile r_fdl_status_t g_icum_fdl_prepare_status;
extern volatile r_fdl_status_t g_icum_fdl_erase_status;
extern volatile r_fdl_status_t g_icum_fdl_blank_status;
extern volatile r_fdl_status_t g_icum_fdl_write_status;
extern volatile r_fdl_status_t g_icum_fdl_read_status;
extern volatile unsigned long g_icum_fdl_mismatch_index;
extern volatile unsigned int g_icum_fdl_test_result;

unsigned int icum_fdl_self_test(void);

#endif
