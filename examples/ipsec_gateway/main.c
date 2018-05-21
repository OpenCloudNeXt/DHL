/*
 * main.c
 *
 *  Created on: May 21, 2018
 *      Author: root
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <sys/queue.h>
#include <unistd.h>
#include <signal.h>

#include <rte_memory.h>
#include <rte_memzone.h>
#include <rte_launch.h>
#include <rte_eal.h>
#include <rte_per_lcore.h>
#include <rte_lcore.h>
#include <rte_debug.h>
#include <rte_mbuf.h>

#include "dhl_nf.h"
#include "dhl_pkt_print.h"


#define MBUF_CACHE_SIZE 256

int mbuf_dataroom = 2048;

struct dhl_nf * nf = NULL;
const char * app_name = "ipsec_gateway";

uint8_t keep_running = 1;

static void handle_signal(int sig) {
	if (sig == SIGINT || sig == SIGTERM) {
		keep_running = 0;
		if(nf != NULL){
			int ret = dhl_unregister(nf);
			while(ret != 0){
				printf("%s unregister failed.\n", app_name);
				sleep(1);
				ret = dhl_unregister(nf);
			}
		}
	}
}

int
main(int argc, char **argv)
{
	int ret;
	struct rte_mempool * mbuf_pool;
	struct rte_mbuf * pkt;

	void * pkts[32];
	unsigned num_pkts = 0;
	unsigned i;

	ret = rte_eal_init(argc, argv);
	if (ret < 0)
		rte_panic("Cannot init EAL\n");

	/* Listen for ^C and terminate stop so we can exit gracefully */
	signal(SIGINT, handle_signal);
	signal(SIGTERM, handle_signal);


	nf = dhl_register(app_name);
	if(nf == NULL){
		rte_exit(EXIT_FAILURE, "register failed.\n");
	}
	printf("nf %s get nf_id %d.\n", nf->info->tag, nf->nf_id);

	mbuf_pool = rte_pktmbuf_pool_create("MBUF_POOL",
			10240, MBUF_CACHE_SIZE, 0,
			mbuf_dataroom + RTE_PKTMBUF_HEADROOM, rte_socket_id());

	if (mbuf_pool == NULL)
		rte_exit(EXIT_FAILURE, "Cannot create mbuf pool.\n");

//	pkt = rte_pktmbuf_alloc(mbuf_pool);
//	if(pkt == NULL )
//		rte_exit(EXIT_FAILURE, "Cannot get pktmbuf from mbuf pool.\n");
//	pkt->buf_len = 100;
//	pkt->data_len = 100;
//	pkt->pkt_len = 100;
//
//	uint8_t * eth;
//	eth = rte_pktmbuf_mtod(pkt, uint8_t *);
//	for(i = 0 ; i< pkt->data_len; i++){
//		eth[i] = 0xAB;
//	}
//	print_pkt_no_check(pkt);

//	rte_ring_enqueue(nf->ibq, (void *) pkt);

	while(keep_running) {
		num_pkts = rte_ring_count(nf->obq);
		if(num_pkts == 0)
			continue;

		if(rte_ring_dequeue_bulk(nf->obq, pkts, num_pkts, NULL ) == 0)
			continue;

		for(i = 0 ; i< num_pkts; i++){
			pkt = (struct rte_mbuf *) pkts[i];
			print_pkt_no_check(pkt);
			rte_pktmbuf_free(pkt);
		}
	}

	rte_eal_mp_wait_lcore();

	rte_mempool_free(mbuf_pool);
	return 0;
}
