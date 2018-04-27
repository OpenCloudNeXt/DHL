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
 **  Reference: UG962
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
 * @file xdma_bdring.c
 *
 * This file implements buffer descriptor ring related functions. For more
 * information on this driver, see xdma.h.
 *
 * MODIFICATION HISTORY:
 *
 * Ver   Date     Changes
 * ----- -------- -------------------------------------------------------
 * 1.0   5/15/12 First release
 ******************************************************************************/

/***************************** Include Files *********************************/

#include "../../../fpga_drivers/xilinx_vc709/dma/xdma_bdring.h"

#include <linux/string.h>
#include <linux/ethtool.h>

#include <rte_common.h>
#include <rte_log.h>

#include "../../../fpga_driver/xilinx_vc709/dma/xbasic_types.h"
#include "../../../fpga_driver/xilinx_vc709/dma/xio.h"
#include "../../../fpga_drivers/xilinx_vc709/dma/xdma_hw.h"
#include "../../../fpga_drivers/xilinx_vc709/dma/xstatus.h"
#include "../../../fpga_drivers/xilinx_vc709/vc709_log.h"
#include "../vc709_type.h"


/************************** Constant Definitions *****************************/


/**************************** Type Definitions *******************************/


/***************** Macros (Inline Functions) Definitions *********************/


/************************** Function Prototypes ******************************/


/************************** Variable Definitions *****************************/


/*****************************************************************************/
/**
 * Using a memory segment allocated by the caller, create and setup the BD list
 * for the given SGDMA ring.
 *
 * @param RingPtr is the ring to be worked on.
 * @param PhysAddr is the physical base address of allocated memory region.
 * @param VirtAddr is the virtual base address of the allocated memory
 *        region.
 * @param Alignment governs the byte alignment of individual BDs. This function
 *        will enforce a minimum alignment of DMA_BD_MINIMUM_ALIGNMENT bytes
 *        with no maximum as long as it is specified as a power of 2.
 * @param BdCount is the number of BDs to set up in the application memory
 *        region. It is assumed the region is large enough to contain the BDs.
 *        Refer to the "SGDMA Descriptor Ring Creation" section  in xdma.h 
 *        for more information. The minimum valid value for this parameter is 1.
 *
 * @return
 *
 * - XST_SUCCESS if initialization was successful
 * - XST_INVALID_PARAM under any of the following conditions: 1) PhysAddr
 *   and/or VirtAddr are not aligned to the given Alignment parameter;
 *   2) Alignment parameter does not meet minimum requirements or is not a
 *   power of 2 value; 3) BdCount is 0.
 * - XST_DMA_SG_LIST_ERROR if the memory segment containing the list spans
 *   over address 0x00000000 in virtual address space.
 *
 *****************************************************************************/
int Dma_BdRingCreate(Dma_BdRing * RingPtr, u64 PhysAddr, u64 VirtAddr, u32 Alignment, unsigned BdCount)
{
	unsigned i;
	u64 BdVirtAddr;
	u64 BdPhysAddr;
	/* In case there is a failure prior to creating list, make sure the
	 * following attributes are 0 to prevent calls to other SG functions
	 * from doing anything
	 */
	RingPtr->AllCnt = 0;
	RingPtr->FreeCnt = 0;
	RingPtr->HwCnt = 0;
	RingPtr->PreCnt = 0;
	RingPtr->PostCnt = 0;
	RingPtr->BDerrs = 0;
	RingPtr->BDSerrs = 0;

	/* Make sure Alignment parameter meets minimum requirements */
	if (Alignment < DMA_BD_MINIMUM_ALIGNMENT) {
		return (XST_INVALID_PARAM);
	}

	/* Make sure Alignment is a power of 2 */
	if ((Alignment - 1) & Alignment) {
		return (XST_INVALID_PARAM);
	}

	/* Make sure PhysAddr and VirtAddr are on same Alignment */
	if ((PhysAddr % Alignment) || (VirtAddr % Alignment)) {
		return (XST_INVALID_PARAM);
	}

	/* Is BdCount reasonable? */
	if (BdCount == 0) {
		return (XST_INVALID_PARAM);
	}

	/* Compute how many bytes will be between the start of adjacent BDs */
	RingPtr->Separation =
		(sizeof(Dma_Bd) + (Alignment - 1)) & ~(Alignment - 1);

	/* Must make sure the ring doesn't span address 0x00000000. If it does,
	 * then the next/prev BD traversal macros will fail.
	 */
	if (VirtAddr > (VirtAddr + (RingPtr->Separation * BdCount) - 1)) {
		return (XST_DMA_SG_LIST_ERROR);
	}

	/* Initial ring setup:
	 *  - Clear the entire space
	 *  - Setup each BD's next pointer with the physical address of the
	 *    next BD
	 */
	RTE_LOG(INFO, PMD,  "Zeroing out BD ring space - %d bytes\n",
			(RingPtr->Separation * BdCount));
	memset((void *) VirtAddr, 0, (RingPtr->Separation * BdCount));

	BdVirtAddr = VirtAddr;
	BdPhysAddr = PhysAddr + RingPtr->Separation;
	for (i = 1; i < BdCount; i++) {
		Dma_mBdWrite(BdVirtAddr, DMA_BD_NDESC_OFFSET, BdPhysAddr);
		BdVirtAddr += RingPtr->Separation;
		BdPhysAddr += RingPtr->Separation;
	}

	/* At the end of the ring, link the last BD back to the top */
	Dma_mBdWrite(BdVirtAddr, DMA_BD_NDESC_OFFSET, PhysAddr);

	/* Set up and initialize pointers and counters */
	RingPtr->RunState = XST_DMA_SG_IS_STOPPED;
	RingPtr->FirstBdAddr = VirtAddr;
	RingPtr->FirstBdPhysAddr = PhysAddr;
	RingPtr->LastBdAddr = BdVirtAddr;
	RingPtr->Length = RingPtr->LastBdAddr - RingPtr->FirstBdAddr +
		RingPtr->Separation;
	RingPtr->AllCnt = BdCount;
	RingPtr->FreeCnt = BdCount;
	RingPtr->FreeHead = (Dma_Bd *) VirtAddr;
	RingPtr->PreHead = (Dma_Bd *) VirtAddr;
	RingPtr->HwHead = (Dma_Bd *) VirtAddr;
	RingPtr->HwTail = (Dma_Bd *) VirtAddr;
	RingPtr->PostHead = (Dma_Bd *) VirtAddr;
	RingPtr->BdaRestart = (Dma_Bd *) PhysAddr;
	{
		if(RingPtr->IsRxChannel)
			RTE_LOG(INFO, PMD,"Seems to be an Rx channel\n");
		RTE_LOG(INFO, PMD, "Ring Ptr %p:\n", RingPtr);
		RTE_LOG(INFO, PMD, "ChanBase 0x%lx, ", RingPtr->ChanBase);
		RTE_LOG(INFO, PMD, "first Bd PA 0x%lx, first Bd VA 0x%lx, last BD VA 0x%lx\n",
				RingPtr->FirstBdPhysAddr, RingPtr->FirstBdAddr, RingPtr->LastBdAddr);
		RTE_LOG(INFO, PMD, "length %d, state 0x%x, separation %d\n",
				RingPtr->Length, RingPtr->RunState, RingPtr->Separation);
		RTE_LOG(INFO, PMD,  "free count %d, pre count %d, HW count %d, post count %d, all count %d\n",
				RingPtr->FreeCnt, RingPtr->PreCnt, RingPtr->HwCnt, RingPtr->PostCnt,
				RingPtr->AllCnt);
		RTE_LOG(INFO, PMD,  "HwTail is at %lx\n", (u64)(RingPtr->HwTail));
	}
	return (XST_SUCCESS);
}

