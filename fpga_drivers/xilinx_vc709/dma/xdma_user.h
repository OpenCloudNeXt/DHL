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
 * @file xdma_user.h
 *
 * This file contains the data required for the interface between the 
 * base DMA driver (xdma) and the application-specific drivers, for example,
 *
 * This interface has been architected in order to make it possible to 
 * easily substitute the application-specific drivers that come with this
 * TRD with other application-specific drivers, without re-writing the
 * common DMA-handling functionality.
 *
 * Some xdma functions are called directly by the application-specific driver
 * and some are callbacks registered by the application-specific driver, 
 * and are called by xdma.
 *
 * <pre>
 * Application-Specific Driver                               xdma Driver
 *                               Driver Registration
 *                 ---------------------------------------------->
 *                             Driver De-registration
 *                 ---------------------------------------------->
 *                           Transmit Block/Packet Data 
 *                 ---------------------------------------------->
 *                            Complete Initialization
 *                <----------------------------------------------
 *                            Complete Driver Removal
 *                <----------------------------------------------
 *                            Complete Interrupt Tasks
 *                <----------------------------------------------
 *                             Get Packet Buffer for RX
 *                <----------------------------------------------
 *                               Return Packet Buffer 
 *                <----------------------------------------------
 *                                    Set State
 *                <----------------------------------------------
 *                                    Get State
 *                <----------------------------------------------
 * </pre>
 * <b> Driver Registration/De-registration </b>
 *
 * The application-specific drivers are dependent on the DMA driver and 
 * can be inserted as Linux kernel modules only after xdma has been loaded.
 *
 * To register itself with xdma, the application-specific driver does the
 * following, while specifying the engine number and BAR number it requires,
 * a set of function callback pointers, and the minimum packet size it
 * normally uses -
 * <pre> Handle = DmaRegister(int Engine, int Bar, UserPtrs * uptr, int PktSize); </pre>
 * The application-specific driver requires to know the kernel logical
 * address of the desired BAR in order to do any device-specific 
 * initializations that may be required. For example, the xgbeth driver
 * requires to program the TEMAC and PHY registers, while the xaui driver
 * requires to control the test configuration.
 *
 * Before returning to the caller, DmaRegister() will invoke the function 
 * callback registered to complete the initialization process, while 
 * specifying the BAR's logical address, and a private data pointer that 
 * was passed to it during registration. privData can be used by the user
 * driver to differentiate between multiple DmaRegister() invocations -
 * <pre> (uptr->UserInit)(BARbase, privData); </pre>
 * <b><i> Note: The application-specific driver's UserInit() function call must
 * be written in such a way that it works fine even when the DmaRegister()
 * call is not yet complete. </i></b>
 *
 * If the registration process is successful, a handle is returned which
 * should be used in all other function calls to xdma.
 *
 * To unregister itself from xdma, the application-specific driver does the
 * following, while passing the handle it received after registration -
 * <pre> DmaUnregister(Handle); </pre>
