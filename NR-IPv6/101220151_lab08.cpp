/*
* THIS FILE IS FOR IP FORWARD TEST
*/
#include "sysinclude.h"

// system support
extern void ipv6_fwd_DiscardPkt(char *pBuffer, int type);
extern void ipv6_fwd_SendtoLower(char *pBuffer, int length, ipv6_addr *nexthop);
extern void getIpv6Address(ipv6_addr *pAddr);
extern void ipv6_fwd_LocalRcv(char *pBuffer, int length);

//自己定义
extern bool is_equal_addr(ipv6_addr* t1, ipv6_addr* t2);
extern bool is_equal_subnet(int masklen, ipv6_addr t1, ipv6_addr t2);

//extern long Subnet(const ipv6_addr* addr, unsigned int masklen);
typedef struct IPv6{
  byte Version;
  unsigned char TrafficClass;
  unsigned int FlowLabel;
  unsigned short PayloadLen;
  unsigned char NextHeader;
  unsigned char HopLimit;
  ipv6_addr S_addr;
  ipv6_addr D_addr;
}Header_v6;

typedef struct Route_node{
	stud_ipv6_route_msg msg;
	Route_node* next;
}Route;

Route *route_first = NULL;

void stud_ipv6_Route_Init()
{
	while(route_first != NULL){
		Route *p = route_first->next;
		delete(route_first);
		route_first = p;
	}
	return;
}

bool is_equal_addr(ipv6_addr* t1, ipv6_addr* t2){
	int k=0;
	for(int i=0; i<16; i++){
		if(t1->bAddr[i] == t2->bAddr[i]) k++;
		else break;
	}
	if(k==16)
         return true;
	else
         return false;
}

bool is_equal_subnet(int masklen, ipv6_addr t1, ipv6_addr t2){
	int num = masklen/8;
	int remain = masklen%8;
	for(int i=0; i<num; i++)
		if(t1.bAddr[i] != t2.bAddr[i])  return false;
	char a = t1.bAddr[num] & (0xff << (8-remain));
	char b = t2.bAddr[num] & (0xff << (8-remain));
	if(a == b)
		return true;
	else
		return false;
}

void stud_ipv6_route_add(stud_ipv6_route_msg *proute)
{
	Route *temp = new Route;
	temp->msg = *proute;
	temp->next = NULL;

    //如果路由表是空的，直接添加到路由表的头部
	if(route_first == NULL){
		route_first = temp;
	}
    //查找路由表项的插入位置，然后插入路由表中
	else{
		Route *p = NULL;
		Route *q = route_first;
		while(q != NULL){
			if(q->msg.masklen <= temp->msg.masklen)
				break;
			p = q;
			q = q->next;
		}

	    if(temp->msg.masklen == temp->msg.masklen && (is_equal_addr(&temp->msg.dest, &q->msg.dest) == true) &&
			(is_equal_addr(&temp->msg.nexthop, &q->msg.nexthop) == true )){
		      printf("\nWarning: The Route Info has already existed !\n");
	          return;
	    }
	    if(q==route_first){
		    temp->next = route_first;
		    route_first = temp;
	    }
	    else{
		   temp->next = q;
		   p->next = temp;
	    }
	    printf("*********** Add an Route info ***********\n");
    }
	return;
}

int stud_ipv6_fwd_deal(char *pBuffer, int length)
{
	Header_v6 head;
	head.Version = (pBuffer[0]>>4) & 0x0f;
	//head.TrafficClass
	//head.FlowLabel
	head.PayloadLen = ntohs(*((unsigned short *)(pBuffer+4)));
	head.NextHeader = pBuffer[6];
	head.HopLimit = pBuffer[7];
    //
	for(int i=0; i<16; i++)   head.S_addr.bAddr[i] = pBuffer[8+i];
	for(int i=0; i<16; i++)   head.D_addr.bAddr[i] = pBuffer[24+i];

	ipv6_addr* localhost = new ipv6_addr;
    getIpv6Address(localhost);

    if(is_equal_addr(localhost, &head.D_addr)==true){
		ipv6_fwd_LocalRcv(pBuffer, length);
		return 0;
	}

	Route *p = route_first;
	while(p != NULL)
	{
		if(is_equal_subnet(p->msg.masklen, p->msg.dest, head.D_addr)==true){
			printf("Find the Right Route Info\n");
			break;
		}
		else
			p=p->next;
	}

	if(p==NULL){
		printf("No Route Info in the list\n");
		ipv6_fwd_DiscardPkt(pBuffer, STUD_IPV6_FORWARD_TEST_NOROUTE);
		return 1;
	}

	if (head.HopLimit <= 1){
		ipv6_fwd_DiscardPkt(pBuffer, STUD_IPV6_FORWARD_TEST_HOPLIMIT_ERROR);
		return 1;
	}

	pBuffer[7]--;

    if(is_equal_subnet(p->msg.masklen, head.D_addr, *localhost)==true){
    	ipv6_fwd_SendtoLower(pBuffer, length, &head.D_addr);
    }
    else{
    	ipv6_fwd_SendtoLower(pBuffer, length, &p->msg.nexthop);
    }

	return 0;
}

