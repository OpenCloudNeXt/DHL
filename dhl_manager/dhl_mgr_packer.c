/*
 * dhl_packer.c
 *
 *  Created on: Apr 9, 2018
 *      Author: lxy
 */
#ifndef MGR_PREFETCH_ENABLE
#define MGR_PREFETCH_ENABLE   1
#endif

#include <rte_branch_prediction.h>

#include "dhl_mgr.h"
#include "dhl_mgr_stats.h"

#include "dhl_pkt_print.h"


/*****************************Internal functions******************************/
/*
 *  Packer thread:
 *  dequeue pkts from shared input buffer queues, re-group them by acc_id
 */
int
packer_thread_main(__attribute__((unused)) void * dummy) {
	unsigned i, j;
	unsigned cur_lcore;
	struct packer_thread * thread = NULL;

	struct packer * packer = &manager.packer;
	uint32_t bsz_out = manager.burst_size_ibq_out;

	cur_lcore = rte_lcore_id();
	for (i = 0; i < packer->num_thread; i++){
		thread = packer->packer_thread + i;

		if(thread == NULL) {
			RTE_LOG(ERR, MANAGER, "Getting packer_thread is NULL, while packer thread %d running at lcore %d.\n",
					i, cur_lcore);
			return -1;
		}

		if(thread->lcore != cur_lcore)
			continue;
		else{
			RTE_LOG(INFO, MANAGER, "Packer thread %d is running at lcore %d in socket %d.\n",
					thread->id, cur_lcore, thread->socket_id);

			struct dhl_bq * bq = thread->bq;
			struct rte_ring * ibq = bq->ring;
			struct rte_mbuf * pkt;
			int n_pkts = 0;

			if(ibq == NULL) {
				RTE_LOG(ERR, MANAGER, "There is no ibq allocated to packer thread %d.\n", thread->id);
				return -1;
			}

			while(manager.worker_keep_running) {
				n_pkts = rte_ring_sc_dequeue_bulk(
						ibq,
						(void **) bq->mbuf_out.array,
						bsz_out,
						NULL);

				if (unlikely(n_pkts == 0))
					continue;

				MGR_PREFETCH1(rte_pktmbuf_mtod(shared_ibq->mbuf_out.array[0], unsigned char *));
				MGR_PREFETCH0(shared_ibq->mbuf_out.array[1]);

				for (j = 0; j < bsz_out; j++) {


					if (likely( j < bsz_out - 1)) {
						MGR_PREFETCH1( rte_pktmbuf_mtod(shared_ibq->mbuf_out.array[j+1], unsigned char *));
					}
					if (likely( j < bsz_out - 2)) {
						MGR_PREFETCH0(shared_ibq->mbuf_out.array[j+2]);
					}

					pkt = bq->mbuf_out.array[j];

#ifdef VIRTUAL_CHANNEL
					if(rte_ring_enqueue(manager.virtual_channel, (void *)pkt) != 0){
						RTE_LOG(ERR, MANAGER, "enqueue pkt from ibq to virtual channel failed!\n");
					}
#else
					/* packer needs to pack packets in terms of their acc_id,
					 *
					 * By now, we just send packets to FPGA without encapsulation.
					 * We will implement packet encapsulating in the future.
					 */



#endif

				}


			}

		}
	}

	RTE_LOG(INFO, MANAGER, "Core %d: Packer thread of DHL Manager done.\n", rte_lcore_id());
	return 0;
}