/*****************************************************************************/
/**
 * Allow DMA transactions to commence on the given channels if descriptors are
 * ready to be processed.
 *
 * @param EngPtr is a pointer to the engine instance to be worked on.
 *
 * @return
 * - XST_SUCCESS if the channel was started.
 * - XST_DMA_SG_NO_LIST if the channel has no initialized BD ring.
 *
 *****************************************************************************/
int Dma_BdRingStart(Dma_BdRing * RingPtr)
{

	/* BD list has yet to be created for this channel */
	if (RingPtr->AllCnt == 0) {
		return (XST_DMA_SG_NO_LIST);
	}

	/* Do nothing if already started */
	if (RingPtr->RunState == XST_DMA_SG_IS_STARTED) {
		return (XST_SUCCESS);
	}

	/* Flag ring as started */
	RingPtr->RunState = XST_DMA_SG_IS_STARTED;

	/* Sync hardware and driver with the last unprocessed BD or the 1st BD
	 * in the ring since this is the first time starting the channel. Update
	 * tail pointer as well, so that both pointers are equal before enabling.
	 */
	Dma_mWriteReg(RingPtr->ChanBase, REG_DMA_ENG_NEXT_BD, 
			RingPtr->BdaRestart);
	Dma_mWriteReg(RingPtr->ChanBase, REG_SW_NEXT_BD, 
			RingPtr->BdaRestart);
	wmb();

	/* Finally enable the DMA engine */
	Dma_mEnable(RingPtr->ChanBase);

	wmb();

	/* If there are unprocessed BDs then we want the channel to begin
	 * processing right away.
	 */
	if (RingPtr->HwCnt > 0) {
		Dma_mWriteReg(RingPtr->ChanBase, REG_SW_NEXT_BD,
				Dma_mVirtToPhys(RingPtr->HwTail));
	}
	if (RingPtr->IsRxChannel)
		Dma_mReadReg(RingPtr->ChanBase, 0x18);

	return (XST_SUCCESS);
}

