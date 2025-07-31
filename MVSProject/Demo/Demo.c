// Demo.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <stdio.h>
#include "my_mem.h"
#include <string.h>
typedef struct datas_t
{
	char name[10];
	int len;
	int nums[10];
}datas;


int main()
{
	datas* data1;
	datas* data2;
	datas* data3;
	My_Mem_Init();
	data1 = (datas *)My_Malloc(sizeof(datas));
	data2 = (datas *)My_Malloc(sizeof(datas));
	data3 = (datas *)My_Malloc(sizeof(datas));
	strcpy_s(data2->name,10,"data2");
	data2->len = 3;
	data2->nums[1] = 9;

	printf("data2-name:%s, data3-name:%s\r\n", data2->name, data3->name);
	My_Free(data1);
	My_Free(data2);
	My_Free(data3);
	printf("test over!\n");
	data1 = (datas *)My_Malloc(sizeof(datas));
	data2 = (datas *)My_Malloc(sizeof(datas));
	data3 = (datas *)My_Malloc(sizeof(datas));
	strcpy_s(data2->name, 10, "data2");
	data2->len = 3;
	data2->nums[1] = 9;

	printf("data2-name:%s, data3-name:%s\r\n", data2->name, data3->name);
	My_Free(data1);
	My_Free(data2);
	My_Free(data3);
	printf("test over!\n");
	data3 = (datas *)My_Malloc(sizeof(datas));
	data1 = (datas *)My_Malloc(sizeof(datas));
	data2 = (datas *)My_Malloc(sizeof(datas));
	strcpy_s(data2->name, 10, "data2");
	data2->len = 3;
	data2->nums[1] = 9;

	printf("data2-name:%s, data3-name:%s\r\n", data2->name, data3->name);
	My_Free(data1);
	My_Free(data2);
	My_Free(data3);
	printf("test over!\n");
	data2 = (datas *)My_Malloc(sizeof(datas));
	data1 = (datas *)My_Malloc(sizeof(datas));
	data3 = (datas *)My_Malloc(sizeof(datas));
	strcpy_s(data2->name, 10, "data2");
	data2->len = 3;
	data2->nums[1] = 9;

	printf("data2-name:%s, data3-name:%s\r\n", data2->name, data3->name);
	My_Free(data1);
	My_Free(data2);
	My_Free(data3);
	printf("test over!\n");
	return 0;
}
