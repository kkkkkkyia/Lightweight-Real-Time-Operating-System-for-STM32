#ifndef MY_SEM_H
#define MY_SEM_H

#include "my_types.h"
#include "my_list.h"

typedef struct __MY_Sem
{
	myList List;
	my_uint32 Count;
	my_uint8 Used;
}My_Sem;

typedef enum _My_SemUse
{
	MY_SEM_UNUSED = 0,
	MY_SEM_USED
}My_SemUse;


void My_SemInit(void);
my_uint32 My_SemCreate(my_uint32 * SemHandle,my_uint32 Count);
my_uint32 My_SemGet(my_uint32 SemHandle);
my_uint32 My_SemTryGet(my_uint32 SemHandle);
my_uint32 My_SemGetTimeout(my_uint32 SemHandle,int Timeout);

my_uint32 My_SemRelease(my_uint32 SemHandle);
my_uint32 My_SemDestroy(my_uint32 SemHandle);


#endif