/*****************************************************************************/
/**
 * Reserve locations in the BD ring. The set of returned BDs may be modified in
 * preparation for future DMA transactions. Once the BDs are ready to be
 * submitted to hardware, the application must call Dma_BdRingToHw() in the
 * same order which they were allocated here. Example:
 *
 * <pre>
 *        NumBd = 2;
 *        Status = Dma_RingBdAlloc(MyRingPtr, NumBd, &MyBdSet);
 *
 *        if (Status != XST_SUCCESS)
 *        {
 *            // Not enough BDs available for the request
 *        }
 *
 *        CurBd = MyBdSet;
 *        for (i=0; i<NumBd; i++)
 *        {
 *            // Prepare CurBd.....
 *
 *            // Onto next BD
 *            CurBd = Dma_mBdRingNext(MyRingPtr, CurBd);
 *        }
 *
 *        // Give list to hardware
 *        Status = Dma_BdRingToHw(MyRingPtr, NumBd, MyBdSet);
 * </pre>
 *
 * A more advanced use of this function may allocate multiple sets of BDs.
 * They must be allocated and given to hardware in the correct sequence:
 * <pre>
 *        // Legal
 *        Dma_BdRingAlloc(MyRingPtr, NumBd1, &MySet1);
 *        Dma_BdRingToHw(MyRingPtr, NumBd1, MySet1);
 *
 *        // Legal
 *        Dma_BdRingAlloc(MyRingPtr, NumBd1, &MySet1);
 *        Dma_BdRingAlloc(MyRingPtr, NumBd2, &MySet2);
 *        Dma_BdRingToHw(MyRingPtr, NumBd1, MySet1);
 *        Dma_BdRingToHw(MyRingPtr, NumBd2, MySet2);
 *
 *        // Not legal
 *        Dma_BdRingAlloc(MyRingPtr, NumBd1, &MySet1);
 *        Dma_BdRingAlloc(MyRingPtr, NumBd2, &MySet2);
 *        Dma_BdRingToHw(MyRingPtr, NumBd2, MySet2);
 *        Dma_BdRingToHw(MyRingPtr, NumBd1, MySet1);
 * </pre>
 *
 * Use the APIs defined in xdma_bd.h to modify individual BDs. Traversal of the
 * BD set can be done using Dma_mBdRingNext() and Dma_mBdRingPrev().
 *
 * @param RingPtr is a pointer to the descriptor ring instance to be worked on.
 * @param NumBd is the number of BDs to allocate
 * @param BdSetPtr is an output parameter, it points to the first BD available
 *        for modification.
 *
 * @return
 *   - XST_SUCCESS if the requested number of BDs was returned in the BdSetPtr
 *     parameter.
 *   - XST_FAILURE if there were not enough free BDs to satisfy the request.
 *
 * @note This function should not be preempted by another Dma_BdRing
 *       function call that modifies the BD space. It is the caller's
 *       responsibility to provide a mutual exclusion mechanism.
 *
 * @note Do not modify more BDs than the number requested with the NumBd
 *       parameter. Doing so will lead to data corruption and system
 *       instability.
 *
 *****************************************************************************/
int Dma_BdRingAlloc(Dma_BdRing * RingPtr, unsigned NumBd, Dma_Bd ** BdSetPtr)
{
	/* Enough free BDs available for the request? */
	if (RingPtr->FreeCnt < NumBd) {
		RTE_LOG(INFO, PMD,  "Ring %lx free count is only %d\n",
				(u64) RingPtr, (RingPtr->FreeCnt));
		return (XST_FAILURE);
	}


	/* Set the return argument and move FreeHead forward */
	*BdSetPtr = RingPtr->FreeHead;
	Dma_mRingSeekahead(RingPtr, RingPtr->FreeHead, NumBd);
	RingPtr->FreeCnt -= NumBd;
	RingPtr->PreCnt += NumBd;


//	RTE_LOG(INFO, PMD,  "Ring %lx free %d pre %d hw %d post %d\n",
//			(u64) RingPtr, RingPtr->FreeCnt, RingPtr->PreCnt, RingPtr->HwCnt,
//			RingPtr->PostCnt);

	return (XST_SUCCESS);
}


/*****************************************************************************/
/**
 * Fully or partially undo a Dma_BdRingAlloc() operation. Use this function
 * if all the BDs allocated by Dma_BdRingAlloc() could not be transferred to
 * hardware with Dma_BdRingToHw().
 *
 * This function helps out in situations when an unrelated error occurs after
 * BDs have been allocated but before they have been given to hardware.
 *
 * This function is not the same as Dma_BdRingFree(). The Free function
 * returns BDs to the free list after they have been processed by hardware,
 * while UnAlloc returns them before being processed by hardware.
 *
 * There are two scenarios where this function can be used. Full UnAlloc or
 * Partial UnAlloc. A Full UnAlloc means all the BDs Alloc'd will be returned:
 *
 * <pre>
 *    Status = Dma_BdRingAlloc(MyRingPtr, 10, &BdPtr);
 *        .
 *        .
 *    if (Error)
 *    {
 *        Status = Dma_BdRingUnAlloc(MyRingPtr, 10, &BdPtr);
 *    }
 * </pre>
 *
 * A partial UnAlloc means some of the BDs Alloc'd will be returned:
 *
 * <pre>
 *    Status = Dma_BdRingAlloc(MyRingPtr, 10, &BdPtr);
 *    BdsLeft = 10;
 *    CurBdPtr = BdPtr;
 *
 *    while (BdsLeft)
 *    {
 *       if (Error)
 *       {
 *          Status = Dma_BdRingUnAlloc(MyRingPtr, BdsLeft, CurBdPtr);
 *       }
 *
 *       CurBdPtr = Dma_mBdRingNext(MyRingPtr, CurBdPtr);
 *       BdsLeft--;
 *    }
 * </pre>
 *
 * A partial UnAlloc must include the last BD in the list that was Alloc'd.
 *
 * @param RingPtr is a pointer to the descriptor ring instance to be worked on.
 * @param NumBd is the number of BDs to unallocate
 * @param BdSetPtr points to the first of the BDs to be returned.
 *
 * @return
 *   - XST_SUCCESS if the BDs were unallocated.
 *   - XST_FAILURE if NumBd parameter was greater that the number of BDs in the
 *     preprocessing state.
 *
 * @note This function should not be preempted by another Dma ring function
 *       call that modifies the BD space. It is the caller's responsibility to
 *       provide a mutual exclusion mechanism.
 *
 *****************************************************************************/
int Dma_BdRingUnAlloc(Dma_BdRing * RingPtr, unsigned NumBd, Dma_Bd * BdSetPtr __rte_unused)
{
	/* Enough BDs in the pre-allocated state for the request? */
	if (RingPtr->PreCnt < NumBd) {
		RTE_LOG(ERR, PMD, "BdRingUnalloc: PreCnt %x NumBd %d\n", RingPtr->PreCnt, NumBd );
		return (XST_FAILURE);
	}

	/* Set the return argument and move FreeHead backward */
	Dma_mRingSeekback(RingPtr, RingPtr->FreeHead, NumBd);
	RingPtr->FreeCnt += NumBd;
	RingPtr->PreCnt -= NumBd;

	return (XST_SUCCESS);
}


