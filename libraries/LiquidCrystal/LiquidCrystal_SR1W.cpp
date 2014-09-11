// ---------------------------------------------------------------------------
// Created/Adapted by Stephen Erisman 2013-07-06
// Copyright 2013 - Under creative commons license 3.0:
//        Attribution-ShareAlike CC BY-SA
//
// This software is furnished "as is", without technical support, and with no 
// warranty, express or implied, as to its usefulness for any purpose.
//
// Thread Safe: No
// Extendable: Yes
//
// @file LiquidCrystal_SR1W.cpp
// Connects a hd44780 LCD using 1 pin from the Arduino, via an 8-bit Latching
// ShiftRegister (SR1W from now on).
// 
// @brief 
// This is an optimized implementation of the 1-wire shift concept developed by
// Roman Black (http://www.romanblack.com/shift1.htm) that also makes use of
// (and merges) the diode-resistor AND "gate" concept (http://www.rentron.com/Myke1.htm)
// as well as introducing some new and original ideas (particularly how HW_CLEAR works).
// 
//
// See the corresponding SR1W header file for full details.
//
// History
// 2013.07.31 serisman - fixed potential interrupt bug and made more performance optimizations
// 2013.07.10 serisman - more performance optimizations and modified the HW_CLEAR circuit a bit
// 2013.07.09 serisman - added an even faster version that performs the clear in hardware
// 2013.07.08 serisman - changed code to shift data MSB first to match SR2W
// 2013.07.07 serisman - major speed optimization
// 2013.07.06 serisman - created/modified from SR2W and FastIO sources to create SR1W
// @author  S. Erisman - arduino@serisman.com
// ---------------------------------------------------------------------------

#include "LiquidCrystal_SR1W.h"

// CONSTRUCTORS
// ---------------------------------------------------------------------------
// Assuming 1 line 8 pixel high font
LiquidCrystal_SR1W::LiquidCrystal_SR1W (uint8_t srdata, t_sr1w_circuitType circuitType, t_backlighPol blpol)
{
	init ( srdata, circuitType, blpol, 1, 0 );
}

// PRIVATE METHODS
// ---------------------------------------------------------------------------

//
// init
void LiquidCrystal_SR1W::init(uint8_t srdata, t_sr1w_circuitType circuitType, t_backlighPol blpol, uint8_t lines, uint8_t font)
{
	_srRegister = fio_pinToOutputRegister(srdata);
	_srMask = fio_pinToBit(srdata);
   
	_circuitType = circuitType;
   
	_blPolarity = blpol;
   
	_displayfunction = LCD_4BITMODE | LCD_1LINE | LCD_5x8DOTS;
   
   clearSR();
   
	backlight(); // set default backlight state to on
}

//
// clearSR
uint8_t LiquidCrystal_SR1W::clearSR()
{
	uint8_t numDelays = 0;
   
	// Store these as local variables for extra performance (and smaller compiled sketch size)
	fio_register srRegister = _srRegister;
	fio_bit srMask = _srMask;
   
	// Set the Serial PIN to a LOW state
	SR1W_ATOMIC_WRITE_LOW(srRegister, srMask);
   
	// We need to delay to make sure the Data and Latch/EN capacitors are fully discharged
	// This also triggers the EN pin because of the falling edge.
	SR1W_DELAY();
   
	ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
	{
		// Pre-calculate these values for extra performance and to make sure the clock pulse is as quick as possible
		fio_bit reg_val = *srRegister;
		fio_bit bit_low = reg_val & ~srMask;
		fio_bit bit_high = reg_val | srMask;
      
		// Clear the shift register (without triggering the Latch/EN pins)
		// We only need to shift 7 bits here because the subsequent HIGH transistion will also shift a '0' in.
		for (int8_t i = 6; i>=0; i--)
		{
			// Shift in a '0' (NOTE: This clock pulse needs to execute as quickly as possible)
			*srRegister = bit_high;
			*srRegister = bit_low;
		}
      
		// Set the Serial PIN to a HIGH state so the next nibble/byte can be loaded
		// This also shifts the 8th '0' bit in.
		*srRegister = bit_high;
	}
   
	// Give the Data capacitor a chance to fully charge
	SR1W_DELAY();
   
	return numDelays;
}

