#include "my_mem.h"
#include "my_config.h"
#include "my_list.h"
#include "my_lib.h"
#include "my_printk.h"
#include "my_critical.h"
#include "my_arch.h"

static const my_uint32 MyMemBlkAlignSize = (sizeof(my_MEMBlock) +(my_uint32)(MY_BYTE_ALIGNMENT -1))&(~((my_uint32)(MY_BYTE_ALIGNMENT-1)));
#define MY_MEM_MIN_BLOCK_SZ  (MyMemBlkAlignSize<<1)
static int My_Mem_Init_Finished = 0;
MEM_STATIC_SPACE my_MemZone MemZone;
MEM_STATIC_SPACE my_uint8 My_MEM_Heap[MY_CONFIG_TOTAL_MEM_SIZE];

/**
 * My_Mem_Init  使用内存管理前进行初始化，填充一些全局数据例如 MemZone
 *
 */
void My_Mem_Init(void)
{
	
		
	my_MEMBlock* FirstFreeList = MY_NULL;
	MY_PRINTK_INFO("MY MEM Init Start..");
	MemZone.StartAddr = MY_DataAlign((my_uint32)My_MEM_Heap, MY_BYTE_ALIGNMENT,MY_BYTE_ALIGNMENT_MASK);
	MemZone.TotalSize = MY_CONFIG_TOTAL_MEM_SIZE-(MemZone.StartAddr-(my_uint32)My_MEM_Heap);
	MemZone.RemainingSize = MemZone.TotalSize;

	ListInit(&MemZone.FreeListHead);
	ListInit(&MemZone.UsedListHead);

	FirstFreeList = (my_MEMBlock *) MemZone.StartAddr;
	FirstFreeList->Size = MemZone.RemainingSize;
	ListAddAfter(&FirstFreeList->List, &(MemZone.FreeListHead));
  My_Mem_Init_Finished  = 1;
	MY_PRINTK_INFO("MY MEM Init Finished!..");
	//MY_TRACE_Memory_Init(MEMZone);
	MY_PRINTK_INFO("Total memory : 0x%08X Bytes, Address at 0x%08X", MemZone.TotalSize, MemZone.StartAddr);
}

/**
 * MY_InsertMemBlockToFreeList 
 * 将一个MemBlock 内存块重新放入空闲链表中，按照内存块从小到大的顺序插入
 * 
 */
static void MY_InsertMemBlockToFreeList(myList * InsertList)
{
	myList * ListIterator;
	my_MEMBlock * MemBlkIterator;
	my_MEMBlock * WaitForInsert = (my_MEMBlock *)InsertList;
	ListForEach(ListIterator,&MemZone.FreeListHead)
	{
		MemBlkIterator = (my_MEMBlock *)ListIterator;
		if(MemBlkIterator->Size> WaitForInsert->Size)
		break;
	}
	if(ListIterator == &MemZone.FreeListHead)
	{
		ListAddBefore(InsertList,&MemZone.FreeListHead);
	}
	else
	{
		ListAddBefore(InsertList,ListIterator);
	}
}



void* My_Malloc(my_uint32 wantSize)
{
	
	if(My_Mem_Init_Finished==0)
	{
		MY_PRINTK_ERROR("Please Called My_Mem_Init Before Useing My_Malloc Function");
		return (void *)0;
	}
	void * pReturnAddr = MY_NULL;

	my_uint32 RequestSize = 0;
	myList * ListPos;
	my_MEMBlock *NewFreeMemBlk = MY_NULL;
	my_MEMBlock *AlloctedMemBlk = MY_NULL;
	MY_ENTER_CRITICAL();
	if(wantSize>0)
	{
		RequestSize = (wantSize+MyMemBlkAlignSize);
		RequestSize = MY_DataAlign(RequestSize, MY_BYTE_ALIGNMENT,MY_BYTE_ALIGNMENT_MASK);
		if(RequestSize<=MemZone.RemainingSize)
		{
			ListForEach(ListPos,&MemZone.FreeListHead)
			{
				AlloctedMemBlk = (my_MEMBlock *)ListPos;
				if(AlloctedMemBlk->Size>=RequestSize)
				{
					pReturnAddr = (void *)((my_uint8 *)AlloctedMemBlk+MyMemBlkAlignSize);
					ListDelete(&AlloctedMemBlk->List);
					ListAddAfter(&AlloctedMemBlk->List,&MemZone.UsedListHead);
					if(AlloctedMemBlk->Size-RequestSize >= MY_MEM_MIN_BLOCK_SZ)
					{
						NewFreeMemBlk = (my_MEMBlock *)((my_uint8 *)AlloctedMemBlk+RequestSize);
						NewFreeMemBlk->Size = AlloctedMemBlk->Size-RequestSize;
						AlloctedMemBlk->Size = RequestSize;
						ListAddAfter(&NewFreeMemBlk->List,&MemZone.FreeListHead);
					}
					MemZone.RemainingSize = MemZone.RemainingSize-AlloctedMemBlk->Size;
					break;
				}
			}
		}
		else
		{
			MY_PRINTK_WARNING("Total Memory Size not enough for Malloc");
		}
	}
	MY_EXIT_CRITICAL();
	return pReturnAddr;
}


/**
 * MY_MergeMemBlock  将list 所在MemBlock 与 空闲链表中相邻的内存块合并 
 * list 需要合并的MemBlock 的list 该list 已经与前后链表断开连接不用再调用ListDelete 
 *
 */
static void MY_MergeMemBlock(myList *list)
{
	my_MEMBlock * WaitFroMerge = (my_MEMBlock *)list;
	myList * ListIterator = MY_NULL;
	my_MEMBlock * MemBlk = MY_NULL;
	my_MEMBlock * InsertMemBlk = WaitFroMerge;
	ListForEach(ListIterator, &MemZone.FreeListHead)
	{
		MemBlk = (my_MEMBlock *) ListIterator;
		if((my_uint32)MemBlk+MemBlk->Size == (my_uint32)WaitFroMerge)
		{
			 InsertMemBlk = MemBlk;
			 InsertMemBlk->Size +=WaitFroMerge->Size;
			 ListDelete(&MemBlk->List);
			 MY_MergeMemBlock(&InsertMemBlk->List);  
			 return;
		}
		if((my_uint32)WaitFroMerge+WaitFroMerge->Size==(my_uint32)MemBlk)
		{
			 InsertMemBlk = WaitFroMerge;
			 InsertMemBlk->Size +=MemBlk->Size;
			 ListDelete(&MemBlk->List);
			 MY_MergeMemBlock(&InsertMemBlk->List);
			 return;
		}
	}
	MY_InsertMemBlockToFreeList(&InsertMemBlk->List);
	
}

/**
 * My_Free  释放之前使用 My_Malloc 分配的内存块
 *
 */
void My_Free(void *addr)
{
	
	if(My_Mem_Init_Finished==0)
	{
		MY_PRINTK_ERROR("Please Called My_Mem_Init Before Useing My_Free Function");
		return ;
	}
	my_MEMBlock *UsedMemBlk =MY_NULL;
	
	myList * ListIterator;
	MY_ENTER_CRITICAL();
	if(addr!=MY_NULL)
	{
		UsedMemBlk = (my_MEMBlock *)((my_uint8 *)addr - MyMemBlkAlignSize);
		ListForEach(ListIterator, &MemZone.UsedListHead)
		{
			
			if(ListIterator==&(UsedMemBlk->List))
			{
				break;
			}
		}
		ListDelete(ListIterator);
		MY_MergeMemBlock(&UsedMemBlk->List);
	}
	MY_EXIT_CRITICAL();
}

