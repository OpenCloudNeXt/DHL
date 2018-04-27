/***********************************************************************

							dhl_stats.h

***********************************************************************/
#ifndef DHL_MANAGER_DHL_MGR_STATS_H_
#define DHL_MANAGER_DHL_MGR_STATS_H_

#define DHL_STR_STATS_STDOUT "stdout"
#define DHL_STR_STATS_STDERR "stderr"
#define DHL_STR_STATS_WEB "web"

typedef enum {
	DHL_STATS_NONE = 0,
	DHL_STATS_STDOUT,
	DHL_STATS_STDERR,
	DHL_STATS_WEB
} DHL_STATS_OUTPUT;



/*********************************Interfaces**********************************/

/*
 * Interface called by the manager to tell the stats module where to print
 * You should only call this once
 *
 * Input: a STATS_OUTPUT enum value representing output destination.  If
 * STATS_NONE is specified, then stats will not be printed to the console or web
 * browser.  If STATS_STDOUT or STATS_STDOUT is specified, then stats will be
 * output the respective stream.
 */
void dhl_stats_set_output(DHL_STATS_OUTPUT output);

/*
 *  Interface to close out file descriptions and clean up memory
 *  To be called when the stats loop is done
 */
void dhl_stats_cleanup(void);

/*
 * Interface called by the DHL Manager to display all statistics
 * available.
 *
 * Input : time passed since last display (to compute packet rate)
 *
 */
void dhl_stats_display_all(unsigned difftime);

#endif /* DHL_MANAGER_DHL_MGR_STATS_H_ */
