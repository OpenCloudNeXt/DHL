
/********************************************************************************************

                                   dhl_init.c

				File containing initialization functions.

********************************************************************************************/
//#include <limits.h>


#include "dhl_mgr.h"
#include "dhl_msg_common.h"


#define PACKER_THREAD_ZONE_NAME "manager_packer_threads"
#define DISTRIBUTOR_THREAD_ZONE_NAME "manager_distributor_threads"


/********************************Global variables*****************************/
/* global variable for number of currently active NFs - extern in header dhl_init.h */



/*****************************Internal functions******************************/
static int
init_packer(void) {
	unsigned i, j;
	unsigned lcore;

	struct packer * packer = &manager.packer;
	struct worker_lcores * packer_params = &packer->packer_params;

	if(packer_params->num <= 0){
		RTE_LOG(ERR, MANAGER, "There is no lcore for packer thread, please run %s by specifying the '-p' parameters.\n", progname);
		return -1;
	}
	packer->num_thread = packer_params->num;
	packer->packer_thread = (struct packer_thread *)rte_zmalloc(PACKER_THREAD_ZONE_NAME,
			sizeof(struct packer_thread) * packer->num_thread,
			RTE_CACHE_LINE_SIZE);

	if(packer->packer_thread == NULL){
		RTE_LOG(ERR, MANAGER, "Cannot allocate memory for packer->packer_thread.\n");
		return -1;
	}


	for(i = 0; i < packer->num_thread; i++) {
		lcore = packer_params->lcores[i];
		if (!rte_lcore_is_enabled(lcore)) {
			rte_exit(EXIT_FAILURE, "lcore %hhu is not enabled in lcore mask.\n", lcore);
			return -1;
		}

		packer->packer_thread[i].id = i;
		packer->packer_thread[i].lcore = lcore;
		packer->packer_thread[i].socket_id = rte_lcore_to_socket_id(lcore);
	}

	return 0;
}

static int
init_distributor(void) {
	unsigned i;
	unsigned lcore;

	struct distributor * distributor = &manager.distributor;
	struct worker_lcores * distributor_params = &distributor->distributor_params;

	uint8_t num_thread = distributor_params->num;

	distributor->num_thread = num_thread;
	distributor->distributor_thread = rte_zmalloc(DISTRIBUTOR_THREAD_ZONE_NAME,
			sizeof(struct distributor_thread) * num_thread,
			0);

	if(distributor->distributor_thread == NULL)
		return -1;

	for(i = 0; i < num_thread; i++) {
		lcore = distributor_params->lcores[i];
		if (!rte_lcore_is_enabled(lcore)) {
			rte_exit(EXIT_FAILURE, "lcore %hhu is not enabled in lcore mask\n", lcore);
			return -1;
		}

		distributor->distributor_thread[i].id = i;
		distributor->distributor_thread[i].lcore = lcore;
		distributor->distributor_thread[i].socket_id = rte_lcore_to_socket_id(lcore);
		/* init the obq_list for this distributor thread*/
		TAILQ_INIT(&distributor->distributor_thread[i].obqs);
	}

	return 0;
}

