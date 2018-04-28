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
 * @file xio.h
 *
 * This file contains the Input/Output functions, and the changes
 * required for swapping endianness. 
 *
 * MODIFICATION HISTORY:
 *
 * Ver   Date     Changes
 * ----- -------- -------------------------------------------------------
 * 1.0   5/15/12 First release
 *
 ******************************************************************************/

#ifndef XIO_H           /* prevent circular inclusions */
#define XIO_H           /* by using protection macros */

#ifdef __cplusplus
extern "C" {
#endif

	/***************************** Include Files *********************************/

#include "xbasic_types.h"
// include u8 u16 u 32
#include "../vc709_type.h"


	/************************** Constant Definitions *****************************/


	/**************************** Type Definitions *******************************/

	/**
	 * Typedef for an I/O address.  Typically correlates to the width of the
	 * address bus.
	 */
	typedef Xuint32 XIo_Address;


	/* The following macros allow the software to be transportable across
	 * processors which use big or little endian memory models.
	 *
	 * Defined first are processor-specific endian conversion macros specific to
	 * the GNU compiler and the x86 family, as well as a no-op endian conversion
	 * macro. These macros are not to be used directly by software. Instead, the
	 * XIo_To/FromLittleEndianXX and XIo_To/FromBigEndianXX macros below are to be
	 * used to allow the endian conversion to only be performed when necessary.
	 */

#define XIo_EndianNoop(Source, DestPtr)    (*DestPtr = Source)



#define XIo_EndianSwap16(Source, DestPtr) \
	{\
		Xuint16 src = (Source); \
		Xuint16 *destptr = (DestPtr); \
		*destptr = src >> 8; \
		*destptr |= (src << 8); \
	}

#define XIo_EndianSwap32(Source, DestPtr) \
	{\
		unsigned int src = (Source); \
		unsigned int *destptr = (DestPtr); \
		*destptr = src >> 24; \
		*destptr |= ((src >> 8)  & 0x0000FF00); \
		*destptr |= ((src << 8)  & 0x00FF0000); \
		*destptr |= ((src << 24) & 0xFF000000); \
	}


#ifdef XLITTLE_ENDIAN
	/* little-endian processor */

#define XIo_ToLittleEndian16                XIo_EndianNoop
#define XIo_ToLittleEndian32                XIo_EndianNoop
#define XIo_FromLittleEndian16              XIo_EndianNoop
#define XIo_FromLittleEndian32              XIo_EndianNoop

#define XIo_ToBigEndian16(Source, DestPtr)  XIo_EndianSwap16(Source, DestPtr)
#define XIo_ToBigEndian32(Source, DestPtr)  XIo_EndianSwap32(Source, DestPtr);
#define XIo_FromBigEndian16                 XIo_ToBigEndian16
#define XIo_FromBigEndian32(Source, DestPtr) XIo_ToBigEndian32(Source, DestPtr);

#else
	/* big-endian processor */

#define XIo_ToLittleEndian16(Source, DestPtr) XIo_EndianSwap16(Source, DestPtr)
#define XIo_ToLittleEndian32(Source, DestPtr) XIo_EndianSwap32(Source, DestPtr)
#define XIo_FromLittleEndian16                XIo_ToLittleEndian16
#define XIo_FromLittleEndian32                XIo_ToLittleEndian32

#define XIo_ToBigEndian16                     XIo_EndianNoop
#define XIo_ToBigEndian32                     XIo_EndianNoop
#define XIo_FromBigEndian16                   XIo_EndianNoop
#define XIo_FromBigEndian32                   XIo_EndianNoop

#endif



	/* NWL DMA design is little-endian, so values need not be swapped.
	*/
#define XIo_In32(addr)      rte_read32((void *)(addr))
#define XIo_Out32(addr, data)  rte_write32((rte_cpu_to_le_32(data)), (void  *)(addr))



#define Xil_In32    XIo_In32
#define Xil_Out32   XIo_Out32




	/* The following functions handle IO addresses where data must be swapped
	 * They cannot be implemented as macros
	 */
	Xuint16 XIo_InSwap16(XIo_Address InAddress);
	Xuint32 XIo_InSwap32(XIo_Address InAddress);
	void XIo_OutSwap16(XIo_Address OutAddress, Xuint16 Value);
	void XIo_OutSwap32(XIo_Address OutAddress, Xuint32 Value);

#ifdef __cplusplus
}
#endif

#endif          /* end of protection macro */