/*****************************************************************************/
/**
 * Enqueue a set of BDs to hardware that were previously allocated by
 * Dma_BdRingAlloc(). Once this function returns, the argument BD set goes
 * under hardware control. Any changes made to these BDs after this point will
 * corrupt the BD list leading to data corruption and system instability.
 *
 * The set will be rejected if the last BD of the set does not mark the end of
 * a packet.
 *
 * @param RingPtr is a pointer to the descriptor ring instance to be worked on.
 * @param NumBd is the number of BDs in the set.
 * @param BdSetPtr is the first BD of the set to commit to hardware.
 *
 * @return
 *   - XST_SUCCESS if the set of BDs was accepted and enqueued to hardware
 *   - XST_FAILURE if the set of BDs was rejected because the first BD
 *     did not have its start-of-packet bit set, the last BD did not have
 *     its end-of-packet bit set, or any one of the BD set has 0 as length
 *     value
 *   - XST_DMA_SG_LIST_ERROR if this function was called out of sequence with
 *     Dma_BdRingAlloc()
 *
 * @note This function should not be preempted by another Dma ring function
 *       call that modifies the BD space. It is the caller's responsibility to
 *       provide a mutual exclusion mechanism.
 *
 *****************************************************************************/
int Dma_BdRingToHw(Dma_BdRing * RingPtr, unsigned NumBd, Dma_Bd * BdSetPtr)
{
	Dma_Bd *CurBdPtr;
	unsigned i;
	u32 BdStsCr;

	/* If the commit set is empty, do nothing */
	if (NumBd == 0) {
		return (XST_SUCCESS);
	}

	/* Make sure we are in sync with Dma_BdRingAlloc() */
	if ((RingPtr->PreCnt < NumBd) || (RingPtr->PreHead != BdSetPtr)) {
		RTE_LOG(ERR, PMD, "PreCnt is %d, PreHead is %lx\n",
				RingPtr->PreCnt, (u64) (RingPtr->PreHead));
		RTE_LOG(ERR, PMD, "returning XST_DMA_SG_LIST_ERROR\n");
		return (XST_DMA_SG_LIST_ERROR);
	}

	CurBdPtr = BdSetPtr;
	BdStsCr = Dma_mBdRead(CurBdPtr, DMA_BD_BUFL_CTRL_OFFSET);

	/* In case of Tx channel, the first BD should have been marked
	 * as start-of-packet.
	 */
	if (!(RingPtr->IsRxChannel) && !(BdStsCr & DMA_BD_SOP_MASK)) {
		RTE_LOG(ERR, PMD, "First TX BD should have SOP\n");
		return (XST_FAILURE);
	}

	/* For each BD being submitted, do checks, and clear the status field. */
	for (i = 0; i < NumBd; i++) {
		/* Check for the control flags */
		BdStsCr = Dma_mBdRead(CurBdPtr, DMA_BD_BUFL_CTRL_OFFSET);

		/* Make sure the length value in the BD is non-zero. Actually, the
		 * last TX BD need not have non-zero length, but to simplify things,
		 * that is ignored.
		 */
		if ((BdStsCr & DMA_BD_BUFL_MASK) == 0) {
			RTE_LOG(ERR, PMD, "BDs should have length.\n");
			break;
		}

		/* Clear status field. */
		Dma_mBdSetStatus(CurBdPtr, 0);

		CurBdPtr = Dma_mBdRingNext(RingPtr, CurBdPtr);
	}

	if(i != NumBd) {
		RTE_LOG(ERR, PMD, "%d BDs instead of %d\n", i, NumBd);
		return (XST_FAILURE);
	}

	/* Last TX BD should have end-of-packet bit set */
	if (!(RingPtr->IsRxChannel) && !(BdStsCr & DMA_BD_EOP_MASK)) {
		RTE_LOG(ERR, PMD, "Last TX BD should have EOP\n");
	}

	/* This set has completed pre-processing, adjust ring pointers and
	 * counters
	 */
	Dma_mRingSeekahead(RingPtr, RingPtr->PreHead, NumBd);
	RingPtr->PreCnt -= NumBd;
	RingPtr->HwTail = CurBdPtr;
	RingPtr->HwCnt += NumBd;

	/* If it was enabled, tell the engine to begin processing */
	if (RingPtr->RunState == XST_DMA_SG_IS_STARTED) {

		/* Ensure that all the descriptor updates are completed before 
		 * informing the hardware.
		 */
		wmb();

		/* In NWL DMA engine, the tail descriptor pointer should actually
		 * point to the next (unused BD). 
		 */
		Dma_mWriteReg(RingPtr->ChanBase, REG_SW_NEXT_BD,
				Dma_mVirtToPhys(RingPtr->HwTail));
//		RTE_LOG(INFO, PMD,  "Writing %p into %lx\n",
//				Dma_mVirtToPhys(RingPtr->HwTail),
//				(RingPtr->ChanBase+ REG_SW_NEXT_BD));
	}
//	RTE_LOG(INFO, PMD,  "ToHw with %d BDs\n", NumBd);

	return (XST_SUCCESS);
}


