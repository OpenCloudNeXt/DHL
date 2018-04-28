/*
 * vc709_rxtx.c
 *
 *  Created on: Jun 20, 2017
 *      Author: root
 */

#include <sys/queue.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <stdarg.h>
#include <unistd.h>
#include <inttypes.h>

#include <rte_mbuf.h>
#include <rte_bus_pci.h>
#include <rte_pci.h>
#include <rte_malloc.h>
#include <rte_log.h>
#include <rte_prefetch.h>
#include <rte_config.h>
#include <rte_cycles.h>

#include <dhl_fpga.h>

#include "vc709_rxtx.h"

#include "dma/xdma.h"
#include "dma/xdma_bd.h"
#include "dma/xdma_user.h"
#include "dma/xdma_hw.h"
#include "dma/xstatus.h"
#include "vc709_fpga.h"
#include "vc709_log.h"
#include "vc709_pci_uio.h"
#include "vc709_specific.h"

#if 1
#define RTE_PMD_USE_PREFETCH
#endif

#ifdef RTE_PMD_USE_PREFETCH
/*
 * Prefetch a cache line into all cache levels.
 */
#define rte_vc709_prefetch(p)   rte_prefetch0(p)
#else
#define rte_vc709_prefetch(p)   do {} while (0)
#endif

static int tmp;


/*
 *	function declarations
 */


/*
 * engine_id 0 2 4 6
 * type: 0-s2c 1:c2s
 */
static void DmaFifoEmptyWait(void * baseVAddr, int engine_id)
{
	u64 barBase = (u64) baseVAddr;
	u64 statusReg = barBase + STATUS_REG_OFFSET;

	u32 data = 0;
	int timeout = FIFO_EMPTY_TIMEOUT;

	int check = 1 << (engine_id + EMPTY_MASK_SHIFT);

	do
	{
		data = XIo_In32(statusReg);
		if (data & check)
		{
			RTE_LOG(INFO, PMD, "DDR FIFO is empty now.\n");
			break;
		}
		else
		{
			RTE_LOG(INFO, PMD, "DDR FIFO Not empty.\n");
			rte_delay_ms(1);
		}
	}while(timeout--);

	if(timeout == -1)
	{
		RTE_LOG(INFO, PMD,"************** Timeout DDR FIFO not empty **************\n");
	}
}

static inline int
vc709_dma_PktHandler(struct dhl_fpga_dev *dev, uint16_t eng_indx){
	struct vc709_adapter * adapter;
	Dma_Engine * eptr;
	Dma_BdRing * rptr;
	Dma_Bd *BdPtr, *BdCurPtr;
	int result = XST_SUCCESS;
	unsigned int bd_processed, bd_processed_save;
	int j;

	adapter = (struct vc709_adapter *)dev->data->dev_private;
	eptr = &(adapter->dmaEngine.Dma[eng_indx]);
	rptr = &(eptr->BdRing);

	/* Handle engine operations */
	bd_processed_save = 0;
	if ((bd_processed = Dma_BdRingFromHw(rptr, adapter->dmaEngine.Dma_Bd_nums[eng_indx], &BdPtr)) > 0)
	{
		bd_processed_save = bd_processed;
		BdCurPtr = BdPtr;
		j = 0;
		do
		{
			Dma_mBdSetId_NULL(BdCurPtr, 0);
			Dma_mBdSetPageAddr(BdCurPtr,0);

			Dma_mBdSetStatus(BdCurPtr,0);

			BdCurPtr = Dma_mBdRingNext(rptr, BdCurPtr);
			bd_processed--;
			j++;

		} while (  bd_processed > 0);

		result = Dma_BdRingFree(rptr, bd_processed_save, BdPtr);
		if (result != XST_SUCCESS) {
			RTE_LOG( ERR, PMD, "PktHandler: BdRingFree() error %d.", result);
			return XST_FAILURE;
		}
		return XST_SUCCESS;
	}else{
		return XST_FAILURE;
	}

}


static int
descriptor_free(struct dhl_fpga_dev *dev,
		uint16_t engine_idx, uint16_t nb_dma_bd){

	Dma_Engine * eptr;
	Dma_BdRing * rptr;
	Dma_Bd *BdPtr, *BdCurPtr;
	struct vc709_dma_engine * dma_engine;

	unsigned int bd_processed, bd_processed_save;
	int j;
	int result;

	dma_engine = VC709_DEV_PRIVATE_TO_DMAENGINE(dev->data->dev_private);
	eptr = &(dma_engine->Dma[engine_idx]);
	rptr = &(eptr->BdRing);


	/* First recover buffers and BDs queued up for DMA, then pass to user */
	bd_processed_save = 0;
	if ((bd_processed = Dma_BdRingForceFromHw(rptr, nb_dma_bd, &BdPtr)) > 0)
	{
		RTE_LOG(INFO, PMD, "engine %d descriptor_free: Forced %d BDs from hw\n",engine_idx, bd_processed);

		bd_processed_save = bd_processed;
		BdCurPtr = BdPtr;
		j = 0;
		do
		{
			/* reset BD id */
			Dma_mBdSetId_NULL(BdCurPtr, 0);
			Dma_mBdSetPageAddr(BdCurPtr,0LL);

			BdCurPtr = Dma_mBdRingNext(rptr, BdCurPtr);
			bd_processed--;
			j++;
		} while (bd_processed > 0);


		result = Dma_BdRingFree(rptr, bd_processed_save, BdPtr);
		if (result != XST_SUCCESS) {
			/* Will be freeing the ring below. */
			RTE_LOG( ERR, PMD,"engine %d BdRingFree() error %d.\n", engine_idx, result);
			return XST_FAILURE;
		}
	}

	return 0;

}

static int
vc709_dev_hw_engine_release(struct dhl_fpga_dev *dev,
	    uint16_t engine_id)
{
	struct vc709_dma_engine * dma_engine;
	struct vc709_bar * bar;
	Dma_Engine * eptr;
	int nb_dma_bd;

	dma_engine = VC709_DEV_PRIVATE_TO_DMAENGINE(dev->data->dev_private);
	bar = VC709_DEV_PRIVATE_TO_BAR(dev->data->dev_private);
	eptr = &dma_engine->Dma[engine_id];


	XIo_Out32 ( (eptr->RegBase + TX_CONFIG_ADDRESS(engine_id)), 0);
	XIo_Out32 ( (eptr->RegBase + RX_CONFIG_ADDRESS(engine_id)), 0);

	DmaFifoEmptyWait((void *)bar->barBase, engine_id);

	// wait for appropriate time to stabalize
	rte_delay_ms(STABILITY_WAIT_TIME);

	/* Is the engine assigned to any user? */
	if(eptr->EngineState != USER_ASSIGNED) {
		RTE_LOG(ERR, PMD, "HW DMA Engine %d is not assigned to any user\n", engine_id);
		return XST_FAILURE;
	}

	/* Change DMA engine state */
	eptr->EngineState = UNREGISTERING;

	Dma_Reset(eptr);

	nb_dma_bd = dma_engine->Dma_Bd_nums[engine_id];

	if( descriptor_free(dev, engine_id, nb_dma_bd) != 0){
		RTE_LOG(ERR, PMD, "HW DMA Engine %d free descriptors failed.\n", engine_id);
		return -1;
	}

	/* Change DMA engine state */
	eptr->EngineState = INITIALIZED;

	return 0;
}

static void __attribute__((cold))
vc709_sw_tx_engine_release_pktbufs(struct vc709_tx_engine * txe)
{
	unsigned i;

	if(txe->sw_ring != NULL) {
		for(i = 0; i< txe->nb_desc; i++) {
			if(txe->sw_ring[i].mbuf != NULL) {
				rte_pktmbuf_free_seg(txe->sw_ring[i].mbuf);
				txe->sw_ring[i].mbuf = NULL;
			}
		}
	}
}

static void __attribute__((cold))
vc709_sw_tx_free_swring(struct vc709_tx_engine * txe)
{
	if (txe != NULL && txe->sw_ring != NULL)
		rte_free(txe->sw_ring);
}

static void __attribute__((cold))
vc709_sw_tx_engine_release(struct vc709_tx_engine * txe)
{
	if(txe != NULL) {
		vc709_sw_tx_engine_release_pktbufs(txe);
		vc709_sw_tx_free_swring(txe);
		rte_free(txe);
	}
}

void __attribute__((cold))
vc709_dev_tx_engine_release(struct dhl_fpga_dev * dev, uint16_t engine_idx)
{
	int hw_engine_idx = engine_idx;
	struct vc709_tx_engine * txe = dev->data->tx_engines[engine_idx];

	vc709_sw_tx_engine_release(txe);
	vc709_dev_hw_engine_release(dev, hw_engine_idx);
}


static void __attribute__((cold))
vc709_sw_rx_engine_release_pktbufs(struct vc709_rx_engine * rxe){
	unsigned i;

	if (rxe->sw_ring != NULL) {
		for (i = 0; i < rxe->nb_desc; i++) {
			if (rxe->sw_ring[i].mbuf != NULL) {
				rte_pktmbuf_free_seg(rxe->sw_ring[i].mbuf);
				rxe->sw_ring[i].mbuf = NULL;
			}
		}
	}
}

