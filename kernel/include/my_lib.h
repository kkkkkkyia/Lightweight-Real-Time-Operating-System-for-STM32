#ifndef MY_LIB_H
#define MY_LIB_H
	
#include "my_types.h"


/**
 * MY_DataAlign 将 数据data 按照一定规则进行字节对齐
 * data 一般为要对齐的地址
 * align 对齐长度 例如 8、16、32
 * mask 对align 对应的掩码 一般为 （align-1）
 */

static inline my_uint32 MY_DataAlign(my_uint32 data, my_uint32 align, my_uint32 mask)
{
	if(data & mask)
	{
		data &= ~mask;
		data+=align;
	}
	return data;
}


static int My_MemCopy(void *des, void *src, my_uint32 size)
{
	if (des == MY_NULL || src == MY_NULL)return 0;
	my_uint8* desAddr = (my_uint8 *)des;
	my_uint8* srcAddr = (my_uint8 *)src;
	my_uint8 data;

	while (size-- > 0)
	{
		data = *(srcAddr++);
		*(desAddr++) = data;
	}
	return size;
}

void inline My_MemSet(void *addr, my_uint8 data, my_uint32 size)
{
	my_uint8 * startAddr = (my_uint8 *)addr;
	while (size-- > 0)
		*startAddr = data;
}


void inline My_MemClear(void *addr, my_uint32 size)
{
	my_uint8 * startAddr = (my_uint8 *)addr;
	while (size-- > 0)
		*startAddr = 0;
}

#endif
