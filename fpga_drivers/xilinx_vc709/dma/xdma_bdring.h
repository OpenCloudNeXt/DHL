/*******************************************************************************
 ** � Copyright 2012 - 2013 Xilinx, Inc. All rights reserved.
 ** This file contains confidential and proprietary information of Xilinx, Inc. and 
 ** is protected under U.S. and international copyright and other intellectual property laws.
 *******************************************************************************
 **   ____  ____ 
 **  /   /\/   / 
 ** /___/  \  /   Vendor: Xilinx 
 ** \   \   \/    
 **  \   \
 **  /   /          
 ** /___/    \
 ** \   \  /  \   Virtex-7 FPGA XT Connectivity Targeted Reference Design
 **  \___\/\___\
 ** 
 **  Device: xc7v690t
 **  Version: 1.0
 **  Reference: UG
 **     
 *******************************************************************************
 **
 **  Disclaimer: 
 **
 **    This disclaimer is not a license and does not grant any rights to the materials 
 **    distributed herewith. Except as otherwise provided in a valid license issued to you 
 **    by Xilinx, and to the maximum extent permitted by applicable law: 
 **    (1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND WITH ALL FAULTS, 
 **    AND XILINX HEREBY DISCLAIMS ALL WARRANTIES AND CONDITIONS, EXPRESS, IMPLIED, OR STATUTORY, 
 **    INCLUDING BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, NON-INFRINGEMENT, OR 
 **    FITNESS FOR ANY PARTICULAR PURPOSE; and (2) Xilinx shall not be liable (whether in contract 
 **    or tort, including negligence, or under any other theory of liability) for any loss or damage 
 **    of any kind or nature related to, arising under or in connection with these materials, 
 **    including for any direct, or any indirect, special, incidental, or consequential loss 
 **    or damage (including loss of data, profits, goodwill, or any type of loss or damage suffered 
 **    as a result of any action brought by a third party) even if such damage or loss was 
 **    reasonably foreseeable or Xilinx had been advised of the possibility of the same.


 **  Critical Applications:
 **
 **    Xilinx products are not designed or intended to be fail-safe, or for use in any application 
 **    requiring fail-safe performance, such as life-support or safety devices or systems, 
 **    Class III medical devices, nuclear facilities, applications related to the deployment of airbags,
 **    or any other applications that could lead to death, personal injury, or severe property or 
 **    environmental damage (individually and collectively, "Critical Applications"). Customer assumes 
 **    the sole risk and liability of any use of Xilinx products in Critical Applications, subject only 
 **    to applicable laws and regulations governing limitations on product liability.

 **  THIS COPYRIGHT NOTICE AND DISCLAIMER MUST BE RETAINED AS PART OF THIS FILE AT ALL TIMES.

 *******************************************************************************/
/*****************************************************************************/
/**
 *
 * @file xdma_bdring.h
 *
 * This file contains DMA engine related structures and constant definitions,
 * as well as function prototypes. Each DMA engine is managed by a Buffer
 * Descriptor ring, and so Dma_BdRing is chosen as the symbol prefix used in
 * this file. See xdma.h for more information.
 *
 * MODIFICATION HISTORY:
 *
 * Ver   Date     Changes
 * ----- -------- -------------------------------------------------------
 * 1.0   5/15/12  First version
 *
 ******************************************************************************/

#ifndef DMA_BDRING_H    /* prevent circular inclusions */
#define DMA_BDRING_H    /* by using protection macros */