static void __attribute__((cold))
vc709_sw_rx_engine_release(struct vc709_rx_engine * rxe)
{
	if(rxe != NULL) {
		vc709_sw_rx_engine_release_pktbufs(rxe);
		rte_free(rxe->sw_ring);
		rte_free(rxe);
	}
}

void __attribute__((cold))
vc709_dev_rx_engine_release(struct dhl_fpga_dev * dev, uint16_t engine_idx){
	int hw_engine_idx = engine_idx + 32;
	struct vc709_rx_engine * rxe = dev->data->rx_engines[engine_idx];

	vc709_sw_rx_engine_release(rxe);
	vc709_dev_hw_engine_release(dev, hw_engine_idx);
}


/* Reset dynamic vc709_rx_engine fields back to defaults */
static void __attribute__((cold))
vc709_reset_sw_rx_engine(struct vc709_rx_engine *eng)
{
	uint16_t prev, i;
	struct vc709_sw_entry * rxe = eng->sw_ring;

	/*
	 * Zero out HW ring memory. Zero out extra memory at the end of
	 * the H/W ring so look-ahead logic in Rx Burst bulk alloc function
	 * reads extra memory as zeros.
	 */
	for (i = 0; i < eng->nb_desc; i++) {
		memset(&(eng->bd_ring[i]), 0, sizeof(Dma_Bd));
	}

	/* Initialize SW ring entries */
	prev = (uint16_t) (eng->nb_desc - 1);
	for (i = 0; i < eng->nb_desc; i++) {
		rxe[i].mbuf = NULL;
		rxe[i].last_id = i;
		rxe[prev].next_id = i;
		prev = i;
	}

	eng->sw_tail = 0;
	eng->sw_rx_head = 0;
}

/* Reset dynamic vc709_tx_engine fields back to defaults */
static void __attribute__((cold))
vc709_reset_hw_tx_engine(struct vc709_tx_engine *eng)
{
	uint16_t prev, i;
	struct vc709_sw_entry * rxe = eng->sw_ring;

	/*
	 * Zero out HW ring memory. Zero out extra memory at the end of
	 * the H/W ring so look-ahead logic in Rx Burst bulk alloc function
	 * reads extra memory as zeros.
	 */
	for (i = 0; i < eng->nb_desc; i++) {
		memset(&(eng->bd_ring[i]), 0, sizeof(Dma_Bd));
	}

	/* Initialize SW ring entries */
	prev = (uint16_t) (eng->nb_desc - 1);
	for (i = 0; i < eng->nb_desc; i++) {
		rxe[i].mbuf = NULL;
		rxe[i].last_id = i;
		rxe[prev].next_id = i;
		prev = i;
	}

	eng->sw_tail = 0;
	eng->sw_rx_head = 0;
}

void
vc709_setup_recv_buffers(struct dhl_fpga_dev *dev,
		uint16_t rx_eng_indx)
{
	int free_bd_count ;
	Dma_Engine * eptr;
	Dma_BdRing * rptr;
	Dma_Bd *BdPtr, *BdCurPtr;
	uint64_t bufPA;
	int result;
	int i, numgot;

	/* like ixgbe */
	struct vc709_rx_engine * engine;
	struct vc709_sw_entry *sw_ring;
	struct vc709_sw_entry *rxe;
	struct rte_mbuf *nmb;
	struct rte_mbuf * tmp_mbuf[2000];
	uint16_t rx_id;

	engine = dev->data->rx_engines[rx_eng_indx];
	eptr = engine->eptr;
	rptr = &(eptr->BdRing);

	/* Check if this engine's pointer is valid */
	if(eptr == NULL)
	{
		RTE_LOG(ERR, PMD, "Handle is a NULL value\n");
		return;
	}

	rx_id = engine->sw_tail;
	sw_ring = engine->sw_ring;
	free_bd_count = Dma_mBdRingGetFreeCnt(rptr);
	/* Maintain a separation between start and end of BD ring. This is
	 * required because DMA will stall if the two pointers coincide -
	 * this will happen whether ring is full or empty.
	 */
	if(free_bd_count > 2)
		free_bd_count -= 2;
	else
		return;

	do{
		/* Allocate BDs from ring */
		result = Dma_BdRingAlloc(rptr, free_bd_count, &BdPtr);
		if (result != XST_SUCCESS) {
			/* We really shouldn't get this. Return unused buffers to app */
			RTE_LOG(ERR, PMD, "Engine %d DmaSetupRecvBuffers: BdRingAlloc unsuccessful (%d)\n", rx_eng_indx, result);
			break;
		}

		numgot = 0;
		BdCurPtr = BdPtr;
		for(i = 0; i < free_bd_count; i++){
			rxe = &sw_ring[rx_id];

			/* allocate a new mbuf to hold the received packet*/
			nmb = rte_mbuf_raw_alloc(engine->mb_pool);
			if (nmb == NULL){
				RTE_LOG(INFO, PMD, "Engine %d Rx mbuf alloc failed\n", rx_eng_indx );
				break;
			}
			bufPA = rte_cpu_to_le_64(rte_mbuf_data_iova_default(nmb));
			Dma_mBdSetBufAddr(BdCurPtr, bufPA);
			Dma_mBdSetCtrlLength(BdCurPtr, dev->data->dev_conf.rx_buf_size);
//			Dma_mBdSetCtrlLength(BdCurPtr, dev->data->dev_conf.max_batching_size);
			Dma_mBdSetId(BdCurPtr, rte_cpu_to_le_64((u64)rte_mbuf_to_baddr(nmb)) );
			Dma_mBdSetCtrl(BdCurPtr, 0);        // Disable interrupts also.
			Dma_mBdSetUserData(BdCurPtr, 0LL);
			rxe->mbuf = nmb;
			tmp_mbuf[numgot] = nmb;

			BdCurPtr = Dma_mBdRingNext(rptr, BdCurPtr);
			numgot++;
			rx_id++;
			if (rx_id == engine->nb_desc)
				rx_id = 0;
		}

		/* Enqueue all Rx BDs with attached buffers such that they are
		 * ready for frame reception.
		 */
		if(numgot == 0)
			break;
		result = Dma_BdRingToHw(rptr, numgot, BdPtr);
		if (result != XST_SUCCESS) {
			RTE_LOG(ERR, PMD, "Engine %d DmaSetupRecvBuffers: BdRingToHw unsuccessful (%d)\n", rx_eng_indx,
					result);
			BdCurPtr = BdPtr;
			for(i = 0; i < numgot; i++)
			{
				Dma_mBdSetId_NULL(BdCurPtr, 0);
				BdCurPtr = Dma_mBdRingNext(rptr, BdCurPtr);
				rte_pktmbuf_free(tmp_mbuf[i]);
			}
			Dma_BdRingUnAlloc(rptr, free_bd_count, BdPtr);
			break;
		}
		free_bd_count -= numgot;
	} while(free_bd_count > 0);

	engine->sw_tail = rx_id;

}