* Before returning to the caller, DmaUnregister() will invoke the function
* callback registered to enable any device-specific programming to be done
* as part of the de-registration process.
* <pre> (uptr->UserRemove)(Handle, privData); </pre>
* Before returning to the caller, DmaUnregister() also returns all the
* packet buffers that had been passed to it for transmission or reception.
* Incase these buffers are still unused, they are flagged as PKT_UNUSED.
* <pre> (uptr->UserPutPkt)(Handle, PktBuf * pkts, int NumPkts, privData); </pre>
* <b><i> Note: UserRemove() is not being invoked in xdma v1.00, and will be
* added in the future. </i></b>
*
* <b> Buffer Handling </b>
*
* For better performance, the DMA driver always sets up large buffer
* descriptor (BD) rings, one for transmission and one for reception. The
* TX BD ring will be consumed only when there is data for transmission.
* The RX BD ring, on the other hand, will be entirely submitted for DMA
* in order to maximize performance. 
* 
* <b> Data Transmission </b>
*
* When the application-specific driver wants to transmit a data block or
* packet, it invokes the following, while passing an array of one or more
* packets to be transmitted -
* <pre> DmaSendPkt(Handle, PktBuf * pkts, int NumPkts); </pre>
* When data transmission is completed, the packet buffers will be returned
* to the application-specific driver by invoking the following -
* <pre> (uptr->UserPutPkt)(Handle, PktBuf * pkts, int NumPkts, privData); </pre>
* It is important to return these buffers to the application-specific
* driver even though they are TX packets, since they would need to be
* freed or re-used as the case may be.
*
* <b> Data Reception </b>
*
* The DMA driver needs to set up the complete BD ring with buffers ready
* for reception. Hence, it invokes the following, while specifying the number
* of packet buffers it wants, and their size -
* <pre> (uptr->UserGetPkt)(Handle, PktBuf * pkts, uint Size, int NumPkts, privData); </pre>
* As data reception happens and buffers get used, they are returned to the
* application-specific driver, by invoking the following -
* <pre> (uptr->UserPutPkt)(Handle, PktBuf * pkts, int NumPkts, privData); </pre>
* The DMA driver will again invoke UserGetPkt() in order to replenish the
* used BDs in its BD ring.
*
* <b> Interrupts </b>
*
* The DMA driver can operate in either a polled mode or interrupt-driven
* mode, by modifying a compile-time flag. The application-specific driver
* can optionally enable application-specific interrupts. When an interrupt
* occurs, the DMA driver will invoke the following, to enable handling of
* these interrupt events -
* <pre> (uptr->UserIntr)(Handle, privData); </pre>
* <b><i> Note: This callback is not being invoked by xdma v1.00, and will be
* added in the future. </i></b>
* 
* <b> Configuration, Status and Statistics </b>
*
* The Xilinx Performance Monitor GUI (xpmon) can be used to initiate a
* data transfer test and measure DMA payload and PCIe link performance. 
* xdma invokes the following, while specifying whether loopback is enabled
* or not, and the minimum/maximum bounds of packet sizes -
* <pre> (uptr->UserSetState)(Handle, UserState * ustate, privData); </pre>
* The same callback is used to start a test, and to stop a test. 
* 
* xdma invokes the following, in order to get current state of the 
* application-specific driver -
* <pre> (uptr->UserGetState)(Handle, UserState * ustate, privData); </pre>
* Information regarding current test state, link state, total number of
* buffers is returned and can be displayed in the xpmon GUI.
*
* MODIFICATION HISTORY:
*
* Ver   Date     Changes
* ----- -------- -------------------------------------------------------
* 1.0   5/15/12 First release
*
******************************************************************************/

#ifndef XDMA_USER_H   /* prevent circular inclusions */
#define XDMA_USER_H   /* by using protection macros */

