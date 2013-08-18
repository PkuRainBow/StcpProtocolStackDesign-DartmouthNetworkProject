//文件名: topology/topology.c
//
//描述: 这个文件实现一些用于解析拓扑文件的辅助函数 
//
//创建日期: 2013年1月

#include "topology.h"

//这个函数返回指定主机的节点ID.
//节点ID是节点IP地址最后8位表示的整数.
//例如, 一个节点的IP地址为202.119.32.12, 它的节点ID就是12.
//如果不能获取节点ID, 返回-1.
in_addr_t getNodeIPfromname(char *hostname){
	char *temp;
	struct hostent *hent;
	hent = gethostbyname(hostname);
	//temp is ip string
	temp = (char *)inet_ntoa(*((struct in_addr *)hent->h_addr_list[0]));
	printf("*******get node ip is :%s\n", temp);
	printf("node ip is : %lu\n", (unsigned long)inet_addr(temp));
	return  inet_addr(temp);
}

int topology_getNodeIDfromname(char* hostname) 
{
  char *buf;
	struct hostent *hent;
	hent = gethostbyname(hostname);
	//temp is ip string
	buf = (char *)inet_ntoa(*((struct in_addr *)hent->h_addr_list[0]));
	char *temp;
	char *delim = ".";
	temp = strtok(buf, delim);
	temp = strtok(NULL, delim);
	temp = strtok(NULL, delim);
	temp = strtok(NULL, delim);
	if(temp==NULL)  return -1;
	int ID = atoi(temp);
	//debug
	//printf("the ID is %d\n", ID);
	return ID;
}

//这个函数返回指定的IP地址的节点ID.
//如果不能获取节点ID, 返回-1.
int topology_getNodeIDfromip(struct in_addr* addr)
{	
	char *buf;
	char *temp;
	char *delim = ".";
	//use inet_ntoa()
	buf = (char *)inet_ntoa(*addr);
	temp = strtok(buf, delim);
	temp = strtok(NULL, delim);
	temp = strtok(NULL, delim);
	temp = strtok(NULL, delim);
	if(temp==NULL)  return -1;
	int ID = atoi(temp);
	return ID;
}

//这个函数返回本机的节点ID
//如果不能获取本机的节点ID, 返回-1.
int topology_getMyNodeID()
{
	char hname[HOSTNAMESIZE];
	char *temp;
	char *buf;
	char *delim = ".";
	struct hostent *hent;
	gethostname(hname, sizeof(hname));
	hent = gethostbyname(hname);
	//inet_ntoa(*((struct in_addr *)h->h_addr_list[i]))
	buf = inet_ntoa(*((struct in_addr *)hent->h_addr_list[0]));
  temp = strtok(buf, delim);
	temp = strtok(NULL, delim);
	temp = strtok(NULL, delim);
	temp = strtok(NULL, delim);
	if(temp==NULL)   return -1;
	int ID = atoi(temp);
	return ID;
}

//这个函数解析保存在文件topology.dat中的拓扑信息.
//返回邻居数.
int topology_getNbrNum()
{
	char hname[HOSTNAMESIZE];
    gethostname(hname, sizeof(hname));
    
    /********* bug ************/
    //init the int variable
    int Count=0;
    //debug
    //printf("host name is %s\n", hname);
    
    FILE *fp;
	  if((fp=fopen("../topology/topology.dat","r"))==NULL) {
		   printf("cannot open this file\n");
		   exit(1);
	  }
	//printf("open the topology.dat sucess...\n");
	char host[20];	
  while( fscanf(fp, "%s", host) > 0){
  	//printf("***** %s\n", host);
		if(strncmp(hname, host, 20)==0)   {		
			Count++;
		}
	  memset(host, 0, 20);
	}	
	fclose(fp);	
	return Count;
}

//这个函数解析保存在文件topology.dat中的拓扑信息.
//返回重叠网络中的总节点数.
//dele the replete element
int find(char a[][20], char *dst, int k){
	int i;
	for(i=0; i<k; i++){
		if(strncmp(dst, a[i], 20)==0){
			return 1;
		}
	}
	return -1;
}

