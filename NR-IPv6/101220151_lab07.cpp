/*
* THIS FILE IS FOR IPv6 TEST
*/
// system support
#include "sysinclude.h"

extern void ipv6_DiscardPkt(char* pBuffer,int type);

extern void ipv6_SendtoLower(char*pBuffer,int length);

extern void ipv6_SendtoUp(char *pBuffer,int length);

extern void getIpv6Address(ipv6_addr *paddr);

extern bool Equal(ipv6_addr* t1, ipv6_addr* t2);

// implemented by students
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

bool Equal(ipv6_addr* t1, ipv6_addr* t2){
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
int stud_ipv6_recv(char *pBuffer, unsigned short length)
{
	Header_v6 head;
	head.Version = (pBuffer[0]>>4) & 0x0f;
	//head.TrafficClass
	//head.FlowLabel
	head.PayloadLen = ntohs(*((unsigned short *)(pBuffer+4)));
	head.NextHeader = pBuffer[6];
	head.HopLimit = pBuffer[7];

	for(int i=0; i<16; i++)   head.S_addr.bAddr[i] = pBuffer[8+i];
	for(int i=0; i<16; i++)   head.D_addr.bAddr[i] = pBuffer[24+i];

	if(head.Version != 6){
		ipv6_DiscardPkt(pBuffer,  STUD_IPV6_TEST_VERSION_ERROR);
		return 1;
	}

	if (head.HopLimit <= 0)
	{
		ipv6_DiscardPkt(pBuffer, STUD_IPV6_TEST_HOPLIMIT_ERROR);
		return 1;
	}

	ipv6_addr* localhost = new ipv6_addr;
	getIpv6Address(localhost);
	if (Equal(localhost, &head.D_addr)==false)
	{
		ipv6_DiscardPkt(pBuffer, STUD_IPV6_TEST_DESTINATION_ERROR);
		return 1;
	}

	ipv6_SendtoUp(pBuffer, length);

	return 0;
}

int stud_ipv6_Upsend(char *pData, unsigned short len,
					 ipv6_addr *srcAddr, ipv6_addr *dstAddr,
					 char hoplimit, char nexthead)
{
	char *pBuffer = new char[40+len];
	memset(pBuffer, 0, 40+len);
	pBuffer[0] = 0x60;
	*(unsigned short *)(pBuffer+3) = htons(len);
	pBuffer[6] = nexthead;
	pBuffer[7] = hoplimit;
	//
	for(int i=0; i<16; i++)   pBuffer[8+i] = srcAddr->bAddr[i];
	for(int i=0; i<16; i++)   pBuffer[24+i]  = dstAddr->bAddr[i];
	
	memcpy(pBuffer+40, pData, len);

      ipv6_SendtoLower(pBuffer, len+40);
	return 0;
}
