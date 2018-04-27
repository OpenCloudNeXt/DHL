/*
 * dhl_pkt_print.c
 *
 *  Created on: Apr 18, 2018
 *      Author: root
 */
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <inttypes.h>
#include <netinet/in.h>

#include "dhl_pkt_print.h"


int prChunkSize = 32;

void macPacketPrint(const unsigned char * buf, const int num)
{
	int i;
	if(num < 60){
		printf("packet size is %d, is too small, the smallest size is 64\n",num);
		return;
	}

	for(i = 0; i < 14; i++){
		printf("%02X ", buf[i]);
		if ( (i+1)% 6 == 0 )
			printf("   ");
	}
	printf("\n");

	for(;i < num; i++){
		printf("%02X ", buf[i]);
		if ( ((i+1-14)% 8 == 0) && ((i+1-14)% prChunkSize != 0))
				printf("   ");

		if ((i+1-14)% prChunkSize == 0)
			printf("\n");
	}
//	if(num%prChunkSize)
		printf("\n");
	for(i =0; i <prChunkSize; i++)
		printf("***");
	printf("\n");
	return;

}

void IpPacketPrint(unsigned char * buf, const int num)
{
	int i;
	if(num < 60){
		printf("packet size is %d, is too small, the smallest size is 64\n",num);
		return;
	}

	for(i = 0; i < 14; i++){
		if(i == 0)
			printf("dst mac:");
		if(i == 6)
			printf("src mac:");
		if(i == 12)
			printf("type:");
		printf("%02X ", buf[i]);
		if ( (i+1)% 6 == 0 )
			printf("   ");

	}
	printf("\n");

	for(;i < 34; i++){
		if(i-14 == 2){
			uint16_t * len = (uint16_t *)(buf +i );
			printf(" len:(%u)",ntohs(*len));
		}

		if(i-14 == 8)
			printf(" TTL:");
		if(i-14 == 9)
			printf(" protocol:");
		if(i-14 == 12)
			printf(" src ip:");
		if(i-14 == 16)
			printf(" dst ip:");
		printf("%02X ", buf[i]);

		if ( ((i-33)% 2 == 0) && ((i-33)% prChunkSize != 0))
			printf("  ");
	}
	printf("\n");

	for(;i < num; i++){
		printf("%02X ", buf[i]);


		if ( ((i-33)% 8 == 0) && ((i-33)% prChunkSize != 0))
				printf("   ");

		if ((i-33)% prChunkSize == 0)
			printf("\n");
	}
//	if(num%prChunkSize)
		printf("\n");
//	for(i =0; i <prChunkSize; i++)
//		printf("===");
	printf("\n");
	return;

}

void
print_pkt_no_check(struct rte_mbuf * pkt)
{
	uint8_t * eth;

	eth = rte_pktmbuf_mtod(pkt, uint8_t *);

	macPacketPrint(eth, pkt->pkt_len);
}

