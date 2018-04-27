
/********************************************************************************************

                                   dhl_nf.c

		This file contains all functions related to NF management

********************************************************************************************/
#include "dhl_mgr.h"

#include "dhl_mgr_stats.h"
#include "dhl_msg_common.h"

/************************Internal functions prototypes************************/

static bool
dhl_nf_id_conflict(uint16_t nf_id);

/*
 * Function registering a NF.
 *
 * Input  : a pointer to the NF's informations
 * Output : an error code
 *
 */
static int
dhl_nf_registering(struct dhl_nf_info *nf_info);

/*
 * Function unregistering a NF.
 *
 * Input  : a pointer to the NF's informations
 * Output : an error code
 *
 */
static int
dhl_nf_unregistering(struct dhl_nf_info *nf_info);

static int
dhl_assign_obq_to_distributor_thread();

static inline struct dhl_bq * dhl_bq_copy(struct dhl_bq * bq) {
	struct dhl_bq * new_bq = (struct dhl_bq *)malloc(sizeof(struct dhl_bq));
	new_bq->bq = bq->bq;
	new_bq->id = bq->id;
	new_bq->socket_id = bq->socket_id;
//	new_bq->next = NULL;

	return new_bq;
}

/********************************Interfaces***********************************/
uint16_t
dhl_next_nf_id(void) {
	uint16_t nf_id;
	struct dhl_nf * nf;
	bool conflict;

	while (manager.next_nf_instance_id < MAX_NF_INSTANCE_ID) {
		nf_id = manager.next_nf_instance_id++;
		conflict = dhl_nf_id_conflict(nf_id);

		if (!conflict)
			break;
	}
	return nf_id;
}

void
dhl_check_nf_message(void) {
	int i, ret;
	void * msgs[MAX_NFS];
	struct dhl_nf_msg * msg;
	struct dhl_nf_info * nf_info;

	int num_msgs = rte_ring_count(manager.incoming_msg_queue);

	if (num_msgs == 0)
		return;

	if (rte_ring_dequeue_bulk(manager.incoming_msg_queue, msgs, num_msgs, NULL) == 0)
		return;

	for (i = 0; i < num_msgs; i++) {
		msg = (struct dhl_nf_msg *) msgs[i];
		nf_info = (struct dhl_nf_info*) msg->msg_data;

		switch (msg->msg_type) {
		case MSG_NF_REGISTERING:
			ret = dhl_nf_registering(nf_info);
			if(ret)
				RTE_LOG(INFO, MANAGER, "NF \"%s\" register failed!.\n", nf_info->tag);
			else
				RTE_LOG(INFO, MANAGER, "NF \"%s\" register Success.\n", nf_info->tag);
			break;
		case MSG_NF_UNREGISTERING:
			ret = dhl_nf_unregistering(nf_info);
			if(ret)
				RTE_LOG(INFO, MANAGER, "NF \"%s\" unregister failed!.\n", nf_info->tag);
			else
				RTE_LOG(INFO, MANAGER, "NF \"%s\" unregister success.\n", nf_info->tag);
			break;
		}

		// give dhl_nf_msg back to its mempool
		rte_mempool_put(manager.nf_msg_pool, (void *)msg);
	}
}



int
dhl_nf_send_msg(uint8_t dest, uint8_t msg_type, void * msg_data) {
	int ret;
	struct dhl_nf_msg * msg;

	struct dhl_nf * nfs = manager.nfs;

	ret = rte_mempool_get(manager.nf_msg_pool, (void **)(&msg));
	if(ret != 0) {
		RTE_LOG(INFO, MANAGER, "The huge page error! Unable to allocate msg from poll. \n");
		return ret;
	}

	msg->msg_type = msg_type;
	msg->msg_data = msg_data;

	if(&nfs[dest] != NULL && nfs[dest].msg_q != NULL){
		return rte_ring_sp_enqueue(nfs[dest].msg_q, (void *) msg);
	}

	return -1;
}



/******************************Internal functions*****************************/
static bool
dhl_nf_id_conflict(uint16_t nf_id) {
	unsigned i;
	struct dhl_nf * nf = NULL;

	struct dhl_nf * nfs = manager.nfs;
	bool conflict = false;
	for(i = 0; i < MAX_NFS; i++) {
		nf = nfs + i;
		if(nf->used && nf->nf_id == nf_id) {
			conflict = true;
			break;
		}
	}
	return conflict;
}