#ifdef __cplusplus
extern "C" {
#endif

	/***************************** Include Files *********************************/

#include "../../../fpga_drivers/xilinx_vc709/dma/xpmon_be.h"
#include <linux/ethtool.h>


	/************************** Constant Definitions *****************************/

	/** @name Driver and engine states.
	 *  @{
	 */
#define UNINITIALIZED       0           /**< State at system start */
#define INITIALIZED         1           /**< After probe */
#define USER_ASSIGNED       2           /**< Engine assigned to user */
#define UNREGISTERING       3           /**< In the process of unregistering */
#ifdef PM_SUPPORT
#define PM_PREPARE          4       /**< Halt registration. PowerManagement initiated */
#endif
	/*@}*/

	/** @name Packet information set/read by the user drivers.
	 *  These flags match with the status reported by DMA. Additional flags 
	 *  should be assigned from available bits.
	 *  @{
	 */
#define PKT_SOP             0x80000000  /**< Buffer is the start-of-packet */
#define PKT_EOP             0x40000000  /**< Buffer is the end-of-packet */
#define PKT_ERROR           0x10000000  /**< Error while processing buffer */
#define PKT_UNUSED          0x00004000  /**< Buffer is being returned unused */
#define PKT_ALL             0x00800000  /**< All fragments must be sent */
	/*@}*/

#define STATUS_REG_OFFSET 0x9008


#define STABILITY_WAIT_TIME   5   // 5 miliseconds
#define FIFO_EMPTY_TIMEOUT    100   // 100 milisecons

#define DIR_TYPE_S2C    0   // maybe redundant
#define DIR_TYPE_C2S    1   // may be redundant

#define EMPTY_MASK_SHIFT  2
#define AXI_MIG_RST_SHIFT 1


#define PERFORMANCE_MODE      0
#define ETHERNET_APPMODE      1
#define RAWETHERNET_MODE      2

	/**************************** Type Definitions *******************************/

	/** Packet information passed between DMA and application-specific 
	 *  drivers while transmitting and receiving data.
	 *  A PktBuf array can be used to pass multiple packets between user
	 *  and DMA drivers. It includes the following information -
	 *  - pktBuf is the virtual address of a packet buffer
	 *  - bufInfo is the per-packet identifier that the user driver may need to
	 *  associate with the packet. When the packet is returned from the
	 *  DMA driver to the user driver, this association remains intact and can
	 *  be retrieved by the user driver.
	 *  - The size of the packet buffer.
	 *  - When the user submits a packet for DMA, the flags can be a combination
	 *    of PKT_SOP, PKT_EOP and PKT_ALL. PKT_ALL indicates to the DMA driver
	 *    that all of the packets in the submitted array must be queued for DMA.
	 *    This will usually be done when the queued packets are fragments of a 
	 *    larger user packet.
	 *  - When the DMA driver returns a packet to the user driver, the flags can
	 *    be a combination of PKT_SOP, PKT_EOP, PKT_ERROR and PKT_UNUSED. 
	 *    PKT_UNUSED indicates that the packet buffer is being returned unused
	 *    and does not contain valid data. This is usually done when the drivers
	 *    are being unloaded and unregistered, and therefore, packets have been
	 *    retrieved without completing DMA.
	 */
	typedef struct {
		uint64_t pktPA;				// physical dma address

		unsigned char * pktBuf;     /**< Virtual Address of packet buffer */
		unsigned char * bufInfo;    /**< Per-packet identifier */
		unsigned int size;          /**< Size of packet buffer */
		unsigned int flags;         /**< Flags associated with packet */
		unsigned long long userInfo;/**< User info associated with packet */ 

		unsigned char * pageAddr;   /**< User page address associated with buffer */
		unsigned int pageOffset;    /**< User Page offset associated with page address */
	} PktBuf;

	/** User State Information passed between DMA and application-specific 
	 *  drivers while changing configuration, and reading state/statistics.
	 *  - LinkState can be LINK_UP or LINK_DOWN
	 *  - When DMA driver sets the test state in the user driver, TestMode can 
	 *    be a combination of TEST_STOP, TEST_START and ENABLE_LOOPBACK.
	 *  - When DMA driver gets the test state from the user driver, TestMode can
	 *    be a combination of TEST_IN_PROGRESS and ENABLE_LOOPBACK.
	 */
	typedef struct {
		int LinkState;              /**< Link State */
		int DataMismatch;           /**< Data Mismatch ocurrence error */
		int Buffers;                /**< Count of buffers */
		int MinPktSize;             /**< Min Packet Size */
		int MaxPktSize;             /**< Max Packet Size */
		unsigned int TestMode;      /**< Test Mode */
	} UserState;

	/** User instance function callbacks. Not all callbacks need to be 
	 *  implemented. 
	 */
#ifdef X86_64

	typedef struct {
		u64 privData;      /**< User-specified private data */
		u64 versionReg;    /**< User-specific version info register */
		unsigned int mode;          /* Performance mode or Ethernet App mode */
		int (* UserInit)(u64 barbase, u64);
		/**< User instance register completion callback */
		int (* UserRemove)(void * handle, u64 privdata);
		/**< User instance de-register completion callback - not
		 * available in v1.00
		 */
		int (* UserIntr)(void * handle, u64 privdata);
		/**< User instance interrupt callback - not available in v1.00 */
		int (* UserGetPkt)(void * handle, PktBuf * vaddr, unsigned int size, int numpkts, u64 privdata);
		/**< User instance callback - get buffers for RX */
		int (* UserPutPkt)(void * handle, PktBuf * vaddr, int numpkts, u64 privdata);
		/**< User instance callback - put RX/TX buffers */
		int (* UserSetState)(void * handle, UserState * ustate, u64 privdata);
		/**< User instance callback - set state */
		int (* UserGetState)(void * handle, UserState * ustate, u64 privdata);
		/**< User instance callback - get state */
#ifdef PM_SUPPORT
		/* PM support */
		int (* UserSuspend_Early)(void * handle, UserState * ustate, u64 privdata);
		/**< User instance callback - suspend send/receive */
		int (* UserSuspend_Late)(void * handle, UserState * ustate, u64 privdata);
		/**< User instance callback - suspend send/receive */
		int (* UserResume)(void * handle, UserState * ustate, u64 privdata);
		/**< User instance callback - resume send/receive */
#endif

	} UserPtrs;
#else	 
	typedef struct {
		unsigned int privData;      /**< User-specified private data */
		unsigned int versionReg;    /**< User-specific version info register */
		unsigned int mode;          /* Performance mode or Ethernet App mode */
		int (* UserInit)(unsigned int barbase, unsigned int privdata);
		/**< User instance register completion callback */
		int (* UserRemove)(void * handle, unsigned int privdata);
		/**< User instance de-register completion callback - not
		 * available in v1.00
		 */
		int (* UserIntr)(void * handle, unsigned int privdata);
		/**< User instance interrupt callback - not available in v1.00 */
		int (* UserGetPkt)(void * handle, PktBuf * vaddr, unsigned int size, int numpkts, unsigned int privdata);
		/**< User instance callback - get buffers for RX */
		int (* UserPutPkt)(void * handle, PktBuf * vaddr, int numpkts, unsigned int privdata);
		/**< User instance callback - put RX/TX buffers */
		int (* UserSetState)(void * handle, UserState * ustate, unsigned int privdata);
		/**< User instance callback - set state */
		int (* UserGetState)(void * handle, UserState * ustate, unsigned int privdata);
		/**< User instance callback - get state */
#ifdef PM_SUPPORT
		/* PM support */
		int (* UserSuspend_Early)(void * handle, UserState * ustate, unsigned int privdata);
		/**< User instance callback - suspend send/receive */
		int (* UserSuspend_Late)(void * handle, UserState * ustate, unsigned int privdata);
		/**< User instance callback - suspend send/receive */
		int (* UserResume)(void * handle, UserState * ustate, unsigned int privdata);
		/**< User instance callback - resume send/receive */
#endif

	} UserPtrs;
#endif
	/***************** Macros (Inline Functions) Definitions *********************/


	/************************** Function Prototypes ******************************/

	/** @name Initialization and control functions in xdma.c
	 *  @{
	 */
	void * DmaRegister(int engine, int bar, UserPtrs * uptr, int pktsize);
	int DmaUnregister(void * handle);
#ifdef FIFO_EMPTY_CHECK
	void DmaFifoEmptyWait(int handleId, u32 type);
#endif
	void * DmaBaseAddress(int bar);
	int DmaMac_WriteReg(int offset, int data);
	int DmaMac_ReadReg(int offset);

	int DmaSendPages_Tx(void * handle, PktBuf ** pkts, int numpkts);
	int DmaSendPages(void * handle, PktBuf ** pkts, int numpkts);
	int DmaSendPkt(void * handle, PktBuf * pkts, int numpkts);
	void Dma_get_ringparam(void *handle, struct ethtool_ringparam *ering);
	/*@}*/

#ifdef __cplusplus
}
#endif

#endif /* end of protection macro */