//void
//vc709_setup_recv_buffers1(struct dhl_fpga_dev *dev,
//		uint16_t eng_indx)
//{
//	int free_bd_count ;
//	Dma_Engine * eptr;
//	Dma_BdRing * rptr;
//	Dma_Bd *BdPtr, *BdCurPtr;
//	uint64_t bufPA;
//	int numbds;
//	int result;
//	int i;
//
//	/* like ixgbe */
//	struct vc709_engine * engine;
//	struct vc709_sw_entry *sw_ring;
//	struct vc709_sw_entry *rxe;
//	struct rte_mbuf * allo_mbuf[2000];
//	struct rte_mbuf *nmb;
//	uint16_t rx_id;
//
//	engine = dev->data->engines[eng_indx];
//	eptr = engine->eptr;
//	rptr = &(eptr->BdRing);
//
//	/* Check if this engine's pointer is valid */
//	if(eptr == NULL)
//	{
//		RTE_LOG(ERR, PMD, "Handle is a NULL value\n");
//		return;
//	}
//
//	rx_id = engine->sw_tail;
//	sw_ring = engine->sw_ring;
//
//	free_bd_count = Dma_mBdRingGetFreeCnt(rptr);
//
//	/* Maintain a separation between start and end of BD ring. This is
//	 * required because DMA will stall if the two pointers coincide -
//	 * this will happen whether ring is full or empty.
//	 */
//	if(free_bd_count > 2)
//		free_bd_count -= 2;
//	else
//		return;
//
//	numbds = 0;
//	do{
//		/* Get buffers from user defined mempool */
//		result = rte_pktmbuf_alloc_bulk(engine->mb_pool,allo_mbuf,free_bd_count);
//		if(result != 0) {
//			RTE_LOG(ERR, PMD, "Engine %d could not get any mbuf for RX\n",eng_indx );
//		}
//
//		/* Allocate BDs from ring */
//		result = Dma_BdRingAlloc(rptr, free_bd_count, &BdPtr);
//		if (result != XST_SUCCESS) {
//			/* We really shouldn't get this. Return unused buffers to app */
//			RTE_LOG(ERR, PMD, "Engine %d DmaSetupRecvBuffers: BdRingAlloc unsuccessful (%d)\n", eng_indx, result);
//			for(i = 0; i < free_bd_count; i++){
//				rte_pktmbuf_free(allo_mbuf[i]);
//			}
//			break;
//		}
//
//		BdCurPtr = BdPtr;
//		for(i = 0; i < free_bd_count; i++){
//			rxe = &sw_ring[rx_id];
//
//			nmb = allo_mbuf[i];
//			bufPA = rte_cpu_to_le_64(rte_mbuf_data_dma_addr_default(nmb));
//			Dma_mBdSetBufAddr(BdCurPtr, bufPA);
//			Dma_mBdSetCtrlLength(BdCurPtr, dev->data->dev_conf.rx_buf_size);
////			Dma_mBdSetCtrlLength(BdCurPtr, dev->data->dev_conf.max_batching_size);
//			Dma_mBdSetId(BdCurPtr, rte_cpu_to_le_64((u64)rte_mbuf_to_baddr(nmb)) );
//			Dma_mBdSetCtrl(BdCurPtr, 0);        // Disable interrupts also.
//			Dma_mBdSetUserData(BdCurPtr, 0LL);
//			rxe->mbuf = nmb;
//
//			BdCurPtr = Dma_mBdRingNext(rptr, BdCurPtr);
//
//			rx_id++;
//			if (rx_id == engine->nb_desc)
//				rx_id = 0;
//		}
//
//		/* Enqueue all Rx BDs with attached buffers such that they are
//		 * ready for frame reception.
//		 */
//		result = Dma_BdRingToHw(rptr, free_bd_count, BdPtr);
//		if (result != XST_SUCCESS) {
//			/* Should not come here. Incase of error, unmap buffers,
//			 * unallocate BDs, and return buffers to app driver.
//			 */
//			RTE_LOG(ERR, PMD, "Engine %d DmaSetupRecvBuffers: BdRingToHw unsuccessful (%d)\n", eng_indx,
//					result);
//			BdCurPtr = BdPtr;
//			for(i = 0; i < free_bd_count; i++)
//			{
//				Dma_mBdSetId_NULL(BdCurPtr, 0);
//				BdCurPtr = Dma_mBdRingNext(rptr, BdCurPtr);
//				rte_pktmbuf_free(allo_mbuf[i]);
//			}
//			Dma_BdRingUnAlloc(rptr, free_bd_count, BdPtr);
//			break;
//		}
//		free_bd_count -= free_bd_count;
//		numbds += free_bd_count;
////		log_verbose("free_bd_count %d, numbds %d, numgot %d\n",free_bd_count, numbds, numgot);
//	} while(free_bd_count > 0);
//
//	engine->sw_tail = rx_id;
//
//}


int16_t
vc709_dma_recv_pkts(struct dhl_fpga_dev *dev,
		uint16_t eng_indx,
		struct rte_mbuf **rx_pkts,
		uint16_t numpkts __rte_unused)
{
	struct vc709_adapter * adapter;

	Dma_Engine * eptr;
	Dma_BdRing * rptr;
	Dma_Bd *BdPtr, *BdCurPtr;
	int result;

	struct vc709_rx_engine * engine;
	struct vc709_sw_entry *sw_ring;
	struct vc709_sw_entry *rxe;
	uint16_t rx_head;
	uint16_t nb_rx;
	uint16_t nb_rx_save;
	uint16_t buf_size;

	adapter = (struct vc709_adapter *)dev->data->dev_private;

	engine = dev->data->rx_engines[eng_indx];
	eptr = engine->eptr;
	rptr = &(eptr->BdRing);

	/* Check if this engine's pointer is valid */
	if(eptr == NULL)
	{
		RTE_LOG(ERR, PMD, "Handle is a NULL value\n");
		return -1;
	}

	if(!rptr->IsRxChannel)
	{
		RTE_LOG(ERR, PMD, "this engine is not a rx engine\n");
		return -1;
	}
	/* Is the engine assigned to any user? */
	if(eptr->EngineState != USER_ASSIGNED) {
		RTE_LOG(ERR, PMD, "Engine is not assigned to any user\n");
		return -1;
	}

	rx_head = engine->sw_rx_head;
	sw_ring = engine->sw_ring;

	int i;
	if((nb_rx = Dma_BdRingFromHw(rptr, adapter->dmaEngine.Dma_Bd_nums[eng_indx], &BdPtr)) > 0)
	{
		i = 0;
		nb_rx_save = nb_rx;
		BdCurPtr = BdPtr;
		do{
			rxe = &sw_ring[rx_head];
			rx_head++;
			if(rx_head == engine->nb_desc)
				rx_head = 0;

			buf_size = Dma_mBdGetStatLength(BdCurPtr);
//			printf("engine %d ,buf_size is %d\n", eng_indx, buf_size);

			Dma_mBdSetId_NULL(BdCurPtr, 0);
			Dma_mBdSetPageAddr(BdCurPtr,0);
			Dma_mBdSetStatus(BdCurPtr,0);

			rxe->mbuf->buf_len = buf_size;
			rxe->mbuf->pkt_len = buf_size;
			rxe->mbuf->data_len = buf_size;
			rx_pkts[i++] = rxe->mbuf;

			BdCurPtr = Dma_mBdRingNext(rptr, BdCurPtr);
			nb_rx--;
		} while (nb_rx > 0);

		result = Dma_BdRingFree(rptr, nb_rx_save, BdPtr);
		if (result != XST_SUCCESS) {
			RTE_LOG( ERR, PMD, "PktHandler: BdRingFree() error %d.", result);
			return -1;
		}
		engine->sw_rx_head = rx_head;
		vc709_setup_recv_buffers(dev,eng_indx);
		return nb_rx_save;
	}else{
		return 0;
	}

}

int16_t
vc709_dma_recv_pkts_calc_latency(struct dhl_fpga_dev *dev,
		uint16_t eng_indx,
		struct rte_mbuf **rx_pkts,
		uint16_t numpkts __rte_unused)
{
	struct vc709_adapter * adapter;

	Dma_Engine * eptr;
	Dma_BdRing * rptr;
	Dma_Bd *BdPtr, *BdCurPtr;
	int result;

	/* like ixgbe */
	struct vc709_rx_engine * engine;
	struct vc709_sw_entry *sw_ring;
	struct vc709_sw_entry *rxe;
	uint16_t rx_head;
	uint16_t nb_rx;
	uint16_t nb_rx_save;
	uint16_t buf_size;

	adapter = (struct vc709_adapter *)dev->data->dev_private;

	engine = dev->data->rx_engines[eng_indx];
	eptr = engine->eptr;
	rptr = &(eptr->BdRing);

	/* Check if this engine's pointer is valid */
	if(eptr == NULL)
	{
		RTE_LOG(ERR, PMD, "Handle is a NULL value\n");
		return -1;
	}

	if(!rptr->IsRxChannel)
	{
		RTE_LOG(ERR, PMD, "this engine is not a rx engine\n");
		return -1;
	}
	/* Is the engine assigned to any user? */
	if(eptr->EngineState != USER_ASSIGNED) {
		RTE_LOG(ERR, PMD, "Engine is not assigned to any user\n");
		return -1;
	}

	rx_head = engine->sw_rx_head;
	sw_ring = engine->sw_ring;
//	rxe = &sw_ring[rx_head];

	int i;
	if((nb_rx = Dma_BdRingFromHw(rptr, adapter->dmaEngine.Dma_Bd_nums[eng_indx], &BdPtr)) > 0)
	{
		i = 0;
		nb_rx_save = nb_rx;
		BdCurPtr = BdPtr;
		do{
			rxe = &sw_ring[rx_head];
			rx_head++;
			if(rx_head == engine->nb_desc)
				rx_head = 0;

			buf_size = Dma_mBdGetStatLength(BdCurPtr);


			Dma_mBdSetId_NULL(BdCurPtr, 0);
			Dma_mBdSetPageAddr(BdCurPtr,0);
			Dma_mBdSetStatus(BdCurPtr,0);

			BdCurPtr = Dma_mBdRingNext(rptr, BdCurPtr);

			rxe->mbuf->buf_len = buf_size;
			rxe->mbuf->pkt_len = buf_size;
			rxe->mbuf->data_len = buf_size;
//			printf("buf_size is %d\n",buf_size);
			dhl_mbuf_calc_latency(rxe->mbuf);
			rx_pkts[i++] = rxe->mbuf;

//			if(nb_rx_save >= numpkts)
//				break;

			nb_rx--;
		} while (nb_rx > 0);

		result = Dma_BdRingFree(rptr, nb_rx_save, BdPtr);
		if (result != XST_SUCCESS) {
			RTE_LOG( ERR, PMD, "PktHandler: BdRingFree() error %d.", result);
			return -1;
		}
		engine->sw_rx_head = rx_head;
		vc709_setup_recv_buffers(dev,eng_indx);
		return nb_rx_save;
	}else{
		return 0;
	}

}

