/*
 * dhl_buffer_queue.h
 *
 *  Created on: Apr 23, 2018
 *      Author: lxy
 */

#ifndef DHL_MANAGER_DHL_BQ_H_
#define DHL_MANAGER_DHL_BQ_H_

#include <sys/queue.h>

/* Bursts */
#ifndef DHL_BQ_MBUF_ARRAY_SIZE
#define DHL_BQ_MBUF_ARRAY_SIZE 512
#endif

#ifndef BQ_DEFAULT_BURST_SIZE_IBQ_OUT
#define BQ_DEFAULT_BURST_SIZE_IBQ_OUT  144
#endif
#if (BQ_DEFAULT_BURST_SIZE_IBQ_OUT > DHL_BQ_MBUF_ARRAY_SIZE)
#error "BQ_DEFAULT_BURST_SIZE_IBQ_OUT is too big"
#endif

#ifndef BQ_DEFAULT_BURST_SIZE_OBQ_OUT
#define BQ_DEFAULT_BURST_SIZE_OBQ_OUT  144
#endif
#if (BQ_DEFAULT_BURST_SIZE_OBQ_OUT > DHL_BQ_MBUF_ARRAY_SIZE)
#error "BQ_DEFAULT_BURST_SIZE_OBQ_OUT is too big"
#endif

//#ifndef BQ_DEFAULT_BURST_SIZE_IBQ_READ
//#define BQ_DEFAULT_BURST_SIZE_IBQ_READ  144
//#endif
//#if (BQ_DEFAULT_BURST_SIZE_IBQ_READ > BQ_MBUF_ARRAY_SIZE)
//#error "BQ_DEFAULT_BURST_SIZE_IBQ_READ is too big"
//#endif
//
//#ifndef BQ_DEFAULT_BURST_SIZE_OBQ_READ
//#define BQ_DEFAULT_BURST_SIZE_OBQ_READ  144
//#endif
//#if (BQ_DEFAULT_BURST_SIZE_OBQ_READ > BQ_MBUF_ARRAY_SIZE)
//#error "BQ_DEFAULT_BURST_SIZE_OBQ_READ is too big"
//#endif




struct bq_mbuf_array {
	struct rte_mbuf * array[DHL_BQ_MBUF_ARRAY_SIZE];
	uint32_t n_mbufs;
};


struct dhl_bq {
	uint16_t id;
	unsigned socket_id;
	struct rte_ring * ring;

	/* Internal buffers */
	struct bq_mbuf_array mbuf_in;
	struct bq_mbuf_array mbuf_out;

	TAILQ_ENTRY(dhl_bq) next;			/** <Next buffer queue in linked lists */
};

/** Double linked list of dhl_ibq */
TAILQ_HEAD(dhl_bq_list, dhl_bq);


#endif /* DHL_MANAGER_DHL_BQ_H_ */
