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

/********************************DHL library*********************************/
#include <dhl_fpga.h>

/******************************Internal headers*******************************/

#include "dhl_includes.h"
#include "dhl_buffer_queue.h"
#include "dhl_mgr_stats.h"



/***********************************Macros************************************/

#define VIRTUAL_CHANNEL
#ifdef VIRTUAL_CHANNEL
#define VIRTUAL_CHANNEL_NAME "VIRTUAL_CHANNEL_NAME"
#define VIRTUAL_CHANNEL_SIZE 16384
#endif

/* define the logtype of this app */
#define RTE_LOGTYPE_MANAGER RTE_LOGTYPE_USER1

#define PACKET_READ_SIZE ((uint16_t)32)

#define IBQ_DEQUEUE_PKTS	1

#define NF_INFO_SIZE sizeof(struct dhl_nf_info)
#define NF_INFO_CACHE_SIZE 4

#define NF_MSG_SIZE sizeof(struct dhl_nf_msg)
#define NF_MSG_CACHE_SIZE 8

#define NF_MSG_QUEUE_SIZE 128


#define NO_FLAGS 0

#define MAX_NF_INSTANCE_ID 65535

/*******************************Data Structures*******************************/
struct worker_lcores {
	uint8_t num;
	uint8_t lcores[RTE_MAX_LCORE];
};

struct packer_thread {
	unsigned id;
	unsigned lcore;			// which cpu core the packer thread working on
	unsigned socket_id;
	struct rte_ring * ibq;	// which shared input buffer queue the packer thread working on

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
//	struct dhl_bq_list ibq_list;

	struct worker_lcores packer_params;
};

struct distributor {
	unsigned num_thread;
	struct distributor_thread * distributor_thread;
//	struct dhl_bq_list obq_list;

	struct worker_lcores distributor_params;
};

struct dhl_fpga {
	uint8_t num_fpgas;
	uint8_t id[DHL_MAX_FPGA_CARDS];
	unsigned socket_id[DHL_MAX_FPGA_CARDS];

	uint8_t	num_socket;
	unsigned socket_ids[MAX_NUMA_NODE];
};



struct hardware_function_table_item {
	const char * hf_name;
	unsigned socket_id;
	unsigned acclerator_id;
	unsigned fpga_id;
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
//	uint32_t burst_size_ibq_read;
	uint32_t burst_size_ibq_out;
//	uint32_t burst_size_obq_read;
	uint32_t burst_size_obq_out;

	/* NFs */
	struct dhl_nf * nfs;
	struct rte_mempool * nf_info_pool;

	/* FPGAs */
	struct dhl_fpga fpgas;
	uint8_t num_fpgas;

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

/***************************    interfaces         ***************************/
int packer_thread_main(void * dummy);
int distributor_thread_main(void * dummy);

/* from dhl_mgr_args.c */
int parse_manager_args(uint8_t max_fpgas, int argc, char * argv[]);

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
