#include "icum_comm.h"
#include "icum_flash_handoff.h"
#include "icum_flash_handoff_protocol.h"
#include "shared_memory.h"

__IOREG(ICUM_ICPE, 0xFFFEEA02, __READ_WRITE, uint16);
__IOREG(ICUM_ICPES, 0xFFFEEA00, __READ_WRITE, uint16);
__IOREG(ICUM_PE2ICUFC, 0xFF1F0048, __WRITE, uint32);
__IOREG(ICUM_PE2ICUIE, 0xFF1F004C, __READ_WRITE, uint32);
__IOREG(ICUM_PE2ICUSFC, 0xFF1F0078, __WRITE, uint32);
__IOREG(ICUM_PE2ICUSIE, 0xFF1F007C, __READ_WRITE, uint32);

volatile uint32 g_icum_comm_status;

void icum_comm_init(void)
{
    g_icum_comm_status = 0u;
    icum_flash_handoff_init();
    ICUM_PE2ICUFC =
        ICUM_FLASH_HANDOFF_FLAG | ICUM_SHARED_MEMORY_FLAG;
    ICUM_PE2ICUSFC = 1u;
    ICUM_ICPE = 0x0040u;
    ICUM_ICPES = 0x0040u;
    ICUM_PE2ICUIE =
        ICUM_FLASH_HANDOFF_FLAG | ICUM_SHARED_MEMORY_FLAG;
    ICUM_PE2ICUSIE = 1u;
}

#pragma ghs interrupt
void INTPE(void)
{
    uint32 index;
    uint32 events;
    unsigned char first_byte;

    events = ICUM_CMDREGICUM_PE2ICUF;
    if ((events & (ICUM_FLASH_HANDOFF_FLAG | ICUM_SHARED_MEMORY_FLAG)) != 0u)
    {
        ICUM_PE2ICUFC =
            events & (ICUM_FLASH_HANDOFF_FLAG | ICUM_SHARED_MEMORY_FLAG);
    }
    icum_flash_handoff_on_pe_event(events);

    if ((events & ICUM_SHARED_MEMORY_FLAG) != 0u)
    {
        first_byte = g_shared_buffer[0];
        for (index = 0u; index < SHARED_BUFFER_SIZE; index++)
        {
            *(g_shared_buffer+index) = 0;
        }
        if (first_byte == 0x55u)
        {
            g_icum_comm_status ^= 1u;
        }
    }
}

#pragma ghs interrupt
void INTPES(void)
{
    ICUM_PE2ICUSFC = 1u;
}
