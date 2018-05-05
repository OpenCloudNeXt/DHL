/***********************************************************************

							dhl_manager.h

		Header file containing all shared headers and data structures

***********************************************************************/
#ifndef DHL_MANAGER_DHL_MGR_H_
#define DHL_MANAGER_DHL_MGR_H_

/******************************Standard C library*****************************/


#include <netinet/ip.h>
#include <stdbool.h>
#include <math.h>
#include <stdbool.h>

/********************************DPDK library*********************************/


#include <rte_byteorder.h>
#include <rte_memcpy.h>
#include <rte_malloc.h>
#include <rte_fbk_hash.h>
#include <rte_prefetch.h>

/********************************DHL library*********************************/
#include <dhl_fpga.h>

/******************************Internal headers*******************************/

#include "dhl_includes.h"
#include "dhl_buffer_queue.h"
#include "dhl_mgr_stats.h"



/* define the logtype of this app */
#define RTE_LOGTYPE_MANAGER RTE_LOGTYPE_USER1

/* Macros related to virtual channel */
#define VIRTUAL_CHANNEL
#ifdef VIRTUAL_CHANNEL
#define VIRTUAL_CHANNEL_NAME "VIRTUAL_CHANNEL_NAME"
#define VIRTUAL_CHANNEL_SIZE 16384
#define VIRTUAL_CHANNEL_DEQUEUE_BURST 144
#endif

/* Macros related to Hardware Function Table */
#define HARDWARE_FUNCTION_TABLE_SIZE 100

/* Macros related to FPGA cards */
#define MAX_FPGA_CARDS 4
#define MAX_ACCELERATOR_PER_FPGA_CARD 10
#define ACCELERATOR_NAME_SIZE 256

#define PER_PACKET_SIZE 64
#define NUM_BD 2000
#define DMA_ENGINES 8

#define MAX_BATCHING_SIZE 64000
#define DHL_PKTMBUF_FPGA_HEADROOM 128

/* Macros related to system */
#define MAX_NUMA_NODE 4
#define MAX_NFS	8
#define MAX_INPUT_BUFFER_QUEUES	4

#define MAX_HARDWARE_FUNCION_TABLE_ITEM_NUM 100

#define IBQ_QUEUE_SIZE 16384	//size of shared input buffer queue
#define OBQ_QUEUE_SIZE 8192		//size of private output buffer queue for each NF

#define PACKET_READ_SIZE ((uint16_t)32)

#ifndef CONFIG_DHL_PACKER_IBQ_DEQUEUE_BURST_MAX
#define CONFIG_DHL_PACKER_IBQ_DEQUEUE_BURST_MAX	64
#endif

#define NF_INFO_SIZE sizeof(struct dhl_nf_info)
#define NF_INFO_CACHE_SIZE 4

#define NF_MSG_SIZE sizeof(struct dhl_nf_msg)
#define NF_MSG_CACHE_SIZE 8

#define NF_MSG_QUEUE_SIZE 128


#define NO_FLAGS 0

#define MAX_NF_INSTANCE_ID 65535

/* Macros related to dhl_mgr_nf */
#define NF_MESSAGE_QUEUE_SIZE 128

/* Macros related to dhl_manager prefetch */
#if MGR_PREFETCH_ENABLE
#define MGR_PREFETCH0(p)	rte_prefetch0(p)
#define MGR_PREFETCH1(p)	rte_prefetch1(p)
#else
#define MGR_PREFETCH0(p)
#define MGR_PREFETCH1(p)
#endif

/* define common names for structures shared between dhl runtime and NFs */
#define MBUF_POOL_FPGA_NAME "MBUF_POOL_FPGA_%u"
#define MANAGER_SHARED_IBQ_NAME "MANAGER_SHARED_IBQ_%u"
#define NF_PRIVATE_OBQ_NAME "NF_PRIVATE_OBQ_%u"
#define NF_MESSAGE_QUEUE_NAME "NF_MESSAGE_QUEUE_%u"

#define MZ_MANAGER_NF_INFO "MANAGER_NF"



/*******************************Data Structures*******************************/
struct worker_lcores {
	uint8_t num;
	uint8_t lcores[RTE_MAX_LCORE];
};

struct packer_thread {
	unsigned id;
	unsigned lcore;			// which cpu core the packer thread working on
	unsigned socket_id;

	struct dhl_bq * bq;		// which shared input buffer queue the packer thread working on
};

struct distributor_thread {
	unsigned id;
	unsigned lcore;			// which cpu core the distributor thread working on
	unsigned socket_id;
	struct dhl_bq_list obqs;
};