int16_t
vc709_dma_send_pkts(struct dhl_fpga_dev *dev,
		uint16_t eng_indx,
		struct rte_mbuf **tx_pkts,
		uint16_t numpkts)
{
	int free_bd_count ;
	Dma_Engine * eptr;
	Dma_BdRing * rptr;
	Dma_Bd *BdPtr, *BdCurPtr;
	uint64_t bufPA;
	int result;
	u32 flags=0;
	int i,j;

	/* like ixgbe */
	struct vc709_tx_engine * engine;
	struct vc709_sw_entry *sw_ring;
	struct vc709_sw_entry *txe, *txn;
	struct rte_mbuf *tx_pkt;
	struct rte_mbuf *m_seg;
	uint16_t tx_id;
	uint16_t tx_last;
	uint16_t nb_tx;
	uint16_t nb_used;
	uint16_t nb_processed;

	engine = dev->data->tx_engines[eng_indx];
	sw_ring = engine->sw_ring;
	tx_id = engine->sw_tail;
	txe = &sw_ring[tx_id];

	eptr = engine->eptr;
	rptr = &(eptr->BdRing);

	/* Check if this engine's pointer is valid */
	if(eptr == NULL)
	{
		RTE_LOG(ERR, PMD, "Engine %d Handle is a NULL value\n", eng_indx);
		return -1;
	}

	/* Is the number valid? */
	if(numpkts <= 0) {
		RTE_LOG(ERR, PMD, "Engine %d ,Packet count should be non-zero\n",eng_indx);
		return -1;
	}

	/* Is the engine assigned to any user? */
	if(eptr->EngineState != USER_ASSIGNED) {
		RTE_LOG(ERR, PMD,"Engine %d is not assigned to any user\n",eng_indx);
		return -1;
	}

	/* Is the engine an S2C one? */
	if(rptr->IsRxChannel)
	{
		RTE_LOG(ERR, PMD,"Engine %d The requested engine cannot send packets\n", eng_indx);
		return -1;
	}

	free_bd_count = Dma_mBdRingGetFreeCnt(rptr) - 2;


	while(free_bd_count < numpkts) {
		vc709_dma_PktHandler(dev, eng_indx);
		free_bd_count = Dma_mBdRingGetFreeCnt(rptr) - 2;
	}

	/* Maintain a separation between start and end of BD ring. This is
	 * required because DMA will stall if the two pointers coincide -
	 * this will happen whether ring is full or empty.
	 */
	free_bd_count = Dma_mBdRingGetFreeCnt(rptr) - 2;


	for(nb_tx = 0; nb_tx < numpkts; nb_tx++){
		tx_pkt	= *tx_pkts++;

		nb_used = (uint16_t) (tx_pkt->nb_segs);
		tx_last = (uint16_t) (tx_id + nb_used - 1);

		/* Circular ring */
		if (tx_last >= engine->nb_desc)
			tx_last = (uint16_t) (tx_last - engine->nb_desc);

		/*
		 * Make sure there are enough TX descriptors available to
		 * transmit the entire packet.
		 * nb_used better be less than or equal to txq->tx_rs_thresh
		 */
		while (nb_used > free_bd_count) {
			log_rxtx("Engine %d Not enough free TX descriptors nb_used=%4u nb_free=%4u ", eng_indx, nb_used, free_bd_count);

			if (vc709_dma_PktHandler(dev, eng_indx) != XST_SUCCESS) {
				/*
				 * Could not clean any
				 * descriptors
				 */
				if (nb_tx == 0)
					return 0;
				goto end_of_tx;
			}
			free_bd_count = Dma_mBdRingGetFreeCnt(rptr) - 2;
		}

		/*
		 * By now there are enough free TX descriptors to transmit
		 * the packet.
		 */

		/* Allocate BDs from ring */
		result = Dma_BdRingAlloc(rptr, nb_used, &BdPtr);
		if (result != XST_SUCCESS) {
			/* we really shouldn't get this */
			VC709_LOG( "Engine %d DmaSendPkt: BdRingAlloc unsuccessful (%d)\n",eng_indx, result);
			return -1;
		}

		BdCurPtr = BdPtr;

		m_seg = tx_pkt;
		nb_processed = 0;
		for(j = 0; j< nb_used; j++ ){
			if(m_seg != NULL){
				txn = &sw_ring[txe->next_id];
				rte_prefetch0(&txn->mbuf->pool);

				if (txe->mbuf != NULL)
					rte_pktmbuf_free_seg(txe->mbuf);
				txe->mbuf = m_seg;

				/*
				 * Set up Transmit Buffer Descriptor.
				 */
				bufPA = rte_cpu_to_le_64(rte_mbuf_data_iova_default(m_seg));
				Dma_mBdSetBufAddr(BdCurPtr, bufPA);
				Dma_mBdSetCtrlLength(BdCurPtr, m_seg->data_len);
				Dma_mBdSetStatLength(BdCurPtr, m_seg->data_len);
				Dma_mBdSetId(BdCurPtr,rte_cpu_to_le_64((u64)rte_mbuf_to_baddr(m_seg)) );

				flags = 0;
				if(j == 0)
					flags |= DMA_BD_SOP_MASK;
				if(j == nb_used - 1)
					flags |= DMA_BD_EOP_MASK;

	//			Dma_mBdSetUserData(BdCurPtr, pbuf->userInfo);
				Dma_mBdSetCtrl(BdCurPtr, flags);                 // No ints also.

				BdCurPtr = Dma_mBdRingNext(rptr, BdCurPtr);

				txe->last_id = tx_last;
				tx_id = txe->next_id;
				txe = txn;
				m_seg = m_seg->next;

				nb_processed++;
			}else{
				break;
			}
		}
		result = Dma_BdRingToHw(rptr, nb_processed, BdPtr);

		if(result != XST_SUCCESS)
		{
			/* We should not come here. Incase of error, unmap buffers,
			 * unallocated BDs, and return zero count so that app driver
			 * can recover unused buffers.
			 */
			log_rxtx("Engine %d DmaSendPkt: BdRingToHw unsuccessful (%d)\n",eng_indx, result);
			BdCurPtr = BdPtr;

			log_rxtx("Engine %d release the partial BDs %d\n",eng_indx, nb_processed);

			for(i=0; i< nb_processed; i++)
			{
				Dma_mBdSetId_NULL(BdCurPtr, 0);
				BdCurPtr = Dma_mBdRingNext(rptr, BdCurPtr);
			}
			Dma_BdRingUnAlloc(rptr, nb_processed, BdPtr);

			goto end_of_tx;
		}
		if(nb_processed < nb_used){
			Dma_BdRingUnAlloc(rptr, nb_used - nb_processed, BdCurPtr);
		}

	}

end_of_tx:
	rte_wmb();

	engine->sw_tail = tx_id;
	return nb_tx;
}

/*
 * make tx_pkts as a single pkt to send out, by initialize multiple BDs
 */