/*****************************************************************************/
/**
 * Returns a set of BDs that have been processed by hardware. The returned
 * BDs may be examined by the caller to determine the outcome of the DMA
 * transactions. Once the BDs have been examined, the caller must call
 * Dma_BdRingFree() in the same order which they were retrieved here.
 *
 * Example:
 *
 * <pre>
 *        NumBd = Dma_BdRingFromHw(MyRingPtr, DMA_ALL_BDS, &MyBdSet);
 *
 *        if (NumBd == 0)
 *        {
 *           // hardware has nothing ready for us yet.
 *        }
 *
 *        CurBd = MyBdSet;
 *        for (i=0; i<NumBd; i++)
 *        {
 *           // Examine CurBd for post processing.....
 *
 *           // Onto next BD
 *           CurBd = Dma_mBdRingNext(MyRingPtr, CurBd);
 *        }
 *
 *        Dma_BdRingFree(MyRingPtr, NumBd, MyBdSet); // Return the list
 * </pre>
 *
 * A more advanced use of this function may allocate multiple sets of BDs.
 * They must be retrieved from hardware and freed in the correct sequence:
 * <pre>
 *        // Legal
 *        Dma_BdRingFromHw(MyRingPtr, NumBd1, &MySet1);
 *        Dma_BdRingFree(MyRingPtr, NumBd1, MySet1);
 *
 *        // Legal
 *        Dma_BdRingFromHw(MyRingPtr, NumBd1, &MySet1);
 *        Dma_BdRingFromHw(MyRingPtr, NumBd2, &MySet2);
 *        Dma_BdRingFree(MyRingPtr, NumBd1, MySet1);
 *        Dma_BdRingFree(MyRingPtr, NumBd2, MySet2);
 *
 *        // Not legal
 *        Dma_BdRingFromHw(MyRingPtr, NumBd1, &MySet1);
 *        Dma_BdRingFromHw(MyRingPtr, NumBd2, &MySet2);
 *        Dma_BdRingFree(MyRingPtr, NumBd2, MySet2);
 *        Dma_BdRingFree(MyRingPtr, NumBd1, MySet1);
 * </pre>
 *
 * If hardware has partially completed a packet spanning multiple BDs, then
 * none of the BDs for that packet will be included in the results.
 *
 * @param RingPtr is a pointer to the descriptor ring instance to be worked on.
 * @param BdLimit is the maximum number of BDs to return in the set. Use
 *        DMA_ALL_BDS to return all BDs that have been processed.
 * @param BdSetPtr is an output parameter, it points to the first BD available
 *        for examination.
 *
 * @return
 *   The number of BDs processed by hardware. A value of 0 indicates that no
 *   data is available. No more than BdLimit BDs will be returned.
 *
 * @note Treat BDs returned by this function as read-only.
 *
 * @note This function should not be preempted by another Dma ring function
 *       call that modifies the BD space. It is the caller's responsibility to
 *       provide a mutual exclusion mechanism.
 *
 *****************************************************************************/
