/*
 * dhl_common.h
 *
 *  Created on: Oct 10, 2017
 *      Author: Xiaoyao Li
 */

#include "dhl_msg_common.h"

#ifndef DHL_NFLIB_DHL_COMMON_H_
#define DHL_NFLIB_DHL_COMMON_H_

/*
 * debug Macro
 */
#define Debug(_fmt, ...)						\
{							\
	fprintf(stderr, "DEBUG INFO: %s,%s() [%d]\n" _fmt ,	\
		__FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__);	\
}

#define MAX_NUMA_NODE 4
#define MAX_NFS	8
#define MAX_INPUT_BUFFER_QUEUES	4

#define MAX_HARDWARE_FUNCION_TABLE_ITEM_NUM 100

#define IBQ_QUEUE_SIZE 16384	//size of shared input buffer queue
#define OBQ_QUEUE_SIZE 8192		//size of private output buffer queue for each NF

#define NF_MESSAGE_QUEUE_SIZE 128


#define MAX_PACKER_CONFIG_PARAMS 10

/*
 * define a structure to describe a NF
 */
struct dhl_nf_info {
	uint16_t nf_id;
	uint16_t acc_id;
	uint8_t status;
	uint8_t index;		//index for the dhl_nf in the nfs array.
	unsigned socket_id;
	char tag[100];
};

struct dhl_nf {
	uint16_t nf_id;
	unsigned socket_id;			// which numa node the nf resides
	struct rte_ring * ibq;		// shared input buffer queue to enqueue pkts into
	struct rte_ring * obq;		// private output buffer queue to get pkts from FPGA
	struct rte_ring * msg_q;	// message queue for nfs to communicate with dhl manager
	struct dhl_nf_info * info;
	uint8_t used;				//indicate this dhl_nf whether is used
};


/* define common names for structures shared between dhl runtime and NFs */
#define MANAGER_SHARED_IBQ_NAME "MANAGER_SHARED_IBQ_%u"
#define NF_PRIVATE_OBQ_NAME "NF_PRIVATE_OBQ_%u"
#define NF_MESSAGE_QUEUE_NAME "NF_MESSAGE_QUEUE_%u"

#define MZ_MANAGER_NF_INFO "MANAGER_NF"





/* define common names for structures shared between server and NF */
#define MANAGER_MSG_QUEUE_NAME	"MANAGER_MSG_QUEUE"

#define NF_INFO_MEMPOOL_NAME "NF_INFO_MEMPOOL"
#define NF_MSG_MEMPOOL_NAME "NF_MSG_MEMPOOL"

/* common names for accelerator id */
#define ACC_ID_UNSIGNED -1
#define NF_NO_INDEX -1

/* common names for NF states */
#define NF_WAITING_FOR_ID 0     // First step in startup process, doesn't have ID confirmed by manager yet
#define NF_REGISTERED 1           // When a NF is in the startup process and already has an id
#define NF_UNREGISTERING 2            // Running normally
#define NF_UNREGISTERED  3            // NF is not receiving packets, but may in the future
#define NF_STOPPED 4            // NF has stopped and in the shutdown process
#define NF_ID_CONFLICT 5        // NF is trying to declare an ID already in use
#define NF_NO_IDS 6             // There are no available IDs for this NF
#define NF_MEMORY_FAILED 7
//#define NF_OBQ_FAILED 8

#define NF_NO_ID -1



static inline const char *
get_input_buffer_queue_name(uint16_t id) {
        /* buffer for return value. Size calculated by %u being replaced
         * by maximum 3 digits (plus an extra byte for safety) */
        static char buffer[sizeof(MANAGER_SHARED_IBQ_NAME) + 2];

        snprintf(buffer, sizeof(buffer) - 1, MANAGER_SHARED_IBQ_NAME, id);
        return buffer;
}

/*
 * Given the name template above,
 * get the private output buffer ring name of NF.
 */
static inline const char *
get_output_buffer_quque_name(uint16_t id) {
	static char buffer[sizeof(NF_PRIVATE_OBQ_NAME) + 2];

	snprintf(buffer, sizeof(buffer) - 1, NF_PRIVATE_OBQ_NAME, id);
	return buffer;
}

/*
 * Given the name template above,
 * get the manager -> NF message name
 */
static inline const char *
get_nf_msg_queue_name(uint16_t nf_id) {
	/* buffer for return value. Size calculated by %u being replaced
	         * by maximum 3 digits (plus an extra byte for safety) */
	static char buffer[sizeof(NF_MESSAGE_QUEUE_NAME) + 2];

	snprintf(buffer, sizeof(buffer) - 1, NF_MESSAGE_QUEUE_NAME, nf_id);
	return buffer;
}

/*
 * Interface checking if a given NF is "valid", meaning if it's registered.
 */
static inline int
dhl_nf_is_valid(struct dhl_nf *nf) {
        return nf && nf->info && nf->info->status == NF_REGISTERED;
}


#endif /* DHL_NFLIB_DHL_COMMON_H_ */
