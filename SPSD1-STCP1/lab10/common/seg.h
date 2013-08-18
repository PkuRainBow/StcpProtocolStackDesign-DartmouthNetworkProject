
#ifndef SEG_H
#define SEG_H

#include "constants.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

#define	SYN 0
#define	SYNACK 1
#define	FIN 2
#define	FINACK 3
#define	DATA 4
#define	DATAACK 5

#define SEGSTART1 0
#define SEGSTART2 1
#define SEGRECV 2
#define SEGSTOP1 3


typedef struct stcp_hdr {
	unsigned int src_port;       
	unsigned int dest_port;      
	unsigned int seq_num;       
	unsigned int ack_num;      
	unsigned short int length;   
	unsigned short int  type;     
	unsigned short int  rcv_win;  
	unsigned short int checksum; 
} stcp_hdr_t;


typedef struct segment {
	stcp_hdr_t header;
	char data[MAX_SEG_LEN];
} seg_t;

//global
int STCP_client;
int STCP_server;

int sip_sendseg(int connection, seg_t* segPtr);

int sip_recvseg(int connection, seg_t* segPtr);
 
int seglost(seg_t* segPtr); 

//static int client_lost_tag = 0;
//static int server_lost_tag = 0;
/*
int seglost() {
	int random = rand()%100;
	if(random<PKT_LOSS_RATE*100) {
		return 1;
	else 
		return 0;
}
*/
//
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//

#endif
