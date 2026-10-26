#include "device.h"
#include "icum_flash_handoff.h"
#include "icum_flash_handoff_protocol.h"

/* INTC2.ICFICUIF0DFRQPE: MK is bit 7 and RF is bit 12. */
__IOREG(PE_ICUM_DFRQPE_ICR, 0xFFFFB2CE, __READ_WRITE, uint16);

#define PE_ICUM_DFRQPE_MASK       0x0080u
#define PE_ICUM_DFRQPE_REQ_FLAG   0x1000u

__IOREG(PE_INTC1_ICICUP, 0xFFFEEA0E, __READ_WRITE, uint16);

static volatile unsigned int g_dfrqpe_masked;
static volatile unsigned int g_dfrqpe_was_masked;

static void pe_icum_flash_handoff_clear_dfrqpe(void)
{
    PE_ICUM_DFRQPE_ICR &= (uint16)~PE_ICUM_DFRQPE_REQ_FLAG;
}

static void pe_icum_flash_handoff_release_dfrqpe(void)
{
    if ((PE_ICUM_DFRQPE_ICR & PE_ICUM_DFRQPE_REQ_FLAG) != 0u)
    {
        pe_icum_flash_handoff_clear_dfrqpe();
    }
    if (g_dfrqpe_was_masked == 0u)
    {
        PE_ICUM_DFRQPE_ICR &= (uint16)~PE_ICUM_DFRQPE_MASK;
    }
    g_dfrqpe_masked = 0u;
}

void pe_icum_flash_handoff_init(void)
{
    g_dfrqpe_masked = 0u;
    g_dfrqpe_was_masked = 0u;
    pe_icum_flash_handoff_clear_dfrqpe();
    PE_ICUM_DFRQPE_ICR = 0x0045u;
    ICUM_CMDREGICUM_PEFLPRG0ACK = 0u;
    PE_INTC1_ICICUP = 0x0045u;
    ICUM_CMDREGICUM_ICU2PEIE |=
        ICUM_FLASH_HANDOFF_FLAG | ICUM_SHARED_MEMORY_FLAG;
}

#pragma ghs reference INTICUP
#pragma ghs interrupt
void INTICUP(void)
{
    uint32 events;
    uint32 reply;

    events = ICUM_CMDREGICUM_ICU2PEF
        & (ICUM_FLASH_HANDOFF_FLAG | ICUM_SHARED_MEMORY_FLAG);
    if (events == 0u)
    {
        return;
    }

    if ((events & ICUM_FLASH_HANDOFF_FLAG) != 0u)
    {
        reply = 0u;
        if (ICUM_CMDREGICUM_ICU2PES == ICUM_FLASH_HANDOFF_START_COMMAND)
        {
            g_dfrqpe_was_masked =
                ((PE_ICUM_DFRQPE_ICR & PE_ICUM_DFRQPE_MASK) != 0u)
                ? 1u : 0u;
            g_dfrqpe_masked = 0u;
            reply = 1u;
        }
        else if (ICUM_CMDREGICUM_ICU2PES
                 == ICUM_FLASH_HANDOFF_STOP_COMMAND)
        {
            if (g_dfrqpe_masked != 0u)
            {
                pe_icum_flash_handoff_release_dfrqpe();
            }
            reply = 1u;
        }

        ICUM_CMDREGICUM_ICU2PEFC = ICUM_FLASH_HANDOFF_FLAG;
        if (reply != 0u)
        {
            ICUM_CMDREGICUM_PE2ICUFS = ICUM_FLASH_HANDOFF_FLAG;
        }
    }
    if ((events & ICUM_SHARED_MEMORY_FLAG) != 0u)
    {
        ICUM_CMDREGICUM_ICU2PEFC = ICUM_SHARED_MEMORY_FLAG;
    }
}

#pragma ghs reference INTFICUIF0DFRQPE
#pragma ghs interrupt
void INTFICUIF0DFRQPE(void)
{
    uint32 request;
    uint32 acknowledgement;

    PE_ICUM_DFRQPE_ICR |= PE_ICUM_DFRQPE_MASK;
    g_dfrqpe_masked = 1u;
    request = ICUM_CMDREGICUM_PEFLPRG0REQ;
    acknowledgement = ICUM_CMDREGICUM_PEFLPRG0ACK;

    if (((request & ICUM_FLASH_HANDOFF_PE_REQ) != 0u)
        && ((acknowledgement
             & ICUM_FLASH_HANDOFF_PE_ACK_ACTIVE) == 0u))
    {
        ICUM_CMDREGICUM_PEFLPRG0ACK |=
            ICUM_FLASH_HANDOFF_PE_ACK;
    }
}