struct packer {
	unsigned num_thread;
	struct packer_thread * packer_thread;

	struct worker_lcores packer_params;
};

struct distributor {
	unsigned num_thread;
	struct distributor_thread * distributor_thread;

	struct worker_lcores distributor_params;
};

struct dhl_fpga {
	uint8_t num_fpgas;
	uint8_t id[DHL_MAX_FPGA_CARDS];
	unsigned socket_id[DHL_MAX_FPGA_CARDS];

	uint8_t	num_socket;
	unsigned socket_ids[MAX_NUMA_NODE];
};

struct dhl_fpga_accelerator {
	char acc_name[ACCELERATOR_NAME_SIZE];
	uint16_t acc_id;
};

struct dhl_mgr_fpga {
	uint8_t id;
	unsigned int socket_id;

	uint16_t num_accs;
	struct dhl_fpga_accelerator accs[MAX_ACCELERATOR_PER_FPGA_CARD];
};


struct hf_table_item {
	const char * hf_name;
	unsigned socket_id;
	unsigned acclerator_id;
	unsigned fpga_id;
};

struct hf_table {
	uint32_t num_items;
	struct hf_table_item hd_item[HARDWARE_FUNCTION_TABLE_SIZE];
};

/*
 * strcut declaration of DHL Manager
 */
struct dhl_mgr {
	/* running flags*/
	bool main_keep_running;
	bool worker_keep_running;

	/* stats */
	uint32_t global_stats_sleep_time;
	DHL_STATS_OUTPUT stats_destination;

	/* variables */
	uint16_t num_registered_nfs;
	uint16_t next_nf_instance_id;

	/* burst size of ibqs and obqs */
//	uint32_t burst_size_ibq_in;
	uint32_t burst_size_ibq_out;
	uint32_t burst_size_obq_in;
	uint32_t burst_size_obq_out;

	/* NFs */
	struct dhl_nf * nfs;
	struct rte_mempool * nf_info_pool;

	/* FPGAs */
	struct dhl_fpga fpgas;
	uint8_t num_fpgas;
	struct rte_mempool * mbuf_pool_fpga[MAX_FPGA_CARDS];

	/* Hardware Function Table */
	struct hf_table hf_table;

	/* nf_msg_pool to allocate dhl_nf_msg structure, defined in dhl_msg_common.h
	 * incoming_msg_queue is a queue to store the dhl_nf_msg[] for manager to handle*/
	struct rte_mempool * nf_msg_pool;
	struct rte_ring * incoming_msg_queue;

	struct packer packer;
	struct distributor distributor;

#ifdef VIRTUAL_CHANNEL
	struct rte_ring * virtual_channel;
#endif

} __rte_cache_aligned;


/***************************Shared global variables***************************/
extern struct dhl_mgr manager;
extern const char *progname;

/***************************    helper functions   ***************************/
static inline const char *
get_mbuf_pool_fpga_name(uint8_t fpgaid) {
	static char buffer[sizeof(MBUF_POOL_FPGA_NAME) + 2];

	snprintf(buffer, sizeof(buffer) - 1, MBUF_POOL_FPGA_NAME, fpgaid);
	return buffer;
}


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


/***************************    interfaces         ***************************/
int packer_thread_main(void * dummy);
int distributor_thread_main(void * dummy);

/* from dhl_mgr_args.c */
int parse_manager_args(int argc, char * argv[]);

/***   from dhl_mgr_init.c        ***/
/*
 * Function that initialize all data structures, memory mapping and global
 * variables.
 *
 * Input  : the number of arguments (following C conventions)
 *          an array of the arguments as strings
 * Output : an error code
 *
 */
int init(int argc, char *argv[]);

/***   from dhl_mgr_nf.c        ***/
/*
 * Interface to allocate a smallest unsigned integer unused for a NF instance.
 *
 * Output: the unsigned integer
 */
uint16_t dhl_nf_next_nf_id(void);

/* Interface looking through all registered NFs*/
void dhl_check_nf_message(void);

/*
 * Interface to send a message to a certain NF.
 *
 * Input  : The destination NF instance ID, a constant denoting the message type
 *          (see dhl_nflib/dhl_msg_common.h), and a pointer to a data argument.
 *          The data argument should be allocated in the hugepage region (so it can
 *          be shared), i.e. using rte_malloc
 * Output : 0 if the message was successfully sent, -1 otherwise
 */
int dhl_nf_send_msg(uint8_t dest, uint8_t msg_type, void * msg_data);

#endif /* DHL_MANAGER_DHL_MGR_H_ */
