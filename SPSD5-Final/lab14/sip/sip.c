//文件名: sip/sip.c
//
//描述: 这个文件实现SIP进程  
//
//创建日期: 2013年1月

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <strings.h>
#include <arpa/inet.h>
#include <signal.h>
#include <netdb.h>
#include <assert.h>
#include <sys/utsname.h>
#include <pthread.h>
#include <unistd.h>

#include "../common/constants.h"
#include "../common/pkt.h"
#include "../common/seg.h"
#include "../topology/topology.h"
#include "sip.h"
#include "nbrcosttable.h"
#include "dvtable.h"
#include "routingtable.h"

//SIP层等待这段时间让SIP路由协议建立路由路径. 
#define SIP_WAITTIME 60

/**************************************************************/
//声明全局变量
/**************************************************************/
int son_conn; 			//到重叠网络的连接
int stcp_conn;			//到STCP的连接
nbr_cost_entry_t* nct;			//邻居代价表
dv_t* dv;				//距离矢量表
pthread_mutex_t* dv_mutex;		//距离矢量表互斥量
routingtable_t* routingtable;		//路由表
pthread_mutex_t* routingtable_mutex;	//路由表互斥量

/**************************************************************/
//实现SIP的函数
/**************************************************************/

//SIP进程使用这个函数连接到本地SON进程的端口SON_PORT.
//成功时返回连接描述符, 否则返回-1.
int connectToSON() { 
	int sockfd;
	struct sockaddr_in servaddr;
	if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0){
		perror("Problem in creating the socket ...\n");
		exit(2);
	}
	//in_addr_t hostaddr = get
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	servaddr.sin_port = htons(SON_PORT);
	
	if(connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) < 0){
		perror("Problem in connecting to the server ...\n");
		exit(3);
	}	
	printf("connect to the local server successfully ... \n");
	return sockfd;	
}

//这个线程每隔ROUTEUPDATE_INTERVAL时间发送路由更新报文.路由更新报文包含这个节点的距离矢量.
//广播是通过设置SIP报文头中的dest_nodeID为BROADCAST_NODEID,并通过son_sendpkt()发送报文来完成的.
void* routeupdate_daemon(void* arg) {
	//if(Y_DEBUG)
    printf("debug : SIP.C routeupdate_daemon()...\n");
      
	int j;
	int index = 0;
	int hostID = topology_getMyNodeID();
	//int neighbor_count = topology_getNbrNum();
	int all_count = topology_getNodeNum();
	//int size_of_data = (neighbor_count+1)*all_count*sizeof(int)*2 + sizeof(int);
	int size_of_data = all_count*sizeof(int)*2 + sizeof(int);
	sip_pkt_t temp;
	temp.header.src_nodeID = hostID;
	temp.header.dest_nodeID = BROADCAST_NODEID;
	temp.header.length = size_of_data;
	temp.header.type = ROUTE_UPDATE;
	//fill in the info into the data field
	pkt_routeupdate_t *data = (pkt_routeupdate_t *)malloc(sizeof(pkt_routeupdate_t));
	data->entryNum = all_count;
	while(1){	
			for(j=0; j<all_count; j++){		
				data->entry[index].nodeID = dv[0].dvEntry[j].nodeID;
				data->entry[index].cost = dv[0].dvEntry[j].cost;
				index++;
			}
			memcpy(temp.data, data, sizeof(pkt_routeupdate_t));	 
			son_sendpkt(BROADCAST_NODEID, &temp, son_conn);
			sleep(5);
	}
}

//这个线程处理来自SON进程的进入报文. 它通过调用son_recvpkt()接收来自SON进程的报文.
//如果报文是SIP报文,并且目的节点就是本节点,就转发报文给STCP进程. 如果目的节点不是本节点,
//就根据路由表转发报文给下一跳.如果报文是路由更新报文,就更新距离矢量表和路由表.
void* pkthandler(void* arg) {  
	sip_pkt_t pkt;
	int hostID = topology_getMyNodeID();
	int neighbor_count = topology_getNbrNum();
	int all_count = topology_getNodeNum();	
	int *all_list = topology_getNodeArray();
	int i, j;
		
	while(son_recvpkt(&pkt, son_conn)>0) {
		//printf("id:%d->%d port%d\n", pkt.header.src_nodeID, pkt.header.dest_nodeID, ((seg_t *)(pkt.data))->header.src_port);		
		if(pkt.header.type==SIP){
			int srcID = pkt.header.src_nodeID;
			int destID = pkt.header.dest_nodeID;
			/*  case 1  */
			if(destID == hostID){
				printf("case1 ...\n");	
        seg_t *seg = malloc(sizeof(seg_t));
        memcpy(seg, &(pkt.data), sizeof(seg_t)); 
				forwardsegToSTCP(stcp_conn, srcID, seg);			
			}
			/*  case 2  */
			else{
				pthread_mutex_lock(routingtable_mutex);
				int nextID = routingtable_getnextnode(routingtable, destID);
        pthread_mutex_unlock(routingtable_mutex);
				if(nextID != -1)  {
					printf("case2  ...\n");
					son_sendpkt(nextID, &pkt, son_conn);
				}
				else {
					printf("fail to get the nextID info ...\n");
				}
			}
		}
		/*  case 3  */
		//route update info
		else{
			int srcID = pkt.header.src_nodeID;
			//lock the cri..
			pthread_mutex_lock(dv_mutex);
			pthread_mutex_lock(routingtable_mutex);
			//update the distance_vector
			//part 0
			/* update the line that the srcnode ID is the srcID */
			for(i=0; i<neighbor_count+1; i++){
				if(dv[i].nodeID == srcID)  break;
			}
			
			for(j=0; j<all_count; j++){
				dv[i].dvEntry[j].cost = ((pkt_routeupdate_t *)(&(pkt.data)))->entry[j].cost;				
			}
			//part  1 
			/* update the line 0 that the srcnode ID is the hostID */
			/* fetch the current distance of the srcID and the hostID  */
			
			int temp_distance = nbrcosttable_getcost(nct, srcID);
			//part  2
			/*  update the distance of the line 0 by compare the distance through srcID  */
			for(i=0; i<all_count; i++){
				if(dv[0].dvEntry[i].cost > (temp_distance + ((pkt_routeupdate_t *)(&(pkt.data)))->entry[i].cost)){
					//update the distance 
					dv[0].dvEntry[i].cost = temp_distance + ((pkt_routeupdate_t *)(&(pkt.data)))->entry[i].cost;
					//update the routing table
					routingtable_setnextnode(routingtable, all_list[i], srcID);				
				}
			}			
			//unlock the cri...
			pthread_mutex_unlock(dv_mutex);
			pthread_mutex_unlock(routingtable_mutex);			
		}
	}
	close(son_conn);
	son_conn = -1;
	pthread_exit(NULL);
}

