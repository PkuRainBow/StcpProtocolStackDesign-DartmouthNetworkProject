#include "pkt.h"

int son_sendpkt(int nextNodeID, sip_pkt_t* pkt, int son_conn)
{
	sendpkt_arg_t temp;
	temp.nextNodeID = nextNodeID;
	memcpy(&(temp.pkt), pkt, sizeof(sip_pkt_t));
	if( send(son_conn, "!", 1, 0) == -1)    return -1;
	if( send(son_conn, "&", 1, 0) == -1)    return -1;
	if( send(son_conn, &temp, sizeof(sendpkt_arg_t), 0) == -1)  return -1;
	if( send(son_conn, "!", 1, 0) == -1)    return -1;
	if( send(son_conn, "#", 1, 0) == -1)    return -1;
  return 1;
}

int son_recvpkt(sip_pkt_t* pkt, int son_conn)
{
	//
	
	int state = PKTSTART1;
	int test = 0;
	int index = 0;
	char buffer[sizeof(sip_pkt_t)];	
	char c;
	memset(buffer, 0, sizeof(sip_pkt_t));
	while(1){
	  if(test==1) break;	  
    switch(state){
		case PKTSTART1:{	
			if(recv(son_conn, &c, 1, 0)==-1) return -1;
			if(c=='!')
			   state = PKTSTART2;
			break;
		}
	       
	  case PKTSTART2:{
			if(recv(son_conn, &c, 1, 0)==-1) return -1;
			if(c=='&')
			   state = PKTRECV;
			else
			   state = PKTSTART1;
			break;		
		}
		
	    case PKTRECV:{
			while(1){
				if(recv(son_conn, &c, 1, 0)==-1) return -1;
				if(c=='!'){
					state = PKTSTOP1;
				    break;
				}
				else
				    buffer[index++] = c;		       
		   }
		   break;   
		}
	
	    case PKTSTOP1:{
			if(recv(son_conn, &c, 1, 0)==-1) return -1;
			if(c == '#'){
			   test = 1;		   			   
		    }
		    if(c != '#'){
			   state = PKTRECV;
			   buffer[index++] = '!';
			   buffer[index++] = c;
			}
			break;
		}
	    default:{
		}
	  }
    }
    //printf("SON recv from the SIP successfully .../n");
	  memcpy(pkt, buffer, sizeof(sip_pkt_t));  
    return 1;
}


int getpktToSend(sip_pkt_t* pkt, int* nextNode,int sip_conn)
{	
	int state = PKTSTART1;
	int test = 0;
	int index = 0;
	char buffer[sizeof(sendpkt_arg_t)];	
	char c;
	memset(buffer, 0, sizeof(sendpkt_arg_t));
	while(1){
	  if(test==1) break;	  
      switch(state){
		case PKTSTART1:{	
			if(recv(sip_conn, &c, 1, 0)==-1) return -1;
			if(c=='!')
			   state = PKTSTART2;
			break;
		}
	       
	    case PKTSTART2:{
			if(recv(sip_conn, &c, 1, 0)==-1) return -1;
			if(c=='&')
			   state = PKTRECV;
			else
			   state = PKTSTART1;
			break;		
		}
		
	    case PKTRECV:{
			while(1){
				if(recv(sip_conn, &c, 1, 0)==-1) return -1;
				if(c=='!'){
					state = PKTSTOP1;
				    break;
				}
				else
				    buffer[index++] = c;		       
		   }
		   break;   
		}
	
	    case PKTSTOP1:{
			if(recv(sip_conn, &c, 1, 0)==-1) return -1; 
			if(c == '#'){
			   test = 1;		   			   
		    }
		    if(c != '#'){
			   state = PKTRECV;
			   buffer[index++] = '!';
			   buffer[index++] = c;
			}
			break;
		}
	    default:{
		    //printf("The state is wrong ...\n");
		}
	  }
    }
    //recv the pkt part only ...
   memcpy(pkt, &(((sendpkt_arg_t *)buffer)->pkt), sizeof(sip_pkt_t));
	 memcpy(nextNode, &(((sendpkt_arg_t *)buffer)->nextNodeID), sizeof(int));
	 //printf("sip_pkt_t : src:%d ----- dest:%d\n", pkt->header.src_nodeID, pkt->header.dest_nodeID);
   return 1;
}

int forwardpktToSIP(sip_pkt_t* pkt, int sip_conn)
{	
	if( send(sip_conn, "!", 1, 0) == -1)    return -1;
	if( send(sip_conn, "&", 1, 0) == -1)    return -1;
	if( send(sip_conn, pkt, sizeof(sip_pkt_t), 0) == -1)  return -1;
	if( send(sip_conn, "!", 1, 0) == -1)    return -1;
	if( send(sip_conn, "#", 1, 0) == -1)    return -1;
  return 1;
}

int sendpkt(sip_pkt_t* pkt, int conn)
{
	if( send(conn, "!", 1, 0) == -1)    return -1;
	if( send(conn, "&", 1, 0) == -1)    return -1;
	if( send(conn, pkt, sizeof(sip_pkt_t), 0) == -1)  return -1;
	if( send(conn, "!", 1, 0) == -1)    return -1;
	if( send(conn, "#", 1, 0) == -1)    return -1;
	return 1;
}

int recvpkt(sip_pkt_t* pkt, int conn)
{	
	int state = PKTSTART1;
	int test = 0;
	int index = 0;
	char buffer[sizeof(sip_pkt_t)];	
	char c;
	memset(buffer, 0, sizeof(sip_pkt_t));
	while(1){
	  if(test==1) break;	  
      switch(state){
		case PKTSTART1:{	
			if(recv(conn, &c, 1, 0)==-1) return -1;
			if(c=='!')
			   state = PKTSTART2;
			break;
		}
	       
	    case PKTSTART2:{
			if(recv(conn, &c, 1, 0)==-1) return -1;
			if(c=='&')
			   state = PKTRECV;
			else
			   state = PKTSTART1;
			break;		
		}
		
	    case PKTRECV:{
			while(1){
				if(recv(conn, &c, 1, 0)==-1) return -1;
				if(c=='!'){
					state = PKTSTOP1;
				    break;
				}
				else
				    buffer[index++] = c;		       
		   }
		   break;   
		}
	
	    case PKTSTOP1:{
			if(recv(conn, &c, 1, 0)==-1) return -1;
			if(c == '#'){
			   test = 1;		   			   
		    }
		    if(c != '#'){
			   state = PKTRECV;
			   buffer[index++] = '!';
			   buffer[index++] = c;
			}
			break;
		}
	    default:{
		    //printf("The state is wrong ...\n");
		}
	  }
    }
	  memcpy(pkt, buffer, sizeof(sip_pkt_t));
    return 1;
}