static int
init_shared_ibqs(void){
	unsigned i,j;
	const char * ibq_name;
	unsigned socket_id;
	struct rte_ring * ibq;
//	struct dhl_bq * buffer_queue;

	unsigned count;
	const unsigned ringsize = IBQ_QUEUE_SIZE;

	struct dhl_fpga * fpgas = &manager.fpgas;
	struct packer * packer = &manager.packer;

	/* set up shared input buffer queues */
	for (i = 0; i< fpgas->num_socket; i++) {
		socket_id = fpgas->socket_id[i];

		ibq_name = get_input_buffer_queue_name(socket_id);
		ibq = rte_ring_create(ibq_name,
				ringsize, socket_id, RING_F_SC_DEQ);

		if(ibq == NULL) {
			RTE_LOG(ERR, MANAGER, "Cannot create shared input buffer queue for socket %u.\n", socket_id);
			return -1;
		}

//		buffer_queue = malloc(sizeof(*buffer_queue));
//		if(buffer_queue == NULL) {
//			RTE_LOG(ERR, MANAGER, "Cannot allocate memory for buffer queue structure.\n");
//			return -1;
//		}
//
//		buffer_queue->id = i;
//		buffer_queue->socket_id = socket_id;
//		buffer_queue->bq = ibq;

//		if (TAILQ_EMPTY(&ibq_list)) {
//			TAILQ_INSERT_TAIL(&ibq_list, buffer_queue, next);
//			RTE_LOG(INFO, MANAGER, "Create ibq %s(id:%d) for socket %d.\n",ibq_name, i,socket_id);
//		} else {
//			struct dhl_bq * buffer_queue2 = NULL;
//			int inserted = 0;
//
//			FOREACH_IBQ(buffer_queue2) {
//				if(buffer_queue->id > buffer_queue2->id){
//					continue;
//				} else if(buffer_queue->id < buffer_queue2->id){
//					TAILQ_INSERT_BEFORE(buffer_queue2, buffer_queue, next);
//					inserted = 1;
//					RTE_LOG(INFO, MANAGER, "Create ibq %s(id:%d) for socket %d.\n",ibq_name, i,socket_id);
//				} else { /*already created*/
//					inserted = 1;
//					//do nothing
//				}
//			}
//
//			if(!inserted) {
//				TAILQ_INSERT_TAIL(&ibq_list, buffer_queue, next);
//				RTE_LOG(INFO, MANAGER, "Create ibq %s(id:%d) for socket %d.\n",ibq_name, i,socket_id);
//			}
//		}

		count = 0;
		for(j = 0; j < packer->num_thread; j++) {
			if(packer->packer_thread[j].socket_id == socket_id) {
				packer->packer_thread[j].ibq = ibq;
				count ++;
			}
		}
		if(count == 0){
			RTE_LOG(ERR, MANAGER, "There are FPGAs in NUMA node %d, and create a shared IBQ for this NUMA node,"
					" but there is no packer thread for this NUMA node.", socket_id);
			return -1;
		}
	}

	return 0;
}

/*
 * initialise the fpgas information
 */
static void
init_fpga_info(unsigned fpga_num) {
	uint8_t i;
	unsigned cardid;
	unsigned socket_id;
	unsigned socket_exist;

	struct dhl_fpga * fpgas = &manager.fpgas;

	fpgas->num_fpgas = fpga_num;
	fpgas->num_socket = 0;
	for (cardid = 0; cardid < fpga_num; cardid++){
		socket_exist = 0;

		socket_id = dhl_fpga_dev_socket_id(cardid);
		fpgas->id[cardid] = cardid;
		fpgas->socket_id[cardid] = socket_id;

		for(i = 0; i < fpgas->num_socket; i++){
			if(socket_id == fpgas->socket_id[i]){
				socket_exist = 1;
				break;
			}else {
				continue;
			}
		}

		if(!socket_exist){
			fpgas->socket_ids[fpgas->num_socket] = socket_id;
			fpgas->num_socket++;
		}
	}
}

static void
init_fake_fpga_info() {
	struct dhl_fpga * fpgas = &manager.fpgas;

	/* init 1 fake fpga in 1 socket */
	fpgas->num_fpgas = 1;
	fpgas->num_socket = 1;

	fpgas->id[0] = 0;
	fpgas->socket_id[0] = 0;

	fpgas->num_socket = 1;
	fpgas->socket_ids[0] = 0;

//	/* init 2 fake fpgas in 2 sockets */
//	fpgas->num_fpgas = 2;
//	fpgas->num_socket = 2;
//
//	fpgas->id[0] = 0;
//	fpgas->id[1] = 1;
//	fpgas->socket_id[0] = 0;
//	fpgas->socket_id[1] = 1;
//
//	fpgas->num_socket = 2;
//	fpgas->socket_ids[0] = 0;
//	fpgas->socket_ids[1] = 1;
}

/*
 * Set up a mempool to store nf_info structs
 */
static int
init_nf_info_pool(void)
{
        /* don't pass single-producer/single-consumer flags to mbuf
         * create as it seems faster to use a cache instead */
        RTE_LOG(INFO, MANAGER,"Creating mempool '%s' for nf_info...\n", NF_INFO_MEMPOOL_NAME);
        manager.nf_info_pool = rte_mempool_create(NF_INFO_MEMPOOL_NAME, MAX_NFS,
                        NF_INFO_SIZE, NF_INFO_CACHE_SIZE,
                        0, NULL, NULL, NULL, NULL, rte_socket_id(), NO_FLAGS);

        return (manager.nf_info_pool == NULL);
}

/*
 * set up a mempool to store nf_msg structs
 */
