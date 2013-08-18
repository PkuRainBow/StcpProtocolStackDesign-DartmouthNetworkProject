
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "../common/constants.h"
#include "../topology/topology.h"
#include "routingtable.h"

//makehash()是由路由表使用的哈希函数.
//它将输入的目的节点ID作为哈希键,并返回针对这个目的节点ID的槽号作为哈希值.
int makehash(int node)
{
	return (node%MAX_ROUTINGTABLE_SLOTS);
}

//这个函数动态创建路由表.表中的所有条目都被初始化为NULL指针.
//然后对有直接链路的邻居,使用邻居本身作为下一跳节点创建路由条目,并插入到路由表中.
//该函数返回动态创建的路由表结构.
routingtable_t* routingtable_create()
{
	printf("31\n");
	
	int i;
	int neighbor_count = topology_getNbrNum();
	int *neighbor_list = topology_getNbrArray();
	routingtable_t* list = (routingtable_t*)malloc(sizeof(routingtable_t));
	//initiate the hash list..
	//hash is the head of the list
	for(i=0; i<MAX_ROUTINGTABLE_SLOTS; i++){
		list->hash[i] = (routingtable_entry_t *)malloc(sizeof(routingtable_entry_t));
		list->hash[i]->destNodeID = -1;
		list->hash[i]->nextNodeID = -1;
		list->hash[i]->next = NULL;
	}
	
	printf("32\n");
	
	for(i=0; i<neighbor_count; i++){
		int hash = makehash(neighbor_list[i]);
		routingtable_entry_t* new_node = (routingtable_entry_t *)malloc(sizeof(routingtable_entry_t));
		new_node->destNodeID = neighbor_list[i];
		new_node->nextNodeID = neighbor_list[i];
		new_node->next = NULL;
		//insert the hash node at the head
		if(list->hash[hash]->next == NULL)
		     list->hash[hash]->next = new_node;
		else{
			new_node->next = list->hash[hash]->next;
			list->hash[hash]->next = new_node;
		}
	}
	
	return list;
}

//这个函数删除路由表.
//所有为路由表动态分配的数据结构将被释放.
void routingtable_destroy(routingtable_t* routingtable)
{
	int i;
	for(i=0; i<MAX_ROUTINGTABLE_SLOTS; i++){
		while(routingtable->hash[i]->next != NULL){
			routingtable_entry_t* temp = routingtable->hash[i]->next;
			routingtable->hash[i]->next = temp->next;
			free(temp);
		}
	}
	free(routingtable);
    return;
}

//这个函数使用给定的目的节点ID和下一跳节点ID更新路由表.
//如果给定目的节点的路由条目已经存在, 就更新已存在的路由条目.如果不存在, 就添加一条.
//路由表中的每个槽包含一个路由条目链表, 这是因为可能有冲突的哈希值存在(不同的哈希键, 即目的节点ID不同, 可能有相同的哈希值, 即槽号相同).
//为在哈希表中添加一个路由条目:
//首先使用哈希函数makehash()获得这个路由条目应被保存的槽号.
//然后将路由条目附加到该槽的链表中.
void routingtable_setnextnode(routingtable_t* routingtable, int destNodeID, int nextNodeID)
{
	int hash = makehash(destNodeID);
	routingtable_entry_t* cur = routingtable->hash[hash]->next;
	while(cur != NULL){
		if(cur->destNodeID == destNodeID){
			cur->nextNodeID = nextNodeID;
			break;
		}
		cur = cur->next;
	}
	//insert at the head 
	if(cur == NULL){
		routingtable_entry_t* new_node = (routingtable_entry_t *)malloc(sizeof(routingtable_entry_t));
		new_node->destNodeID = destNodeID;
		new_node->nextNodeID = nextNodeID;
		new_node->next = routingtable->hash[hash]->next;
		routingtable->hash[hash]->next = new_node;
	}
    return;
}

//这个函数在路由表中查找指定的目标节点ID.
//为找到一个目的节点的路由条目, 你应该首先使用哈希函数makehash()获得槽号,
//然后遍历该槽中的链表以搜索路由条目.如果发现destNodeID, 就返回针对这个目的节点的下一跳节点ID, 否则返回-1.
int routingtable_getnextnode(routingtable_t* routingtable, int destNodeID)
{
	int hash = makehash(destNodeID);
	routingtable_entry_t* cur = routingtable->hash[hash]->next;
	while(cur != NULL){
		if(cur->destNodeID == destNodeID)  return cur->nextNodeID;
		cur = cur->next;
	}
    return -1;
}

//这个函数打印路由表的内容
void routingtable_print(routingtable_t* routingtable)
{
	printf("\n***********   Routing Table  ************\n");
	int i;
	printf("destNodeID\t");
	printf("nextNodeID\t");
	printf("\n");
	
	for(i=0; i<MAX_ROUTINGTABLE_SLOTS; i++){
		routingtable_entry_t* cur = routingtable->hash[i]->next;
		while(cur != NULL){
			printf("%d\t", cur->destNodeID);
			printf("%d\t", cur->nextNodeID);
			printf("\n");
			cur = cur->next;
		}	
	}
	printf("*************   END   ***************\n");
  return;
}
