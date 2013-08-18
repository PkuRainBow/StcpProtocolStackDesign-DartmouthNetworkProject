
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "../common/constants.h"
#include "../topology/topology.h"
#include "dvtable.h"

//这个函数动态创建距离矢量表.
//距离矢量表包含n+1个条目, 其中n是这个节点的邻居数,剩下1个是这个节点本身.
//距离矢量表中的每个条目是一个dv_t结构,它包含一个源节点ID和一个有N个dv_entry_t结构的数组, 其中N是重叠网络中节点总数.
//每个dv_entry_t包含一个目的节点地址和从该源节点到该目的节点的链路代价.
//距离矢量表也在这个函数中初始化.从这个节点到其邻居的链路代价使用提取自topology.dat文件中的直接链路代价初始化.
//其他链路代价被初始化为INFINITE_COST.
//该函数返回动态创建的距离矢量表.
dv_t* dvtable_create()
{
	int hostID = topology_getMyNodeID();
	int neighbor_count = topology_getNbrNum();
	int all_count = topology_getNodeNum();
	
	printf("all_count is %d\n", all_count);
	
	dv_t *dis_vector = (dv_t *)malloc((neighbor_count+1)*sizeof(dv_t));
	//the neighbor_list store the neighbor ID
	//the all_list store all the ID
	int *neighbor_list, *all_list;
	int i, j;
	neighbor_list = topology_getNbrArray();
	all_list = topology_getNodeArray();
	
	printf("getNodeArray ok ...\n");
	
	//store the local distance_vector
	dis_vector[0].dvEntry = (dv_entry_t*)malloc(all_count * sizeof(dv_entry_t));
	for(i=0; i<all_count; i++){
		dis_vector[0].nodeID = hostID;
		dis_vector[0].dvEntry[i].nodeID = all_list[i];
		dis_vector[0].dvEntry[i].cost = topology_getCost(hostID, all_list[i]);
	}
	
	printf("dis_vector[0] assign ok...\n");
	//init the srcID
	//alloc space for the dv_Entry
	for(i=0; i<neighbor_count; i++){
		dis_vector[i+1].nodeID = neighbor_list[i];
		dis_vector[i+1].dvEntry = (dv_entry_t*)malloc(all_count * sizeof(dv_entry_t));
		for(j=0; j<all_count; j++){
			dis_vector[i+1].dvEntry[j].nodeID = all_list[j];
			dis_vector[i+1].dvEntry[j].cost = INFINITE_COST;	
		}
	}
	
	//printf("dis_vector[i] assign ok...\n");
	return dis_vector;
}

//这个函数删除距离矢量表.
//它释放所有为距离矢量表动态分配的内存.
void dvtable_destroy(dv_t* dvtable)
{
	int neighbor_count = topology_getNbrNum();
	int i;
	for(i=0; i<neighbor_count+1; i++){
		free(dvtable[i].dvEntry);
	}
	free(dvtable);
	//printf("free the dvtable ok ...\n");
  return;
}

//这个函数设置距离矢量表中2个节点之间的链路代价.
//如果这2个节点在表中发现了,并且链路代价也被成功设置了,就返回1,否则返回-1.
int dvtable_setcost(dv_t* dvtable,int fromNodeID,int toNodeID, unsigned int cost)
{
	int neighbor_count = topology_getNbrNum();
	int all_count = topology_getNodeNum();
	int i, j;
	for(i=0; i<neighbor_count+1; i++){
		if(dvtable[i].nodeID == fromNodeID)  break;
	}
	for(j=0; j<all_count; i++){
		if(dvtable[i].dvEntry[j].nodeID == toNodeID){
			//find it ...
			dvtable[i].dvEntry[j].cost = cost;
			return 1;
		}
	}	
    return -1;
}

//这个函数返回距离矢量表中2个节点之间的链路代价.
//如果这2个节点在表中发现了,就返回链路代价,否则返回INFINITE_COST.
unsigned int dvtable_getcost(dv_t* dvtable, int fromNodeID, int toNodeID)
{
	int neighbor_count = topology_getNbrNum();
	int all_count = topology_getNodeNum();
	int i, j;
	for(i=0; i<neighbor_count+1; i++){
		if(dvtable[i].nodeID == fromNodeID)  break;
	}
	for(j=0; j<all_count; i++){
		if(dvtable[i].dvEntry[j].nodeID == toNodeID){
			return dvtable[i].dvEntry[j].cost;
		}
	}
	return INFINITE_COST;
}

//这个函数打印距离矢量表的内容.
void dvtable_print(dv_t* dvtable)
{
	int neighbor_count = topology_getNbrNum();
	int all_count = topology_getNodeNum();
	
	int i, j;
	printf("\n*********  Distance Vector Table  *********\n");
	printf("\t");
	//print all the csnetlab* in the net work
	for(i=0; i<all_count; i++){
		printf("%d\t", dvtable[0].dvEntry[i].nodeID);
	}
	printf("\n");
	//printf the vector from the host ID
	printf("%d\t", dvtable[0].nodeID);
	for(i=0; i<all_count; i++){
		printf("%d\t", dvtable[0].dvEntry[i].cost);
	}
	printf("\n");
	for(i=1; i<neighbor_count+1; i++){
		printf("%d\t", dvtable[i].nodeID);
		for(j=0; j<all_count; j++){
			printf("%d\t", dvtable[i].dvEntry[j].cost);
		}
		printf("\n");	
	}
	printf("*********          END        *********\n\n");
    return;
}
