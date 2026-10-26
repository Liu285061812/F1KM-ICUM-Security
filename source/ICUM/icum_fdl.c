#include "icum_fdl.h"
#include "r_fdl_global.h"

#define ICUM_FDL_CPU_FREQUENCY_MHZ 240u

static const r_fdl_descriptor_t icum_fdl_descriptor =
{
    ICUM_FDL_CPU_FREQUENCY_MHZ,
    ICUM_FDL_POOL_BLOCK_COUNT,
    0u,
    0u
};

uint32_t PSWSafe;

static r_fdl_status_t icum_fdl_execute(r_fdl_request_t *request)
{
    R_FDL_Execute(request);
    while (R_FDL_BUSY == request->status_enu) {
        R_FDL_Handler();
    }
    return request->status_enu;
}

static int icum_fdl_data_range_valid(unsigned long byte_index,
                                     unsigned int word_count)
{
    unsigned long byte_count;

    byte_count = (unsigned long)word_count * 4ul;
    return (word_count != 0u)
        && ((byte_index & 3ul) == 0ul)
        && (byte_index >= ICUM_FDL_REGION_OFFSET)
        && (byte_index <= ICUM_FDL_END_OFFSET)
        && (byte_count <= (ICUM_FDL_END_OFFSET - byte_index));
}

static r_fdl_status_t icum_fdl_recover_if_not_ready(void)
{
    if (R_FDL_TRUE == R_FDL_FCUFct_ChkReady()) {
        return R_FDL_OK;
    }

    return R_FDL_FCUFct_ForcedStop();
}

void FDL_User_CriticalSetionBegin(void)
{
    asm("ldsr    r0, 31");
    asm("stsr    0x05, r7");
    asm("mov     _PSWSafe, r6");
    asm("st.w    r7, 0[r6]");
    asm("movea   0x00A0, r0, r6");
    asm("or      r7, r6");
    asm("ldsr    r6, 5");
}

void FDL_User_CriticalSetionEnd(void)
{
    asm("ldsr    r0, 31");
    asm("mov     _PSWSafe, r6");
    asm("ld.w    0[r6], r7");
    asm("ldsr    r7, 5");
}

r_fdl_status_t icum_fdl_init(void)
{
    return R_FDL_Init(&icum_fdl_descriptor);
}

r_fdl_status_t icum_fdl_prepare(void)
{
    r_fdl_request_t request;
//    r_fdl_status_t status;

//    status = icum_fdl_recover_if_not_ready();
//    if (R_FDL_OK != status) {
//        return status;
//    }

    request.command_enu = R_FDL_CMD_PREPARE_ENV;
    request.idx_u32 = 0u;
    request.cnt_u16 = 0u;
    request.bufAddr_u32 = 0u;
    request.accessType_enu = R_FDL_ACCESS_NONE;
    return icum_fdl_execute(&request);
}

r_fdl_status_t icum_fdl_erase(unsigned long first_block,
                              unsigned int block_count)
{
    r_fdl_request_t request;

    if ((block_count == 0u)
        || (first_block < ICUM_FDL_FIRST_BLOCK)
        || (first_block >= ICUM_FDL_POOL_BLOCK_COUNT)
        || ((unsigned long)block_count
            > (ICUM_FDL_POOL_BLOCK_COUNT - first_block))) {
        return R_FDL_ERR_PARAMETER;
    }

    request.command_enu = R_FDL_CMD_ERASE;
    request.idx_u32 = (uint32_t)first_block;
    request.cnt_u16 = (uint16_t)block_count;
    request.bufAddr_u32 = 0u;
    request.accessType_enu = R_FDL_ACCESS_USER;
    return icum_fdl_execute(&request);
}

r_fdl_status_t icum_fdl_blank_check(unsigned long byte_index,
                                    unsigned int word_count)
{
    r_fdl_request_t request;

    if (!icum_fdl_data_range_valid(byte_index, word_count)) {
        return R_FDL_ERR_PARAMETER;
    }

    request.command_enu = R_FDL_CMD_BLANKCHECK;
    request.idx_u32 = (uint32_t)byte_index;
    request.cnt_u16 = (uint16_t)word_count;
    request.bufAddr_u32 = 0u;
    request.accessType_enu = R_FDL_ACCESS_USER;
    return icum_fdl_execute(&request);
}

r_fdl_status_t icum_fdl_write(unsigned long byte_index,
                              const unsigned int *data,
                              unsigned int word_count)
{
    r_fdl_request_t request;

    if ((data == (const unsigned int *)0)
        || !icum_fdl_data_range_valid(byte_index, word_count)) {
        return R_FDL_ERR_PARAMETER;
    }

    request.command_enu = R_FDL_CMD_WRITE;
    request.idx_u32 = (uint32_t)byte_index;
    request.cnt_u16 = (uint16_t)word_count;
    request.bufAddr_u32 = (uint32_t)data;
    request.accessType_enu = R_FDL_ACCESS_USER;
    return icum_fdl_execute(&request);
}

r_fdl_status_t icum_fdl_read(unsigned long byte_index,
                             unsigned int *data,
                             unsigned int word_count)
{
    r_fdl_request_t request;

    if ((data == (unsigned int *)0)
        || !icum_fdl_data_range_valid(byte_index, word_count)) {
        return R_FDL_ERR_PARAMETER;
    }

    request.command_enu = R_FDL_CMD_READ;
    request.idx_u32 = (uint32_t)byte_index;
    request.cnt_u16 = (uint16_t)word_count;
    request.bufAddr_u32 = (uint32_t)data;
    request.accessType_enu = R_FDL_ACCESS_USER;
    return icum_fdl_execute(&request);
}