//这个函数终止SIP进程, 当SIP进程收到信号SIGINT时会调用这个函数. 
//它关闭所有连接, 释放所有动态分配的内存.
void sip_stop() {
	close(son_conn);
	close(stcp_conn);
	dvtable_destroy(dv);
	nbrcosttable_destroy(nct);
	routingtable_destroy(routingtable);
	return;
}

//这个函数打开端口SIP_PORT并等待来自本地STCP进程的TCP连接.
//在连接建立后, 这个函数从STCP进程处持续接收包含段及其目的节点ID的sendseg_arg_t. 
//接收的段被封装进数据报(一个段在一个数据报中), 然后使用son_sendpkt发送该报文到下一跳. 下一跳节点ID提取自路由表.
//当本地STCP进程断开连接时, 这个函数等待下一个STCP进程的连接.
void waitSTCP() {   
	int hostID = topology_getMyNodeID();
	socklen_t clilen;
	int listenfd;
	//connfd = 0;
	struct sockaddr_in cliaddr, servaddr;
    
	if((listenfd = socket(AF_INET, SOCK_STREAM, 0)) <0 ){
		perror("Problem in creating the server socket...\n");
		exit(2);
	}
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(SIP_PORT);
  
	bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
	listen(listenfd, 20);
	printf("SIP level : Server running ... wait for connections ... \n");
	//当本地STCP进程断开连接时, 这个函数等待下一个STCP进程的连接.
	while(1){
		clilen = sizeof(cliaddr);
		stcp_conn = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);
		int destID, nextID;
		seg_t segPtr;
		sip_pkt_t pkt;
		while(getsegToSend(stcp_conn, &destID, &segPtr) > 0){
			//initiate the pkt_part data ...
			//printf("dest:%d %d->%d %d\n", destID, segPtr.header.src_port, segPtr.header.dest_port, segPtr.header.type);
			//memcpy(&(pkt.data), &segPtr, sizeof(seg_t));
			/*
			 * fix a bug...
			 */			
			pkt.header.src_nodeID = hostID;
			pkt.header.dest_nodeID = destID;
			pkt.header.length = sizeof(seg_t);
			pkt.header.type = SIP;
			memcpy(pkt.data, &segPtr, sizeof(seg_t));		
			//check the routing table
			pthread_mutex_lock(routingtable_mutex);
			nextID = routingtable_getnextnode(routingtable, destID);
			pthread_mutex_unlock(routingtable_mutex);
			//debug
			//printf("__~~~~~SEND SIP TYPE_ %d\n", pkt.header.type);
			son_sendpkt(nextID, &pkt, son_conn);
			//son_sendpkt(BROADCAST_NODEID, &temp, son_conn);
		}
		close(stcp_conn);
	}	
	close(listenfd);
	return;
}

int main(int argc, char *argv[]) {
	printf("SIP layer is starting, pls wait...\n");

	//初始化全局变量
	nct = nbrcosttable_create();
	dv = dvtable_create();
	dv_mutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(dv_mutex,NULL);
	routingtable = routingtable_create();
	routingtable_mutex = (pthread_mutex_t*)malloc(sizeof(pthread_mutex_t));
	pthread_mutex_init(routingtable_mutex,NULL);
	son_conn = -1;
	stcp_conn = -1;

	nbrcosttable_print(nct);
	dvtable_print(dv);
	routingtable_print(routingtable);

	//注册用于终止进程的信号句柄
	signal(SIGINT, sip_stop);

	//连接到本地SON进程 
	son_conn = connectToSON();
	if(son_conn<0) {
		printf("can't connect to SON process\n");
		exit(1);		
	}
	
	//启动线程处理来自SON进程的进入报文 
	pthread_t pkt_handler_thread; 
	pthread_create(&pkt_handler_thread,NULL,pkthandler,(void*)0);

	//启动路由更新线程 
	pthread_t routeupdate_thread;
	pthread_create(&routeupdate_thread,NULL,routeupdate_daemon,(void*)0);	

	printf("SIP layer is started...\n");
	printf("waiting for routes to be established\n");
	sleep(SIP_WAITTIME);
	routingtable_print(routingtable);

	//等待来自STCP进程的连接
	printf("waiting for connection from STCP process\n");
	waitSTCP(); 

}


