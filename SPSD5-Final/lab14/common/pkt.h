#ifndef PKT_H
#define PKT_H

#include <stdlib.h>
#include <stdio.h>
#include "constants.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#define	ROUTE_UPDATE 1
#define SIP 2	

#define PKTSTART1 0
#define PKTSTART2 1
#define PKTRECV 2
#define PKTSTOP1 3

typedef struct sipheader {
  int src_nodeID;		         
  int dest_nodeID;		          
  unsigned short int length;	  
  unsigned short int type;	      
} sip_hdr_t;

typedef struct packet {
  sip_hdr_t header;
  char data[MAX_PKT_LEN];
} sip_pkt_t;


typedef struct routeupdate_entry {
        unsigned int nodeID;	
        unsigned int cost;	    
} routeupdate_entry_t;

typedef struct pktrt{
        unsigned int entryNum;	
        routeupdate_entry_t entry[MAX_NODE_NUM];
} pkt_routeupdate_t;


typedef struct sendpktargument {
  int nextNodeID;        
  sip_pkt_t pkt;         
} sendpkt_arg_t;

int son_sendpkt(int nextNodeID, sip_pkt_t* pkt, int son_conn);
int son_recvpkt(sip_pkt_t* pkt, int son_conn);
int getpktToSend(sip_pkt_t* pkt, int* nextNode,int sip_conn);
int forwardpktToSIP(sip_pkt_t* pkt, int sip_conn);
int sendpkt(sip_pkt_t* pkt, int conn);
int recvpkt(sip_pkt_t* pkt, int conn);

#endif
