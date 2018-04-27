/*
 * vc709_specific.h
 *
 *  Created on: Jun 22, 2017
 *      Author: root
 */

#ifndef DMA_VC709_SPECIFIC_H_
#define DMA_VC709_SPECIFIC_H_

/* Driver states */
#define UNINITIALIZED   0	/* Not yet come up */
#define INITIALIZED     1	/* But not yet set up for polling */
#define UNREGISTERED    2       /* Unregistering with DMA */
#define POLLING         3	/* But not yet registered with DMA */
#define REGISTERED      4	/* Registered with DMA */
#define CLOSED          5	/* Driver is being brought down */


#define DESIGN_MODE_ADDRESS 0x9004		/* Used to configure HW for different modes */
//#define PERF_DESIGN_MODE   0x00000000
#define PERF_DESIGN_MODE   0x00000003

#define BURST_SIZE      256

#define XXGE_RCW0_OFFSET        0x00000400 /**< Rx Configuration Word 0 */
#define XXGE_RCW1_OFFSET        0x00000404 /**< Rx Configuration Word 1 */
#define XXGE_TC_OFFSET          0x00000408 /**< Tx Configuration */


#define TX_CONFIG_ADDRESS(_i)       (0x9108 + ((_i) *0x100)) /* Reg for controlling TX data */
#define RX_CONFIG_ADDRESS(_i)       (0x9100 + ((_i) *0x100)) /* Reg for controlling RX pkt generator */
#define PKT_SIZE_ADDRESS(_i)       (0x9104 + ((_i) *0x100)) /* Reg for programming packet size */
#define STATUS_ADDRESS(_i)          (0x910C + ((_i) *0x100)) /* Reg for checking TX pkt checker status */
#define SEQNO_WRAP_REG(_i)          (0x9110 + ((_i) *0x100)) /* Reg for sequence number wrap around */


/* Test start / stop conditions */
#define PKTCHKR             0x00000001	/* Enable TX packet checker */
#define PKTGENR             0x00000001	/* Enable RX packet generator */
#define CHKR_MISMATCH       0x00000001	/* TX checker reported data mismatch */
#define LOOPBACK            0x00000002	/* Enable TX data loopback onto RX */

#define ENGINE_TX0       0
#define ENGINE_RX0       32
#define NW_PATH_OFFSET0         0xB000
#define NW_PATH_OFFSET_OTHER0   0xC000

#define ENGINE_TX1       1
#define ENGINE_RX1       33
#define NW_PATH_OFFSET1         0xC000
#define NW_PATH_OFFSET_OTHER1   0xB000


#define ENGINE_TX2       2
#define ENGINE_RX2       34
#define NW_PATH_OFFSET2         0xD000
#define NW_PATH_OFFSET_OTHER2   0xE000

#define ENGINE_TX3       3
#define ENGINE_RX3       35
#define NW_PATH_OFFSET3         0xE000
#define NW_PATH_OFFSET_OTHER3   0xD000

/* Packet characteristics */
#define PAGE_SIZE 		(4096)
#define BUFSIZE         (PAGE_SIZE)
#define MAXPKTSIZE      (4*PAGE_SIZE - 1)



#define MINPKTSIZE      (64)
#define NUM_BUFS        2048
#define BUFALIGN        8
#define BYTEMULTIPLE    8   /**< Lowest sub-multiple of memory path */

#endif /* DMA_VC709_SPECIFIC_H_ */
