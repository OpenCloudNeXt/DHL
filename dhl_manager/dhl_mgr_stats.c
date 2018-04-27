
/********************************************************************************************

                                   dhl_stats.c
	this file contain the implementation of all functions related to
	statistics display in the manager.

********************************************************************************************/

#include "dhl_mgr_stats.h"

#include <unistd.h>
#include <time.h>
#include <stdio.h>

#include "dhl_mgr.h"

/************************Internal Functions Prototypes************************/

/*
 * Function displaying statistics for all dma channels
 *
 * Input : time passed since last display (to compute packet rate)
 *
 */
static void
dhl_stats_display_dmas(unsigned difftime);

/*
 * Function displaying statistics for all NFs
 *
 */
static void
dhl_stats_display_nfs(unsigned difftime);

/*
 * Function clearing the terminal and moving back the cursor to the top left.
 *
 */
static void
dhl_stats_clear_terminal(void);

static void
dhl_stats_flush(void);

static void
dhl_stats_truncate(void);


/*********************Stats Output Streams************************************/

static FILE *stats_out;


/****************************Interfaces***************************************/

void
dhl_stats_set_output(DHL_STATS_OUTPUT output) {
	if (output != DHL_STATS_NONE) {
		switch (output) {
		case DHL_STATS_STDOUT:
			stats_out = stdout;
			break;
		case DHL_STATS_STDERR:
			stats_out = stderr;
			break;
		case DHL_STATS_WEB:
			stats_out = NULL;
			RTE_LOG(INFO, MANAGER, "web server has not been supported!\n");
			break;
		default:
			rte_exit(-1, "Error handling stats output file\n");
			break;
		}
	}
}

void
dhl_stats_cleanup(void) {
	if (manager.stats_destination == DHL_STATS_WEB) {
		fclose(stats_out);
//		fclose(json_stats_out);
	}
}

void
dhl_stats_display_all(unsigned difftime) {
	if (stats_out == stdout) {
		dhl_stats_clear_terminal();
	}else {
		dhl_stats_truncate();
	}

	dhl_stats_display_dmas(difftime);
	dhl_stats_display_nfs(difftime);

	dhl_stats_flush();
}



/****************************Internal functions*******************************/

static void
dhl_stats_display_dmas(unsigned difftime) {

}

static void
dhl_stats_display_nfs(unsigned difftime) {
	char * nf_label = NULL;
	unsigned i = 0;

}


/***************************Helper functions**********************************/
static void
dhl_stats_clear_terminal(void) {
	const char clr[] = { 27, '[', '2', 'J', '\0' };
	const char topLeft[] = { 27, '[', '1', ';', '1', 'H', '\0' };

	fprintf(stats_out, "%s%s", clr, topLeft);
}


static void
dhl_stats_flush(void) {
	if (stats_out == stdout || stats_out == stderr) {
		return;
	}

	fflush(stats_out);
}

static void
dhl_stats_truncate(void) {
	if (stats_out == stdout || stats_out == stderr) {
		return;
	}
}
