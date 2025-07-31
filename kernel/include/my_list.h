#ifndef MY_LIST_H
#define MY_LIST_H

typedef struct list_Head
{
	struct list_Head *pre;
	struct list_Head *next;
}myList;

#define INVALID_LIST ((myList *) 0)
/**
 *	OffsetOf 获取 MEMBER 在 TYPE中的偏移
 * 	MEMBER 结构体成员
 *  TYPE  结构体类型
 */
#define OffsetOf(TYPE,MEMBER) ((my_uint32)&(((TYPE *)0)->MEMBER))


/**
 *	ContainerOf 根据 结构体内某个成员变量的地址获取该结构体首地址
 *  ptr   该成员变量地址
 *  type  结构体类型
 */

#define ContainerOf(ptr, type, member) ( (type *)((my_uint8 *)(ptr) - OffsetOf(type, member)) )

	
#define ListEntry(ptr,type,member) ContainerOf(ptr,type,member)
#define ListFirstEntry(ptr,type,member) ListEntry((ptr)->next,type,member)
#define ListForEach(pos,head)	for(pos = (head)->next;pos!=(head);pos = pos->next)

/**
 * ListInit 对链表进行初始化
 *
 *
 */
static inline void ListInit(myList *list)
{
	list->next = list;
	list->pre = list;
}


/**
 * ListAddAfter 把 新节点 添加到原有节点的后面
 * 
 *
 */
static inline void __ListAdd(myList* addList,myList* prev,myList* next)
{
	addList->next = next;
	addList->pre = prev;
	prev->next = addList;
	next->pre = addList;
}
static inline void ListAddAfter(myList *newList, myList* nowList)
{
	__ListAdd(newList, nowList, nowList->next);
}


/**
 * ListAddBefore 添加新节点到原有节点的前面
 *
 *
 */
static inline void ListAddBefore(myList *newList,myList* nowList)
{

	__ListAdd(newList, nowList->pre, nowList);
}


/**
 *  ListDelete  删除节点 并将该节点前后节点重新连接
 *
 *
 */
static inline void ListDelete(myList* list)
{
	list->pre->next = list->next;
	list->next->pre = list->pre;
	list->pre = INVALID_LIST;
	list->next= INVALID_LIST;
}


/**
 *  ListMoveBefore  把一个原有节点 移动到 另一个节点的后面
 *
 *
 */
static inline void ListMoveBefore(myList * old, myList* head)
{
	ListDelete(old);
	ListAddBefore(old,head);
}


/**
 * ListMoveAfter 把一个原有节点 移动到 另一个节点的后面
 *
 *
 */
static inline void ListMoveAfter(myList * old,myList * head)
{
	ListDelete(old);
	ListAddAfter(old,head);
}


/**
 * ListIsFirst 判断 list 节点是否是 以head为首节点的链表 中的第一个节点
 *
 *
 */
static inline int ListIsFirst(const myList * list,const myList *head)
{
	return list->pre==head;
}

static inline int ListIsEmpty(const myList *head)
{
	return head->next == head;
}

/**
 * ListIsLast 判断 list 节点是否是 以head为首节点的链表 中的最后一个节点
 *
 *
 */
static inline int ListIsLast(const myList *list,const myList*head)
{
	return list->next==head;
}



/**
 *  GetListLast  获取 以head为头节点的链表的最后一个节点
 *
 *
 */
static inline myList * GetListLast(const myList * head)
{
	return head->pre;
}


/**
 * ListReplace 将一个链表中的原有节点用新节点代替
 *
 *
 */
static inline void ListReplace(myList* oldList,myList *newList)
{
	oldList->pre->next = newList;
	oldList->next->pre = newList;
	newList->pre = oldList->pre;
	newList->next = oldList->next;
	oldList->pre = INVALID_LIST;
	oldList->next = INVALID_LIST;
}

/**
 * ListSwap 将一个链表中的两个节点 交换位置
 *
 *
 */
static inline void ListSwap(myList * list1, myList *list2)
{
	myList * oldPos = list1->pre;
	ListDelete(list1);
	ListReplace(list2, list1);
	if(oldPos==list2)
		oldPos = list1;
	ListAddAfter(list2, oldPos);
}

/** 
 *
 *
 *
*/
static inline void ListMoveTail(myList *list, myList *head)
{
	ListDelete(list);
	ListAddBefore(list, head);
}
	
#endif