int16_t
vc709_dma_send_pkts_noseg(struct dhl_fpga_dev *dev,
		uint16_t eng_indx,
		struct rte_mbuf **tx_pkts,
		uint16_t numpkts)
{
	int free_bd_count ;
	Dma_Engine * eptr;
	Dma_BdRing * rptr;
	Dma_Bd *BdPtr, *BdCurPtr;
	uint64_t bufPA;
	int result;
	u32 flags=0;
	int i,j;

	/* like ixgbe */
	struct vc709_tx_engine * engine;
	struct vc709_sw_entry *sw_ring;
	struct vc709_sw_entry *txe, *txn;
	struct rte_mbuf *tx_pkt;
	uint16_t tx_id;
	uint16_t nb_processed;

	engine = dev->data->tx_engines[eng_indx];
	sw_ring = engine->sw_ring;
	tx_id = engine->sw_tail;

	eptr = engine->eptr;
	rptr = &(eptr->BdRing);

	/* Check if this engine's pointer is valid */
	if(eptr == NULL)
	{
		RTE_LOG(ERR, PMD, "Engine %d Handle is a NULL value\n", eng_indx);
		return -1;
	}

	/* Is the number valid? */
	if(numpkts <= 0) {
		RTE_LOG(ERR, PMD, "Engine %d ,Packet count should be non-zero\n",eng_indx);
		return -1;
	}

	/* Is the engine assigned to any user? */
	if(eptr->EngineState != USER_ASSIGNED) {
		RTE_LOG(ERR, PMD,"Engine %d is not assigned to any user\n",eng_indx);
		return -1;
	}

	/* Is the engine an S2C one? */
	if(rptr->IsRxChannel)
	{
		RTE_LOG(ERR, PMD,"Engine %d The requested engine cannot send packets\n", eng_indx);
		return -1;
	}

	vc709_dma_PktHandler(dev, eng_indx);
	free_bd_count = Dma_mBdRingGetFreeCnt(rptr) - 2;

	while(free_bd_count < numpkts) {
		vc709_dma_PktHandler(dev, eng_indx);
		free_bd_count = Dma_mBdRingGetFreeCnt(rptr) - 2;
	}

	/* Maintain a separation between start and end of BD ring. This is
	 * required because DMA will stall if the two pointers coincide -
	 * this will happen whether ring is full or empty.
	 */
	free_bd_count = Dma_mBdRingGetFreeCnt(rptr) - 2;

	/*
	 * By now there are enough free TX descriptors to transmit
	 * the packet.
	 */

	/* Allocate BDs from ring */
	result = Dma_BdRingAlloc(rptr, numpkts, &BdPtr);
	if (result != XST_SUCCESS) {
		/* we really shouldn't get this */
		VC709_LOG( "Engine %d DmaSendPkt: BdRingAlloc unsuccessful (%d)\n",eng_indx, result);
		return -1;
	}

	BdCurPtr = BdPtr;
	txe = &sw_ring[tx_id];
	tx_id++;
	if (tx_id == engine->nb_desc)
		tx_id = 0;

	nb_processed = 0;
	for(j = 0; j< numpkts; j++ ){
		tx_pkt	= *tx_pkts++;
		tmp++;
		if(tx_pkt != NULL){
			txn = &sw_ring[tx_id];
			rte_prefetch0(&txn->mbuf->pool);


			if (txe->mbuf != NULL){
//				printf("rte_mbuf_refcnt_read(m) is %d \n",rte_mbuf_refcnt_read(txe->mbuf));
				rte_pktmbuf_free(txe->mbuf);
			}
			txe->mbuf = tx_pkt;
			/*
			 * Set up Transmit Buffer Descriptor.
			 */
			bufPA = rte_cpu_to_le_64(rte_mbuf_data_iova_default(tx_pkt));
			Dma_mBdSetBufAddr(BdCurPtr, bufPA);
			Dma_mBdSetCtrlLength(BdCurPtr, tx_pkt->data_len);
			Dma_mBdSetStatLength(BdCurPtr, tx_pkt->data_len);
			Dma_mBdSetId(BdCurPtr,rte_cpu_to_le_64((u64)rte_mbuf_to_baddr(tx_pkt)) );

			flags = 0;
			flags |= DMA_BD_SOP_MASK;
			flags |= DMA_BD_EOP_MASK;

			Dma_mBdSetCtrl(BdCurPtr, flags);                 // No ints also.

			BdCurPtr = Dma_mBdRingNext(rptr, BdCurPtr);

			txe = txn;

			nb_processed++;
		}else{
			break;
		}
	}
	result = Dma_BdRingToHw(rptr, nb_processed, BdPtr);

	if(result != XST_SUCCESS)
	{
		/* We should not come here. Incase of error, unmap buffers,
		 * unallocated BDs, and return zero count so that app driver
		 * can recover unused buffers.
		 */
		log_rxtx("tmp %d, Engine %d DmaSendPkt: BdRingToHw unsuccessful (%d)\n",tmp, eng_indx, result);
		BdCurPtr = BdPtr;

		log_rxtx("tmp %d, Engine %d release the partial BDs %d\n",tmp, eng_indx, nb_processed);

		for(i=0; i< nb_processed; i++)
		{
			Dma_mBdSetId_NULL(BdCurPtr, 0);
			BdCurPtr = Dma_mBdRingNext(rptr, BdCurPtr);
		}
		Dma_BdRingUnAlloc(rptr, nb_processed, BdPtr);

		goto end_of_tx;
	}
	if(nb_processed < numpkts){
		Dma_BdRingUnAlloc(rptr, numpkts - nb_processed, BdCurPtr);
	}

end_of_tx:
	rte_wmb();

	engine->sw_tail = tx_id;
	return nb_processed;
}

int16_t
vc709_dma_send_pkts_noseg_add_timestamp(struct dhl_fpga_dev *dev,
		uint16_t eng_indx,
		struct rte_mbuf **tx_pkts,
		uint16_t numpkts)
{
	int free_bd_count ;
	Dma_Engine * eptr;
	Dma_BdRing * rptr;
	Dma_Bd *BdPtr, *BdCurPtr;
	uint64_t bufPA;
	int result;
	u32 flags=0;
	int i,j;

	/* like ixgbe */
	struct vc709_tx_engine * engine;
	struct vc709_sw_entry *sw_ring;
	struct vc709_sw_entry *txe, *txn;
	struct rte_mbuf *tx_pkt;
	uint16_t tx_id;
	uint16_t tx_last;
	uint16_t nb_processed;

	engine = dev->data->tx_engines[eng_indx];
	sw_ring = engine->sw_ring;
	tx_id = engine->sw_tail;
	txe = &sw_ring[tx_id];

	eptr = engine->eptr;
	rptr = &(eptr->BdRing);

	/* Check if this engine's pointer is valid */
	if(eptr == NULL)
	{
		RTE_LOG(ERR, PMD, "Engine %d Handle is a NULL value\n", eng_indx);
		return -1;
	}

	/* Is the number valid? */
	if(numpkts <= 0) {
		RTE_LOG(ERR, PMD, "Engine %d ,Packet count should be non-zero\n",eng_indx);
		return -1;
	}

	/* Is the engine assigned to any user? */
	if(eptr->EngineState != USER_ASSIGNED) {
		RTE_LOG(ERR, PMD,"Engine %d is not assigned to any user\n",eng_indx);
		return -1;
	}

	/* Is the engine an S2C one? */
	if(rptr->IsRxChannel)
	{
		RTE_LOG(ERR, PMD,"Engine %d The requested engine cannot send packets\n", eng_indx);
		return -1;
	}

	vc709_dma_PktHandler(dev, eng_indx);
	free_bd_count = Dma_mBdRingGetFreeCnt(rptr) - 2;

	while(free_bd_count < numpkts) {
		vc709_dma_PktHandler(dev, eng_indx);
		free_bd_count = Dma_mBdRingGetFreeCnt(rptr) - 2;
	}

	/* Maintain a separation between start and end of BD ring. This is
	 * required because DMA will stall if the two pointers coincide -
	 * this will happen whether ring is full or empty.
	 */
	free_bd_count = Dma_mBdRingGetFreeCnt(rptr) - 2;

	/*
	 * By now there are enough free TX descriptors to transmit
	 * the packet.
	 */

	/* Allocate BDs from ring */
	result = Dma_BdRingAlloc(rptr, numpkts, &BdPtr);
	if (result != XST_SUCCESS) {
		/* we really shouldn't get this */
		VC709_LOG( "Engine %d DmaSendPkt: BdRingAlloc unsuccessful (%d)\n",eng_indx, result);
		return -1;
	}

	tx_last = (uint16_t) (tx_id + numpkts - 1);
	/* Circular ring */
	if (tx_last >= engine->nb_desc)
		tx_last = (uint16_t) (tx_last - engine->nb_desc);

	BdCurPtr = BdPtr;

	nb_processed = 0;
	for(j = 0; j< numpkts; j++ ){
		tx_pkt	= *tx_pkts++;
		tmp++;
		if(tx_pkt != NULL){
			txn = &sw_ring[txe->next_id];
			rte_prefetch0(&txn->mbuf->pool);

			if (txe->mbuf != NULL)
				rte_pktmbuf_free(txe->mbuf);
			txe->mbuf = tx_pkt;
			dhl_mbuf_add_timestamp(tx_pkt);
			/*
			 * Set up Transmit Buffer Descriptor.
			 */
			bufPA = rte_cpu_to_le_64(rte_mbuf_data_iova_default(tx_pkt));
			Dma_mBdSetBufAddr(BdCurPtr, bufPA);
			Dma_mBdSetCtrlLength(BdCurPtr, tx_pkt->data_len);
			Dma_mBdSetStatLength(BdCurPtr, tx_pkt->data_len);
			Dma_mBdSetId(BdCurPtr,rte_cpu_to_le_64((u64)rte_mbuf_to_baddr(tx_pkt)) );

			flags = 0;
			flags |= DMA_BD_SOP_MASK;
			flags |= DMA_BD_EOP_MASK;

			Dma_mBdSetCtrl(BdCurPtr, flags);                 // No ints also.

			BdCurPtr = Dma_mBdRingNext(rptr, BdCurPtr);

			txe->last_id = tx_last;
			tx_id = txe->next_id;
			txe = txn;

			nb_processed++;
		}else{
			break;
		}
	}
	result = Dma_BdRingToHw(rptr, nb_processed, BdPtr);

	if(result != XST_SUCCESS)
	{
		/* We should not come here. Incase of error, unmap buffers,
		 * unallocated BDs, and return zero count so that app driver
		 * can recover unused buffers.
		 */
		log_rxtx("tmp %d, Engine %d DmaSendPkt: BdRingToHw unsuccessful (%d)\n",tmp, eng_indx, result);
		BdCurPtr = BdPtr;

		log_rxtx("tmp %d, Engine %d release the partial BDs %d\n",tmp, eng_indx, nb_processed);

		for(i=0; i< nb_processed; i++)
		{
			Dma_mBdSetId_NULL(BdCurPtr, 0);
			BdCurPtr = Dma_mBdRingNext(rptr, BdCurPtr);
		}
		Dma_BdRingUnAlloc(rptr, nb_processed, BdPtr);

		goto end_of_tx;
	}
	if(nb_processed < numpkts){
		Dma_BdRingUnAlloc(rptr, numpkts - nb_processed, BdCurPtr);
	}

end_of_tx:
	rte_wmb();

	engine->sw_tail = tx_id;
	return nb_processed;
}

