/*
 * xdma.h
 *
 *  Created on: Jun 21, 2017
 *      Author: root
 */

#ifndef DMA_XDMA_H_
#define DMA_XDMA_H_

#ifdef __cplusplus
	extern "C" {
#endif

	/***************************** Include Files *********************************/

#include "../../../fpga_drivers/xilinx_vc709/dma/xdma_bdring.h"
#include "../../../fpga_drivers/xilinx_vc709/dma/xdma_user.h"
#include "../../../fpga_driver/xilinx_vc709/dma/xpmon_be.h"
#include "../../../fpga_driver/xilinx_vc709/dma/xbasic_types.h"


	/************************** Constant Definitions *****************************/

#define MAX_BARS                6       /**< Maximum number of BARs */
#define MAX_DMA_ENGINES         64      /**< Maximum number of DMA engines */

#define DMA_ALL_BDS         0xFFFFFFFF  /**< Indicates all valid BDs */

/**************************** Type Definitions *******************************/

/** @name DMA engine instance data
 * @{
 */
	typedef struct {
		u64 RegBase;            /**< Virtual base address of DMA engine */

		u32 EngineState;        /**< State of the DMA engine */
		Dma_BdRing BdRing;      /**< BD container for DMA engine */
		u32 Type;               /**< Type of DMA engine - C2S or S2C */
		int engine_No;
		int pktSize;            /**< User-specified usual size of packets */

		u64 descSpacePA; 		/**< Physical address of BD space */
		u32 descSpaceSize;      /**< Size of BD space in bytes */
		u64 * descSpaceVA;      /**< Virtual address of BD space */
		u32 delta;              /**< Shift from original start for alignment */
	} Dma_Engine;

	/************************** Function Prototypes ******************************/

	/** @name Initialization and control functions in xdma.c
	 * @{
	 */
//	int descriptor_init(struct pci_dev *pdev, Dma_Engine * eptr);
//	void descriptor_free(struct pci_dev *pdev, Dma_Engine * eptr);
	void Dma_Initialize(Dma_Engine * InstancePtr, u64 BaseAddress, u32 Type, int i);
	void Dma_Reset(Dma_Engine * InstancePtr);
	/*@}*/


#ifdef __cplusplus
	}
#endif


#endif /* DMA_XDMA_H_ */
