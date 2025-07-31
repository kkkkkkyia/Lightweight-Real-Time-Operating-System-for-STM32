#include <stdio.h>
#include "my_mem.h"

typedef struct datas_t
{
	char name[10];
	int len;
	int nums[10];
}datas;
int main()
{
	datas* data1 =(datas *) MY_Malloc(sizeof(datas));
	datas* data2 =(datas *) MY_Malloc(sizeof(datas));
	datas* data3 =(datas *) MY_Malloc(sizeof(datas));
	data2->name = "data2";
	datas->nums[1] = 10;
	datas->name = "data3";

	printf("data2-name:%s, data3-name:%s\r\n",data2->name,data3->name);
	MY_Free(data1);
	MY_Free(data2);
	MY_Free(data3);
	
}