/*
 * make tx_pkts as a single pkt to send out, by initialize multiple BDs
 */
int16_t
vc709_dma_send_pkts_burst(struct dhl_fpga_dev *dev,
		uint16_t eng_indx,
		struct rte_mbuf **tx_pkts,
		uint16_t numpkts)
{
	int free_bd_count ;
	Dma_Engine * eptr;
	Dma_BdRing * rptr;
	Dma_Bd *BdPtr, *BdCurPtr;
	uint64_t bufPA;
	int result;
	u32 flags=0;
	int i,j;

	/* like ixgbe */
	struct vc709_tx_engine * engine;
	struct vc709_sw_entry *sw_ring;
	struct vc709_sw_entry *txe, *txn;
	struct rte_mbuf *tx_pkt;
	uint16_t tx_id;
	uint16_t tx_last;
	uint16_t nb_processed;

	engine = dev->data->tx_engines[eng_indx];
	sw_ring = engine->sw_ring;
	tx_id = engine->sw_tail;
	txe = &sw_ring[tx_id];

	eptr = engine->eptr;
	rptr = &(eptr->BdRing);

	/* Check if this engine's pointer is valid */
	if(eptr == NULL)
	{
		RTE_LOG(ERR, PMD, "Engine %d Handle is a NULL value\n", eng_indx);
		return -1;
	}

	/* Is the number valid? */
	if(numpkts <= 0) {
		RTE_LOG(ERR, PMD, "Engine %d ,Packet count should be non-zero\n",eng_indx);
		return -1;
	}

	/* Is the engine assigned to any user? */
	if(eptr->EngineState != USER_ASSIGNED) {
		RTE_LOG(ERR, PMD,"Engine %d is not assigned to any user\n",eng_indx);
		return -1;
	}

	/* Is the engine an S2C one? */
	if(rptr->IsRxChannel)
	{
		RTE_LOG(ERR, PMD,"Engine %d The requested engine cannot send packets\n", eng_indx);
		return -1;
	}

	free_bd_count = Dma_mBdRingGetFreeCnt(rptr) - 2;

	while(free_bd_count < numpkts) {
		vc709_dma_PktHandler(dev, eng_indx);
		free_bd_count = Dma_mBdRingGetFreeCnt(rptr) - 2;
	}

	/* Maintain a separation between start and end of BD ring. This is
	 * required because DMA will stall if the two pointers coincide -
	 * this will happen whether ring is full or empty.
	 */
	free_bd_count = Dma_mBdRingGetFreeCnt(rptr) - 2;


	/*
	 * By now there are enough free TX descriptors to transmit
	 * the packet.
	 */

	/* Allocate BDs from ring */
	result = Dma_BdRingAlloc(rptr, numpkts, &BdPtr);
	if (result != XST_SUCCESS) {
		/* we really shouldn't get this */
		VC709_LOG( "Engine %d DmaSendPkt: BdRingAlloc unsuccessful (%d)\n",eng_indx, result);
		return -1;
	}

	tx_last = (uint16_t) (tx_id + numpkts - 1);
	/* Circular ring */
	if (tx_last >= engine->nb_desc)
		tx_last = (uint16_t) (tx_last - engine->nb_desc);

	BdCurPtr = BdPtr;

	nb_processed = 0;
	for(j = 0; j< numpkts; j++ ){
		tx_pkt	= *tx_pkts++;
		if(tx_pkt != NULL){
			txn = &sw_ring[txe->next_id];
			rte_prefetch0(&txn->mbuf->pool);

			if (txe->mbuf != NULL)
				rte_pktmbuf_free(txe->mbuf);
			txe->mbuf = tx_pkt;

			/*
			 * Set up Transmit Buffer Descriptor.
			 */
			bufPA = rte_cpu_to_le_64(rte_mbuf_data_iova_default(tx_pkt));
			Dma_mBdSetBufAddr(BdCurPtr, bufPA);
			Dma_mBdSetCtrlLength(BdCurPtr, tx_pkt->data_len);
			Dma_mBdSetStatLength(BdCurPtr, tx_pkt->data_len);
			Dma_mBdSetId(BdCurPtr,rte_cpu_to_le_64((u64)rte_mbuf_to_baddr(tx_pkt)) );

			flags = 0;
			if(j == 0)
				flags |= DMA_BD_SOP_MASK;
			if(j == numpkts - 1)
				flags |= DMA_BD_EOP_MASK;

//			Dma_mBdSetUserData(BdCurPtr, pbuf->userInfo);
			Dma_mBdSetCtrl(BdCurPtr, flags);                 // No ints also.

			BdCurPtr = Dma_mBdRingNext(rptr, BdCurPtr);

			txe->last_id = tx_last;
			tx_id = txe->next_id;
			txe = txn;

			nb_processed++;
		}else{
			break;
		}
	}
	result = Dma_BdRingToHw(rptr, nb_processed, BdPtr);

	if(result != XST_SUCCESS)
	{
		/* We should not come here. Incase of error, unmap buffers,
		 * unallocated BDs, and return zero count so that app driver
		 * can recover unused buffers.
		 */
		log_rxtx("Engine %d DmaSendPkt: BdRingToHw unsuccessful (%d)\n",eng_indx, result);
		BdCurPtr = BdPtr;

		log_rxtx("Engine %d release the partial BDs %d\n",eng_indx, nb_processed);

		for(i=0; i< nb_processed; i++)
		{
			Dma_mBdSetId_NULL(BdCurPtr, 0);
			BdCurPtr = Dma_mBdRingNext(rptr, BdCurPtr);
		}
		Dma_BdRingUnAlloc(rptr, nb_processed, BdPtr);

		goto end_of_tx;
	}
	if(nb_processed < numpkts){
		Dma_BdRingUnAlloc(rptr, numpkts - nb_processed, BdCurPtr);
	}

end_of_tx:
	rte_wmb();

	engine->sw_tail = tx_id;
	return nb_processed;
}


static int
rx_engine_descriptor_init(struct dhl_fpga_dev *dev,
		uint16_t hw_engine_idx, uint16_t nb_dma_bd,
		unsigned int socket_id, struct rte_mempool *mp)
{
	struct vc709_dma_engine * dma_engine;
	struct vc709_mapped_kernel_resource * bd_res;
	struct vc709_rx_engine *engine;

	Dma_Engine * eptr;
	int result;

	u64 *BdPtr;
	u64 BdPhyAddr;
	u32 delta = 0;
	int dftsize;
	int bd_res_map_id = 0;

	int sw_engine_idx = hw_engine_idx - 32;
	dma_engine = VC709_DEV_PRIVATE_TO_DMAENGINE(dev->data->dev_private);
	bd_res = VC709_DEV_PRIVATE_TO_BDRES(dev->data->dev_private);

	/*
	 * Validate number of receive descriptors.
	 * It must not exceed hardware maximum, and must be multiple
	 * of IXGBE_ALIGN.
	 */
	if( (nb_dma_bd > VC709_MAX_RING_DESC) || nb_dma_bd < VC709_MIN_RING_DESC){
		return -EINVAL;
	}
	eptr = &(dma_engine->Dma[hw_engine_idx]);

	eptr->pktSize = 4096 * 4096;
	dma_engine->Dma_Bd_nums[hw_engine_idx] = nb_dma_bd;


/*
 * start to init the descriptors
 */

	/* Free memory prior to re-allocation if needed... */
	if (dev->data->rx_engines[sw_engine_idx] != NULL) {
		vc709_sw_rx_engine_release(dev->data->rx_engines[sw_engine_idx]);
		dev->data->rx_engines[sw_engine_idx] = NULL;
	}

	/* First allocate the engine data structure */
	engine = rte_zmalloc_socket("fpga dev Engines", sizeof(struct vc709_rx_engine),
				 RTE_CACHE_LINE_SIZE, socket_id);
	if (engine == NULL)
		return -ENOMEM;

	engine->mb_pool = mp;
	engine->nb_desc = nb_dma_bd;
	engine->hw_engine_id = hw_engine_idx;
	engine->sw_engine_id = sw_engine_idx;
	engine->card_id = dev->data->card_id;
	engine->eptr = eptr;

	/*
	 * bd_res_map's size is 4,
	 * TX engine 0 and RX engine 32 is using bd_res_map[0]
	 */
	bd_res_map_id = sw_engine_idx;


	BdPhyAddr = bd_res->maps[bd_res_map_id].phaddr;
	BdPtr = (u64 *)bd_res->maps[bd_res_map_id].addr;
	dftsize = (int)bd_res->maps[bd_res_map_id].size;

	Dma_BdRingAlign((u64)BdPtr, dftsize, DMA_BD_MINIMUM_ALIGNMENT, &delta);


	eptr->descSpacePA = BdPhyAddr + delta + 2048 * sizeof(Dma_Bd);
	eptr->descSpaceVA =(u64*) ((u64)BdPtr + delta + 2048 * sizeof(Dma_Bd));
	eptr->descSpaceSize = nb_dma_bd * sizeof(Dma_Bd);
	eptr->delta = delta;

	/*
	 * mmap() get from vc709_uio map1 - map4
	 */

	engine->bd_ring_phys_addr = eptr->descSpacePA;
	engine->bd_ring = (Dma_Bd *) eptr->descSpaceVA;


	/* Allocate software ring */
	engine->sw_ring = rte_zmalloc_socket("engine->sw_ring",
				sizeof(struct vc709_sw_entry) * nb_dma_bd,
				RTE_CACHE_LINE_SIZE, socket_id);
	if (!engine->sw_ring) {
		vc709_sw_rx_engine_release(engine);
		return -ENOMEM;
	}

	RTE_LOG(INFO, PMD, "rx engine %d: sw_ring=%p hw_ring=%p dma_addr=0x%"PRIx64 "\n",
			hw_engine_idx, engine->sw_ring, engine->bd_ring, engine->bd_ring_phys_addr);

	vc709_reset_sw_rx_engine(engine);

	dev->data->rx_engines[sw_engine_idx] = engine;

/***********************************
 * xdma
 ***********************************/
	result = Dma_BdRingCreate(&(eptr->BdRing), eptr->descSpacePA,
			 (u64)eptr->descSpaceVA, DMA_BD_MINIMUM_ALIGNMENT, nb_dma_bd);


	if (result != XST_SUCCESS)
	{
		RTE_LOG( ERR, PMD, "Rx engine %d DMA Ring Create. Error: %d",hw_engine_idx, result);
		return -EIO;
	}

	if((eptr->Type & DMA_ENG_DIRECTION_MASK) == DMA_ENG_C2S)
			vc709_setup_recv_buffers(dev,sw_engine_idx);


/*********************************
 * end xdma
 ********************************/

	return 0;
}

