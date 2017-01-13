#include "sysinclude.h"
#include <iostream>
#include <queue>
#include <cstdio>
using namespace std;

extern void SendFRAMEPacket(unsigned char* pData, unsigned int len);

#define WINDOW_SIZE_STOP_WAIT 1
#define WINDOW_SIZE_BACK_N_FRAME 4

typedef enum{data, ack, nak} frame_kind;
typedef struct frame_head{
	frame_kind kind; //帧类型
	unsigned int seq; //序列号 
	unsigned int ack; //确认号 
	unsigned char data[100]; //数据
};
typedef struct frame{
	frame_head head; //帧头
	unsigned int size; //数据的大小
};
// for convinience when calling SendFRAMEPacket
typedef struct store{
	frame* p;
	unsigned int len;
};
/*
* 停等协议测试函数
*/
deque <store> qbuff1;
deque <store> qbuff2;
deque <store> qbuff3;
unsigned int windowSize1 = 0;
unsigned int windowSize2 = 0;
unsigned int windowSize3 = 0;
int stud_slide_window_stop_and_wait(char *pBuffer, int bufferSize, UINT8 messageType)
{
	store ns;
	if (messageType == MSG_TYPE_SEND){
		// save new frame in queue
		ns.p = new frame;
		*ns.p = *((frame*)pBuffer);
		ns.len = bufferSize;
		qbuff1.push_back(ns);
		// window not full
		if (windowSize1 < WINDOW_SIZE_STOP_WAIT){
			ns = qbuff1[windowSize1++];
			SendFRAMEPacket((unsigned char*)(ns.p), ns.len);
		}
	}
	else if (messageType == MSG_TYPE_RECEIVE){
		unsigned int tmpack = ntohl(((frame*)pBuffer)->head.ack);
		if (tmpack == ntohl(qbuff1[0].p->head.seq)){
			qbuff1.pop_front();
			windowSize1--;
			if (qbuff1.size() > windowSize1 && windowSize1 < WINDOW_SIZE_STOP_WAIT){
				ns = qbuff1[windowSize1++];
				SendFRAMEPacket((unsigned char*)(ns.p), ns.len);
			}
		}
	}
	else if (messageType == MSG_TYPE_TIMEOUT){
		for (unsigned int i = 0; i < windowSize1; ++i)
			SendFRAMEPacket((unsigned char*)(qbuff1[i].p), qbuff1[i].len);
	}
	return 0;
}

/*
* 回退n帧测试函数
*/
int stud_slide_window_back_n_frame(char *pBuffer, int bufferSize, UINT8 messageType)
{
	store ns;
	if (messageType == MSG_TYPE_SEND){
		// save new frame in queue
		ns.p = new frame;
		*ns.p = *((frame*)pBuffer);
		ns.len = bufferSize;
		qbuff2.push_back(ns);
		// window not full
		if (windowSize2 < WINDOW_SIZE_BACK_N_FRAME){
			ns = qbuff2[windowSize2++];
			SendFRAMEPacket((unsigned char*)(ns.p), ns.len);
		}
	}
	else if (messageType == MSG_TYPE_RECEIVE){
		unsigned int tmpack = ntohl(((frame*)pBuffer)->head.ack);
		for (int i = 0; i < windowSize2; ++i){
			if (tmpack == ntohl(qbuff2[i].p->head.seq)){
				for (int j = 0; j <= i; ++j){
					qbuff2.pop_front();
					windowSize2--;
					if (qbuff2.size() > windowSize2 && windowSize2 < WINDOW_SIZE_BACK_N_FRAME){
						ns = qbuff2[windowSize2++];
						SendFRAMEPacket((unsigned char*)(ns.p), ns.len);
					}
				}
			}
		}
	}
	else if (messageType == MSG_TYPE_TIMEOUT){
		for (unsigned int i = 0; i < windowSize2; ++i)
			SendFRAMEPacket((unsigned char*)(qbuff2[i].p), qbuff2[i].len);
	}
	return 0;
}

/*
* 选择性重传测试函数
*/
int stud_slide_window_choice_frame_resend(char *pBuffer, int bufferSize, UINT8 messageType)
{
	store ns;
	if (messageType == MSG_TYPE_SEND){
		// save new frame in queue
		ns.p = new frame;
		*ns.p = *((frame*)pBuffer);
		ns.len = bufferSize;
		qbuff3.push_back(ns);
		// window not full
		if (windowSize3 < WINDOW_SIZE_BACK_N_FRAME){
			ns = qbuff3[windowSize3++];
			SendFRAMEPacket((unsigned char*)(ns.p), ns.len);
		}
	}
	else if (messageType == MSG_TYPE_RECEIVE && ntohl(((frame*)pBuffer)->head.kind) == ack){
		unsigned int tmpack = ntohl(((frame*)pBuffer)->head.ack);
		for (int i = 0; i < windowSize3; ++i){
			if (tmpack == ntohl(qbuff3[i].p->head.seq)){
				for (int j = 0; j <= i; ++j){
					qbuff3.pop_front();
					windowSize3--;
					if (qbuff3.size() > windowSize3 && windowSize3 < WINDOW_SIZE_BACK_N_FRAME){
						ns = qbuff3[windowSize3++];
						SendFRAMEPacket((unsigned char*)(ns.p), ns.len);
					}
				}
			}
		}
	}
	else if (messageType == MSG_TYPE_RECEIVE && ntohl(((frame*)pBuffer)->head.kind) == nak){
		unsigned int naknum = ntohl(((frame*)pBuffer)->head.ack);
		for (unsigned int i = 0; i < windowSize3; ++i){
			if (naknum == ntohl(qbuff3[i].p->head.seq)){
				SendFRAMEPacket((unsigned char*)(qbuff3[i].p), qbuff3[i].len);
				break;
			}
		}
	}	
	return 0;
}
