#ifndef ICUM_FLASH_HANDOFF_H
#define ICUM_FLASH_HANDOFF_H

#include "device.h"

void icum_flash_handoff_init(void);
int icum_flash_handoff_begin(void);
int icum_flash_handoff_end(void);
void icum_flash_handoff_on_pe_event(uint32 events);

#endif
