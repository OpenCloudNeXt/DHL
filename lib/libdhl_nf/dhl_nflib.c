/*
 * dhl_nflib.c
 *
 *  Created on: Apr 9, 2018
 *      Author: lxy
 */

#include "dhl_includes.h"
#include "dhl_nflib.h"


/******************************Global Variables*******************************/

// ring used for NF -> mgr messages (like startup & shutdown)
static struct rte_ring *mgr_msg_queue;

// ring used for mgr -> NF messages
//static struct rte_ring *nf_msg_ring;

// Shared data from DHL manager. We update statistics here
struct dhl_nf *nfs;

// Shared pool for all NFs info
static struct rte_mempool * nf_info_mp;

// Shared pool for mgr <--> NF messages
static struct rte_mempool *nf_msg_pool;

// User-given NF Client ID (defaults to manager assigned)
static uint16_t initial_nf_id = NF_NO_ID;


/***********************Internal Functions Prototypes*************************/
static struct dhl_nf_info *
dhl_nflib_info_init(const char * tag);

/************************************API**************************************/

struct dhl_nf *
dhl_register(const char * nf_tag) {
	const struct rte_memzone *mz_nf;
	struct dhl_nf_msg * register_msg;
	struct dhl_nf_info *nf_info;

	/* Lookup mempool for nf_info struct */
	nf_info_mp = rte_mempool_lookup(NF_INFO_MEMPOOL_NAME);
	if (nf_info_mp == NULL) {
		RTE_LOG(ERR, NF, "No NF Info mempool - bye\n");
		return NULL;
	}

	/* Lookup mempool for NF messages */
	nf_msg_pool = rte_mempool_lookup(NF_MSG_MEMPOOL_NAME);
	if (nf_msg_pool == NULL) {
		RTE_LOG(ERR, NF, "No NF Message mempool - bye\n");
		return NULL;
	}

	/* Initialize the info struct */
	nf_info = dhl_nflib_info_init(nf_tag);

	/* Lookup mempool for NF structs */
	mz_nf = rte_memzone_lookup(MZ_MANAGER_NF_INFO);
	if (mz_nf == NULL) {
		RTE_LOG(ERR, NF, "Cannot get NF structure mempool\n");
		return NULL;
	}
	nfs = mz_nf->addr;

	mgr_msg_queue = rte_ring_lookup(MANAGER_MSG_QUEUE_NAME);
	if (mgr_msg_queue == NULL) {
		RTE_LOG(ERR, NF, "Cannot get nf_info ring");
		return NULL;
	}

	/* Put this NF's info struct onto queue for manager to process startup */
	if (rte_mempool_get(nf_msg_pool, (void**)(&register_msg)) != 0) {
			rte_mempool_put(nf_info_mp, nf_info); // give back memory
			RTE_LOG(ERR, NF, "Cannot create startup msg");
			return NULL;
	}

	register_msg->msg_type = MSG_NF_REGISTERING;
	register_msg->msg_data = nf_info;

	if (rte_ring_enqueue(mgr_msg_queue, register_msg) < 0) {
		rte_mempool_put(nf_info_mp, nf_info); //give back memory
		rte_mempool_put(nf_msg_pool, register_msg);
		RTE_LOG(ERR, NF, "Cannot send register message to DHL manager.\n");
		return NULL;
	}

	/* Wait for a NF id to be assigned by the manager */
	RTE_LOG(INFO, NF, "Waiting for manager to assign an ID...\n");
	for (; nf_info->status == (uint8_t)NF_WAITING_FOR_ID ;) {
			sleep(1);
	}

	/* This NF is trying to declare an ID already in use. */
	if (nf_info->status == NF_ID_CONFLICT) {
		rte_mempool_put(nf_info_mp, nf_info);
		RTE_LOG(ERR, NF, "Selected ID already in use. Exiting...\n");
		return NULL;
	} else if(nf_info->status == NF_NO_IDS) {
		rte_mempool_put(nf_info_mp, nf_info);
		RTE_LOG(ERR, NF, "There are no ids available for this NF\n");
		return NULL;
	} else if(nf_info->status == NF_MEMORY_FAILED) {
		rte_mempool_put(nf_info_mp, nf_info);
		RTE_LOG(ERR, NF, "There are no memory to create the structure like obq, msq_queue, etc, for this NF\n");
		return NULL;
	} else if(nf_info->status != NF_REGISTERED) {
		rte_mempool_put(nf_info_mp, nf_info);
		RTE_LOG(ERR, NF, "Error occurred during manager initialization\n");
		return NULL;
	}
	RTE_LOG(INFO, NF, "Using NF ID %d\n", nf_info->nf_id);

	return &nfs[nf_info->index];
}

int
dhl_unregister(struct dhl_nf* nf) {
	struct dhl_nf_info *nf_info;
	struct dhl_nf_msg * unregister_msg;

	nf_info = nf->info;
	if(nf == NULL || nf_info == NULL || nf_info->status != NF_REGISTERED) {
		RTE_LOG(ERR, NF, "Unregister the error nf!\n");
		return -1;
	}

	/* Put this NF's info struct onto queue for manager to process startup */
	if (rte_mempool_get(nf_msg_pool, (void**)(&unregister_msg)) != 0) {
			RTE_LOG(ERR, NF, "Cannot create unregister msg");
			return -1;
	}

	nf_info->status = NF_UNREGISTERING;
	unregister_msg->msg_type = MSG_NF_UNREGISTERING;
	unregister_msg->msg_data = nf_info;

	if (rte_ring_enqueue(mgr_msg_queue, unregister_msg) < 0) {
		rte_mempool_put(nf_msg_pool, unregister_msg);
		RTE_LOG(ERR, NF, "Cannot send unregister message to DHL manager.\n");
		return -1;
	}

	/* Wait for a NF id to be assigned by the manager */
	RTE_LOG(INFO, NF, "Waiting for manager to unregister nf %s ...\n", nf_info->tag);
	for (; nf_info->status == (uint8_t)NF_UNREGISTERING ;) {
			sleep(1);
	}

	if(nf_info->status != NF_UNREGISTERED) {
		RTE_LOG(ERR, NF, "Error in ungistering the nf %s.\n", nf_info->tag);
		return -1;
	}

	rte_mempool_put(nf_info_mp, nf_info);
	RTE_LOG(INFO, NF, "NF %s id %d unregister success.\n", nf_info->tag, nf_info->nf_id);

	return 0;
}

int
dhl_search_accelerator(const char * acc_name __rte_unused) {
	return 0;
}

int
dhl_load_pr(const char * file_path __rte_unused) {
	return 0;
}

int
dhl_configure_accelerator(int acc_id __rte_unused) {
	return 0;
}

int
dhl_send_packets(struct rte_mbuf * packets __rte_unused) {
	return 0;
}

int
dhl_get_packets(struct rte_mbuf ** packets __rte_unused){
	return 0;
}


/******************************Helper functions*******************************/
static struct dhl_nf_info *
dhl_nflib_info_init(const char * tag) {
	struct dhl_nf_info * info;

	if (rte_mempool_get(nf_info_mp, (void **)&info) < 0 ){
		rte_exit(EXIT_FAILURE, "Failed to an NF info struct from mempool.\n");
	}

	if (info == NULL) {
		rte_exit(EXIT_FAILURE, "Client Info struct not allocated\n");
	}

	info->nf_id = initial_nf_id;
	info->acc_id = ACC_ID_UNSIGNED;
	info->status = NF_WAITING_FOR_ID;
	info->index = NF_NO_INDEX;
	info->socket_id = rte_socket_id();
	strcpy(info->tag,tag);

	return info;
}









