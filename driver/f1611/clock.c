/*
 * clock.c
 *
 *  Created on: 2013. 1. 14.
 *      Author: OSSI
 */
#include"clock.h"

// main clock will be shared via system
static volatile uint8_t clockMode;

uint8_t clock_getMode(void)
{
	return clockMode;
}

uint8_t clock_setDefaultDCO(void)
{

	// Chage DCO_DELTA to change default DCO frequency
	// default DCO_DELTA = 255 and fDCO ~ 1MHz
	uint16_t Compare, Oldcapture = 0;

	_DINT();
	// TODO: save previous ACLK divider
	BCSCTL1 |=  DIVA_3;             		  //  ACLK= LFXT1CLK/8
	CCTL2 = CM_1 + CCIS_1 + CAP;              // CAP, ACLK
	TACTL = TASSEL_2 + MC_2 + TACLR;          // SMCLK, cont-mode, clear

	while (1)
	{
		while (!(CCIFG & CCTL2));               // Wait until capture occured
		CCTL2 &= ~CCIFG;                        // Capture occured, clear flag
		Compare = CCR2;                         // Get current captured SMCLK
		Compare = Compare - Oldcapture;         // SMCLK difference
		Oldcapture = CCR2;                      // Save current captured SMCLK

		if (DCO_DELTA == Compare) break;            // If equal, leave "while(1)"
		else if (DCO_DELTA < Compare)               // DCO is too fast, slow it down
		{
		  DCOCTL--;
		  if (DCOCTL == 0xFF)
		  {
			if (!(BCSCTL1 == (XT2OFF + DIVA_3)))
			BCSCTL1--;                          // Did DCO roll under?, Sel lower RSEL
		  }
		}
		else
		{
		  DCOCTL++;
		  if (DCOCTL == 0x00)
			{
			  if (!(BCSCTL1 == (XT2OFF + DIVA_3 + 0x07)))
			  BCSCTL1++;                        // Did DCO roll over? Sel higher RSEL
			}
		}
	}

	CCTL2 = 0;                                // Stop CCR2
	TACTL = 0;
	// TODO: restore previous ACLK divider rather than clear it
	BCSCTL1 &= ~DIVA_3;                       // Back to ACLK= LFXT1CLK
	_EINT();

	return 1;
}

uint8_t clock_setup(void)
{
	// We want to use LFXT1CLK for 32.768kHz / DCO = ~ 1MHz
	// after POR, default DCO frequency is 800kHz


	// CLOCK_DCO_LFXT1 mode DCOCLK = ~1MHz ACLK = 32.768kHz
	// Set DCO frequency in case XT2 fails
	BCSCTL1 &= ~XTS; // default value is 0 but to make sure
	BCSCTL1 |= XT2OFF;							// XT2 OFF
	BCSCTL2 = 0x00;								// reset BCSCTL2
	clock_setDefaultDCO(); 						// TODO: fix the setting later
	clockMode = CLOCK_DCO_LFXT1;
	return CLOCK_DCO_LFXT1;

}

void clock_dividerSetup(uint8_t MCLKDividerSelect, uint8_t SMCLKDividerSelect, uint8_t ACLKDividerSelect)
{
	// reset divider first

	// reset ACLK divider
	BCSCTL1 &= 0xCF; // Clear BIT5 and BIT4
	// select ACLK Divider
	BCSCTL1 |= ACLKDividerSelect;

	// reset MCLK, SMCLK divider
	BCSCTL2 &= 0xC9; // Clear BIT5,BIT4, BIT2, BIT1
	// select MCLK Divider
	BCSCTL2 |= MCLKDividerSelect;
	// select SMCLK Divider
	BCSCTL2 |= SMCLKDividerSelect;
}
