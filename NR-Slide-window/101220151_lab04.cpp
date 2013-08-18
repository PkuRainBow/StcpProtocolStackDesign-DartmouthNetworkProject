/*
 * lab04.cpp
 *
 *  Created on: 2013-4-4
 *      Author: njucsyyh
 */

#include "sysinclude.h"
#include <deque>
using namespace std;

extern void sendFRAMEPacket(unsigned char* pData, int len);

#define WINDOW_SIZE_STOP_WAIT 1
#define WINDOW_SIZE_BACK_N_FRAME 4
#define WINDOW_SIZE_CHOICE_FRAME_RESEND 4

//֡����
typedef enum {data, ack, nak} frame_kind;

//֡ͷ
typedef struct frame_head
{
	frame_kind kind;	//֡����
	int seq;			//���к�
	int ack;			//ȷ�Ϻ�
	unsigned char data[100];	//����
};

//֡
typedef struct frame
{
	frame_head head; //֡ͷ
	int size;		//���ݵĴ�С
};

//���ʹ���
typedef struct Windows
{
	frame *pframe;	 //ָ֡��
	int size;		//���ݵĳ���
};


deque <Windows> deque1;
deque <Windows> deque2;
deque <Windows> deque3;
bool IsFull = false;

/*
* ͣ��Э����Ժ���
*/
int stud_slide_window_stop_and_wait(char *pBuffer, int bufferSize, UINT8 messageType)
{
	int ack;
	int num;
	Windows buff;
      //
	if(MSG_TYPE_SEND == messageType){
		buff.pframe = new frame;
		*buff.pframe = *(frame *)pBuffer;
		buff.size = bufferSize;
		deque1.push_back(buff);
		if(!IsFull)
		{
			buff = deque1.back();
			//���÷���֡����
			SendFRAMEPacket((unsigned char *)(buff.pframe), buff.size);
			//���ʹ�������
			IsFull = true;
		}
	   }
      //
      if(MSG_TYPE_RECEIVE == messageType){
		buff = deque1.front();
		ack = ntohl(((frame *)pBuffer)->head.ack);
		if(ntohl(buff.pframe->head.seq) == ack)
            //if(*buff.pframe.head.seq == ack)
		{
			deque1.pop_front();
			if(deque1.size() != 0 )
			{
				buff = deque1.front();
				SendFRAMEPacket(((unsigned char *)buff.pframe), buff.size);
			}
			else
			{
				//���÷��ʹ���Ϊδ��
				IsFull = false;
			}
		}
	 }

	//��ʱ�Ĵ���
      if(MSG_TYPE_TIMEOUT == messageType){
		num = ntohl(*(int *)pBuffer);
		buff = deque1.front();
		if(num == buff.pframe->head.seq)
		{
			//���÷���֡����
			SendFRAMEPacket((unsigned char *)(buff.pframe), buff.size);
		}
	}
	return 0;
}

