//============================================================================
// PROJECT  :  Example project
//============================================================================
//                           C O P Y R I G H T
//============================================================================
// Copyright (c) 2013 by Renesas Electronics Europe GmbH. All rights reserved.
// Arcadiastr.10
// D-40472 Duesseldorf
// Germany
//============================================================================
//
// Warranty Disclaimer :
// Because the Product(s) is licensed free of charge, there is no warranty of
// any kind whatsoever and expressly disclaimed and excluded by Renesas, either
// expressed or implied, including but not limited to those for non-
// infringement of intellectual property, merchantability and/or fitness for
// the particular purpose. Renesas shall not have any obligation to maintain,
// service or provide bug fixes for the supplied Product(s) and/or the
// Application.
//
// Each User is solely responsible for determining the appropriateness of
// using the Product(s) and assumes all risks associated with its exercise
// of rights under this Agreement, including, but not limited to the risks
// and costs of program errors, compliance with applicable laws, damage to
// or loss of data, programs or equipment, and unavailability or
// interruption of operations.
//
// Limitation of Liability :
// In no event shall Renesas be liable to the User for any incidental,
// consequential, indirect, or punitive damage (including but not limited to
// lost profits) regardless of whether such liability is based on breach of
// contract, tort, strict liability, breach of warranties, failure of
// essential purpose or otherwise and even if advised of the possibility of
// such damages. Renesas shall not be liable for any services or products provided
// by third party vendors, developers or consultants identified or referred
// to the User by Renesas in connection with the Product(s) and/or the
// Application.
//
//============================================================================
// Enviroment : Devices    : RH850/F1x (dr7f701569)
//              Debugger   : GHS Multi 2000
//============================================================================
//
// GHS Multi Example project for use with RH850/F1x PiggyBack board
// with 16 MHz MainOsc
// 
//
//============================================================================


//============================================================================
// Includes
//============================================================================
#include "device.h"
#include "icum_crypto.h"
#include "icum_ostm.h"

volatile icum_u32 g_icum_crypto_test_result;


void init_clocks(void)
{ 
    /* Prepare 16MHz MainOsc */
  if((CLKCTLMOSCS&0x04u) != 0x4u)                                       /* Check if MainOsc needs to be started */
  {
   CLKCTLMOSCC=0x06;                                                    /* Set MainOSC gain for 16MHz */
   CLKCTLMOSCST=0x00003E80;                                             /* Set MainOSC stabilization time to 2ms */
   protected_write(WPROTRPROTCMD0,WPROTRPROTS0,CLKCTLMOSCE,0x01u);      /* Trigger Enable (protected write) */
   while ((CLKCTLMOSCS&0x04u) != 0x04u);                                /* Wait for active MainOSC */
  }

  if((CLKCTLPLL0S&0x04u) != 0x04u)                                      /* Check if PLL needs to be started */
  {
    /* Prepare PLL0 */
    CLKCTLPLL0C=0x6340483B;                                             /* 16 MHz MainOSC -> 480MHz VCOUT */
                                                                        /* 5% @ 100kHz SSCG */
                                                                        
    protected_write(WPROTRPROTCMD0,WPROTRPROTS0,CLKCTLPLL0ST,0x1B80);   /* Set stabilization time to recommended value */
    protected_write(WPROTRPROTCMD1,WPROTRPROTS1,CLKCTLPLL0E,0x01u);     /* Enable PLL */
    while((CLKCTLPLL0S&0x04u) != 0x04u){}                               /* Wait for active PLL */
  }
  
  if((CLKCTLPLL1S&0x04u) != 0x04u)                                      /* Check if PLL needs to be started */
  {
    /* Prepare PLL1 */
    CLKCTLPLL1C=0x10B3B;                                                /* 16 MHz MainOSC -> 480MHz VCOUT, 80MHz PPLLOUT */
    protected_write(WPROTRPROTCMD1,WPROTRPROTS1,CLKCTLPLL1E,0x01u);     /* Enable PLL */
    while((CLKCTLPLL1S&0x04u) != 0x04u){}                               /* Wait for active PLL */
  }
  
  /* CPLLOUT = VCOOUT � 1/2 = 240 MHz */
  protected_write(WPROTRPROTCMD1,WPROTRPROTS1,CLKCTLCKSC_CPUCLKD_CTL,0x29);
  while(CLKCTLCKSC_CPUCLKD_ACT!=0x29);  
  
  /* CPLLOUT -> CPU Clock */ 
  protected_write(WPROTRPROTCMD1,WPROTRPROTS1,CLKCTLCKSC_CPUCLKS_CTL,0x00u);
  while(CLKCTLCKSC_CPUCLKS_ACT!=0x00u);
  
  /* PPLLOUT -> PPLLCLK = 80MHz */
  protected_write(WPROTRPROTCMD1,WPROTRPROTS1,CLKCTLCKSC_PPLLCLKS_CTL,0x3);
  while(CLKCTLCKSC_PPLLCLKS_ACT!=0x3);  
}

void main_icum(void)
{
    init_clocks();
    icum_ostm_init();
    g_icum_crypto_test_result = icum_crypto_self_test(0x01000000ul);
    while(1)
    {
        // Main loop
    }
}

void main()
{
    main_icum();
    while(1)
    {
        // Main loop
    }
}