static int
init_nf_msg_pool(void)
{
	/*
	 *
	 */
	RTE_LOG(INFO, MANAGER, "Creating mempool '%s' for nf_msg...\n", NF_MSG_MEMPOOL_NAME);
	manager.nf_msg_pool = rte_mempool_create(NF_MSG_MEMPOOL_NAME, MAX_NFS * NF_MSG_QUEUE_SIZE,
			NF_MSG_SIZE, NF_MSG_CACHE_SIZE,
			0, NULL, NULL, NULL, NULL, rte_socket_id(), NO_FLAGS);

	return (manager.nf_msg_pool == NULL);	/* 0 on success */
}



/*
 *  Allocate a rte_ring for newly created NFs
 */
static int
init_message_queue(void)
{
	manager.incoming_msg_queue = rte_ring_create(MANAGER_MSG_QUEUE_NAME,
			MAX_NFS,
			rte_socket_id(),
			RING_F_SC_DEQ);	//Multiple producer enqueue (default), single consumer dequeue.

	if(manager.incoming_msg_queue == NULL)
		rte_exit(EXIT_FAILURE, "Cannot create incoming msg queue\n");

	return 0;
}


static int
init_virtual_channel(void)
{
	manager.virtual_channel = rte_ring_create(VIRTUAL_CHANNEL_NAME,
			VIRTUAL_CHANNEL_SIZE,
			rte_socket_id(),
			NO_FLAGS);

	if(manager.virtual_channel == NULL)
			rte_exit(EXIT_FAILURE, "Cannot create virtual channel queue\n");

	return 0;
}


/*********************************Interfaces**********************************/

int
init(int argc, char * argv[] ) {
	int retval;
	int total_fpgas;
	const struct rte_memzone *mz_nf;

	/* init EAL, parsing EAL args */
	retval = rte_eal_init(argc, argv);
	if ( retval < 0)
		return -1;
	argc -= retval;
	argv += retval;

	/* get total number of fpga boards in the system */
	total_fpgas = dhl_fpga_dev_count();
	printf("\n");
		RTE_LOG(INFO, MANAGER, "There are %d FPGA cards in the system.\n", total_fpgas );
	manager.num_fpgas = total_fpgas;

	/* set up array for NFs */
	mz_nf = rte_memzone_reserve(MZ_MANAGER_NF_INFO, sizeof(struct dhl_nf) * MAX_NFS, rte_socket_id(), NO_FLAGS);
	if (mz_nf == NULL)
		rte_exit(EXIT_FAILURE, "Cannot reserve memory zone for nf information.\n");
	else
	{
		memset(mz_nf->addr, 0, sizeof(struct dhl_nf) * MAX_NFS);
		manager.nfs = mz_nf->addr;
	}

	/* parse application specific arguments */
	retval = parse_manager_args(total_fpgas, argc, argv);
	if (retval != 0)
		return -1;

	/* initialise fpga info */
//	init_fpga_info(total_fpgas);
	init_fake_fpga_info();

	/* initialise packer */
	retval = init_packer();
	if (retval != 0) {
		rte_exit(EXIT_FAILURE, "Cannot initialize manager packer.\n");
	}

	/* initialise packer */
	retval = init_distributor();
	if (retval != 0) {
		rte_exit(EXIT_FAILURE, "Cannot initialize manager distributor: %s\n", rte_strerror(rte_errno));
	}

	retval = init_shared_ibqs();
	if (retval != 0) {
		rte_exit(EXIT_FAILURE, "Cannot initialize shared input buffer queues: %s\n", rte_strerror(rte_errno));
	}

	 /* initialise nf info pool */
	retval = init_nf_info_pool();
	if (retval != 0) {
			rte_exit(EXIT_FAILURE, "Cannot create nf info mbuf pool: %s\n", rte_strerror(rte_errno));
	}

	/* initialise pool for NF messages */
	retval = init_nf_msg_pool();
	if (retval != 0) {
			rte_exit(EXIT_FAILURE, "Cannot create nf message pool: %s\n", rte_strerror(rte_errno));
	}

	/* initialise a queue for newly created NFs */
	init_message_queue();

#ifdef VIRTUAL_CHANNEL
	/* initialise a virtual channel between packer and distributor for debug,
	 * which is a rte_ring that simply forward the pkts from packer to distributor
	 */
	init_virtual_channel();
#endif

	return 0;
}
