#ifdef ENABLE_QAC_TEST
    #pragma PRQA_MESSAGES_OFF 0292
#endif
/*********************************************************************************************************************
 * File Name     : $Source: fdl_cfg.h $
 * Mod. Revision : $Revision: 1.2 $
 * Mod. Date     : $Date: 2016/06/01 11:51:20JST $
 * Device(s)     : RV40 Flash based RH850 microcontroller
 * Description   : FDL pre-compile definitions
 *********************************************************************************************************************/

/*********************************************************************************************************************
 * DISCLAIMER
 * This software is supplied by Renesas Electronics Corporation and is only  intended for use with Renesas products.
 * No other uses are authorized. This software is owned by Renesas Electronics Corporation and is protected under all
 * applicable laws, including copyright laws.
 * THIS SOFTWARE IS PROVIDED "AS IS" AND RENESAS MAKES NO WARRANTIES REGARDING THIS SOFTWARE, WHETHER EXPRESS, IMPLIED
 * OR STATUTORY, INCLUDING BUT NOT LIMITED TO WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NON-INFRINGEMENT. ALL SUCH WARRANTIES ARE EXPRESSLY DISCLAIMED.
 * TO THE MAXIMUM EXTENT PERMITTED NOT PROHIBITED BY LAW, NEITHER RENESAS ELECTRONICS CORPORATION NOR ANY OF ITS
 * AFFILIATED COMPANIES SHALL BE LIABLE FOR ANY DIRECT, INDIRECT, SPECIAL, INCIDENTAL OR CONSEQUENTIAL DAMAGES FOR ANY
 * REASON RELATED TO THIS SOFTWARE, EVEN IF RENESAS OR ITS AFFILIATES HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGES.
 * Renesas reserves the right, without notice, to make changes to this software and to discontinue the availability of
 * this software. By using this software, you agree to the additional terms and conditions found by accessing the
 * following link:
 * http://www.renesas.com/disclaimer
 *
 * Copyright (C) 2015-2016 Renesas Electronics Corporation. All rights reserved.
 *********************************************************************************************************************/

#ifdef ENABLE_QAC_TEST
    #pragma PRQA_MESSAGES_ON 0292
#endif

/*********************************************************************************************************************
 * MISRA Rule:   MISRA-C 2004 rule 3.1 (QAC message 0292)
 * Reason:       To support automatic insertion of revision, module name etc. by the source revision control system
 *               it is necessary to violate the rule, because the system uses non basic characters as placeholders.
 * Verification: The placeholders are used in commentars only. Therefore rule violation cannot influency code
 *               compilation.
 *********************************************************************************************************************/

#ifndef FDL_CFG_H
#define FDL_CFG_H

#define FLMD0_PROTECTION_OFF    (0x01u)
#define FLMD0_PROTECTION_ON     (0x00u)

#define R_FDL_ICUM_SUPPORT

#define INIT_FLASHACCESS                                            \
    do {                                                            \
        unsigned int flash_access_delay;                            \
        FLMDPCMD = 0xA5u;                                           \
        FLMDCNT = FLMD0_PROTECTION_OFF;                             \
        FLMDCNT = (unsigned char)~FLMD0_PROTECTION_OFF;             \
        FLMDCNT = FLMD0_PROTECTION_OFF;                             \
        for (flash_access_delay = 0u; flash_access_delay < 10000u;  \
             ++flash_access_delay) {                                \
        }                                                           \
    } while (0)

#define DISABLE_FLASHACCESS                                         \
    do {                                                            \
        unsigned int flash_access_delay;                            \
        FLMDPCMD = 0xA5u;                                           \
        FLMDCNT = FLMD0_PROTECTION_ON;                              \
        FLMDCNT = (unsigned char)~FLMD0_PROTECTION_ON;              \
        FLMDCNT = FLMD0_PROTECTION_ON;                              \
        for (flash_access_delay = 0u; flash_access_delay < 10000u;  \
             ++flash_access_delay) {                                \
        }                                                           \
    } while (0)

/*********************************************************************************************************************
 * device specific define
 *********************************************************************************************************************/

    #define R_FDL_NO_BFA_SWITCH

    /******************************************************************************************************************
    * Callback defines to disable interrupts during execution of critical code sections in the 
    * function R_FDL_Init.
    * All possibly occurring exceptions must be disabled because during execution of the critical 
    * sections the Flash is not accessible
    ******************************************************************************************************************/
    void FDL_User_CriticalSetionBegin (void);
    void FDL_User_CriticalSetionEnd (void);

    #define FDL_CRITICAL_SECTION_BEGIN     FDL_User_CriticalSetionBegin ();      /* PRQA S 3412 */
    #define FDL_CRITICAL_SECTION_END       FDL_User_CriticalSetionEnd ();        /* PRQA S 3412 */

    /******************************************************************************************************************
    * Optional configuration
    ******************************************************************************************************************/

/*********************************************************************************************************************/
#endif /* #ifndef FDL_CFG_H */