static int
tx_engine_descriptor_init(struct dhl_fpga_dev *dev,
		uint16_t hw_engine_idx, uint16_t nb_dma_bd,
		unsigned int socket_id)
{
	struct vc709_dma_engine * dma_engine;
	struct vc709_mapped_kernel_resource * bd_res;
	struct vc709_tx_engine *engine;

	Dma_Engine * eptr;
	int result;

	u64 *BdPtr;
	u64 BdPhyAddr;
	u32 delta = 0;
	int dftsize;
	int bd_res_map_id = 0;

	int sw_engine_idx = hw_engine_idx;
	dma_engine = VC709_DEV_PRIVATE_TO_DMAENGINE(dev->data->dev_private);
	bd_res = VC709_DEV_PRIVATE_TO_BDRES(dev->data->dev_private);

	/*
	 * Validate number of receive descriptors.
	 * It must not exceed hardware maximum, and must be multiple
	 * of IXGBE_ALIGN.
	 */
	if( (nb_dma_bd > VC709_MAX_RING_DESC) || nb_dma_bd < VC709_MIN_RING_DESC){
		return -EINVAL;
	}
	eptr = &(dma_engine->Dma[hw_engine_idx]);

	eptr->pktSize = 4096 * 4096;
	dma_engine->Dma_Bd_nums[hw_engine_idx] = nb_dma_bd;


/*
 * start to init the descriptors
 */

	/* Free memory prior to re-allocation if needed... */
	if (dev->data->tx_engines[sw_engine_idx] != NULL) {
		vc709_sw_tx_engine_release(dev->data->tx_engines[sw_engine_idx]);
		dev->data->tx_engines[sw_engine_idx] = NULL;
	}

	/* First allocate the engine data structure */
	engine = rte_zmalloc_socket("fpga dev Engines", sizeof(struct vc709_tx_engine),
				 RTE_CACHE_LINE_SIZE, socket_id);
	if (engine == NULL)
		return -ENOMEM;

	engine->nb_desc = nb_dma_bd;
	engine->hw_engine_id = hw_engine_idx;
	engine->sw_engine_id = sw_engine_idx;
	engine->card_id = dev->data->card_id;
	engine->eptr = eptr;

	/*
	 * bd_res_map's size is 4,
	 * TX engine 0 and RX engine 32 is using bd_res_map[0]
	 */
	bd_res_map_id = sw_engine_idx;

	BdPhyAddr = bd_res->maps[bd_res_map_id].phaddr;
	BdPtr = (u64 *)bd_res->maps[bd_res_map_id].addr;
	dftsize = (int)bd_res->maps[bd_res_map_id].size;

	Dma_BdRingAlign((u64)BdPtr, dftsize, DMA_BD_MINIMUM_ALIGNMENT, &delta);

	eptr->descSpacePA = BdPhyAddr + delta;
	eptr->descSpaceVA = BdPtr + delta;
	eptr->descSpaceSize = nb_dma_bd * sizeof(Dma_Bd);
	eptr->delta = delta;

	/*
	 * mmap() get from vc709_uio map1 - map4
	 */

	engine->bd_ring_phys_addr = eptr->descSpacePA;
	engine->bd_ring = (Dma_Bd *) eptr->descSpaceVA;


	/* Allocate software ring */
	engine->sw_ring = rte_zmalloc_socket("engine->sw_ring",
				sizeof(struct vc709_sw_entry) * nb_dma_bd,
				RTE_CACHE_LINE_SIZE, socket_id);
	if (!engine->sw_ring) {
		vc709_sw_tx_engine_release(engine);
		return -ENOMEM;
	}

	RTE_LOG(INFO, PMD, "Tx engine %d: sw_ring=%p hw_ring=%p dma_addr=0x%"PRIx64 "\n",
			hw_engine_idx, engine->sw_ring, engine->bd_ring, engine->bd_ring_phys_addr);

	vc709_reset_hw_tx_engine(engine);

	dev->data->tx_engines[sw_engine_idx] = engine;

/***********************************
 * xdma
 ***********************************/
	result = Dma_BdRingCreate(&(eptr->BdRing), eptr->descSpacePA,
			 (u64)eptr->descSpaceVA, DMA_BD_MINIMUM_ALIGNMENT, nb_dma_bd);


	if (result != XST_SUCCESS)
	{
		RTE_LOG( ERR, PMD, "Rx engine %d DMA Ring Create. Error: %d",hw_engine_idx, result);
		return -EIO;
	}

//	if((eptr->Type & DMA_ENG_DIRECTION_MASK) == DMA_ENG_C2S)
//			vc709_setup_recv_buffers(dev,hw_engine_idx);

/*********************************
 * end xdma
 ********************************/

	return 0;
}


/******************************************************************************
 *
 *   engine management functions
 *
 ******************************************************************************/


/*
 * for vc709 westlogic dma IP core, which has 64 dma engines.
 * No 0 - 31 is Tx engine, and 32 - 63 is Tx engine.
 * What's more, 0 - 3, and 32-35 is valid.
 * So in this functions. the right rx engine number should be rx_engine_id + 32
 */
int __attribute__((cold))
vc709_dev_rx_engine_setup(struct dhl_fpga_dev *dev,
		uint16_t sw_engine_id,
		uint16_t nb_rx_desc,
		unsigned int socket_id,
		const struct dhl_fpga_rxconf *rx_conf,
		struct rte_mempool *mb_pool)
{
	uint16_t hw_engine_id; // engine id for vc709 hardware [0-63]
	struct vc709_adapter * adapter;
	struct vc709_dma_engine * dma_engine;
	struct vc709_bar * bar;

	Dma_Engine * eptr;
	u64 barBase;

	/* in west logic DMA IP core,
	*  0-31 for TX, 32 - 63 for Rx
	*/
	hw_engine_id = sw_engine_id + 32;
	adapter = (struct vc709_adapter *)dev->data->dev_private;
	dma_engine = VC709_DEV_PRIVATE_TO_DMAENGINE(dev->data->dev_private);
	bar = &(adapter->bar);

#ifdef PM_SUPPORT
	if(adapter->DriverState == PM_PREPARE)
	{
		RTE_LOG(ERR, PMD, "DMA driver state %d - entering into Power Down states\n", adapter->DriverState);
		return -1;
	}
#endif

	if(adapter->DriverState != INITIALIZED)
	{
		RTE_LOG(ERR, PMD, "DMA driver state %d - not ready\n", adapter->DriverState);
		return -1;
	}

	if(!((dma_engine->engineMask) & (1LL << hw_engine_id))) {
		RTE_LOG(ERR, PMD, "Requested Rx engine %d does not exist\n", hw_engine_id);
		return -1;
	}
	eptr = &(dma_engine->Dma[hw_engine_id]);
	barBase = (u64) (bar->barInfo[0].baseVAddr);

	if(eptr->EngineState != INITIALIZED) {
		RTE_LOG(ERR, PMD, "Requested Rx engine %d is not free\n", hw_engine_id);
		return -1;
	}

	if(rx_conf == NULL)
		RTE_LOG(ERR, PMD, "rx_conf is NULL\n");

	/* Stop any running tests. The driver could have been unloaded without
	 * stopping running tests the last time. Hence, good to reset everything.
	 */
	XIo_Out32 (barBase + TX_CONFIG_ADDRESS(sw_engine_id), 0);
	XIo_Out32 (barBase + RX_CONFIG_ADDRESS(sw_engine_id), 0);

	if (rx_engine_descriptor_init(dev, hw_engine_id, nb_rx_desc, socket_id, mb_pool) != 0 ){
		RTE_LOG(ERR, PMD, "Rx Engine %d init descriptors failed.\n", hw_engine_id);
		return -1;
	}

