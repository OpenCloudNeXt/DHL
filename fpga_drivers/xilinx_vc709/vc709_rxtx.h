/*
 * vc709_rxtx.h
 *
 *  Created on: Jun 20, 2017
 *      Author: root
 */

#ifndef VC709_RXTX_H_
#define VC709_RXTX_H_

/* for Dma_Bd struct */
#include "../../fpga_driver/xilinx_vc709/dma/xdma.h"
#include "../../fpga_driver/xilinx_vc709/dma/xdma_bd.h"

/*
 * Rings setup and release.
 *
 * TDBA/RDBA should be aligned on 16 byte boundary. But TDLEN/RDLEN should be
 * multiple of 128 bytes. So we align TDBA/RDBA on 128 byte boundary. This will
 * also optimize cache line size effect. H/W supports up to cache line size 128.
 */
#define	VC709_ALIGN	0x40


#define	VC709_MIN_RING_DESC	32
#define	VC709_MAX_RING_DESC	2040

#define RTE_PMD_VC709_TX_MAX_BURST 32
#define RTE_PMD_VC709_RX_MAX_BURST 32
#define RTE_VC709_TX_MAX_FREE_BUF_SZ 64

#define DMA_BDRING_SZ (VC709_MAX_RING_DESC * \
		    sizeof(Dma_Bd))

/* This defines the direction arg to the DMA mapping routines. */
#define PCI_DMA_BIDIRECTIONAL	0
#define PCI_DMA_TODEVICE	1
#define PCI_DMA_FROMDEVICE	2
#define PCI_DMA_NONE		3

#define ENCAP_MBUF_CACHE_SIZE	256

/**
 * Structure associated with each descriptor of the RX ring of a RX queue.
 */
struct vc709_sw_entry {
	struct rte_mbuf *mbuf; /**< mbuf associated with TX desc, if any. */
	uint16_t next_id; /**< Index of next descriptor in ring. */
	uint16_t last_id; /**< Index of last scattered descriptor. */
};


struct vc709_rx_engine {
	struct rte_mempool *mb_pool; 			/**< mbuf pool to populate RX ring. */

	Dma_Bd * bd_ring; 						/**< BD ring virtual address. */
	uint64_t bd_ring_phys_addr; 			/**< DMA BD ring physical address. */

	Dma_Engine * eptr;

	struct vc709_sw_entry *sw_ring; 		/**< address of dma software ring. */
	uint16_t            nb_desc; 		/**< number of RX descriptors. */
	uint16_t            hw_engine_id; 		/**< DMA RX engine index for DMA IP core. */
	uint16_t            sw_engine_id; 		/**< DMA RX engine index for data->rx_engines. */
	uint8_t             card_id;  			/**< Device port identifier. */
	uint16_t 			sw_tail;
	uint16_t			sw_rx_head;			/**rx should read from this head */

};

struct vc709_tx_engine {
	Dma_Bd * bd_ring; 						/**< BD ring virtual address. */
	uint64_t bd_ring_phys_addr; 			/**< DMA BD ring physical address. */

	Dma_Engine * eptr;

	struct vc709_sw_entry *sw_ring; 		/**< address of dma software ring. */
	uint16_t            nb_desc; 		/**< number of RX descriptors. */
	uint16_t            hw_engine_id; 		/**< DMA TX engine index for DMA IP core. */
	uint16_t            sw_engine_id; 		/**< DMA TX engine index for data->tx_engines. */
	uint8_t             card_id;  			/**< Device port identifier. */
	uint16_t 			sw_tail;
	uint16_t			sw_rx_head;			/**rx should read from this head */
};

void vc709_dev_tx_engine_release(struct dhl_fpga_dev * dev, uint16_t engine_idx);

void vc709_dev_rx_engine_release(struct dhl_fpga_dev * dev, uint16_t engine_idx);

int vc709_dev_rx_engine_setup(struct dhl_fpga_dev *dev,
		uint16_t sw_engine_id,
		uint16_t nb_rx_desc,
		unsigned int socket_id,
		const struct dhl_fpga_rxconf *rx_conf,
		struct rte_mempool *mb_pool);

int vc709_dev_tx_engine_setup(struct dhl_fpga_dev * dev,
		uint16_t sw_engine_id,
		uint16_t nb_tx_desc,
		unsigned int socket_id,
		const struct dhl_fpga_txconf *tx_conf);

int vc709_dev_rx_engine_start(struct dhl_fpga_dev * dev, uint16_t sw_engine_id);

int vc709_dev_tx_engine_start(struct dhl_fpga_dev * dev, uint16_t engine_id);

int vc709_dev_rx_engine_stop(struct dhl_fpga_dev * dev, uint16_t rx_engine_id);

int vc709_dev_tx_engine_stop(struct dhl_fpga_dev * dev, uint16_t engine_id);



void
vc709_setup_recv_buffers(struct dhl_fpga_dev *dev,
		uint16_t eng_indx);

int16_t
vc709_dma_recv_pkts(struct dhl_fpga_dev *dev,
		uint16_t eng_indx,
		struct rte_mbuf **rx_pkts,
		uint16_t numpkts);

int16_t
vc709_dma_recv_pkts_calc_latency(struct dhl_fpga_dev *dev,
		uint16_t eng_indx,
		struct rte_mbuf **rx_pkts,
		uint16_t numpkts);

int16_t vc709_dma_send_pkts(struct dhl_fpga_dev *dev,
		uint16_t eng_indx,
		struct rte_mbuf **tx_pkts,
		uint16_t numpkts);

int16_t vc709_dma_send_pkts_noseg(struct dhl_fpga_dev *dev,
		uint16_t eng_indx,
		struct rte_mbuf **tx_pkts,
		uint16_t numpkts);

int16_t vc709_dma_send_pkts_noseg_add_timestamp(struct dhl_fpga_dev *dev,
		uint16_t eng_indx,
		struct rte_mbuf **tx_pkts,
		uint16_t numpkts);

int16_t vc709_dma_send_pkts_burst(struct dhl_fpga_dev *dev,
		uint16_t eng_indx,
		struct rte_mbuf **tx_pkts,
		uint16_t numpkts);





#endif /* VC709_RXTX_H_ */