static uint8_t
dhl_nf_get_valid_index() {
	uint8_t index;
	struct dhl_nf * nf;

	struct dhl_nf * nfs = manager.nfs;
	if(manager.num_registered_nfs >= MAX_NFS)
		return NF_NO_INDEX;

	for(index = 0; index < MAX_NFS; index++) {
		nf = nfs + index;
		if(nf->used)
			continue;
		else
			return index;
	}
	return NF_NO_INDEX;
}


static int
dhl_assign_obq_to_distributor_thread(struct dhl_bq * bq) {
	unsigned i;
	struct distributor_thread * thread;

	struct distributor * distributor = &manager.distributor;
	for(i = 0; i < distributor->num_thread; i++) {
		thread = distributor->distributor_thread + i;
		if(thread == NULL) {
			RTE_LOG(ERR, MANAGER, "distributor_thread %d is NULL.\n", thread->id);
			return -1;
		}
		if(thread->socket_id != bq->socket_id)
			continue;

		TAILQ_INSERT_TAIL(&thread->obqs, bq, next);
		return 0;
	}

	RTE_LOG(ERR, MANAGER, "Created obq is located in socket %d,"
			" but there is no distributor_thread in the same socket.\n", bq->socket_id);
	return -1;
}

static int
dhl_remove_obq_from_distirbutor_thread(struct dhl_nf_info * nf_info) {
	unsigned i;
	struct distributor_thread * thread;
	struct dhl_bq * obq;

	struct distributor * distributor = &manager.distributor;
	for(i = 0; i < distributor->num_thread; i++) {
		thread = distributor->distributor_thread + i;
		if(thread == NULL) {
			RTE_LOG(ERR, MANAGER, "distributor_thread %d is NULL.\n", thread->id);
			return -1;
		}
		if(thread->socket_id != nf_info->socket_id)
			continue;

		TAILQ_FOREACH(obq, &thread->obqs, next) {
			if(obq->id == nf_info->nf_id){
				TAILQ_REMOVE(&thread->obqs, obq, next);
				free(obq);
				break;
			}
		}
		return 0;
	}
	RTE_LOG(ERR, MANAGER, "Remove obq from distributor_thread failed.\n");
	return -1;
}

/*
 * get shared input buffer queue, based on socket_id
 */
static int
dhl_get_shared_ibq(struct dhl_nf_info * nf_info) {
	unsigned i;
	struct rte_ring * ibq;
	struct packer_thread * thread;
	unsigned socket_id, nf_socket_id;

	struct packer * packer = &manager.packer;
	nf_socket_id = nf_info->socket_id;

	for(i = 0; i < packer->num_thread; i++) {
		thread = packer->packer_thread + i;
		socket_id = thread->socket_id;
		if(socket_id == nf_socket_id) {
			ibq = thread->ibq;
			break;
		}
	}

	if(ibq == NULL) {
		RTE_LOG(ERR, MANAGER, "There is no shared ibq to be assigned to nf \"%s\".\n", nf_info->tag);
		nf_info->status = NF_MEMORY_FAILED;
		return -1;
	}

	struct dhl_nf * nfs = manager.nfs;
	nfs[nf_info->index].ibq = ibq;
	return 0;
}

static int
dhl_create_obq(struct dhl_nf_info * nf_info) {
	const char * private_obq_name;
	const unsigned obq_size = OBQ_QUEUE_SIZE;

	unsigned nf_socket_id;
	uint16_t nf_id;
	struct dhl_bq * buffer_queue;
	struct rte_ring * obq;

	if(nf_info == NULL) {
		RTE_LOG(ERR, MANAGER, "While create obq, the nf_info is NULL.\n");
		return -1;
	}

	nf_id = nf_info->nf_id;
	nf_socket_id = nf_info->socket_id;

	// Create private output buffer queue
	private_obq_name = get_output_buffer_quque_name(nf_id);
	RTE_LOG(INFO, MANAGER, "Create obq for NF %s in socket %d.\n", nf_info->tag, nf_socket_id);
	obq = rte_ring_create(private_obq_name,
			obq_size, nf_socket_id,
			RING_F_SC_DEQ);			/* multi prod, single cons */

	if (obq == NULL) {
		RTE_LOG(ERR, MANAGER, "Creating private OBQ for NF \"%s\" failed, error %d: %s\n", nf_info->tag, rte_errno, rte_strerror(rte_errno));
		nf_info->status = NF_MEMORY_FAILED;
		return -1;
	}

	// create bq struct for tailq obq_list and add it to obq_list
	buffer_queue = malloc(sizeof(*buffer_queue));
	if(buffer_queue == NULL) {
		RTE_LOG(ERR, MANAGER, "Cannot allocate memory for buffer queue structure (obq_list).\n");
		nf_info->status = NF_MEMORY_FAILED;
		rte_ring_free(obq);
		return -1;
	}

	buffer_queue->id = nf_id;
	buffer_queue->socket_id = nf_socket_id;
	buffer_queue->bq = obq;

	if(dhl_assign_obq_to_distributor_thread(buffer_queue)) {
		nf_info->status = NF_MEMORY_FAILED;
		rte_ring_free(obq);
		free(buffer_queue);
		return -1;
	}

	struct dhl_nf * nfs = manager.nfs;
	nfs[nf_info->index].obq = obq;
	return 0;
}