	/* Change the state of the engine, and increment the user count */
	eptr->EngineState = USER_ASSIGNED;

	/* Start the DMA engine */
	if (Dma_BdRingStart(&(eptr->BdRing)) == XST_FAILURE) {
		RTE_LOG( ERR, PMD, "Could not start Dma Rx engine %d.\n", hw_engine_id);
		return -1;
	}

	rte_delay_ms(10);
	RTE_LOG(INFO, PMD, "Rx engine %d CR(control register offset is 0x00000004) "
			"is now 0x%x\n", hw_engine_id, Dma_mGetCrSr(eptr));
	return 0;
}

int __attribute__((cold))
vc709_dev_rx_engine_start(struct dhl_fpga_dev * dev __rte_unused, uint16_t sw_engine_id __rte_unused)
{
	RTE_LOG(ERR, PMD, "In vc709 board, only TX engine can be started\n");
	return -EFAULT;
}

int __attribute__((cold))
vc709_dev_rx_engine_stop(struct dhl_fpga_dev * dev __rte_unused, uint16_t rx_engine_id __rte_unused)
{
	RTE_LOG(ERR, PMD, "In vc709 board, only TX engine can be stopped\n");
	return -EFAULT;
}

int __attribute__((cold))
vc709_dev_tx_engine_setup(struct dhl_fpga_dev * dev,
		uint16_t sw_engine_id,
		uint16_t nb_tx_desc,
		unsigned int socket_id,
		const struct dhl_fpga_txconf *tx_conf __rte_unused)
{
	uint16_t hw_engine_id;
	struct vc709_adapter * adapter;
	struct vc709_dma_engine * dma_engine;
	struct vc709_bar * bar;

	Dma_Engine * eptr;
	u64 barBase;

	adapter = (struct vc709_adapter *)dev->data->dev_private;
	dma_engine = VC709_DEV_PRIVATE_TO_DMAENGINE(dev->data->dev_private);
	bar = &(adapter->bar);

	hw_engine_id = sw_engine_id;

#ifdef PM_SUPPORT
	if(adapter->DriverState == PM_PREPARE)
	{
		RTE_LOG(ERR, PMD, "DMA driver state %d - entering into Power Down states\n", adapter->DriverState);
		return -1;
	}
#endif

	if(adapter->DriverState != INITIALIZED)
	{
		RTE_LOG(ERR, PMD, "DMA driver state %d - not ready\n", adapter->DriverState);
		return -1;
	}

	if(!((dma_engine->engineMask) & (1LL << hw_engine_id))) {
		RTE_LOG(ERR, PMD, "Requested TX engine %d does not exist\n", hw_engine_id);
		return -1;
	}
	eptr = &(dma_engine->Dma[hw_engine_id]);
	barBase = (u64) (bar->barInfo[0].baseVAddr);

	if(eptr->EngineState != INITIALIZED) {
		RTE_LOG(ERR, PMD, "Requested Tx engine %d is not free\n", hw_engine_id);
		return -1;
	}


	/* Stop any running tests. The driver could have been unloaded without
	 * stopping running tests the last time. Hence, good to reset everything.
	 */
	XIo_Out32 (barBase + TX_CONFIG_ADDRESS(sw_engine_id), 0);
	XIo_Out32 (barBase + RX_CONFIG_ADDRESS(sw_engine_id), 0);

	if (tx_engine_descriptor_init(dev, hw_engine_id, nb_tx_desc, socket_id) != 0 ){
		RTE_LOG(ERR, PMD, "Engine %d init descriptors failed.\n", hw_engine_id);
		return -1;
	}

	/* Change the state of the engine, and increment the user count */
	eptr->EngineState = USER_ASSIGNED;

	/* Start the DMA engine */
	if (Dma_BdRingStart(&(eptr->BdRing)) == XST_FAILURE) {
		RTE_LOG( ERR, PMD, "Could not start Dma Tx engine %d.\n", hw_engine_id);
		return -1;
	}

	rte_delay_ms(10);
	RTE_LOG(INFO, PMD, "Tx engine %d CR(control register offset is 0x00000004) "
			"is now 0x%x\n", hw_engine_id, Dma_mGetCrSr(eptr));
	return 0;
}

int __attribute__((cold))
vc709_dev_tx_engine_start(struct dhl_fpga_dev * dev, uint16_t engine_id)
{
	uint16_t hw_engine_id = engine_id;

	u32 maxPktSize;
	unsigned int testmode;
	struct vc709_dma_engine * dma_engine;
	struct vc709_bar * bar;
	Dma_Engine * eptr;
	u64 barbase;

	dma_engine = VC709_DEV_PRIVATE_TO_DMAENGINE(dev->data->dev_private);
	bar = VC709_DEV_PRIVATE_TO_BAR(dev->data->dev_private);

	/* First, check if requested engine is valid */
	if((hw_engine_id >= MAX_DMA_ENGINES) || (!((dma_engine->engineMask) & (1LL << hw_engine_id))))
	{
		RTE_LOG(ERR, PMD, "Invalid engine %d\n", hw_engine_id);
		return -EFAULT;
	}

	eptr = &(dma_engine->Dma[hw_engine_id]);
	barbase =(u64) (bar->barBase);

	/* Then check if the user function registered */
	if((eptr->EngineState != USER_ASSIGNED))
	{
		RTE_LOG(ERR, PMD, "this engine is not set up\n");
		return -EFAULT;
	}

	/* only TX engine can be start */
	if(eptr->BdRing.IsRxChannel){
		RTE_LOG(ERR, PMD, "Engine %d is a Rx Engine, can not be start\n", hw_engine_id);
		return -EFAULT;
	}

	dma_engine->RawTestMode[hw_engine_id] = (TEST_START | ENABLE_LOOPBACK);

	testmode = 0;
	testmode |= LOOPBACK;
//	testmode |= PKTCHKR;
//	testmode |= PKTGENR;

	VC709_LOG("Start TX engine %d with RawTestMode %x ,testmode value %x\n",
			hw_engine_id, dma_engine->RawTestMode[hw_engine_id], testmode);

	XIo_Out32 ( (barbase + DESIGN_MODE_ADDRESS), PERF_DESIGN_MODE);

	maxPktSize = 2000;
	XIo_Out32 ( (barbase + PKT_SIZE_ADDRESS(hw_engine_id)), maxPktSize);

	int seqno = 512;
	XIo_Out32 ( (barbase+ SEQNO_WRAP_REG(hw_engine_id)) , seqno);

	rte_delay_ms(1);
	/* Incase the last test was a loopback test, that bit may not be cleared */
	XIo_Out32 ( (barbase + TX_CONFIG_ADDRESS(hw_engine_id)), 0);
	rte_delay_ms(1);
	XIo_Out32 ( (barbase + TX_CONFIG_ADDRESS(hw_engine_id)), testmode);

	return 0;
}

int __attribute__((cold))
vc709_dev_tx_engine_stop(struct dhl_fpga_dev * dev, uint16_t engine_id)
{
	uint16_t hw_engine_id = engine_id;

	unsigned int testmode;
	struct vc709_dma_engine * dma_engine;
	struct vc709_bar * bar;
	Dma_Engine * eptr;
	u64 barbase;

	dma_engine = VC709_DEV_PRIVATE_TO_DMAENGINE(dev->data->dev_private);
	bar = VC709_DEV_PRIVATE_TO_BAR(dev->data->dev_private);

	/* First, check if requested engine is valid */
	if((hw_engine_id >= MAX_DMA_ENGINES) || (!((dma_engine->engineMask) & (1LL << hw_engine_id))))
	{
		RTE_LOG(ERR, PMD, "Invalid engine %d\n", hw_engine_id);
		return -EFAULT;
	}

	eptr = &(dma_engine->Dma[hw_engine_id]);
	barbase =(u64) (bar->barBase);

	/* Then check if the user function registered */
	if((eptr->EngineState != USER_ASSIGNED))
	{
		RTE_LOG(ERR, PMD, "this engine is not set up");
		return -EFAULT;
	}

	/* only TX engine can be start */
	if(eptr->BdRing.IsRxChannel){
		RTE_LOG(ERR, PMD, "Engine %d is a Rx Engine, can not be stop", hw_engine_id);
		return -EFAULT;
	}

	dma_engine->RawTestMode[hw_engine_id] = (TEST_STOP | ENABLE_LOOPBACK);

	testmode = 0;
	testmode &= ~LOOPBACK;
//	testmode |= PKTCHKR;
//	testmode |= PKTGENR;

	VC709_LOG("Stop TX engine %d with RawTestMode %x ,testmode value %x\n",
			engine_id, dma_engine->RawTestMode[hw_engine_id], testmode);

	rte_delay_ms(50);
	XIo_Out32 ( (barbase + TX_CONFIG_ADDRESS(hw_engine_id)), testmode);
	rte_delay_ms(10);

	XIo_Out32 ( (barbase + RX_CONFIG_ADDRESS(hw_engine_id)), testmode);
	rte_delay_ms(100);

	return 0;
}
