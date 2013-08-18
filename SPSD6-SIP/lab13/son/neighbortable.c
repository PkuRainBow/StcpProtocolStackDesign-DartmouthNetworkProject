//文件名: son/neighbortable.c
//
//描述: 这个文件实现用于邻居表的API
//
//创建日期: 2013年1月

#include "neighbortable.h"
#include "../topology/topology.h"

//这个函数首先动态创建一个邻居表. 然后解析文件topology/topology.dat, 填充所有条目中的nodeID和nodeIP字段, 将conn字段初始化为-1.
//返回创建的邻居表.
nbr_entry_t* nt_create()
{
	char hname[HOSTNAMESIZE];
    gethostname(hname, sizeof(hname));

    printf("the host name is %s\n", hname);
	int NbrCount = 0;
	FILE *fp;
	if((fp=fopen("../topology/topology.dat","r"))==NULL) {
		printf("cannot open this file\n");
		exit(1);
	}
	
	NbrCount = topology_getNbrNum();
	char host[MAXHOST][20], distance[20];	
	nbr_entry_t*  head;
	head = (nbr_entry_t*)malloc(NbrCount*sizeof(nbr_entry_t));
		
	int i=0;
	int index=0;
    while(fscanf(fp, "%s", host[i++]) > 0){
		//printf("host[%d]is  %s \n",i-1, host[i-1]);
		fscanf(fp, "%s", host[i++]);
		fscanf(fp, "%s", distance);
		if(strncmp(host[i-1], hname, 20) == 0){
			head[index].nodeID = topology_getNodeIDfromname(host[i-2]);
			head[index].nodeIP = getNodeIPfromname(host[i-2]);
			head[index].conn = -1;
			index++;
		}
	    if(strncmp(host[i-2], hname, 20) == 0){
			head[index].nodeID = topology_getNodeIDfromname(host[i-1]);
			head[index].nodeIP = getNodeIPfromname(host[i-1]);
			head[index].conn = -1;
			index++;
		}
	}
	printf("finish the nt_create ...\n");
	
	//print the route table
	printf("\n\n********   The Route table  ********\n");
	//struct in_addr in;
	//in.s_addr = head[index].nodeIP;
	for(i=0; i<NbrCount; i++){
		printf("nodeID: %d -- nodeIP: %lu -- conn:%d\n", head[i].nodeID, (unsigned long)head[i].nodeIP, head[i].conn);
	}
	printf("***************  END  **************\n\n");
	return head;
}


//这个函数删除一个邻居表. 它关闭所有连接, 释放所有动态分配的内存.
void nt_destroy(nbr_entry_t* nt){
	int NbrCount = topology_getNbrNum();
	int i;
	for(i=0; i<NbrCount; i++){
		close(nt[i].conn);
	}
	free(nt);
}


//这个函数为邻居表中指定的邻居节点条目分配一个TCP连接. 如果分配成功, 返回1, 否则返回-1.
int nt_addconn(nbr_entry_t* nt, int nodeID, int conn)
{
	int NbrCount = topology_getNbrNum();
	int i;
	for(i=0; i<NbrCount; i++){
		if(nt[i].nodeID==nodeID){
			nt[i].conn = conn;
			return 1;
	    }
	}
	return -1;
}