//
// loadSR
uint8_t LiquidCrystal_SR1W::loadSR(uint8_t val)
{
	uint8_t numDelays = 0;
   
	// Store these as local variables for extra performance (and smaller compiled sketch size)
	fio_register srRegister = _srRegister;
	fio_bit srMask = _srMask;
   
	// NOTE: This assumes the Serial PIN is already HIGH and the Data capacitor is fully charged
	uint8_t previousBit = 1;
   
	// Send the data to the shift register (MSB first)
	for (int8_t i = 7; i>=0; i--)
	{
		if (val & 0x80)
		{
			if (previousBit == 0)
			{
				// We need to make sure the Data capacitor has fully recharged
				SR1W_DELAY();
			}
         
			previousBit = 1;
         
			ATOMIC_BLOCK(ATOMIC_RESTORESTATE)
			{
				// Pre-calculate these values to make sure the clock pulse is as quick as possible
				fio_bit reg_val = *srRegister;
				fio_bit bit_low = reg_val & ~srMask;
				fio_bit bit_high = reg_val | srMask;
            
				// Shift in a '1' (NOTE: This clock pulse needs to execute as quickly as possible)
				*srRegister = bit_low;
				*srRegister = bit_high;
			}
		}
		else
		{
			// Shift in a '0'
			SR1W_ATOMIC_WRITE_LOW(srRegister, srMask);
         
			// We need to make sure the Data capacitor has fully discharged
			SR1W_DELAY();
         
			previousBit = 0;
         
			SR1W_ATOMIC_WRITE_HIGH(srRegister, srMask);
		}
		val <<= 1;
	}
   
	// NOTE: Serial PIN is currently HIGH
   
	// For SW_CLEAR, we need to delay to make sure the Latch/EN capacitor is fully charged.
	//   This triggers the Latch pin because of the rising edge.
	// For HW_CLEAR, we need to delay to give the hardware time to perform the clear.
	//   This also gives the Data capacitor a chance to fully charge
	SR1W_DELAY();
   
   if (_circuitType == SW_CLEAR)
	{
		// Clear the shift register to get ready for the next nibble/byte
		// This also discharges the Latch/EN capacitor which finally triggers the EN pin because of the falling edge.
		numDelays += clearSR();
   }
	else
	{
		// For some reason HW_CLEAR isn't totally stable unless we delay a little bit more.
		// TODO... figure this out...
		SR1W_DELAY();
	}
   
	return numDelays;
}

// PUBLIC METHODS
// ---------------------------------------------------------------------------


/************ low level data pushing commands **********/
//
// send
void LiquidCrystal_SR1W::send(uint8_t value, uint8_t mode)
{
	uint8_t numDelays = 0;
   
	uint8_t data;
   
	if ( mode != FOUR_BITS )
	{
		// upper nibble
		data = ( mode == DATA ) ? SR1W_RS_MASK : 0;
		data |= SR1W_EN_MASK | SR1W_UNUSED_MASK;
		data |= _blMask;
      
		if (value & _BV(4))	data |= SR1W_D4_MASK;
		if (value & _BV(5)) data |= SR1W_D5_MASK;
		if (value & _BV(6)) data |= SR1W_D6_MASK;
		if (value & _BV(7)) data |= SR1W_D7_MASK;
      
		numDelays += loadSR(data);
	}
   
	// lower nibble
	data = ( mode == DATA ) ? SR1W_RS_MASK : 0;
	data |= SR1W_EN_MASK | SR1W_UNUSED_MASK;
	data |= _blMask;
   
	if (value & _BV(0))	data |= SR1W_D4_MASK;
	if (value & _BV(1)) data |= SR1W_D5_MASK;
	if (value & _BV(2)) data |= SR1W_D6_MASK;
	if (value & _BV(3)) data |= SR1W_D7_MASK;
   
	numDelays += loadSR(data);
   
	// Make sure we wait at least 40 uS between bytes.
	unsigned int totalDelay = numDelays * SR1W_DELAY_US;
	if (totalDelay < 40)
		delayMicroseconds(40 - totalDelay);
}

//
// setBacklight
void LiquidCrystal_SR1W::setBacklight ( uint8_t value )
{ 
	// Check for polarity to configure mask accordingly
	// ----------------------------------------------------------
	if  ( ((_blPolarity == POSITIVE) && (value > 0)) || 
        ((_blPolarity == NEGATIVE ) && ( value == 0 )) )
	{
		_blMask = SR1W_BL_MASK;
	}
	else 
	{
		_blMask = 0;
	}
   
	// Send a dummy (non-existant) command to allow the backlight PIN to be latched.
	// The seems to be safe because the LCD appears to treat this as a NOP.
	send(0, COMMAND);
}