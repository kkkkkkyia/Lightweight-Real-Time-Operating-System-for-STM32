#ifndef MY_TRACE_H
#define MY_TRACE_H
#include <stdio.h>

#ifdef MY_TRACE_MemInit
	#define MY_TRACE_Memory_Init(MemZone) printf("<MY_TRACE> MEMZone : StartAddr(0x%X),TOTALSIZE(0x%X)\r\n",MEMZone.StartAddr,MEMZone.TotalSize); 
#else
	#define MY_TRACE_Memory_Init(MemZone)
#endif


#endif