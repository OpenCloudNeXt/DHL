/*
 * dhl_nflib.h
 *
 *  Created on: Apr 9, 2018
 *      Author: lxy
 */

#ifndef DHL_NFLIB_DHL_NFLIB_H_
#define DHL_NFLIB_DHL_NFLIB_H_

#include <rte_mbuf.h>

#include "dhl_common.h"
#include "dhl_pkt_common.h"


/* define the logtype of NF application using DHL */
#define RTE_LOGTYPE_NF RTE_LOGTYPE_USER2

/************************************API**************************************/

/*
 * For software NF to register with DHL runtime manager
 * nf_tag : claimed NF name
 *
 * @return
 * 	the pointer of dhl_nf struct, which is allocated in DHL manager
 */
struct dhl_nf *
dhl_register(const char * nf_tag);

/*
 *  For a registered NF to unregister with DHL runtime manger,
 *  giving back all the DHL related resources to manger.
 *
 *  nf	: the pointer of struct dhl_nf, which is giving of registering.
 *
 *  @return :
 *  	-1: failed;
 *  	0 : succeed;
 */
int
dhl_unregister(struct dhl_nf * nf);


int
dhl_search_accelerator(const char * acc_name);

int
dhl_load_pr(const char * file_path);

int
dhl_configure_accelerator(int acc_id);

int
dhl_send_packets(struct rte_mbuf * packets);

int
dhl_get_packets(struct rte_mbuf ** packets);
#endif /* DHL_NFLIB_DHL_NFLIB_H_ */