static int
dhl_create_msg_queue(struct dhl_nf_info * nf_info) {
	const char * msg_queue_name;
	const unsigned msq_queue_size = NF_MESSAGE_QUEUE_SIZE;
	struct rte_ring * msg_queue;
	unsigned socket_id;

	socket_id = nf_info->socket_id;
	msg_queue_name = get_nf_msg_queue_name(nf_info->nf_id);
	msg_queue = rte_ring_create(msg_queue_name,
			msq_queue_size, socket_id,
			RING_F_SC_DEQ); 		/* multi prod, single cons */
	if (msg_queue == NULL) {
		RTE_LOG(ERR, MANAGER, "Creating message queue for NF \"%s\" failed.\n", nf_info->tag);
		nf_info->status = NF_MEMORY_FAILED;
		return -1;
	}

	struct dhl_nf * nfs = manager.nfs;
	nfs[nf_info->index].msg_q = msg_queue;
	return 0;
}

static int
dhl_nf_registering(struct dhl_nf_info * nf_info) {
	uint8_t index;
	if(nf_info == NULL || nf_info->status != NF_WAITING_FOR_ID)
		return 1;

	//First, check whether there is free dhl_nf elemet in nfs to allocate.
	index = dhl_nf_get_valid_index();
	if(index == NF_NO_INDEX){
		RTE_LOG(ERR, MANAGER, "The count of registered NFs has reached maximum (%d), cannot register new NF any more.\n",MAX_NFS);
		return 1;
	}

	// if NF passed its own id on the command line, don't assign here
	// assume user is smart enough to avoid duplicates
	uint16_t nf_id = nf_info->nf_id == (uint16_t)NF_NO_ID
			? dhl_next_nf_id()
			: nf_info->nf_id;

	if (nf_id >= MAX_NF_INSTANCE_ID){
		// There are no more available IDs for this NF
		nf_info->status = NF_NO_IDS;
		return 1;
	}

	if (dhl_nf_id_conflict(nf_id)) {
		// This NF is trying to declare an ID already in use
		nf_info->status = NF_ID_CONFLICT;
		return 1;
	}

	// Keep reference to this NF in the manager
	RTE_LOG(INFO, MANAGER, "Assigning nf id '%d' to \"%s\".\n", nf_id, nf_info->tag);
	nf_info->nf_id = nf_id;
	nf_info->index = index;

	//get shared ibq
	if(dhl_get_shared_ibq(nf_info))
		return -1;

	//create private obq
	if(dhl_create_obq(nf_info))
		return -1;

	//create nf message ring to receive message from manager
	if(dhl_create_msg_queue(nf_info))
		return -1;

	struct dhl_nf * nfs = manager.nfs;
	nfs[index].nf_id = nf_id;
	nfs[index].socket_id = nf_info->socket_id;
	nfs[index].info = nf_info;
	nfs[index].used = 1;
	manager.num_registered_nfs++;
	// Let the NF continue its init process
	nf_info->status = NF_REGISTERED;

	return 0;
}

static int
dhl_nf_unregistering(struct dhl_nf_info * nf_info) {
	uint16_t nf_id;
	uint8_t index;

	struct dhl_nf * nfs = manager.nfs;

	if(nf_info == NULL || nf_info->status != NF_UNREGISTERING)
		return -1;

	RTE_LOG(INFO, MANAGER, "NF %s is unregistering.\n", nf_info->tag);
	nf_id = nf_info->nf_id;
	index = nf_info->index;

	if(nfs[index].obq != NULL)
		rte_ring_free(nfs[index].obq);

	dhl_remove_obq_from_distirbutor_thread(nf_info);

	if(nfs[index].msg_q != NULL)
		rte_ring_free(nfs[index].msg_q);

	nfs[index].nf_id = NF_NO_ID;
	nfs[index].ibq = NULL;
	nfs[index].obq = NULL;
	nfs[index].msg_q = NULL;
	nfs[index].info = NULL;
	nfs[index].used = 0;

	manager.num_registered_nfs--;
	nf_info->status = NF_UNREGISTERED;
	return 0;
}