unsigned Dma_BdRingFromHw(Dma_BdRing * RingPtr, unsigned BdLimit,
		Dma_Bd ** BdSetPtr)
{
	Dma_Bd *CurBdPtr;
	unsigned BdCount;
	unsigned BdPartialCount;
	u32 BdStsCr, BdCtrl;
	unsigned long long userInfo;

	CurBdPtr = RingPtr->HwHead;
	BdCount = 0;
	BdPartialCount = 0;

	/* If no BDs in work group, then there's nothing to search */
	if (RingPtr->HwCnt == 0) {
		*BdSetPtr = NULL;
		return (0);
	}

	/* Starting at HwHead, keep moving forward in the list until one of
	 * of the following conditions is reached:
	 *  - A BD is encountered with its completed bit and error bit clear 
	 *    in the status word which means hardware has not completed 
	 *    processing of that BD.
	 *  - TX: Check EOP bit in control field, RX: check EOP bit in status
	 *  - In RX channel case, a BD has EOP flag set, with non-zero User
	 *    Status, but its User Status fields are still showing zero.
	 *    This means hardware has not completed updating the RX BD structure.
	 *  - RingPtr->HwTail is reached
	 *  - The number of requested BDs has been processed
	 */
	while (BdCount < BdLimit) {
		/* Read the status and control fields */
		BdStsCr = Dma_mBdGetStatus(CurBdPtr);
		BdCtrl = Dma_mBdGetCtrl(CurBdPtr);
		userInfo = Dma_mBdGetUserData(CurBdPtr);

		/* How to handle errors? Not doing anything for now. */
		if (BdStsCr & DMA_BD_ERROR_MASK) 
		{
			if (!(RingPtr->IsRxChannel))
				RTE_LOG(ERR, PMD, "TX: BD %p had error\n", CurBdPtr);
			else
				RTE_LOG(ERR, PMD, "RX: BD %p had error\n", CurBdPtr);
			RingPtr->BDerrs ++;
			RTE_LOG(ERR, PMD,"BD status %x next BD's PA %x\n", BdStsCr, Dma_mBdRead((CurBdPtr), DMA_BD_NDESC_OFFSET));
			RTE_LOG(ERR, PMD,"Buffer PA %x\n", Dma_mBdRead((CurBdPtr), DMA_BD_BUFAL_OFFSET));
			RTE_LOG(ERR, PMD,"DMA Engine Control %x\n", Dma_mReadReg(RingPtr->ChanBase, 0x4));
			RTE_LOG(ERR, PMD,"DMA Head Ptr %x\n", Dma_mReadReg(RingPtr->ChanBase, 0x8));
			RTE_LOG(ERR, PMD,"DMA SW Desc Ptr %x\n", Dma_mReadReg(RingPtr->ChanBase, 0xC));
			RTE_LOG(ERR, PMD,"DMA Comp Desc Ptr %x\n", Dma_mReadReg(RingPtr->ChanBase, 0x10));
		}

		/* For a TX BD, short processing is an error. */
		if (!(RingPtr->IsRxChannel) && (BdStsCr & DMA_BD_SHORT_MASK)) 
		{
			RTE_LOG(ERR, PMD, "TX BD %p had short error\n", CurBdPtr);
			RTE_LOG(ERR, PMD,"BD status %x next BD's PA %x\n", BdStsCr, Dma_mBdRead((CurBdPtr), DMA_BD_NDESC_OFFSET));
			RTE_LOG(ERR, PMD,"Buffer PA %x\n", Dma_mBdRead((CurBdPtr), DMA_BD_BUFAL_OFFSET));
			RingPtr->BDSerrs ++;
		}

		/* If the hardware still hasn't processed this BD then we are done */
		if (!(BdStsCr & DMA_BD_COMP_MASK))
		{
			break;
		}

		/* Is it really done? For RX EOP BDs, check the user info field also */
		if ((RingPtr->IsRxChannel) && (BdStsCr & DMA_BD_EOP_MASK)) 
		{
			if(!(BdStsCr & DMA_BD_USER_HIGH_ZERO_MASK) &&
					!(userInfo & 0xFFFFFFFF00000000LL)) break;
			if(!(BdStsCr & DMA_BD_USER_LOW_ZERO_MASK) &&
					!(userInfo & 0xFFFFFFFFLL)) break;
		}



		BdCount++;

		/* Hardware has processed this BD so check the EOP bit. If
		 * it is clear, then there are more BDs for the current packet.
		 * Keep a count of these partial packet BDs.
		 * Note that - for RX, check EOP bit in status field, and for
		 * TX, check EOP bit in control field.
		 * If error bits are set, EOP should not be a requirement. 
		 */
		if (((RingPtr->IsRxChannel) && (BdStsCr & DMA_BD_EOP_MASK)) ||
				(!(RingPtr->IsRxChannel) && (BdCtrl & DMA_BD_EOP_MASK)) ||
				(BdStsCr & DMA_BD_ERROR_MASK))
		{
			BdPartialCount = 0;
		}
		else {
			BdPartialCount++;
		}

		/* Move on to next BD in work group */
		CurBdPtr = Dma_mBdRingNext(RingPtr, CurBdPtr);

		/* Reached the end of the work group */
		if (CurBdPtr == RingPtr->HwTail) {
			break;
		}
	}
	/* Subtract off any partial packet BDs found */
	BdCount -= BdPartialCount;

	/* If BdCount is non-zero then BDs were found to return. Set return
	 * parameters, update pointers and counters, return success
	 */
	if (BdCount) {
		*BdSetPtr = RingPtr->HwHead;
		RingPtr->HwCnt -= BdCount;
		RingPtr->PostCnt += BdCount;
		Dma_mRingSeekahead(RingPtr, RingPtr->HwHead, BdCount);
		return (BdCount);
	}
	else {
		*BdSetPtr = NULL;
		return (0);
	}
}

/*****************************************************************************/
/**
 * Returns a set of BDs that had been enqueued to hardware, whether the
 * hardware has processed them yet or not. This should be called only
 * when the driver is freeing up buffers and about to free the ring.
 * The returned BDs may be examined by the caller in order to recover
 * the buffers. Once the BDs have been examined, the caller must call
 * Dma_BdRingFree() in the same order which they were retrieved here.
 *
 * @param RingPtr is a pointer to the descriptor ring instance to be worked on.
 * @param BdLimit is the maximum number of BDs to return in the set. Use
 *        DMA_ALL_BDS to return all BDs that have been processed.
 * @param BdSetPtr is an output parameter, it points to the first BD available
 *        for examination.
 *
 * @return
 *   The number of BDs processed by hardware. A value of 0 indicates that no
 *   data is available. No more than BdLimit BDs will be returned.
 *
 * @note Treat BDs returned by this function as read-only.
 *
 * @note This function should not be preempted by another Dma ring function
 *       call that modifies the BD space. It is the caller's responsibility to
 *       provide a mutual exclusion mechanism.
 *
 *****************************************************************************/
unsigned Dma_BdRingForceFromHw(Dma_BdRing * RingPtr, unsigned BdLimit,
		Dma_Bd ** BdSetPtr)
{
	Dma_Bd *CurBdPtr;
	unsigned BdCount;
	u32 BdStsCr;

	CurBdPtr = RingPtr->HwHead;
	BdCount = 0;

	RTE_LOG(INFO, PMD,  "ForceFromHw: HwCnt is %d\n", RingPtr->HwCnt);
	/* If no BDs in work group, then there's nothing to search */
	if (RingPtr->HwCnt == 0) {
		*BdSetPtr = NULL;
		return (0);
	}

	/* Starting at HwHead, keep moving forward in the list until one of
	 * of the following conditions is reached:
	 *  - RingPtr->HwTail is reached
	 *  - The number of requested BDs has been processed
	 */
	while (BdCount < BdLimit) {
		/* Read the status */
		BdStsCr = Dma_mBdGetStatus(CurBdPtr);
//		RTE_LOG(INFO, PMD,  "BD Status is %x\n", BdStsCr);

		/* How to handle errors? Not doing anything for now. */
		if (BdStsCr & DMA_BD_ERROR_MASK) 
			RTE_LOG(ERR, PMD, "BD %p had error\n", CurBdPtr);

		BdCount++;

		/* Move on to next BD in work group */
		CurBdPtr = Dma_mBdRingNext(RingPtr, CurBdPtr);

		/* Reached the end of the work group */
		if (CurBdPtr == RingPtr->HwTail) {
			break;
		}
	}

	/* If BdCount is non-zero then no BDs were found to return. Set return
	 * parameters, update pointers and counters, return success.
	 */
	if (BdCount) {
		*BdSetPtr = RingPtr->HwHead;
		RingPtr->HwCnt -= BdCount;
		RingPtr->PostCnt += BdCount;
		Dma_mRingSeekahead(RingPtr, RingPtr->HwHead, BdCount);
		return (BdCount);
	}
	else {
		*BdSetPtr = NULL;
		return (0);
	}
}

