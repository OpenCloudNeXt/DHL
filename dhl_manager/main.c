
/********************************************************************************************
                                   main.c

		File containing the main function of the DHL manager and all its worker threads.

********************************************************************************************/

#include <signal.h>

#include "dhl_mgr.h"
#include "dhl_mgr_stats.h"


/******************************Internal Declarations***************************************/
#define MAX_SHUTDOWN_ITERS 10


static void handle_signal(int sig);

struct dhl_mgr manager;

/*******************************Worker threads********************************/

/*
 * Stats thread periodically prints DMA statics.
 */
static void
master_thread_main(void) {
	uint16_t i;
	int shutdown_iter_count;
	const unsigned sleeptime = manager.global_stats_sleep_time;

	RTE_LOG(INFO, MANAGER, "Core %d: Running master thread\n", rte_lcore_id());
	/* Initial pause so above printf is seen */
	sleep(5);

#ifdef DHL_STATS_WEB
	if(stats_destination == DHL_STATS_WEB) {
		RTE_LOG(INFO, MANAGER, "DHL stats can be viewed through the web console\n");
		RTE_LOG(INFO, MANAGER, "\tTo activate, please run $DHL_HOME/dhl_web/start_web_console.sh\n");
	}
#endif

	/* Loop forever: sleep always return 0 or <= parm */
	while( manager.main_keep_running && sleep(sleeptime) <= sleeptime ){
		dhl_check_nf_message();
		if (manager.stats_destination != DHL_STATS_NONE)
			dhl_stats_display_all(sleeptime);
	}

	/* close out file refrences and things */
	dhl_stats_cleanup();

	RTE_LOG(INFO, MANAGER, "Core %d: Initiating shutdown sequence\n", rte_lcore_id());

	/* Stop all the component, including Pakcer, Distributor and Controller*/
	manager.worker_keep_running = false;

	/* Notify all NFs to disable DHL framework */
	for (i = 0; i < MAX_NFS; i++){
		if ( manager.nfs[i].info == NULL )
			continue;

		RTE_LOG(INFO, MANAGER, "Core %d: Notifying NF %"PRIu8" to disable DHL\n", rte_lcore_id(), i);
//		dhl_nf_send_msg(i, MSG_STOP, NULL);
	}

	/* wait to all the process exits */
	for (shutdown_iter_count = 0;
			shutdown_iter_count < MAX_SHUTDOWN_ITERS && manager.num_registered_nfs > 0;
			shutdown_iter_count++) {
		dhl_check_nf_message();
		RTE_LOG(INFO, MANAGER, "Core %d: Waiting for \n", rte_lcore_id());

		sleep(sleeptime);
	}

	if(manager.num_registered_nfs > 0) {

	}

	RTE_LOG(INFO, MANAGER, "Core %d: DHL runtime controller thread done\n", rte_lcore_id());
}




static void handle_signal(int sig) {
	if (sig == SIGINT || sig == SIGTERM ) {
		manager.main_keep_running = false;
	}
}


/*******************************Main function*********************************/
int
main(int argc, char ** argv) {
	unsigned i;
	int ret;
	unsigned cur_lcore, lcore_id;

	/* Reserve ID 0 for internal manager things */
	manager.next_nf_instance_id = 1;

	// Start the DHL manger with 0 NF registered
	manager.num_registered_nfs = 0;

	manager.main_keep_running = true;
	manager.worker_keep_running = true;

	if (init(argc, argv) < 0 )
		return -1;
	RTE_LOG(INFO, MANAGER, "Finish Process Init.\n");

	/* Listen for ^C and terminate stop so we can exit gracefully */
	signal(SIGINT, handle_signal);
	signal(SIGTERM, handle_signal);

	/* reserve cores for : 1 Stats, 1  and DHL_NUM_DMA for transfer data between manager and FPGA*/
	cur_lcore = rte_lcore_id();

	for(i = 0; i < manager.packer.num_thread; i++){
		cur_lcore = manager.packer.packer_thread[i].lcore;

		if(rte_eal_remote_launch(packer_thread_main, NULL, cur_lcore) == -EBUSY) {
			RTE_LOG(ERR, MANAGER, "Core %d is already busy, cannot start packer thread %d\n",
					cur_lcore, manager.packer.packer_thread[i].id);
			return -1;
		}
	}

	for(i = 0; i < manager.distributor.num_thread ; i++) {
		cur_lcore = manager.distributor.distributor_thread[i].lcore;

		if(rte_eal_remote_launch(distributor_thread_main, NULL, cur_lcore) == -EBUSY) {
			RTE_LOG(ERR, MANAGER, "Core %d is already busy, cannot start distributor thread %d\n",
					cur_lcore, manager.distributor.distributor_thread[i].id);
			return -1;
		}
	}

	/* Master thread handles statistics and NF management */
//	rte_eal_remote_launch(master_thread_main, NULL, CALL_MASTER);
	master_thread_main();


	RTE_LCORE_FOREACH_SLAVE(lcore_id) {
		if (rte_eal_wait_lcore(lcore_id) < 0) {
			ret = -1;
			break;
		}
	}
	return 0;

}