/*
* ����n֡���Ժ���
*/
int stud_slide_window_back_n_frame(char *pBuffer, int bufferSize, UINT8 messageType)
{
	static int Count2 = 0;
	int ACK;
	int num;
	int i,j;
	Windows buff;

	if(MSG_TYPE_SEND == messageType){
		buff.pframe = new frame;
		(*buff.pframe) = *(frame *)pBuffer;
		buff.size = bufferSize;
		deque2.push_back(buff);
		if(Count2 < WINDOW_SIZE_BACK_N_FRAME)
		{
			buff = deque2.back();
			SendFRAMEPacket((unsigned char *)(buff.pframe), buff.size);
			Count2++;
		}
      }

	if(MSG_TYPE_RECEIVE == messageType){
			ACK = ntohl(((frame *)pBuffer)->head.ack);
			for(i = 0; i < WINDOW_SIZE_BACK_N_FRAME && i < deque2.size(); i++)
			{
				buff = deque2[i];
				if(ACK == ntohl((*buff.pframe).head.seq))
				{
					break;
				}
			}
			if(i < deque2.size() && i < WINDOW_SIZE_BACK_N_FRAME)
			{
				for (j = 0; j <= i; j++)
				{
					deque2.pop_front();
					Count2--;
				}
			}
			for (; Count2 < WINDOW_SIZE_BACK_N_FRAME && Count2 < deque2.size(); Count2++)
			{
				buff = deque2[Count2];
				SendFRAMEPacket((unsigned char *)(buff.pframe), buff.size);
			}
	   }
	//��ʱ�Ĵ���
	if(MSG_TYPE_TIMEOUT == messageType){
		for (i = 0; i < WINDOW_SIZE_BACK_N_FRAME && i < deque2.size(); i++)
		{
                  buff = deque2[i];
			SendFRAMEPacket((unsigned char *)(buff.pframe), buff.size);
		}
	}
	return 0;
}
/*
* ѡ�����ش����Ժ���
*/
int stud_slide_window_choice_frame_resend(char *pBuffer, int bufferSize, UINT8 messageType)
{
	static int Count3 = 0;
	int ACK;
	int NAK;
    int num;
	int i,j;
	Windows buff;

	if(MSG_TYPE_SEND == messageType){
		buff.pframe = new frame;
		*buff.pframe = *(frame *)pBuffer;
		buff.size = bufferSize;
		deque3.push_back(buff);
		if(Count3 < WINDOW_SIZE_CHOICE_FRAME_RESEND)
		{
			//��÷��Ͷ���������ѹ���֡
			buff = deque3.back();
			SendFRAMEPacket((unsigned char *)(buff.pframe), buff.size);
			Count3++;
		}
	}

	//����֡�Ĵ���
    if(MSG_TYPE_RECEIVE == messageType){
		int frame_kind = ntohl(((frame *)pBuffer)->head.kind);
		//���֡�𻵣���ô�ѳ����֡���·���һ��
		if(frame_kind == nak)
		{
			NAK = ntohl(((frame *)pBuffer)->head.ack);
			for(i = 0; i < WINDOW_SIZE_CHOICE_FRAME_RESEND && i < deque3.size(); i ++)
			{
				if(NAK == ntohl((*(deque3[i].pframe)).head.seq))
				{
					SendFRAMEPacket((unsigned char *)(deque3[i].pframe), deque3[i].size);
					break;
				}
			}
		}
		else
		{
			ACK = ntohl(((frame *)pBuffer)->head.ack);
			for(i = 0; i < WINDOW_SIZE_CHOICE_FRAME_RESEND && i < deque3.size(); i++)
			{
				buff = deque3[i];
				if(ACK == ntohl((*buff.pframe).head.seq))
				{
					break;
				}
			}
			if(i < deque3.size() && i < WINDOW_SIZE_CHOICE_FRAME_RESEND)
			{
				for (j = 0; j <= i; j++)
				{
					deque3.pop_front();
					Count3--;
				}
			}
			//��ȡ��ǰ��������ֵ
			//j = Count3;
			//�����Ͷ����п��Է��͵�֡ȫ������
			for (;Count3 < deque3.size() && Count3 < WINDOW_SIZE_CHOICE_FRAME_RESEND; Count3++)
			{
				buff = deque3[Count3];
				//���÷��ͺ���
				SendFRAMEPacket((unsigned char *)(buff.pframe), buff.size);
			}
		}
    }

	if(MSG_TYPE_TIMEOUT == messageType){
		//��һ��32λ���������ֽ�˳��ת��Ϊ�����ֽ�˳��
		num = ntohl(*(unsigned int *)pBuffer);
		//ͨ��ѭ���ҵ���ʱ��֡�����к�
		for (i = 0; i < deque3.size() && i < WINDOW_SIZE_BACK_N_FRAME; i++)
		{
			buff = deque3[i];
			if ((*buff.pframe).head.seq == num)
				break;
		}
		buff = deque3[i];
		SendFRAMEPacket((unsigned char *)(buff.pframe), buff.size);
	}

	return 0;
}
