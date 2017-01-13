/*
* THIS FILE IS FOR IP FORWARD TEST
*/
#include "sysInclude.h"
#include <vector>
using namespace std;

// system support
extern void fwd_LocalRcv(char *pBuffer, int length);

extern void fwd_SendtoLower(char *pBuffer, int length, unsigned int nexthop);

extern void fwd_DiscardPkt(char *pBuffer, int type);

extern unsigned int getIpv4Address( );

// implemented by students
struct item{
	int dst;
	int nexthop;
};
vector<item> table; // Router Table 
void stud_Route_Init()
{
	table.clear();
	return;
}

void stud_route_add(stud_route_msg *proute)
{
	item tmp;
	tmp.dst = ntohl(proute->dest) & (0xffffffff << (32 - ntohl(proute->masklen)));
	tmp.nexthop = ntohl(proute->nexthop);
	table.push_back(tmp);
	return;
}


int stud_fwd_deal(char *pBuffer, int length)
{
	// same as the previous lab
	int version = pBuffer[0] >> 4;
	int ihl = pBuffer[0] & 0xf;
	int ttl = (int)pBuffer[8];
	int checkSum = ntohs(*(short unsigned int*)(pBuffer + 10));
	int dstip = ntohl(*(unsigned int*)(pBuffer + 16));
	if (dstip == getIpv4Address()){
		fwd_LocalRcv(pBuffer, length);
		return 0;
	}
	if (ttl <= 0){
		fwd_DiscardPkt(pBuffer, STUD_FORWARD_TEST_TTLERROR);
		return 1;
	}
	vector<item>::iterator it;
	for (it = table.begin(); it != table.end(); it++){
		if (it->dst == dstip){
			char* buf = new char[length];
			memcpy(buf, pBuffer, length);
			buf[8]--;
			unsigned short int newchecksum = 0;
			for (int i = 0; i < 2 * ihl; ++i){
				if (i == 5)
					continue;
				newchecksum += (buf[i*2] << 8) + (buf[i*2+1]);
			}
			newchecksum = htons(0xffff-newchecksum);
			memcpy(buf + 10, &newchecksum, sizeof(unsigned short int));
			fwd_SendtoLower(buf, length, it->nexthop);
			return 0;
		}
	}
	fwd_DiscardPkt(pBuffer, STUD_FORWARD_TEST_NOROUTE);
	return 1;
}
