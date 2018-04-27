/*
 * distributor.c
 *
 *  Created on: Apr 9, 2018
 *      Author: lxy
 */

#include "dhl_mgr_stats.h"

#include "dhl_mgr.h"
#include "dhl_pkt_print.h"

/*
 *  Distributor thread:
 *  enqueue pkts to private output buffer queues
 */
int
distributor_thread_main(__attribute__((unused)) void * dummy) {
	unsigned i, j, cur_lcore;
	struct distributor_thread * thread = NULL;

	struct dhl_bq * obq = NULL;

	void * pkts[IBQ_DEQUEUE_PKTS];
	struct rte_mbuf * pkt;
	int num_pkts = 0;

	cur_lcore = rte_lcore_id();

	struct distributor * distributor = &manager.distributor;
	for (i = 0; i < distributor->num_thread; i++) {
		thread = distributor->distributor_thread + i;

		if(thread == NULL) {
			RTE_LOG(ERR, MANAGER, "Getting distributor_thread is NULL, when distributor thread %d running at lcore %d.\n",
					i, cur_lcore);
			return -1;
		}

		if(thread->lcore != cur_lcore)
			continue;
		else {
			RTE_LOG(INFO, MANAGER, "Distributor thread %d is running at lcore %d in socket %d.\n",
					thread->id, cur_lcore, thread->socket_id);

			while(manager.worker_keep_running) {
				num_pkts = rte_ring_count(manager.virtual_channel);

				if(num_pkts < IBQ_DEQUEUE_PKTS)
					continue;

				if(rte_ring_dequeue_bulk(manager.virtual_channel, pkts, IBQ_DEQUEUE_PKTS, NULL) == 0)
					continue;

				obq = (struct dhl_bq *)TAILQ_FIRST(&thread->obqs);

				if(obq == NULL) {
					RTE_LOG(ERR, MANAGER, "There is no obq allocated to distributor thread %d.\n", thread->id);
					return -1;
				}
				RTE_LOG(INFO, MANAGER, "Distributor thread get obq id %d.\n",obq->id);


				for(j = 0; j < IBQ_DEQUEUE_PKTS; j++){
					pkt = (struct rte_mbuf *)pkts[j];
					RTE_LOG(INFO, MANAGER, "Distributor get packet from virtural channel:\n");
					print_pkt_no_check(pkt);
					rte_ring_enqueue(obq->bq, (void *)pkt);
				}
			}

		}

	}

	RTE_LOG(INFO, MANAGER, "Core %d: Distributor thread of DHL Manager done.\n", rte_lcore_id());
	return 0;
}
