#ifndef SHARED_MEMORY_H
#define SHARED_MEMORY_H

#define SHARED_BUFFER_SIZE 128u
#define SHARED_MEMORY_BASE 0xFEFF8000u

#define g_shared_buffer ((volatile unsigned char *)SHARED_MEMORY_BASE)

#endif
