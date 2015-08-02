// ---------------------------------------------------------------------------
// Created by GHPS on 5/06/2012.
// Copyright 2012 - Under creative commons license 3.0:
//        Attribution-ShareAlike CC BY-SA
//
// This software is furnished "as is", without technical support, and with no 
// warranty, express or implied, as to its usefulness for any purpose.
//
// Thread Safe: No
// Extendable: Yes
//
// @file LiquidCrystal_I2C_ByVac.c
// This file implements a basic liquid crystal library that comes as standard
// in the Arduino SDK but using the extension board BV4218/BV4208 from ByVac.
// 
// @brief 
// This is a basic implementation of the LiquidCrystal library of the
// Arduino SDK. The original library has been reworked in such a way that 
// this class implements the all methods to command an LCD based
// on the Hitachi HD44780 and compatible chipsets using I2C extension
// backpack BV4218 from ByVac.
//
// The functionality provided by this class and its base class is identical
// to the original functionality of the Arduino LiquidCrystal library.
//
// @author GHPS - ghps@users.sourceforge.net
// ---------------------------------------------------------------------------
#if (ARDUINO <  100)
#include <WProgram.h>
#else
#include <Arduino.h>
#endif
#include <inttypes.h>
#include "LiquidCrystal_I2C_ByVac.h"

// CONSTRUCTORS
// ---------------------------------------------------------------------------
LiquidCrystal_I2C_ByVac::LiquidCrystal_I2C_ByVac( uint8_t lcd_Addr )
{
   _Addr = lcd_Addr;
   _polarity = NEGATIVE;
}

// PUBLIC METHODS
// ---------------------------------------------------------------------------

//
// begin
void LiquidCrystal_I2C_ByVac::begin(uint8_t cols, uint8_t lines, uint8_t dotsize) 
{
   Wire.begin();
   LCD::begin ( cols, lines, dotsize );   
}

// User commands - users can expand this section
//----------------------------------------------------------------------------
// Turn the integrated backlight off/on

// setBacklight
void LiquidCrystal_I2C_ByVac::setBacklight( uint8_t value ) 
{
  Wire.beginTransmission(_Addr);
  Wire.write(0x03); 					//  ByVac command code 0x03 for backlight
  if (value==0) Wire.write(1); else Wire.write((byte)0); 	// 1 for off since polarity is NEGATIVE
  Wire.endTransmission();
}

// Turn the contrast off/on

// setContrast
void LiquidCrystal_I2C_ByVac::setContrast( uint8_t value ) 
{
  Wire.beginTransmission(_Addr);
  Wire.write(0x05); 					//  ByVac command code 0x05 for contrast
  if (value==0) Wire.write((byte)0); else Wire.write(1); 
  Wire.endTransmission();
}

// PRIVATE METHODS
// ---------------------------------------------------------------------------

//
// init
int LiquidCrystal_I2C_ByVac::init()
{
   int status = 0;
   
   // ByVac backpack initialized by onboard firmware
   // ------------------------------------------------------------------------
   status=1;
   return ( status );
}

// low level data pushing commands
//----------------------------------------------------------------------------

//
// send - write either command or data
void LiquidCrystal_I2C_ByVac::send(uint8_t value, uint8_t mode) 
{
  Wire.beginTransmission(_Addr);
  Wire.write(mode+1); // map COMMAND (0) -> ByVac command code 0x01/ DATA  (1) ->  ByVac command code 0x02
  Wire.write(value);
  Wire.endTransmission();
}
