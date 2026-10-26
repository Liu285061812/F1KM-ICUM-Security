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
#include "icum_flash_handoff.h"
#include "icum_flash_handoff_protocol.h"
#include "shared_memory.h"

__IOREG(PE_OSTM0_ICUM_PE2ICUFS, 0xFF1F0044, __WRITE, uint32);
__IOREG(PE_OSTM0_ICUM_PE2ICUSFS, 0xFF1F0074, __WRITE, uint32);

#define PE_OSTM0_CLOCK_HZ       60000000UL
#define PE_OSTM0_INTERVAL_MS    1000UL
#define PE_OSTM0_COMPARE_VALUE \
    ((PE_OSTM0_CLOCK_HZ / 1000UL) * PE_OSTM0_INTERVAL_MS - 1UL)


//============================================================================
// Defines
//============================================================================
//#define _SharedMemoryTest_


//============================================================================
// Prototypes
//============================================================================
void            main                ( void );
void            init_clocks         ( void );


//============================================================================
// Variables
//============================================================================
extern const long int ddf_size;
extern const unsigned char ddf[524288];
unsigned int dma0_sample=0;
unsigned int dma0_samples_count=0;
//============================================================================
// Sub Functions
//============================================================================


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
void csih0_initPorts(void)
{
  /* CSIH0SC @ P0_2  */ 
  PORTPMC0 |= (1<<2);
  PORTPFC0 |= (1<<2);
  PORTPFCE0 |= (1<<2);
  PORTPFCAE0 &= ~(1<<2);
  PORTPM0 |= (1<<2);
  
  /* CSIH0S0 @ P0_3  */ 
  PORTPMC0 |= (1<<3);
  PORTPFC0 |= (1<<3);
  PORTPFCE0 |= (1<<3);
  PORTPFCAE0 &= ~(1<<3);
  PORTPM0 &= ~(1<<3);
}

void TAUD0_initPorts(void)
{
  // Enable TAUD0 channels 2 and 4 output on  P0_0 and P0_1
  protected_write(PORTPPCMD0,PORTPPROTS0,PORTPODC0,0x00);	// switch P0_0 from Open Drain to Push Pull
  PORTPMSR0  = 0x30000;		// P0_0 and P0_1 as output
  PORTPMCSR0 = 0x30003;		// P0_0 and P0_1 as alternative function
  // no further init required, as AF1 is selected by default register settings
}

void TAUD0_init(void) 
{
    /* TAUA0 settings for Interval Timer Function, running on 80MHz*/
   TAUD0TPS = 0x0000;	// Prescaler = PCLK / 2^0 for CKn  (80MHz/(2^0) = 80MHz)
   
   TAUD0TOM = 0x0000;	// Independent channel operation mode for output
   TAUD0TOC = 0x0000;	// Channel output operation mode 1
   TAUD0TOL = 0x0000;	// Positive logic output
      TAUD0TO  = 0x0014;	// Outputs low level on TOUTn pins (init)
   TAUD0TOE = 0x0014;	// Channel output operation on ch. 2 and ch. 4 

 
   TAUD0RDE = 0x0000;	// Disables simultaneous rewrite of the data register
   TAUD0RDM = 0x0000;	// TAUA0RDM selects the signal that controls simultaneous rewrite
   
   /* Channel 2 Config interval timer as I�S Word Select clock*/
   /* Mode selection for Interval timer */
   TAUD0CMOR2 = 0x0000;	
   TAUD0CMUR2 = 0x0000; // (not used)
   TAUD0CDR2  = 832-1-52; // 80MHz/832= 96,154kHz (Fixed I�S sampling rate = 48kHz)

   /* enable interrupt for TAUD0 channel 1*/
   //ICTAUD0I2 = 0x0047; 		// enable reference table jump

   /* Channel 4 Config: interval timer as clock source for I�S data stream */
   /* Mode selection for Interval timer */
   TAUD0CMOR4 = 0x0001;	
   TAUD0CMUR4 = 0x0000; // (not used)
   //TAUD0CDR4  = 0x0019; // Needs to be 16x TAUD0 ch.2 (>= 48kHz*32bit = 1,536Mbps)
   TAUD0CDR4  = (26 - 1); // Needs to be 32x TAUD0 ch.2 (>= 48kHz*32bit = 1,536Mbps)
   /* enable interrupt for TAUD0 channel 4*/
   //ICTAUD0I4 = 0x0047; 		// enable reference table jump
}


  
void csih0_init(void)
{
  
  // CSIH2 config (transmit only and receive mode, fifo)
  CSIH0CTL0 =   0x01;            // disable CSI
  
  CSIH0CTL1  =  0x00010080;      //Bit24:0   internal synchronization timing for receive data input is rising edge of PCLK
                                 //Bit17:0   The default level of CSIHTSCK is high
                                 //Bit16:1   As soon as the contents of the CSIHnTX0W/H register are transferred to the shift register, an interrupt is generated
                                 //Bit8-15:0 Chip select is active low.
                                 //Bit7:1    Enables extended data length mode
                                 //Bit6:0    disables job mode.
                                 //Bit5:0    Disables data consistency check.
                                 //Bit4:0    Chip select signal returns to the inactive level after last data transfer//////
                                 //Bit3:0    Deactivates loop-back mode (LBM)
                                 //Bit2:0    No interrupt delay mode
                                 //Bit1:0    Disables the handshake function.
                                 //Bit0:0    disables the slave select function.

  CSIH0CTL2 = 0xe000;            // Clock selection: Slave mode
  
  CSIH0STCR0 = 0xFFFF;           // clear all status flags

  CSIH0MCTL0 = 0x0000;           // Bit8-9:0 Fifo mode
                                 // Bit0-4:0 no time-out detection
                                  
                                  
  CSIH0MCTL1 = 0x00000000;      //generate receive interrupt when 128 frames are received
  
  
  CSIH0MRWP0 = 0x00000000;      // reset read and write pointers
 
  CSIH0BRS0 = 0x0000;           // 5 MHz
  

 
  CSIH0CFG0 = 0x00000000|(1<<19);   // configure CS0: Baudrate BRS0, 16 Bit, MSB first,Recessive

  INTC1ICCSIH0IC=0x0040;

 
}