#ifdef __cplusplus
extern "C" {
#endif

#include "../../../fpga_drivers/xilinx_vc709/dma/xdma_bd.h"

/**************************** Type Definitions *******************************/

	/** Container structure for buffer descriptor (BD) storage. All addresses
	 * and pointers excluding FirstBdPhysAddr are expressed in terms of the
	 * virtual address.
	 *
	 * The BD ring is not arranged as a conventional linked list. Instead,
	 * a separation value is added to navigate to the next BD, since all the
	 * BDs are contiguously placed, and the ring is placed from lower memory
	 * to higher memory and back.
	 */

typedef struct {
	u64 ChanBase;
	u32 IsRxChannel;        /**< Is this a receive channel ? */
	u32 RunState;         /**< Flag to indicate state of engine/ring */
	u64 FirstBdPhysAddr;    /**< Physical address of 1st BD in list */
	u64 FirstBdAddr;        /**< Virtual address of 1st BD in list */
	u64 LastBdAddr;         /**< Virtual address of last BD in the list */
	u32 Length;             /**< Total size of ring in bytes */
	u32 Separation;         /**< Number of bytes between the starting
				  address of adjacent BDs */
	Dma_Bd *FreeHead;       /**< First BD in the free group */
	Dma_Bd *PreHead;        /**< First BD in the pre-work group */
	Dma_Bd *HwHead;         /**< First BD in the work group */
	Dma_Bd *HwTail;         /**< Last BD in the work group */
	Dma_Bd *PostHead;       /**< First BD in the post-work group */
	Dma_Bd *BdaRestart;     /**< BD to load when channel is started */
	u32 FreeCnt;          /**< Number of allocatable BDs in free group */
	u32 PreCnt;             /**< Number of BDs in pre-work group */
	u32 HwCnt;              /**< Number of BDs in work group */
	u32 PostCnt;          /**< Number of BDs in post-work group */
	u32 AllCnt;             /**< Total Number of BDs for channel */

	u32 BDerrs;             /**< Total BD errors reported by DMA */
	u32 BDSerrs;            /**< Total TX BD short errors reported by DMA */
} Dma_BdRing;





	/***************** Macros (Inline Functions) Definitions *********************/

	/****************************************************************************/
	/**
	 * Retrieve the ring object. This object can be used in the various Ring
	 * API functions. This could be a C2S or S2C ring.
	 *
	 * @param  InstancePtr is the DMA engine to operate on.
	 *
	 * @return BdRing object
	 *
	 * @note
	 * C-style signature:
	 *    Dma_BdRing Dma_mGetRing(Dma_Engine * InstancePtr)
	 *
	 *****************************************************************************/
#define Dma_mGetRing(InstancePtr) ((InstancePtr)->BdRing)


	/****************************************************************************/
	/**
	 * Return the total number of BDs allocated by this channel with
	 * Dma_BdRingCreate().
	 *
	 * @param  RingPtr is the BD ring to operate on.
	 *
	 * @return The total number of BDs allocated for this channel.
	 *
	 * @note
	 * C-style signature:
	 *    u32 Dma_mBdRingGetCnt(Dma_BdRing* RingPtr)
	 *
	 *****************************************************************************/
#define Dma_mBdRingGetCnt(RingPtr) ((RingPtr)->AllCnt)


	/****************************************************************************/
	/**
	 * Return the number of BDs allocatable with Dma_BdRingAlloc() for pre-
	 * processing.
	 *
	 * @param  RingPtr is the BD ring to operate on.
	 *
	 * @return The number of BDs currently allocatable.
	 *
	 * @note
	 * C-style signature:
	 *    u32 Dma_mBdRingGetFreeCnt(Dma_BdRing* RingPtr)
	 *
	 *****************************************************************************/
#define Dma_mBdRingGetFreeCnt(RingPtr)  ((RingPtr)->FreeCnt)

	/* Maintain a separation between start and end of BD ring. This is
	 * required because DMA will stall if the two pointers coincide - 
	 * this will happen whether ring is full or empty. 
	 */


	/****************************************************************************/
	/**
	 * Snap shot the latest BD that a BD ring is processing.
	 *
	 * @param  RingPtr is the BD ring to operate on.
	 *
	 * @return None
	 *
	 * @note
	 * C-style signature:
	 *    void Dma_mBdRingSnapShotCurrBd(Dma_BdRing* RingPtr)
	 *
	 *****************************************************************************/
#define Dma_mBdRingSnapShotCurrBd(RingPtr)            \
	{                                           \
		(RingPtr)->BdaRestart =                         \
		(Dma_Bd *)Dma_mReadReg((RingPtr)->ChanBase, REG_DMA_ENG_NEXT_BD);   \
	}


	/****************************************************************************/
	/**
	 * Return the next BD in the ring.
	 *
	 * @param  RingPtr is the BD ring to operate on.
	 * @param  BdPtr is the current BD.
	 *
	 * @return The next BD in the ring relative to the BdPtr parameter.
	 *
	 * @note
	 * C-style signature:
	 *    Dma_Bd *Dma_mBdRingNext(Dma_BdRing* RingPtr, Dma_Bd *BdPtr)
	 *
	 *****************************************************************************/
#define Dma_mBdRingNext(RingPtr, BdPtr)                 \
	(((u64)(BdPtr) >= (RingPtr)->LastBdAddr) ?          \
	 (Dma_Bd*)(RingPtr)->FirstBdAddr :                 \
	 (Dma_Bd*)((u64)(BdPtr) + (RingPtr)->Separation))

	/****************************************************************************/
	/**
	 * Return the previous BD in the ring.
	 *
	 * @param  InstancePtr is the DMA channel to operate on.
	 * @param  BdPtr is the current BD.
	 *
	 * @return The previous BD in the ring relative to the BdPtr parameter.
	 *
	 * @note
	 * C-style signature:
	 *    Dma_Bd *Dma_mBdRingPrev(Dma_BdRing* RingPtr, Dma_Bd *BdPtr)
	 *
	 *****************************************************************************/
#define Dma_mBdRingPrev(RingPtr, BdPtr)               \
	(((BdPtr) <= (RingPtr)->FirstBdAddr) ?       \
	 (Dma_Bd*)(RingPtr)->LastBdAddr :                \
	 (Dma_Bd*)((BdPtr) - (RingPtr)->Separation))


	/******************************************************************************
	 * Move the BdPtr argument ahead an arbitrary number of BDs wrapping around
	 * to the beginning of the ring if needed.
	 *
	 * We know that a wraparound should occur if the new BdPtr is greater than
	 * the high address in the ring OR if the new BdPtr crosses the 0xFFFFFFFF
	 * to 0 boundary.
	 *
	 * @param RingPtr is the ring BdPtr appears in
	 * @param BdPtr on input is the starting BD position and on output is the
	 *        final BD position
	 * @param NumBd is the number of BD spaces to increment
	 *
	 *****************************************************************************/
#define Dma_mRingSeekahead(RingPtr, BdPtr, NumBd)         \
	{                   \
		u64 Addr = (u64)(BdPtr);            \
		\
		Addr += ((RingPtr)->Separation * (NumBd));        \
		if ((Addr > (RingPtr)->LastBdAddr) || ((u64)(BdPtr) > Addr))\
		{                 \
			Addr -= (RingPtr)->Length;          \
		}                 \
		\
		(BdPtr) = (Dma_Bd*)Addr;            \
	}

	/******************************************************************************
	 * Move the BdPtr argument backwards an arbitrary number of BDs wrapping
	 * around to the end of the ring if needed.
	 *
	 * We know that a wraparound should occur if the new BdPtr is less than
	 * the base address in the ring OR if the new BdPtr crosses the 0xFFFFFFFF
	 * to 0 boundary.
	 *
	 * @param RingPtr is the ring BdPtr appears in
	 * @param BdPtr on input is the starting BD position and on output is the
	 *        final BD position
	 * @param NumBd is the number of BD spaces to increment
	 *
	 *****************************************************************************/
	 
#define Dma_mRingSeekback(RingPtr, BdPtr, NumBd)            \
	{                                                                     \
		u64 Addr =(u64) (BdPtr);              \
		\
		Addr -= ((RingPtr)->Separation * (NumBd));          \
		if ((Addr < (RingPtr)->FirstBdAddr) || ((u64)(BdPtr) < Addr)) \
		{                   \
			Addr += (RingPtr)->Length;            \
		}                   \
		\
		(BdPtr) = (Dma_Bd*)Addr;              \
	}

	/************************* Function Prototypes ******************************/

	/*
	 * Descriptor ring functions xdma_bdring.c
	 */
	int Dma_BdRingCreate(Dma_BdRing * RingPtr, u64 PhysAddr,
			u64 VirtAddr, u32 Alignment, unsigned BdCount);

	int Dma_BdRingStart(Dma_BdRing * RingPtr);
	int Dma_BdRingAlloc(Dma_BdRing * RingPtr, unsigned NumBd, Dma_Bd ** BdSetPtr);
	int Dma_BdRingUnAlloc(Dma_BdRing * RingPtr, unsigned NumBd, Dma_Bd * BdSetPtr);
	int Dma_BdRingToHw(Dma_BdRing * RingPtr, unsigned NumBd, Dma_Bd * BdSetPtr);
	unsigned Dma_BdRingFromHw(Dma_BdRing * RingPtr, unsigned BdLimit,
		Dma_Bd ** BdSetPtr);
	unsigned Dma_BdRingForceFromHw(Dma_BdRing * RingPtr, unsigned BdLimit,
			Dma_Bd ** BdSetPtr);
	int Dma_BdRingFree(Dma_BdRing * RingPtr, unsigned NumBd, Dma_Bd * BdSetPtr);
	int Dma_BdRingCheck(Dma_BdRing * RingPtr);
	u32 Dma_BdRingAlign(u64 AllocPtr, u32 Size, u32 Align, u32 * Delta);

#ifdef __cplusplus
}
#endif

#endif /* end of protection macro */

