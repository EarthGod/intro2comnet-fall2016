/*
* THIS FILE IS FOR IP TEST
*/
// system support
#include "sysInclude.h"

extern void ip_DiscardPkt(char* pBuffer,int type);

extern void ip_SendtoLower(char*pBuffer,int length);

extern void ip_SendtoUp(char *pBuffer,int length);

extern unsigned int getIpv4Address();

// implemented by students

int stud_ip_recv(char *pBuffer,unsigned short length)
{
	int version = ((int)pBuffer[0]) >> 4;
	if (version != 4){
		ip_DiscardPkt(pBuffer, STUD_IP_TEST_VERSION_ERROR);
		return 1;
	}
	int ihl = ((int)pBuffer[0]) & 0xf;
	if (ihl < 5){
		ip_DiscardPkt(pBuffer, STUD_IP_TEST_HEADLEN_ERROR);
		return 1;
	}
	int ttl = (int)pBuffer[8];
	if (ttl == 0){
		ip_DiscardPkt(pBuffer, STUD_IP_TEST_TTL_ERROR);
		return 1;
	}
	int dest = ntohl(*((unsigned int*)(pBuffer + 16)));
	if (dest != getIpv4Address() && dest != 0xffffffff){
		ip_DiscardPkt(pBuffer, STUD_IP_TEST_DESTINATION_ERROR);
		return 1;
	}

	// Fast algo from RFC1071
	unsigned int checksum = 0;
	for (int i = 0; i < 20; i += 2){
		checksum += (pBuffer[i] & 0xff) << 8;
		checksum += (pBuffer[i + 1] & 0xff);
	}
	while (checksum >> 16)
		checksum = (checksum >> 16) + checksum;
	checksum = (~checksum) & 0xffff;
	if (checksum){
		ip_DiscardPkt(pBuffer, STUD_IP_TEST_CHECKSUM_ERROR);
		return 1;
	}

	ip_SendtoUp(pBuffer, (int) length);
	return 0;
}

int stud_ip_Upsend(char *pBuffer,unsigned short len,unsigned int srcAddr,
				   unsigned int dstAddr,byte protocol,byte ttl)
{
	char* buf = new char(len + 20);
	memset(buf, 0, len + 20);
	buf[0] = 0x45; // ver & headlen
	unsigned short ttllen = htons(len + 20);
	memcpy(buf + 2, &ttllen, sizeof(unsigned short)); // ttllen
	buf[8] = ttl; // ttl
	buf[9] = protocol; // protocol
	unsigned int srcadd = htonl(srcAddr);
	unsigned int dstadd = htonl(dstAddr);
	memcpy(buf + 12, &srcadd, sizeof(unsigned int));
	memcpy(buf + 16, &dstadd, sizeof(unsigned int));

	unsigned int checksum = 0;
	for (int i = 0; i < 20; i += 2){
		checksum += (buf[i] & 0xff) << 8;
		checksum += (buf[i + 1] & 0xff);
		checksum %= 65535;
		printf("checksum: %x, num1: %x, num2: %x\n", checksum, (buf[i] & 0xff) << 8, (buf[i + 1] & 0xff));
	}
	unsigned short res = htons(0xffff-(unsigned short) checksum);
	memcpy(buf + 10, &res, sizeof(unsigned short));
	
	memcpy(buf + 20, pBuffer, len);
	ip_SendtoLower(buf, (int)(len+20));
	return 0;
}