/*****************************************************************************/
/**
 * Frees a set of BDs that had been previously retrieved with
 * Dma_BdRingFromHw(). This function also clear all control/status bits
 * (like SOP/EOP) of each BD in the set. 
 *
 * @param RingPtr is a pointer to the descriptor ring instance to be worked on.
 * @param NumBd is the number of BDs to free.
 * @param BdSetPtr is the head of a list of BDs returned by Dma_BdRingFromHw().
 *
 * @return
 *   - XST_SUCCESS if the set of BDs was freed.
 *   - XST_DMA_SG_LIST_ERROR if this function was called out of sequence with
 *     Dma_BdRingFromHw().
 *
 * @note This function should not be preempted by another Dma function call
 *       that modifies the BD space. It is the caller's responsibility to
 *       provide a mutual exclusion mechanism.
 *
 * @internal
 *   The Interrupt handler provided by application MUST clear pending
 *   interrupts before handling them by calling the call back. Otherwise
 *   the following corner case could raise some issue:
 *
 *   - A packet was transmitted and asserted an TX interrupt, and if
 *     the interrupt handler calls the call back before clears the
 *     interrupt, another packet could get transmitted (and assert the
 *     interrupt) between when the call back function returned and when
 *     the interrupt clearing operation begins, and the interrupt
 *     clearing operation will clear the interrupt raised by the second
 *     packet and won't never process its according buffer descriptors
 *     until a new interrupt occurs.
 *
 *   Changing the sequence to "Clear interrupts, then handle" solves this
 *   issue. If the interrupt raised by the second packet is before the
 *   the interrupt clearing operation, the descriptors associated with
 *   the second packet must have been finished by hardware and ready for
 *   the handling by the call back; otherwise, the interrupt raised by
 *   the second packet is after the interrupt clearing operation,
 *   the packet's buffer descriptors will be handled by the call back in
 *   current pass, if the descriptors are finished before the call back
 *   is invoked, or next pass otherwise.
 *
 *   Please note that if the second packet is handled by the call back
 *   in current pass, the next pass could find no buffer descriptor
 *   finished by the hardware. (i.e., Dma_BdRingFromHw() returns 0).
 *   As Dma_BdRingFromHw() and Dma_BdRingFree() are used in pair,
 *   Dma_BdRingFree() covers this situation by checking if the BD
 *   list to free is empty.
 *****************************************************************************/
int Dma_BdRingFree(Dma_BdRing * RingPtr, unsigned NumBd, Dma_Bd * BdSetPtr)
{
	Dma_Bd * CurBdPtr;
	unsigned i;

	/* If the BD Set to free is empty, return immediately with value
	 * XST_SUCCESS. See the @internal comment block above for detailed
	 * information.
	 */
	if (NumBd == 0) {
		return XST_SUCCESS;
	}

	/* Make sure we are in sync with Dma_BdRingFromHw() */
	if ((RingPtr->PostCnt < NumBd) || (RingPtr->PostHead != BdSetPtr)) {
		RTE_LOG(ERR, PMD,"Some out-of-sync error.\n");
		return (XST_DMA_SG_LIST_ERROR);
	}

	/* Clear Status/Control field for all BDs in the set. Since the BDs
	 * have been freed anyway, we are not bothering to keep the length info.
	 */
	CurBdPtr = BdSetPtr;
	for (i = 0; i < NumBd; i++) {
		Dma_mBdWrite(CurBdPtr, DMA_BD_BUFL_STATUS_OFFSET, 0);
		Dma_mBdWrite(CurBdPtr, DMA_BD_BUFL_CTRL_OFFSET, 0);

		/* Move on to next BD in work group */
		CurBdPtr = Dma_mBdRingNext(RingPtr, CurBdPtr);
	}

	/* Update pointers and counters */
	RingPtr->FreeCnt += NumBd;
	RingPtr->PostCnt -= NumBd;
	Dma_mRingSeekahead(RingPtr, RingPtr->PostHead, NumBd);

//	RTE_LOG(INFO, PMD,  "Ring %lx free %d pre %d hw %d post %d\n",
//			(u64) RingPtr, RingPtr->FreeCnt, RingPtr->PreCnt, RingPtr->HwCnt,
//			RingPtr->PostCnt);

	return (XST_SUCCESS);
}


/*****************************************************************************/
/**
 * Check the internal data structures of the BD ring for the provided channel.
 * The following checks are made:
 *
 *   - Is the BD ring linked correctly in physical address space.
 *   - Do the internal pointers point to BDs in the ring.
 *   - Do the internal counters add up.
 *
 * The channel should be stopped prior to calling this function.
 *
 * @param RingPtr is a pointer to the descriptor ring to be worked on.
 *
 * @return
 *   - XST_SUCCESS if no errors were found.
 *   - XST_DMA_SG_NO_LIST if the ring has not been created.
 *   - XST_IS_STARTED if the channel is not stopped.
 *   - XST_DMA_SG_LIST_ERROR if a problem is found with the internal data
 *     structures. If this value is returned, the channel should be reset to
 *     avoid data corruption or system instability.
 *
 * @note This function should not be preempted by another Dma ring function
 *       call that modifies the BD space. It is the caller's responsibility to
 *       provide a mutual exclusion mechanism.
 *
 *****************************************************************************/