int topology_getNodeNum()
{
	int NodeNum;
	FILE *fp;
	if((fp=fopen("../topology/topology.dat","r"))==NULL) {
		printf("cannot open this file\n");
		exit(1);
	}
	//printf("open the topology.dat sucess...\n");
	
	char host[MAXHOST][20], distance[20];
	int i=0;
  while(fscanf(fp, "%s", host[i++])> 0){
		if(find(host, host[i-1], i-1) == 1)  { i--;}
		fscanf(fp, "%s", host[i++]);
		if(find(host, host[i-1], i-1) == 1)  { i--;}
		fscanf(fp, "%s", distance);
	}
	NodeNum = i-1;
	fclose(fp);	
    return NodeNum;
}

//这个函数解析保存在文件topology.dat中的拓扑信息.
//返回一个动态分配的数组, 它包含重叠网络中所有节点的ID. 
int* topology_getNodeArray()
{
	int NodeNum;
	FILE *fp;
	if((fp=fopen("../topology/topology.dat","r"))==NULL) {
		printf("cannot open this file\n");
		exit(1);
	}
	//printf("open the topology.dat sucess...\n");
	char host[MAXHOST][20], distance[20];
	int i=0;
  while(fscanf(fp, "%s", host[i++]) > 0){  	
		if(find(host, host[i-1], i-1) == 1)  { i--;}
		fscanf(fp, "%s", host[i++]);
		if(find(host, host[i-1], i-1) == 1)  { i--;}
		fscanf(fp, "%s", distance);
	}
	
	NodeNum = i-1;
	
	int *ID = malloc(NodeNum*sizeof(int));
	
	printf("start to get the ID..\n");
	printf("all node NUM is %d\n", NodeNum);
	
	for(i=0; i<NodeNum; i++){
		ID[i] = topology_getNodeIDfromname(host[i]);	
		//printf("---%d\n", i);	
	}
	return ID;
}

//这个函数解析保存在文件topology.dat中的拓扑信息.
//返回一个动态分配的数组, 它包含所有邻居的节点ID.  
int* topology_getNbrArray()
{
	char hname[HOSTNAMESIZE];
    gethostname(hname, sizeof(hname));
	int NbrCount = 0;
	FILE *fp;
	if((fp=fopen("../topology/topology.dat","r"))==NULL) {
		printf("cannot open this file\n");
		exit(1);
	}
	
	//printf("open the topology.dat sucess...\n");
	
	NbrCount = topology_getNbrNum();
	char host[MAXHOST][20], distance[20];
	int* ID	= malloc(NbrCount*sizeof(int));
	int count=0;
	int i=0;
    while(fscanf(fp, "%s", host[i++]) > 0){
		fscanf(fp, "%s", host[i++]);
		fscanf(fp, "%s", distance);
		if(strncmp(host[i-1], hname, 20) == 0){
			ID[count++] = topology_getNodeIDfromname(host[i-2]);
		}
	    if(strncmp(host[i-2], hname, 20) == 0){
			ID[count++] = topology_getNodeIDfromname(host[i-1]);
		}	
	}
	fclose(fp);	
    return ID;
}
//de dao bi ben di ID da de zhu ji de ID de shumu
int getBigIDnum(){
	int count = 0;
	char hname[HOSTNAMESIZE];
    gethostname(hname, sizeof(hname));
    int hostID = topology_getMyNodeID();
	FILE *fp;
	if((fp=fopen("../topology/topology.dat","r"))==NULL) {
		printf("cannot open this file\n");
		exit(1);
	}
	
	//printf("open the topology.dat sucess...\n");
	
	char host[MAXHOST][20], distance[20];
	
	int i=0;
    while(fscanf(fp, "%s", host[i++]) >0 ){
		fscanf(fp, "%s", host[i++]);
		fscanf(fp, "%s", distance);
		if(strncmp(host[i-1], hname, 20) == 0){
			if(topology_getNodeIDfromname(host[i-2]) > hostID)
			    count++;
		}
	    if(strncmp(host[i-2], hname, 20) == 0){
			if(topology_getNodeIDfromname(host[i-1]) > hostID)
			    count++;
		}
	}
	fclose(fp);	
	return count;
}

//这个函数解析保存在文件topology.dat中的拓扑信息.
//返回指定两个节点之间的直接链路代价. 
//如果指定两个节点之间没有直接链路, 返回INFINITE_COST.
unsigned int topology_getCost(int fromNodeID, int toNodeID)
{
	return 0;
}
