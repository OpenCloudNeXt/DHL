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
 **  This disclaimer is not a license and does not grant any rights to the materials 
 **  distributed herewith. Except as otherwise provided in a valid license issued to you 
 **  by Xilinx, and to the maximum extent permitted by applicable law: 
 **  (1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND WITH ALL FAULTS, 
 **  AND XILINX HEREBY DISCLAIMS ALL WARRANTIES AND CONDITIONS, EXPRESS, IMPLIED, OR STATUTORY, 
 **  INCLUDING BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, NON-INFRINGEMENT, OR 
 **  FITNESS FOR ANY PARTICULAR PURPOSE; and (2) Xilinx shall not be liable (whether in contract 
 **  or tort, including negligence, or under any other theory of liability) for any loss or damage 
 **  of any kind or nature related to, arising under or in connection with these materials, 
 **  including for any direct, or any indirect, special, incidental, or consequential loss 
 **  or damage (including loss of data, profits, goodwill, or any type of loss or damage suffered 
 **  as a result of any action brought by a third party) even if such damage or loss was 
 **  reasonably foreseeable or Xilinx had been advised of the possibility of the same.


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
 * @file xdma_bd.h
 *
 * This header provides operations to manage buffer descriptors (BDs) in 
 * support of Northwest Logic's scatter-gather DMA (see xdma.h).
 *
 * The API exported by this header defines abstracted macros that allow the
 * driver to read/write specific BD fields.
 *
 * <b>Buffer Descriptors</b>
 *
 * A buffer descriptor defines a DMA transaction. The macros defined by this 
 * header file allow access to most fields within a BD to tailor a DMA 
 * transaction according to application and hardware requirements. 
 *
 * The Dma_Bd structure defines a BD. The organization of this structure is
 * driven mainly by the hardware for use in scatter-gather DMA transfers.
 *
 * <b>Accessor Macros</b>
 *
 * Most of the BD attributes can be accessed through macro functions defined
 * here in this API. The BD fields should be accessed using Dma_mBdRead() 
 * and Dma_mBdWrite() macros. The USR fields are used to carry application
 * information. For example, they may implement checksum offloading fields 
 * for Ethernet devices. 
 *
 * <b>Alignment</b>
 *
 * In order to improve performance, BDs are required by the DMA engine to 
 * be 32-byte aligned, and are themselves 32 bytes in size, from the hardware
 * perspective. The software can have a larger data structure for BDs in
 * order to incorporate private information.
 *
 * MODIFICATION HISTORY:
 *
 * Ver   Date      Changes
 * ----- -------- -------------------------------------------------------
 * 1.0   5/15/12  First release
 *
 *****************************************************************************/

#ifndef XDMA_BD_H   /* prevent circular inclusions */
#define XDMA_BD_H   /* by using protection macros */

#ifdef __cplusplus
extern "C" {
#endif

	/***************************** Include Files *********************************/

#include "xio.h"

	/************************** Constant Definitions *****************************/

	/** @name Buffer Descriptor Alignment
	 *  @{
	 */
#define DMA_BD_MINIMUM_ALIGNMENT    0x40  /**< Minimum byte alignment
					    requirement for descriptors to
					    satisfy both hardware/software
					    needs */
	/* Other BD characteristics */
#define DMA_BD_SW_NUM_WORDS         16  /**< SW view of BD size in words */
#define DMA_BD_HW_NUM_WORDS         8   /**< HW view of BD size in words */

	/*@}*/

	/** @name Buffer Descriptor offsets
	 *  USR fields are defined by higher level IP. For example, checksum offload
	 *  setup for EMAC type devices. The 1st 8 words are utilized by hardware. Any
	 *  words after the 8th are for software use only.
	 *  @{
	 */
#define DMA_BD_BUFL_STATUS_OFFSET   0x00 /**< Buffer length + status */
#define DMA_BD_USRL_OFFSET          0x04 /**< User logic specific - LSBytes */
#define DMA_BD_USRH_OFFSET          0x08 /**< User logic specific - MSBytes */
#define DMA_BD_CARDA_OFFSET         0x0C /**< Card address */
#define DMA_BD_BUFL_CTRL_OFFSET     0x10 /**< Buffer length + control */
#define DMA_BD_BUFAL_OFFSET         0x14 /**< Buffer address LSBytes */
#define DMA_BD_BUFAH_OFFSET         0x18 /**< Buffer address MSBytes */
#define DMA_BD_NDESC_OFFSET         0x1C /**< Next descriptor pointer */

	/* Driver stores private information beyond the main BD as understood by
	 * the hardware, such as the virtual address of the associated data buffer, 
	 * which it requires for post-DMA processing. 
	 */
	//#define DMA_BD_VBUFAL_OFFSET        DMA_BD_CARDA_OFFSET
#define DMA_BD_VBUFAL_OFFSET        0x20 /**< Buffer virtual address LSBytes */
#define DMA_BD_VBUFAH_OFFSET        0x24 /**< Buffer virtual address MSBytes */
#ifdef X86_64
#define DMA_BD_PAGEADDRL_OFFSET	    0x28 /**<  Page data info */
#define DMA_BD_PAGEADDRH_OFFSET     0x32 
#else
#define DMA_BD_PAGEADDR			    0x28 /**<  Page data info */
#endif

	/* Bit masks for some BD fields */
#define DMA_BD_BUFL_MASK            0x000FFFFF /**< Byte count */
#define DMA_BD_STATUS_MASK          0xFF000000 /**< Status Flags */
#define DMA_BD_CTRL_MASK            0xFF000000 /**< Control Flags */

	/* Bit masks for BD control field */
#define DMA_BD_INT_ERROR_MASK       0x02000000 /**< Intr on error */
#define DMA_BD_INT_COMP_MASK        0x01000000 /**< Intr on BD completion */

	/* Bit masks for BD status field */
#define DMA_BD_SOP_MASK             0x80000000 /**< Start of packet */
#define DMA_BD_EOP_MASK             0x40000000 /**< End of packet */
#define DMA_BD_ERROR_MASK           0x10000000 /**< BD had error */
#define DMA_BD_USER_HIGH_ZERO_MASK  0x08000000 /**< User High Status zero */
#define DMA_BD_USER_LOW_ZERO_MASK   0x04000000 /**< User Low Status zero */
#define DMA_BD_SHORT_MASK           0x02000000 /**< BD not fully used */
#define DMA_BD_COMP_MASK            0x01000000 /**< BD completed */

	/*@}*/


	/**************************** Type Definitions *******************************/

	/**
	 * Dma_Bd is the type for buffer descriptors (BDs).
	 */
	typedef u32 Dma_Bd[DMA_BD_SW_NUM_WORDS];


	/***************** Macros (Inline Functions) Definitions *********************/

	/*****************************************************************************/
	/**
	 *
	 * Read the given Buffer Descriptor word.
	 *
	 * @param    BaseAddress is the base address of the BD to read
	 * @param    Offset is the word offset to be read
	 *
	 * @return   The 32-bit value of the field
	 *
	 * @note
	 * C-style signature:
	 *    u32 Dma_mBdRead(u32 BaseAddress, u32 Offset)
	 *
	 ******************************************************************************/
#define Dma_mBdRead(BaseAddress, Offset)        \
	XIo_In32( ((u64)(BaseAddress) + (unsigned int)(Offset)) )

	/*****************************************************************************/
	/**
	 *
	 * Write the given Buffer Descriptor word.
	 *
	 * @param    BaseAddress is the base address of the BD to write
	 * @param    Offset is the word offset to be written
	 * @param    Data is the 32-bit value to write to the field
	 *
	 * @return   None.
	 *
	 * @note
	 * C-style signature:
	 *    void Dma_mBdWrite(u32 BaseAddress, u32 RegOffset, u32 Data)
	 *
	 ******************************************************************************/
	 
#define Dma_mBdWrite(BaseAddress, Offset, Data)     \
	XIo_Out32( ((u64)BaseAddress+(unsigned int)Offset), (unsigned int)Data )

	/*****************************************************************************/
	/**
	 * Zero out all BD fields
	 *
	 * @param  BdPtr is the BD to operate on
	 *
	 * @return Nothing
	 *
	 * @note
	 * C-style signature:
	 *    void Dma_mBdClear(Dma_Bd* BdPtr)
	 *
	 *****************************************************************************/
#define Dma_mBdClear(BdPtr)                    \
	memset((BdPtr), 0, sizeof(Dma_Bd))


	/*****************************************************************************/
	/**
	 * Set the BD's Control field. This operation is implemented as a read-
	 * modify-write operation. 
	 *
	 * @param  BdPtr is the BD to operate on
	 * @param  Data is the value to write to the Control field. 
	 *
	 * @note
	 * C-style signature:
	 *    u32 Dma_mBdSetCtrl(Dma_Bd* BdPtr, u32 Data)
	 *
	 *****************************************************************************/
#define Dma_mBdSetCtrl(BdPtr, Data)                                 \
	Dma_mBdWrite((BdPtr), DMA_BD_BUFL_CTRL_OFFSET,                  \
			(Dma_mBdRead((BdPtr), DMA_BD_BUFL_CTRL_OFFSET) &            \
			 DMA_BD_BUFL_MASK) | ((Data) & DMA_BD_CTRL_MASK))


	/*****************************************************************************/
	/**
	 * Read the BD's Control field. 
	 *
	 * @param  BdPtr is the BD to operate on
	 *
	 * @return Word at offset DMA_BD_CTRL_OFFSET. 
	 *
	 * @note
	 * C-style signature:
	 *    u32 Dma_mBdGetCtrl(Dma_Bd* BdPtr)
	 *
	 *****************************************************************************/
#define Dma_mBdGetCtrl(BdPtr)                                       \
	(Dma_mBdRead((BdPtr), DMA_BD_BUFL_CTRL_OFFSET) & DMA_BD_CTRL_MASK)


	/*****************************************************************************/
	/**
	 * Set the BD's Status field. This operation is implemented as a read-
	 * modify-write operation. 
	 *
	 * @param  BdPtr is the BD to operate on
	 * @param  Data is the value to write to the Status field. 
	 *
	 * @note
	 * C-style signature:
	 *    u32 Dma_mBdSetStatus(Dma_Bd* BdPtr, u32 Data)
	 *
	 *****************************************************************************/
#define Dma_mBdSetStatus(BdPtr, Data)                                 \
	Dma_mBdWrite((BdPtr), DMA_BD_BUFL_STATUS_OFFSET,                  \
			(Dma_mBdRead((BdPtr), DMA_BD_BUFL_STATUS_OFFSET) &            \
			 DMA_BD_BUFL_MASK) | ((Data) & DMA_BD_STATUS_MASK))


	/*****************************************************************************/
	/**
	 * Read the BD's Status field. 
	 *
	 * @param  BdPtr is the BD to operate on
	 *
	 * @return Word at offset DMA_BD_STATUS_OFFSET. 
	 *
	 * @note
	 * C-style signature:
	 *    u32 Dma_mBdGetStatus(Dma_Bd* BdPtr)
	 *
	 *****************************************************************************/
#define Dma_mBdGetStatus(BdPtr)                                       \
	(Dma_mBdRead((BdPtr), DMA_BD_BUFL_STATUS_OFFSET) & DMA_BD_STATUS_MASK)


	/*****************************************************************************/
	/**
	 * Set the length field for the given BD. The length must be set each time 
	 * before a BD is submitted to hardware. The DMA engine requires the value
	 * to be updated both in the control and status fields - ONLY for TX.
	 *
	 * For TX channels, the value passed in should be the number of bytes to 
	 * transmit from the TX buffer associated with the given BD.
	 *
	 * For RX channels, the value passed in should be the size of the RX buffer 
	 * associated with the given BD in bytes. This is to notify the RX channel
	 * the capability of the RX buffer to avoid buffer overflow. 
	 *
	 * @param  BdPtr is the BD to operate on.
	 * @param  LenBytes is the number of bytes to transfer for TX channel or the
	 *         size of receive buffer in bytes for RX channel.
	 *
	 * @note
	 * C-style signature:
	 *    void Dma_mBdSetCtrlLength(Dma_Bd* BdPtr, u32 LenBytes)
	 *    void Dma_mBdSetStatLength(Dma_Bd* BdPtr, u32 LenBytes)
	 *
	 *****************************************************************************/
#define Dma_mBdSetCtrlLength(BdPtr, LenBytes)                       \
	{                                                                   \
		Dma_mBdWrite((BdPtr), DMA_BD_BUFL_CTRL_OFFSET,                  \
				((Dma_mBdRead((BdPtr), DMA_BD_BUFL_CTRL_OFFSET) &           \
				  DMA_BD_CTRL_MASK) | (LenBytes & DMA_BD_BUFL_MASK)));       \
	}

#define Dma_mBdSetStatLength(BdPtr, LenBytes)                       \
	{                                                                   \
		Dma_mBdWrite((BdPtr), DMA_BD_BUFL_STATUS_OFFSET,                \
				((Dma_mBdRead((BdPtr), DMA_BD_BUFL_STATUS_OFFSET) &         \
				  DMA_BD_STATUS_MASK) | (LenBytes & DMA_BD_BUFL_MASK)));     \
	}


	/*****************************************************************************/
	/**
	 * Retrieve the length field value from the given BD.  The returned value is
	 * read from the length field updated by the DMA engine. 
	 *
	 * For both TX and RX, the value is read from DMA_BD_BUFL_STATUS_OFFSET. 
	 * The actual received data could be equal to or smaller than the length
	 * of the original buffer. This field is updated by the DMA engine - it 
	 * indicates number of transmitted bytes in the case of S2C and number of 
	 * received bytes in the case of C2S.
	 *
	 * @param  BdPtr is the BD to operate on.
	 *
	 * @return The BD length field value set by the DMA engine.
	 *
	 * @note
	 * C-style signature:
	 *    u32 Dma_mBdGetStatLength(Dma_Bd* BdPtr)
	 *
	 *****************************************************************************/
#define Dma_mBdGetStatLength(BdPtr)                      \
	(Dma_mBdRead((BdPtr), DMA_BD_BUFL_STATUS_OFFSET) & DMA_BD_BUFL_MASK)


	/*****************************************************************************/
	/**
	 * Retrieve the length field value from the given BD.  The returned value is
	 * read from the length field provided to the DMA engine. 
	 *
	 * This value is read from DMA_BD_BUFL_CTRL_OFFSET. This macro is called
	 * when the driver wants information on the packet yet to be transmitted.
	 *
	 * @param  BdPtr is the BD to operate on.
	 *
	 * @return The BD length field value set by the DMA engine.
	 *
	 * @note
	 * C-style signature:
	 *    u32 Dma_mBdGetCtrlLength(Dma_Bd* BdPtr)
	 *
	 *****************************************************************************/
#define Dma_mBdGetCtrlLength(BdPtr)                      \
	(Dma_mBdRead((BdPtr), DMA_BD_BUFL_CTRL_OFFSET) & DMA_BD_BUFL_MASK)


	/*****************************************************************************/
	/**
	 * Set the ID field of the given BD. The ID is an arbitrary piece of data the
	 * application can associate with a specific BD. For example, the virtual
	 * address of a packet buffer can be stored in the BD, to be used during
	 * post-DMA processing.
	 *
	 * @param  BdPtr is the BD to operate on
	 * @param  Id is a 32 bit quantity to set in the BD
	 *
	 * @note
	 * C-style signature:
	 *    void Dma_mBdSetId(Dma_Bd* BdPtr, void Id)
	 *
	 *****************************************************************************/
#define Dma_mBdSetId(BdPtr, Id)                                      \
{                                                                    \
                u32 val;                                                 \
		val = (u32)(Id);                               \
		Dma_mBdWrite((BdPtr), DMA_BD_VBUFAL_OFFSET, val);    \
                val = (u32)((((u64)(Id))>>32));             \
		Dma_mBdWrite((BdPtr), DMA_BD_VBUFAH_OFFSET, val);         \
}

#define Dma_mBdSetId_NULL(BdPtr,Id)                                      \
{                                                                    \
		Dma_mBdWrite((BdPtr), DMA_BD_VBUFAL_OFFSET,Id);    \
		Dma_mBdWrite((BdPtr), DMA_BD_VBUFAH_OFFSET,Id);         \
}


#define Dma_mBdSetPageAddr(BdPtr, Id)                                      \
{                                                                    \
                u32 val;                                                 \
		val = (u32)(Id);                               \
		Dma_mBdWrite((BdPtr), DMA_BD_PAGEADDRL_OFFSET, val);    \
                val = (u32)((((u64)(Id))>>32));             \
		Dma_mBdWrite((BdPtr), DMA_BD_PAGEADDRH_OFFSET, val);         \
}


	/*****************************************************************************/
	/**
	 * Retrieve the ID field of the given BD previously set with Dma_mBdSetId.
	 *
	 * @param  BdPtr is the BD to operate on
	 *
	 * @note
	 * C-style signature:
	 *    u32 Dma_mBdGetId(Dma_Bd* BdPtr)
	 *
	 *****************************************************************************/
	static inline u64 Dma_mBdGetId(Dma_Bd * BdPtr)
	{
		u64 val;
		u32 val1, val2;
		val1 = (Dma_mBdRead((BdPtr),DMA_BD_VBUFAH_OFFSET));
		val2 = (Dma_mBdRead((BdPtr),DMA_BD_VBUFAL_OFFSET));
		val = val1;
		val <<= 32;
		val |= val2;
		return val;
	}

	static inline u64 Dma_mBdGetPageAddr(Dma_Bd * BdPtr)
	{
		u64 val;
		u32 val1, val2;
		val1 = (Dma_mBdRead((BdPtr),DMA_BD_PAGEADDRH_OFFSET));
		val2 = (Dma_mBdRead((BdPtr),DMA_BD_PAGEADDRL_OFFSET));
		val = val1;
		val <<= 32;
		val |= val2;
		return val;
	}


	/*****************************************************************************/
	/**
	 * Set the BD's buffer address. The driver currently supports only 
	 * 32-bit addresses.
	 *
	 * @param  BdPtr is the BD to operate on
	 * @param  Addr is the address to set
	 *
	 * @note
	 * C-style signature:
	 *    void Dma_mBdSetBufAddr(Dma_Bd* BdPtr, u32 Addr)
	 *
	 *****************************************************************************/
#define Dma_mBdSetBufAddr(BdPtr, Addr)                          \
	{                                                               \
                u32 val;                                                 \
		val = (u32)(Addr);                               \
		Dma_mBdWrite((BdPtr), DMA_BD_BUFAL_OFFSET, val);    \
                val = (u32)((((u64)(Addr))>>32));             \
		Dma_mBdWrite((BdPtr), DMA_BD_BUFAH_OFFSET, val);         \
	}

	/*****************************************************************************/
	/**
	 * Get the BD's buffer address
	 *
	 * @param  BdPtr is the BD to operate on
	 *
	 * @note
	 * C-style signature:
	 *    u32 Dma_mBdGetBufAddr(Dma_Bd* BdPtr)
	 *
	 *****************************************************************************/
	static inline u64 Dma_mBdGetBufAddr(Dma_Bd * BdPtr)
	{
		u64 val;
		u32 val1, val2;
		val1 = (Dma_mBdRead((BdPtr),DMA_BD_BUFAH_OFFSET));
		val2 = (Dma_mBdRead((BdPtr),DMA_BD_BUFAL_OFFSET));
		val = val1;
		val <<= 32;
		val |= val2;
		return val;
	}


	/*****************************************************************************/
	/**
	 * Set the BD's user data. 
	 *
	 * @param  BdPtr is the BD to operate on
	 * @param  User is the value to set
	 *
	 * @note
	 * C-style signature:
	 *    void Dma_mBdSetUserData(Dma_Bd* BdPtr, unsigned long long User)
	 *
	 *****************************************************************************/
#define Dma_mBdSetUserData(BdPtr, User)                             \
	{                                                                   \
		u32 val;                                                        \
		val = (u32)(User & 0xFFFFFFFFLL);                               \
		Dma_mBdWrite((BdPtr), DMA_BD_USRL_OFFSET, val);                 \
		val = (u32)((User>>32) & 0xFFFFFFFFLL);                         \
		Dma_mBdWrite((BdPtr), DMA_BD_USRH_OFFSET, val);                 \
	}


	/*****************************************************************************/
	/**
	 * Get the BD's user data.
	 *
	 * @param  BdPtr is the BD to operate on
	 *
	 * @note
	 * C-style signature:
	 *    u32 Dma_mBdGetUserData(Dma_Bd* BdPtr)
	 *
	 *****************************************************************************/
	static inline unsigned long long Dma_mBdGetUserData(Dma_Bd * BdPtr)
	{
		unsigned long long val;
		u32 val1, val2;
		val1 = (Dma_mBdRead((BdPtr), DMA_BD_USRH_OFFSET));
		val2 = (Dma_mBdRead((BdPtr), DMA_BD_USRL_OFFSET));
		val = val1;
		val <<= 32;
		val |= val2;
		return val;
	}


	/*****************************************************************************/
	/**
	 * Compute the virtual address of a descriptor from its physical address
	 *
	 * @param BdPtr is the physical address of the BD
	 *
	 * @returns Virtual address of BdPtr
	 *
	 * @note Assume BdPtr is always a valid BD in the ring
	 * @note RingPtr is an implicit parameter
	 *****************************************************************************/
#define Dma_mPhysToVirt(BdPtr) \
	((BdPtr) + (RingPtr->FirstBdAddr - RingPtr->FirstBdPhysAddr))

	/*****************************************************************************/
	/**
	 * Compute the physical address of a descriptor from its virtual address
	 *
	 * @param BdPtr is the virtual address of the BD
	 *
	 * @returns Physical address of BdPtr
	 *
	 * @note Assume BdPtr is always a valid BD in the ring
	 * @note RingPtr is an implicit parameter
	 *****************************************************************************/
#define Dma_mVirtToPhys(BdPtr) \
	((Dma_Bd *)(BdPtr) - ((Dma_Bd *)RingPtr->FirstBdAddr - (Dma_Bd *)RingPtr->FirstBdPhysAddr))

	/************************** Function Prototypes ******************************/

#ifdef __cplusplus
}
#endif

#endif /* end of protection macro */

