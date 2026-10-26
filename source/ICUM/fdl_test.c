#include "fdl_test.h"
#include "device.h"
#include "icum_flash_handoff.h"

volatile r_fdl_status_t g_icum_fdl_init_status;
volatile r_fdl_status_t g_icum_fdl_prepare_status;
volatile r_fdl_status_t g_icum_fdl_erase_status;
volatile r_fdl_status_t g_icum_fdl_blank_status;
volatile r_fdl_status_t g_icum_fdl_write_status;
volatile r_fdl_status_t g_icum_fdl_read_status;
volatile unsigned long g_icum_fdl_mismatch_index;
volatile unsigned int g_icum_fdl_test_result;

unsigned int icum_fdl_self_test(void)
{
    unsigned int write_data[ICUM_FDL_TEST_WORD_COUNT];
    unsigned int read_data[ICUM_FDL_TEST_WORD_COUNT];
    unsigned int i;
    unsigned int result;

    result = 0u;
    g_icum_fdl_test_result = 0u;
    g_icum_fdl_mismatch_index = 0xFFFFFFFFul;
    g_icum_fdl_init_status = R_FDL_ERR_REJECTED;
    g_icum_fdl_prepare_status = R_FDL_ERR_REJECTED;
    g_icum_fdl_erase_status = R_FDL_ERR_REJECTED;
    g_icum_fdl_blank_status = R_FDL_ERR_REJECTED;
    g_icum_fdl_write_status = R_FDL_ERR_REJECTED;
    g_icum_fdl_read_status = R_FDL_ERR_REJECTED;

    if (icum_flash_handoff_begin() == 0)
    {
        (void)icum_flash_handoff_end();
        return 0u;
    }

    INIT_FLASHACCESS;

    g_icum_fdl_init_status = icum_fdl_init();
    if (R_FDL_OK != g_icum_fdl_init_status) {
        goto cleanup;
    }

    g_icum_fdl_prepare_status = icum_fdl_prepare();
    if (R_FDL_OK != g_icum_fdl_prepare_status) {
        goto cleanup;
    }

    g_icum_fdl_erase_status =
        icum_fdl_erase(ICUM_FDL_TEST_BLOCK_INDEX, 1u);
    if (R_FDL_OK != g_icum_fdl_erase_status) {
        goto cleanup;
    }

    g_icum_fdl_blank_status =
        icum_fdl_blank_check(ICUM_FDL_REGION_OFFSET,
                             ICUM_FDL_TEST_WORD_COUNT);
    if (R_FDL_OK != g_icum_fdl_blank_status) {
        goto cleanup;
    }

    for (i = 0u; i < ICUM_FDL_TEST_WORD_COUNT; ++i) {
        write_data[i] = 0xA5A55A5Au | i;
        read_data[i] = 0u;
    }

    g_icum_fdl_write_status =
        icum_fdl_write(ICUM_FDL_REGION_OFFSET,
                       write_data,
                       ICUM_FDL_TEST_WORD_COUNT);
    if (R_FDL_OK != g_icum_fdl_write_status) {
        goto cleanup;
    }

    g_icum_fdl_read_status =
        icum_fdl_read(ICUM_FDL_REGION_OFFSET,
                      read_data,
                      ICUM_FDL_TEST_WORD_COUNT);
    if (R_FDL_OK != g_icum_fdl_read_status) {
        goto cleanup;
    }

    for (i = 0u; i < ICUM_FDL_TEST_WORD_COUNT; ++i) {
        if (read_data[i] != write_data[i]) {
            g_icum_fdl_mismatch_index = i;
            goto cleanup;
        }
    }

    result = 1u;

cleanup:
    DISABLE_FLASHACCESS;
    if (icum_flash_handoff_end() == 0)
    {
        result = 0u;
    }
    g_icum_fdl_test_result = result;
    return result;
}
