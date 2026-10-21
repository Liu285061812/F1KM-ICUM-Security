#include "icum_comm.h"

__IOREG(ICUM_ICPES, 0xFFFEEA00, __READ_WRITE, uint16);
__IOREG(ICUM_PE2ICUSFC, 0xFF1F0078, __WRITE, uint32);
__IOREG(ICUM_PE2ICUSIE, 0xFF1F007C, __READ_WRITE, uint32);

volatile uint32 g_icum_comm_status;

void icum_comm_init(void)
{
    g_icum_comm_status = 0u;
    ICUM_PE2ICUSFC = 1u;
    ICUM_ICPES = 0x0040u;
    ICUM_PE2ICUSIE = 1u;
}

#pragma ghs interrupt
void INTPES(void)
{
    ICUM_PE2ICUSFC = 1u;
    g_icum_comm_status ^= 1u;
}
