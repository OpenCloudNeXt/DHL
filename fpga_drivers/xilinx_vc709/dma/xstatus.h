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
 **              distributed herewith. Except as otherwise provided in a valid license issued to you 
 **              by Xilinx, and to the maximum extent permitted by applicable law: 
 **              (1) THESE MATERIALS ARE MADE AVAILABLE "AS IS" AND WITH ALL FAULTS, 
 **              AND XILINX HEREBY DISCLAIMS ALL WARRANTIES AND CONDITIONS, EXPRESS, IMPLIED, OR STATUTORY, 
 **              INCLUDING BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, NON-INFRINGEMENT, OR 
 **              FITNESS FOR ANY PARTICULAR PURPOSE; and (2) Xilinx shall not be liable (whether in contract 
 **              or tort, including negligence, or under any other theory of liability) for any loss or damage 
 **              of any kind or nature related to, arising under or in connection with these materials, 
 **              including for any direct, or any indirect, special, incidental, or consequential loss 
 **              or damage (including loss of data, profits, goodwill, or any type of loss or damage suffered 
 **              as a result of any action brought by a third party) even if such damage or loss was 
 **              reasonably foreseeable or Xilinx had been advised of the possibility of the same.


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
 * @file xstatus.h
 *
 * This file contains Xilinx software status codes.  Status codes have their
 * own data type called int.  These codes are used throughout the Xilinx
 * device drivers.
 *
 * MODIFICATION HISTORY:
 *
 * Ver   Date     Changes
 * ----- -------- -------------------------------------------------------
 * 1.0   5/15/12 First release
 *
 *
 ******************************************************************************/

#ifndef XSTATUS_H   /* prevent circular inclusions */
#define XSTATUS_H   /* by using protection macros */

#ifdef __cplusplus
extern "C" {
#endif

	/***************************** Include Files *********************************/


	/************************** Constant Definitions *****************************/

	/*********************** Common status 0 - 500 *****************************/

#define XST_SUCCESS                     0L
#define XST_FAILURE                     1L
#define XST_DEVICE_IS_STARTED           5L
#define XST_DEVICE_IS_STOPPED           6L
#define XST_INVALID_PARAM               15L /* an invalid parameter was passed into the function */
#define XST_IS_STARTED                  23L /* used when part of device is already started i.e. sub channel */
#define XST_IS_STOPPED                  24L /* used when part of device is already stopped i.e. sub channel */

	/************************** DMA status 511 - 530 ***************************/

#define XST_DMA_SG_LIST_EMPTY           513L  /* scatter gather list contains no buffer descriptors ready to be processed */
#define XST_DMA_SG_IS_STARTED           514L  /* scatter gather not stopped */
#define XST_DMA_SG_IS_STOPPED           515L  /* scatter gather not running */
#define XST_DMA_SG_LIST_FULL            517L  /* all the buffer desciptors of the scatter gather list are being used */
#define XST_DMA_SG_NO_LIST              523L  /* no scatter gather list has been created */
#define XST_DMA_SG_LIST_ERROR           526L  /* general purpose list access error */

	/***************** Macros (Inline Functions) Definitions *********************/


	/************************** Function Prototypes ******************************/

#ifdef __cplusplus
}
#endif

#endif /* end of protection macro */
