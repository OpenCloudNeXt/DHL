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
 * @file xdma_hw.h
 *
 * This header file contains identifiers and register-level driver functions
 * and macros that can be used to access the Northwest Logic Scatter-gather 
 * Direct Memory Access device.
 *
 * For more information about the operation of this device, see the hardware
 * specification documentation.
 *
 * MODIFICATION HISTORY:
 *
 * Ver   Date     Changes
 * ----- -------- -------------------------------------------------------
 * 1.0   5/15/12  First version
 *
 ******************************************************************************/

#ifndef XDMA_HW_H   /* prevent circular inclusions */
#define XDMA_HW_H   /* by using protection macros */

#ifdef __cplusplus
extern "C" {
#endif

	/***************************** Include Files *********************************/

#include "xio.h"


	/************************** Constant Definitions *****************************/

	/** @name Device register offset definitions. Register access is 32-bit.
	 *  @{
	 */

	/* Common DMA registers */
#define REG_DMA_CTRL_STATUS     0x4000      /**< DMA Common Ctrl & Status */

	/* These engine registers are applicable to both S2C and C2S channels. 
	 * Register field mask and shift definitions are later in this file.
	 */

#define REG_DMA_ENG_CAP         0x00000000  /**< DMA Engine Capabilities */
#define REG_DMA_ENG_CTRL_STATUS 0x00000004  /**< DMA Engine Control */
#define REG_DMA_ENG_NEXT_BD     0x00000008  /**< HW Next desc pointer */
#define REG_SW_NEXT_BD          0x0000000C  /**< SW Next desc pointer */
#define REG_DMA_ENG_LAST_BD     0x00000010  /**< HW Last completed pointer */
#define REG_DMA_ENG_ACTIVE_TIME 0x00000014  /**< DMA Engine Active Time */
#define REG_DMA_ENG_WAIT_TIME   0x00000018  /**< DMA Engine Wait Time */
#define REG_DMA_ENG_COMP_BYTES  0x0000001C  /**< DMA Engine Completed Bytes */

	/*@}*/

	/* Register masks. The following constants define bit locations of various
	 * control bits in the registers. For further information on the meaning of 
	 * the various bit masks, refer to the hardware spec.
	 *
	 * Masks have been written assuming HW bits 0-31 correspond to SW bits 0-31 
	 */

	/** @name Bitmasks of REG_DMA_CTRL_STATUS register.
	 * @{
	 */
#define DMA_INT_ENABLE              0x00000001  /**< Enable global interrupts */
#define DMA_INT_DISABLE             0x00000000  /**< Disable interrupts */
#define DMA_INT_ACTIVE_MASK         0x00000002  /**< Interrupt active? */
#define DMA_INT_PENDING_MASK        0x00000004  /**< Engine interrupt pending */
#define DMA_INT_MSI_MODE            0x00000008  /**< MSI or Legacy mode? */
#define DMA_USER_INT_ENABLE         0x00000010  /**< Enable user interrupts */
#define DMA_USER_INT_ACTIVE_MASK    0x00000020  /**< Int - user interrupt */
#define DMA_USER_INT_ACK            0x00000020  /**< Acknowledge */
#define DMA_MPS_USED                0x00000700  /**< MPS Used */
#define DMA_MRRS_USED               0x00007000  /**< MRRS Used */
#define DMA_S2C_ENG_INT_VAL         0x00FF0000  /**< IRQ value of 1st 8 S2Cs */
#define DMA_C2S_ENG_INT_VAL         0xFF000000  /**< IRQ value of 1st 8 C2Ss */

	/** @name Bitmasks of REG_DMA_ENG_CAP register.
	 * @{
	 */
	/* DMA engine characteristics */
#define DMA_ENG_PRESENT_MASK    0x00000001  /**< DMA engine present? */
#define DMA_ENG_DIRECTION_MASK  0x00000002  /**< DMA engine direction */
#define DMA_ENG_C2S             0x00000002  /**< DMA engine - C2S */
#define DMA_ENG_S2C             0x00000000  /**< DMA engine - S2C */
#define DMA_ENG_TYPE_MASK       0x00000010  /**< DMA engine type */
#define DMA_ENG_BLOCK           0x00000000  /**< DMA engine - Block type */
#define DMA_ENG_PACKET          0x00000010  /**< DMA engine - Packet type */
#define DMA_ENG_NUMBER          0x0000FF00  /**< DMA engine number */
#define DMA_ENG_BD_MAX_BC       0x3F000000  /**< DMA engine max buffer size */
#define DMA_ENG_SCAL_FACT       0xC0000000  /**< DMA engine scalling factor */ 

	/* Shift constants for selected masks */
#define DMA_ENG_NUMBER_SHIFT        8
#define DMA_ENG_BD_MAX_BC_SHIFT     24

	/*@}*/


	/** @name Bitmasks of REG_DMA_ENG_CTRL_STATUS register.
	 * @{
	 */
	/* Interrupt activity and acknowledgement bits */
#define DMA_ENG_INT_ENABLE          0x00000001  /**< Enable interrupts */
#define DMA_ENG_INT_DISABLE         0x00000000  /**< Disable interrupts */
#define DMA_ENG_INT_ACTIVE_MASK     0x00000002  /**< Interrupt active? */
#define DMA_ENG_INT_ACK             0x00000002  /**< Interrupt ack */
#define DMA_ENG_INT_BDCOMP          0x00000004  /**< Int - BD completion */
#define DMA_ENG_INT_BDCOMP_ACK      0x00000004  /**< Acknowledge */
#define DMA_ENG_INT_ALERR           0x00000008  /**< Int - BD align error */
#define DMA_ENG_INT_ALERR_ACK       0x00000008  /**< Acknowledge */
#define DMA_ENG_INT_FETERR          0x00000010  /**< Int - BD fetch error */
#define DMA_ENG_INT_FETERR_ACK      0x00000010  /**< Acknowledge */
#define DMA_ENG_INT_ABORTERR        0x00000020  /**< Int - DMA abort error */
#define DMA_ENG_INT_ABORTERR_ACK    0x00000020  /**< Acknowledge */
#define DMA_ENG_INT_CHAINEND        0x00000080  /**< Int - BD chain ended */
#define DMA_ENG_INT_CHAINEND_ACK    0x00000080  /**< Acknowledge */

	/* DMA engine control */
#define DMA_ENG_ENABLE_MASK         0x00000100  /**< DMA enabled? */
#define DMA_ENG_ENABLE              0x00000100  /**< Enable DMA */
#define DMA_ENG_DISABLE             0x00000000  /**< Disable DMA */
#define DMA_ENG_STATE_MASK          0x00000C00  /**< Current DMA state? */
#define DMA_ENG_RUNNING             0x00000400  /**< DMA running */
#define DMA_ENG_IDLE                0x00000000  /**< DMA idle */
#define DMA_ENG_WAITING             0x00000800  /**< DMA waiting */
#define DMA_ENG_STATE_WAITED        0x00001000  /**< DMA waited earlier */
#define DMA_ENG_WAITED_ACK          0x00001000  /**< Acknowledge */
#define DMA_ENG_USER_RESET          0x00004000  /**< Reset only user logic */
#define DMA_ENG_RESET               0x00008000  /**< Reset DMA engine + user */

#define DMA_ENG_ALLINT_MASK         0x000000BE  /**< To get only int events */

	/*@}*/

	/** @name Bitmasks of performance registers.
	 * @{
	 */

#define REG_DMA_SAMPLE_CTR_MASK     0x00000003  /**< Sync counter for regs */

	/* Shift constants for performance registers */
#define REG_DMA_ENG_ACTIVE_TIME_SHIFT   2
#define REG_DMA_ENG_WAIT_TIME_SHIFT     2
#define REG_DMA_ENG_COMP_BYTES_SHIFT    2

	/*@}*/

	/**************************** Type Definitions *******************************/

	/***************** Macros (Inline Functions) Definitions *********************/

#ifdef TH_BH_ISR
#define INT_COAL_CNT        32   /* Org:16 Interrupt coalesce count */
#endif

	/* Basic DMA read/write functions - are 32-bit */

//#define Dma_mIn32  XIo_In32
//#define Dma_mOut32 XIo_Out32

	/*****************************************************************************/
	/**
	 *
	 * Read the given register.
	 *
	 * @param    BaseAddress is the base address of the device
	 * @param    RegOffset is the register offset to be read
	 *
	 * @return   The 32-bit value of the register
	 *
	 * @note
	 * C-style signature:
	 *    u32 Dma_mReadReg(u32 BaseAddress, u32 RegOffset)
	 *
	 ******************************************************************************/
#define Dma_mReadReg(BaseAddress, RegOffset)             \
	XIo_In32( ( (BaseAddress) + (RegOffset) ) )

	/*****************************************************************************/
	/**
	 *
	 * Write the given register.
	 *
	 * @param    BaseAddress is the base address of the device
	 * @param    RegOffset is the register offset to be written
	 * @param    Data is the 32-bit value to write to the register
	 *
	 * @return   None.
	 *
	 * @note
	 * C-style signature:
	 *    void Dma_mWriteReg(u32 BaseAddress, u32 RegOffset, u32 Data)
	 *
	 ******************************************************************************/
#define Dma_mWriteReg(BaseAddress, RegOffset, Data)     \
	XIo_Out32( ((BaseAddress) + (RegOffset)), (u64)(Data))

	/****************************************************************************/
	/**
	 * Enable global interrupt bits. This operation will read-modify-write 
	 * the REG_DMA_CTRL_STATUS register.
	 *
	 * @param  BaseAddress is the BAR0 address.
	 *
	 * @note
	 * C-style signature:
	 *    void Dma_mIntEnable(u32 BaseAddress)
	 *
	 *****************************************************************************/
#define Dma_mIntEnable(BaseAddress)        \
	{                           \
		u32 Reg = Dma_mReadReg(BaseAddress, REG_DMA_CTRL_STATUS);   \
		Reg |= (DMA_INT_ENABLE | DMA_USER_INT_ENABLE);            \
		Dma_mWriteReg(BaseAddress, REG_DMA_CTRL_STATUS, Reg);       \
	}


	/****************************************************************************/
	/**
	 * Clear global interrupt enable bits. This operation will read-modify-write 
	 * the REG_DMA_CTRL_STATUS register.
	 *
	 * @param  BaseAddress is the BAR0 address.
	 *
	 * @note
	 * C-style signature:
	 *    void Dma_mIntDisable(u32 BaseAddress)
	 *
	 *****************************************************************************/
#define Dma_mIntDisable(BaseAddress)        \
	{                           \
		u32 Reg = Dma_mReadReg(BaseAddress, REG_DMA_CTRL_STATUS);   \
		Reg &= ~(DMA_INT_ENABLE | DMA_USER_INT_ENABLE);           \
		Dma_mWriteReg(BaseAddress, REG_DMA_CTRL_STATUS, Reg);       \
	}


	/****************************************************************************/
	/**
	 * Acknowledge asserted global interrupts.
	 *
	 * @param  BaseAddress is the base address of the device
	 * @param  Mask has the interrupt signals to be acknowledged and is made 
	 *         by the caller OR'ing one or more of the INT_*_ACK bits.
	 *
	 * @note
	 * C-style signature:
	 *    u32 Dma_mIntAck(u32 BaseAddress, u32 Mask)
	 *
	 *****************************************************************************/
	/* Currently implemented like this. May have a performance hit. In
	 * that case, will re-implement to avoid the extra read. !!!!
	 */
#define Dma_mIntAck(BaseAddress, Mask)  \
	{                       \
		u32 Reg = Dma_mReadReg(BaseAddress, REG_DMA_CTRL_STATUS);   \
		Reg |= Mask;                    \
		Dma_mWriteReg(BaseAddress, REG_DMA_CTRL_STATUS, Reg);       \
	}


	/****************************************************************************/
	/**
	 * Retrieve the contents of the DMA engine control & status register 
	 * REG_DMA_ENG_CTRL_STATUS.
	 *
	 * @param  InstancePtr is the DMA engine instance to operate on.
	 *
	 * @return Current contents of the DMA engine control & status register.
	 *
	 * @note
	 * C-style signature:
	 *    u32 Dma_mGetCrSr(Dma_Engine * InstancePtr)
	 *
	 *****************************************************************************/
#define Dma_mGetCrSr(InstancePtr)                                   \
	Dma_mReadReg((InstancePtr)->RegBase, REG_DMA_ENG_CTRL_STATUS)   \


	/****************************************************************************/
	/**
	 * Set the contents of the DMA engine control & status register 
	 * REG_DMA_ENG_CTRL_STATUS. 
	 *
	 * @param  InstancePtr is the DMA engine instance to operate on.
	 * @param  Data is the data to write to the DMA engine control register.
	 *
	 * @note
	 * C-style signature:
	 *    u32 Dma_mSetCrSr(Dma_Engine* InstancePtr, u32 Data)
	 *
	 *****************************************************************************/
#define Dma_mSetCrSr(InstancePtr, Data)                                     \
	Dma_mWriteReg((InstancePtr)->RegBase, REG_DMA_ENG_CTRL_STATUS, (Data))


	/****************************************************************************/
	/**
	 * Set DMA enable bit for an instance. This operation will read-modify-write 
	 * the REG_DMA_ENG_CTRL_STATUS register.
	 *
	 * @param  BaseAddress is the base address of the device
	 *
	 * @note
	 * C-style signature:
	 *    void Dma_mEnable(u32 BaseAddress)
	 *
	 *****************************************************************************/
#define Dma_mEnable(BaseAddress)                                  \
	{                                                         \
		u32 val = Dma_mReadReg(BaseAddress, REG_DMA_ENG_CTRL_STATUS);       \
		val |= DMA_ENG_ENABLE;                                              \
		Dma_mWriteReg(BaseAddress, REG_DMA_ENG_CTRL_STATUS, val);           \
	}


	/****************************************************************************/
	/**
	 * Check if the current DMA engine is busy with a DMA operation.
	 *
	 * @param  InstancePtr is the engine instance to operate on.
	 *
	 * @return TRUE if the DMA is busy. FALSE otherwise.
	 *
	 * @note
	 * C-style signature:
	 *    XBoolean Dma_mRunning(Dma_Engine* InstancePtr)
	 *
	 *****************************************************************************/
#define Dma_mBdRingBusy(InstancePtr)                          \
	((Dma_mGetCrSr(InstancePtr) & DMA_ENG_RUNNING) ? TRUE : FALSE)


	/****************************************************************************/
	/**
	 * Set interrupt enable bits for an instance. This operation will 
	 * read-modify-write the REG_DMA_ENG_CTRL_STATUS register.
	 *
	 * @param  InstancePtr is the DMA engine instance to operate on.
	 *
	 * @note
	 * C-style signature:
	 *    void Dma_mEngIntEnable(Dma_Engine* InstancePtr)
	 *
	 *****************************************************************************/
#define Dma_mEngIntEnable(InstancePtr)                                  \
	{                                                         \
		/* Interrupts should not be enabled before DMA engine is ready */   \
		if((InstancePtr)->BdRing.RunState == XST_DMA_SG_IS_STARTED)         \
		{                                                   \
			u32 Reg = Dma_mGetCrSr(InstancePtr);                            \
			Reg |= DMA_ENG_INT_ENABLE;                                      \
			Dma_mSetCrSr(InstancePtr, Reg);                                 \
		}                                                     \
		else log_normal(KERN_NOTICE "DMA Engine not yet ready to enable interrupts\n"); \
	}


	/****************************************************************************/
	/**
	 * Clear interrupt enable bits for a channel. This operation will 
	 * read-modify-write the REG_DMA_ENG_CTRL_STATUS register.
	 *
	 * @param  InstancePtr is the DMA engine instance to operate on.
	 *
	 * @note
	 * C-style signature:
	 *    void Dma_mEngIntDisable(Dma_Engine* InstancePtr)
	 *
	 *****************************************************************************/
#define Dma_mEngIntDisable(InstancePtr)        \
	{                           \
		u32 Reg = Dma_mGetCrSr(InstancePtr);    \
		Reg &= ~(DMA_ENG_INT_ENABLE);       \
		Dma_mSetCrSr(InstancePtr, Reg);         \
	}


	/****************************************************************************/
	/**
	 * Acknowledge asserted engine interrupts.
	 *
	 * @param  InstancePtr is the DMA engine instance to operate on.
	 * @param  Mask has the interrupt signals to be acknowledged and is made 
	 *         by the caller OR'ing one or more of the INT_*_ACK bits.
	 *
	 * @note
	 * C-style signature:
	 *    u32 Dma_mEngIntAck(Dma_Engine* InstancePtr, u32 Mask)
	 *
	 *****************************************************************************/
	/* Currently implemented like this. May have a performance hit. In
	 * that case, will re-implement to avoid the extra read. !!!!
	 */
#define Dma_mEngIntAck(InstancePtr, Mask)   \
	{                           \
		u32 Reg = Dma_mGetCrSr(InstancePtr);    \
		Reg |= Mask;                        \
		Dma_mSetCrSr(InstancePtr, Reg);         \
	}


	/************************** Function Prototypes ******************************/

#ifdef __cplusplus
}
#endif

#endif /* end of protection macro */

