#ifndef MY_ARCH_H
#define MY_ARCH_H
#include "my_types.h"

my_uint8 ATCH_IsInterruptContext(void);

void * ARCH_PrepareStack(void* StartOfstack, void * Param);
#endif