void csih0_start(void)
{
 CSIH0CTL0 |=  0xC0;            // enable CSIH, transmit mode   
 CSIH0TX0H = 0x0000;
 
}

void csih0_stop(void)
{
 CSIH0CTL0 = 0x00;          	//disable CSIH0
}

void TAUD0_start(void) 
{
   /* Start Channels 4 and 2 */
   INTC1RFTAUD0I4=0;
   TAUD0TS = (1<<4)|(1<<2); // start channel 4 channel 2
   
   while(0==INTC1RFTAUD0I4);
    TAUD0CDR2  = 832-1; // Needs to be 32x TAUD0 ch.2 (>= 48kHz*32bit = 1,536Mbps)


}
void TAUD0_stop(void) 
{
   TAUD0TT = (1<<4)|(1<<2); // stop channel 4 channel 2
 }
void DMA_init(void)
{
  /* Initialize DMA for sending bytes */                     
   DMAC0DCEN0 = 0x00u;                             /*Disable DMA */
   DMAC0DM00CM = 0x10;
   DMAC0CMVC = 0x33;                                /* clear status */
   
   dma0_sample=0;
   
                /*Number of times the DMA transfers*/
   DMAC0DSA0 = (uint32)ddf;    /*Source Address = ADC reg*/
   DMAC0DDA0 = (uint32)&CSIH0TX0H;;                         /*Write Data to  RAM*/
   DMAC0DTC0 = 0; 
   
   dma0_sample+=0x10000;
   dma0_samples_count=ddf_size/2;

	
	DMAC0DRSA0= (uint32)&ddf + 0x10000*2;
	DMAC0DRTC0 = 0;  
	
	dma0_sample+=0x10000;
   DMAC0DTCT0 = (1<<26)|(1<<14)|(1<<13)|(1<<9)|(1<<8)|(1<<2);            /* Hardware DMA transfer request
                                              Transfer completion interrupt enable,
                                              Destination address fixed,#
                                              Source address Increment
                                              Continuous transfer enable 
                                              16 bits                            
                                              reload function 1*/
                                              
   DMAC0DTFR0 = (0x46<<1)|1;                          /*Trigger Enabled / Source = INTCSIH0IC transmit interrupt */
   DMAC0DCSTC0 = 0xb1;                         /* clear status */
   DMAC0DTFRRQC0  = 1u;                              /*Clear DMA channel 0 request  */
   DMAC0DCEN0 = 0x01u;                     

  INTC2ICDMA0=0x0040;
  }

void Debug_initPorts()
{
		PORTPM0 &= ~(1<<4);
	}


void pe_icum_comm_timer_init(void)
{
    OSTM0TT = 1u;
    OSTM0CTL = 0u;
    OSTM0CMP = PE_OSTM0_COMPARE_VALUE;
    INTC2ICOSTM0 = 0x0040u;
    OSTM0TS = 1u;
}


//============================================================================
// Interrupt Service Routines - ISR
//============================================================================
#pragma ghs interrupt
void INTOSTM0(void)
{
    uint32 index;
    uint8 value = 0x55u;

    g_shared_buffer[0] = value;
    for (index = 1u; index < SHARED_BUFFER_SIZE; index++)
    {
        value++;
        g_shared_buffer[index] = value;
    }
#if defined _SharedMemoryTest_
    PE_OSTM0_ICUM_PE2ICUFS = ICUM_SHARED_MEMORY_FLAG;
    PE_OSTM0_ICUM_PE2ICUSFS = 1u;
#endif
}

#pragma ghs interrupt

void INTCSIH0IC(void)
{

	PORTPNOT0=(1<<4);
}

#pragma ghs interrupt
void INTDMA0 (void)
{
	
		
	if(dma0_sample >= dma0_samples_count)
	{
DMAC0DTCT0&=~(1<<13); 

	}
	else if((dma0_sample) < dma0_samples_count)
	{
	
	DMAC0DRSA0 = (uint32)&ddf[0]+dma0_sample*2;
	DMAC0DRTC0 = 0;
	dma0_sample+=0x10000;

	}
	
	
	
}



//============================================================================
// Main function
//============================================================================
void main(void) {
	unsigned int i=0;
	  /* initialize clocks */
  init_clocks();
  
  Debug_initPorts();
  
  
	DMA_init();
	TAUD0_init();
	TAUD0_initPorts();
	
	csih0_init();
	csih0_initPorts();
	for(i=0;i<10000000;i++);
	pe_icum_comm_timer_init();
	pe_icum_flash_handoff_init();
	__EI();
	
	csih0_start();
	TAUD0_start();
  while(1);

}
