

/***************************** Include Files *********************************/

#include "../../../fpga_drivers/xilinx_vc709/dma/xdma.h"

#include <linux/string.h>
#include <linux/kernel.h>

#include "../../../fpga_drivers/xilinx_vc709/dma/xdma_hw.h"
#include "../../../fpga_drivers/xilinx_vc709/dma/xstatus.h"
#include "../../../fpga_drivers/xilinx_vc709/vc709_log.h"


/*****************************************************************************/
/**
 * This function initializes a DMA engine.  This function must be called
 * prior to using the DMA engine. Initialization of an engine includes setting
 * up the register base address, setting up the instance data, and ensuring the
 * hardware is in a quiescent state.
 *
 * @param  InstancePtr is a pointer to the DMA engine instance to be worked on.
 * @param  BaseAddress is where the registers for this engine can be found.
 *
 * @return None.
 *
 *****************************************************************************/
void Dma_Initialize(Dma_Engine * InstancePtr, u64 BaseAddress, u32 Type, int i)
{
	RTE_LOG(INFO, PMD,"Initializing DMA()\n");

	/* Set up the instance */
	memset(InstancePtr, 0, sizeof(Dma_Engine));

	InstancePtr->engine_No = i;

	RTE_LOG(INFO, PMD,"DMA base address is 0x%lx\n", BaseAddress);
	InstancePtr->RegBase = BaseAddress;
	InstancePtr->Type = Type;

	/* Initialize the engine and ring states. */
	InstancePtr->BdRing.RunState = XST_DMA_SG_IS_STOPPED;
	InstancePtr->EngineState = INITIALIZED;

	/* Initialize the ring structure */
	InstancePtr->BdRing.ChanBase = BaseAddress;
	if(Type == DMA_ENG_C2S)
		InstancePtr->BdRing.IsRxChannel = 1;
	else
		InstancePtr->BdRing.IsRxChannel = 0;

	/* Reset the device and return */
	Dma_Reset(InstancePtr);
}

/*****************************************************************************/
/**
 * Reset the DMA engine.
 *
 * Should not be invoked during initialization stage because hardware has
 * just come out of a system reset. Should be invoked during shutdown stage.
 *
 * New BD fetches will stop immediately. Reset will be completed once the
 * user logic completes its reset. DMA disable will be completed when the
 * BDs already being processed are completed.
 *
 * @param  InstancePtr is a pointer to the DMA engine instance to be worked on.
 *
 * @return None.
 *
 * @note
 *   - If the hardware is not working properly, and the self-clearing reset
 *     bits do not clear, this function will be terminated after a timeout.
 *
 ******************************************************************************/
void Dma_Reset(Dma_Engine * InstancePtr)
{
	Dma_BdRing *RingPtr;
	int i=0;
	u32 dirqval;


	RingPtr = &Dma_mGetRing(InstancePtr);

	/* Disable engine interrupts before issuing software reset */
	Dma_mEngIntDisable(InstancePtr);

	/* Start reset process then wait for completion. Disable DMA and 
	 * assert reset request at the same time. This causes user logic to
	 * be reset.
	 */
	i=0;
	Dma_mSetCrSr(InstancePtr, (DMA_ENG_DISABLE|DMA_ENG_USER_RESET));

	/* Loop until the reset is done. The bits will self-clear. */
	while (Dma_mGetCrSr(InstancePtr) & 
			(DMA_ENG_STATE_MASK|DMA_ENG_USER_RESET)) {
		i++;
		if(i >= 100000) 
		{
			RTE_LOG(INFO, PMD,"CR(control register offset is 0x00000004) is now 0x%x\n", Dma_mGetCrSr(InstancePtr));
			break;
		}
	}

	/* Now reset the DMA engine, and wait for its completion. */
	i=0;
	Dma_mSetCrSr(InstancePtr, (DMA_ENG_RESET));

	/* Loop until the reset is done. The bit will self-clear. */
	while (Dma_mGetCrSr(InstancePtr) & DMA_ENG_RESET) {
		i++;
		if(i >= 100000) 
		{
			RTE_LOG(INFO, PMD,"CR(control register offset is 0x00000004) is now 0x%x\n", Dma_mGetCrSr(InstancePtr));
			break;
		}
	}

	/* Clear Interrupt register. Not doing so may cause interrupts 
	 * to be asserted after the engine reset if there is any
	 * interrupt left over from before.
	 */
	dirqval = Dma_mGetCrSr(InstancePtr);
	RTE_LOG(INFO, PMD,"While resetting engine %d, got 0x%x in eng status reg (CR)\n", InstancePtr->engine_No, dirqval);
	if(dirqval & DMA_ENG_INT_ACTIVE_MASK)
		Dma_mEngIntAck(InstancePtr, (dirqval & DMA_ENG_ALLINT_MASK));

	RingPtr->RunState = XST_DMA_SG_IS_STOPPED;
}