int Dma_BdRingCheck(Dma_BdRing * RingPtr)
{
	u32 AddrV, AddrP;
	unsigned i;

	/* Is the list created */
	if (RingPtr->AllCnt == 0) {
		return (XST_DMA_SG_NO_LIST);
	}

	/* Can't check if channel is running */
	if (RingPtr->RunState == XST_DMA_SG_IS_STARTED) {
		return (XST_IS_STARTED);
	}

	/* RunState doesn't make sense */
	else if (RingPtr->RunState != XST_DMA_SG_IS_STOPPED) {
		return (XST_DMA_SG_LIST_ERROR);
	}

	/* Verify internal pointers point to correct memory space */
	AddrV = (u64) RingPtr->FreeHead;
	if ((AddrV < RingPtr->FirstBdAddr) || (AddrV > RingPtr->LastBdAddr)) {
		return (XST_DMA_SG_LIST_ERROR);
	}

	AddrV = (u64) RingPtr->PreHead;
	if ((AddrV < RingPtr->FirstBdAddr) || (AddrV > RingPtr->LastBdAddr)) {
		return (XST_DMA_SG_LIST_ERROR);
	}

	AddrV = (u64) RingPtr->HwHead;
	if ((AddrV < RingPtr->FirstBdAddr) || (AddrV > RingPtr->LastBdAddr)) {
		return (XST_DMA_SG_LIST_ERROR);
	}

	AddrV = (u64) RingPtr->HwTail;
	if ((AddrV < RingPtr->FirstBdAddr) || (AddrV > RingPtr->LastBdAddr)) {
		return (XST_DMA_SG_LIST_ERROR);
	}

	AddrV = (u64) RingPtr->PostHead;
	if ((AddrV < RingPtr->FirstBdAddr) || (AddrV > RingPtr->LastBdAddr)) {
		return (XST_DMA_SG_LIST_ERROR);
	}

	/* Verify internal counters add up */
	if ((RingPtr->HwCnt + RingPtr->PreCnt + RingPtr->FreeCnt +
				RingPtr->PostCnt) != RingPtr->AllCnt) {
		return (XST_DMA_SG_LIST_ERROR);
	}

	/* Verify BDs are linked correctly */
	AddrV = RingPtr->FirstBdAddr;
	AddrP = RingPtr->FirstBdPhysAddr + RingPtr->Separation;
	for (i = 1; i < RingPtr->AllCnt; i++) {
		/* Check next pointer for this BD. It should equal to the
		 * physical address of next BD
		 */
		if (Dma_mBdRead(AddrV, DMA_BD_NDESC_OFFSET) != AddrP) {
			return (XST_DMA_SG_LIST_ERROR);
		}

		/* Move on to next BD */
		AddrV += RingPtr->Separation;
		AddrP += RingPtr->Separation;
	}

	/* Last BD should point back to the beginning of ring */
	if (Dma_mBdRead(AddrV, DMA_BD_NDESC_OFFSET) != RingPtr->FirstBdPhysAddr) {
		return (XST_DMA_SG_LIST_ERROR);
	}

	/* No problems found */
	return (XST_SUCCESS);
}

/*****************************************************************************/
/**
 * Align the BD space.
 *
 * The DMA engine requires aligned BDs. This function aligns the space as
 * required by the caller.
 *
 * @param  AllocPtr is the virtual address of the allocated BD space.
 * @param  Size is the size of the allocated BD space.
 * @param  Align is the number of bytes of alignment required.
 * @param  Delta is the shift required in virtual and physical addresses
 *         in order to achieve the alignment. This should be applied by the
 *         caller.
 *
 * @return Number of BDs that will fit in the re-aligned BD space.
 *
 * @note
 *
 ******************************************************************************/
u32 Dma_BdRingAlign(u64 AllocPtr, u32 Size, u32 Align, u32 * Delta)
{
	u32 numbds;
	u32 i;
	int bytealign;

	RTE_LOG(INFO, PMD,  "BD space %lx, Size %d, Align %d\n",
			AllocPtr, Size, Align);

	/* Check for valid arguments. Minimum size check also. */
	if(!AllocPtr || !Size || (Size < Align))
	{
		RTE_LOG(ERR, PMD, "Bad arguments Alloc %lx Size %d\n",
				AllocPtr, Size);
		*Delta = 0;
		return 0;
	}

	bytealign = (sizeof(u32)*DMA_BD_SW_NUM_WORDS);
	numbds = Size / bytealign;
	RTE_LOG(INFO, PMD,  "Number of BDs before aligning is %d\n", numbds);

	if(AllocPtr % bytealign)
	{
		/* Alignment is required. */
		RTE_LOG(INFO, PMD,  "Realignment is required\n");

		i = bytealign - (AllocPtr & 0xFF);
		Size -= i;
		*Delta = i;
		numbds = Size / bytealign;
	}
	else
		RTE_LOG(INFO, PMD,  "Alignment is fine\n");

	RTE_LOG(ERR, PMD, "BD space should be shifted by %d bytes\n", *Delta);
	RTE_LOG(ERR, PMD, "After aligning, # BDs %d, size %d\n", numbds, Size);
	return numbds;
}

