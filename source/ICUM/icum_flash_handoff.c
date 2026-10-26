#include "icum_flash_handoff.h"
#include "icum_flash_handoff_protocol.h"

__IOREG(ICUM_FLASH_ICU2PEFS, 0xFF1F0054, __WRITE, uint32);

static volatile uint32 g_flash_handoff_response;

static int icum_flash_handoff_wait(void)
{
    uint32 timeout;

    timeout = ICUM_FLASH_HANDOFF_TIMEOUT_LOOPS;
    while ((g_flash_handoff_response == 0u) && (timeout != 0u))
    {
        --timeout;
    }

    if (g_flash_handoff_response == 0u)
    {
        return 0;
    }

    g_flash_handoff_response = 0u;
    return 1;
}

void icum_flash_handoff_init(void)
{
    g_flash_handoff_response = 0u;
}

int icum_flash_handoff_begin(void)
{
    g_flash_handoff_response = 0u;
    ICUM_CMDREGICUM_ICU2PES = ICUM_FLASH_HANDOFF_START_COMMAND;
    ICUM_FLASH_ICU2PEFS = ICUM_FLASH_HANDOFF_FLAG;

    return icum_flash_handoff_wait();
}

int icum_flash_handoff_end(void)
{
    g_flash_handoff_response = 0u;
    ICUM_CMDREGICUM_ICU2PES = ICUM_FLASH_HANDOFF_STOP_COMMAND;
    ICUM_FLASH_ICU2PEFS = ICUM_FLASH_HANDOFF_FLAG;

    return icum_flash_handoff_wait();
}

void icum_flash_handoff_on_pe_event(uint32 events)
{
    if ((events & ICUM_FLASH_HANDOFF_FLAG) != 0u)
    {
        g_flash_handoff_response = 1u;
    }
}
