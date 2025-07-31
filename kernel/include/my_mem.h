#ifndef MY_MEM_H
#define MY_MEM_H

#include "my_list.h"
#include "my_types.h"

typedef struct MEMZone
{
	myList FreeListHead;
	myList UsedListHead;
	my_uint32 StartAddr;
	my_uint32 TotalSize;
	my_uint32 RemainingSize;
}my_MemZone;

typedef struct MEMBlock
{
	myList List;
	my_uint32 Size;
}my_MEMBlock;


void My_Mem_Init(void);
void* My_Malloc(my_uint32 sz);
void My_Free(void *addr);

#endif
