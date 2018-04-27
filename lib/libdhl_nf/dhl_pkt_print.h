/*
 * dhl_pkt_print.h
 *
 *  Created on: Apr 18, 2018
 *      Author: root
 */

#ifndef DHL_NFLIB_DHL_PKT_PRINT_H_
#define DHL_NFLIB_DHL_PKT_PRINT_H_

#include <rte_mbuf.h>

void macPacketPrint(const unsigned char * buf, const int num);

void IpPacketPrint(unsigned char * buf, const int num);

void print_pkt_no_check(struct rte_mbuf * pkt);

#endif /* DHL_NFLIB_DHL_PKT_PRINT_H_ */
