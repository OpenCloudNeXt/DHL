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
* @file xbasic_types.h
*
* This file contains basic types for Xilinx software IP.  These types do not
* follow the standard naming convention with respect to using the component
* name in front of each name because they are considered to be primitives.
*
* @note
*
* This file contains items which are architecture dependent.
*
* MODIFICATION HISTORY:
*
* Ver    Date   Changes
* ---- -------- -------------------------------------------------------
* 1.0   5/15/12 First release
*
******************************************************************************/

#ifndef XBASIC_TYPES_H  /* prevent circular inclusions */
#define XBASIC_TYPES_H  /* by using protection macros */

#ifdef __cplusplus
extern "C" {
#endif


/***************************** Include Files *********************************/


/************************** Constant Definitions *****************************/

#ifndef TRUE
#  define TRUE    1
#endif

#ifndef FALSE
#  define FALSE   0
#endif

#ifndef NULL
#define NULL    0
#endif

#define XNULL   NULL    /**< Legacy return type. Deprecated. */
#define XTRUE   TRUE    /**< Legacy return type. Deprecated. */
#define XFALSE    FALSE   /**< Legacy return type. Deprecated. */

#define XCOMPONENT_IS_READY     0x11111111  /**< Component has been initialized */
#define XCOMPONENT_IS_STARTED   0x22222222  /**< Component has been started */

#define XIL_COMPONENT_IS_READY      XCOMPONENT_IS_READY
#define XIL_COMPONENT_IS_STARTED    XCOMPONENT_IS_STARTED

/* The following constants and declarations are for unit test purposes and are
 * designed to be used in test applications.
 */
#define XTEST_PASSED    0
#define XTEST_FAILED    1

#define XASSERT_NONE     0
#define XASSERT_OCCURRED 1

extern unsigned int XAssertStatus;
extern void XAssert(char *, int);

/**************************** Type Definitions *******************************/

/** @name Legacy types
 * Deprecated legacy types.
 * @{
 */
typedef unsigned char Xuint8;   /**< unsigned 8-bit */
typedef char        Xint8;    /**< signed 8-bit */
typedef unsigned short  Xuint16;  /**< unsigned 16-bit */
typedef short       Xint16;   /**< signed 16-bit */
typedef unsigned long Xuint32;  /**< unsigned 32-bit */
typedef long        Xint32;   /**< signed 32-bit */
typedef float       Xfloat32; /**< 32-bit floating point */
typedef double        Xfloat64; /**< 64-bit double precision FP */
typedef unsigned long Xboolean; /**< boolean (XTRUE or XFALSE) */

typedef struct
{
  Xuint32 Upper;
  Xuint32 Lower;
} Xuint64;

/*@}*/

/**
 * This data type defines an interrupt handler for a device.
 * The argument points to the instance of the component
 */
typedef void (*XInterruptHandler) (void *InstancePtr);

/**
 * This data type defines an exception handler for a processor.
 * The argument points to the instance of the component
 */
typedef void (*XExceptionHandler) (void *InstancePtr);

/**
 * This data type defines a callback to be invoked when an
 * assert occurs. The callback is invoked only when asserts are enabled
 */
typedef void (*XAssertCallback) (char *FilenamePtr, int LineNumber);

/***************** Macros (Inline Functions) Definitions *********************/

/*****************************************************************************/
/**
* Return the most significant half of the 64 bit data type.
*
* @param    x is the 64 bit word.
*
* @return   The upper 32 bits of the 64 bit word.
*
* @note     None.
*
******************************************************************************/
#define XUINT64_MSW(x) ((x).Upper)

/*****************************************************************************/
/**
* Return the least significant half of the 64 bit data type.
*
* @param    x is the 64 bit word.
*
* @return   The lower 32 bits of the 64 bit word.
*
* @note     None.
*
******************************************************************************/
#define XUINT64_LSW(x) ((x).Lower)


#ifndef NDEBUG

/*****************************************************************************/
/**
* This assert macro is to be used for functions that do not return anything
* (void). This in conjunction with the XWaitInAssert boolean can be used to
* accomodate tests so that asserts which fail allow execution to continue.
*
* @param    expression is the expression to evaluate. If it evaluates to
*           false, the assert occurs.
*
* @return   Returns void unless the XWaitInAssert variable is true, in which
*           case no return is made and an infinite loop is entered.
*
* @note     None.
*
******************************************************************************/
#define XASSERT_VOID(expression)                   \
{                                                  \
    if (expression)                                \
    {                                              \
        XAssertStatus = XASSERT_NONE;              \
    }                                              \
    else                                           \
    {                                              \
        XAssert(__FILE__, __LINE__);               \
                XAssertStatus = XASSERT_OCCURRED;  \
        return;                                    \
    }                                              \
}

/*****************************************************************************/
/**
* This assert macro is to be used for functions that do return a value. This in
* conjunction with the XWaitInAssert boolean can be used to accomodate tests so
* that asserts which fail allow execution to continue.
*
* @param    expression is the expression to evaluate. If it evaluates to false,
*           the assert occurs.
*
* @return   Returns 0 unless the XWaitInAssert variable is true, in which case
*           no return is made and an infinite loop is entered.
*
* @note     None.
*
******************************************************************************/
#define XASSERT_NONVOID(expression)                \
{                                                  \
    if (expression)                                \
    {                                              \
        XAssertStatus = XASSERT_NONE;              \
    }                                              \
    else                                           \
    {                                              \
        XAssert(__FILE__, __LINE__);               \
                XAssertStatus = XASSERT_OCCURRED;  \
        return 0;                                  \
    }                                              \
}

/*****************************************************************************/
/**
* Always assert. This assert macro is to be used for functions that do not
* return anything (void). Use for instances where an assert should always
* occur.
*
* @return Returns void unless the XWaitInAssert variable is true, in which case
*         no return is made and an infinite loop is entered.
*
* @note   None.
*
******************************************************************************/
#define XASSERT_VOID_ALWAYS()                      \
{                                                  \
   XAssert(__FILE__, __LINE__);                    \
           XAssertStatus = XASSERT_OCCURRED;       \
   return;                                         \
}

/*****************************************************************************/
/**
* Always assert. This assert macro is to be used for functions that do return
* a value. Use for instances where an assert should always occur.
*
* @return Returns void unless the XWaitInAssert variable is true, in which case
*         no return is made and an infinite loop is entered.
*
* @note   None.
*
******************************************************************************/
#define XASSERT_NONVOID_ALWAYS()                   \
{                                                  \
   XAssert(__FILE__, __LINE__);                    \
           XAssertStatus = XASSERT_OCCURRED;       \
   return 0;                                       \
}

#define Xil_AssertVoid          XASSERT_VOID
#define Xil_AssertNonvoid       XASSERT_NONVOID
#define Xil_AssertVoidAlways    XASSERT_VOID_ALWAYS
#define Xil_AssertNonVoidAlways XASSERT_NONVOID_ALWAYS

#else

#define XASSERT_VOID(expression)
#define XASSERT_VOID_ALWAYS()
#define XASSERT_NONVOID(expression)
#define XASSERT_NONVOID_ALWAYS()
#endif

/************************** Function Prototypes ******************************/

void XAssertSetCallback(XAssertCallback Routine);
void XNullHandler(void *NullParameter);

#ifdef __cplusplus
}
#endif

#endif  /* end of protection macro */
