
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "nbrcosttable.h"
#include "../common/constants.h"
#include "../topology/topology.h"

//这个函数动态创建邻居代价表并使用邻居节点ID和直接链路代价初始化该表.
//邻居的节点ID和直接链路代价提取自文件topology.dat. 
nbr_cost_entry_t* nbrcosttable_create()
{
	int i;
	int hostID = topology_getMyNodeID();
	int neighbor_count = topology_getNbrNum();
	int *neighbor_list;
	//printf("11\n");
	neighbor_list = topology_getNbrArray();
	/*
	 * bug may be there
	 */
	nbr_cost_entry_t* list = (nbr_cost_entry_t*)malloc(neighbor_count * sizeof(nbr_cost_entry_t));
	for(i=0; i<neighbor_count; i++){
		list[i].nodeID = neighbor_list[i];
		list[i].cost = topology_getCost(hostID, list[i].nodeID);
		//printf("11：%d\n", i);
	}	
	return list;
}

//这个函数删除邻居代价表.
//它释放所有用于邻居代价表的动态分配内存.
void nbrcosttable_destroy(nbr_cost_entry_t* nct)
{
	free(nct);
}

//这个函数用于获取邻居的直接链路代价.
//如果邻居节点在表中发现,就返回直接链路代价.否则返回INFINITE_COST.
unsigned int nbrcosttable_getcost(nbr_cost_entry_t* nct, int nodeID)
{
	int i;
	int neighbor_count = topology_getNbrNum();
	for(i=0; i<neighbor_count; i++){
		if(nct[i].nodeID == nodeID)  return nct[i].cost;
	}
    return INFINITE_COST;
}

//这个函数打印邻居代价表的内容.
void nbrcosttable_print(nbr_cost_entry_t* nct)
{
	int i;
	int hostID = topology_getMyNodeID(); 
	int neighbor_count = topology_getNbrNum();
	printf("\n***********    Neighbor Cost Table    *************\n");
	printf("\t");
	for(i=0; i<neighbor_count; i++){
		printf("%d\t", nct[i].nodeID);
	}
	printf("\n");
	
	printf("%d\t", hostID);
	for(i=0; i<neighbor_count; i++){
		printf("%d\t", nct[i].cost);
	}	
	printf("\n");
    printf("*******************     END     *********************\n\n");
    return;
}
