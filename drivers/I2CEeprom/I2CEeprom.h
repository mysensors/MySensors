// Copyright (C) 2016 Krister W. <kisse66@hobbylabs.org>
//
// Original SPI flash driver this is based on:
// Copyright (c) 2013-2015 by Felix Rusu, LowPowerLab.com
//
// I2C EEPROM library for MySensors OTA. Based on SPI Flash memory library for
// arduino/moteino.
// This driver is made to look like the SPI flash driver so changes needed to
// MySensors OTA code is minimized.
// This works with 32 or 64kB I2C EEPROM like an 24(L)C256. AVR HW I2C is assumed
// and error handling is quite minimal.
// DEPENDS ON: Arduino Wire library
// **********************************************************************************
// License
// **********************************************************************************
// This program is free software; you can redistribute it 
// and/or modify it under the terms of the GNU General    
// Public License as published by the Free Software       
// Foundation; either version 3 of the License, or        
// (at your option) any later version.                    
//                                                        
// This program is distributed in the hope that it will   
// be useful, but WITHOUT ANY WARRANTY; without even the  
// implied warranty of MERCHANTABILITY or FITNESS FOR A   
// PARTICULAR PURPOSE. See the GNU General Public        
// License for more details.                              
//                                                        
// You should have received a copy of the GNU General    
// Public License along with this program.
// If not, see <http://www.gnu.org/licenses/>.
//                                                        
// Licence can be viewed at                               
// http://www.gnu.org/licenses/gpl-3.0.txt
//
// Please maintain this license information along with authorship
// and copyright notices in any redistribution of this code

///
/// @file I2CEeprom.h
///
/// @brief I2CEeprom provides access to a I2C EEPROM IC for OTA update or storing data
///
/// Code assumes the AVR hardware I2C is used and the EEPROM uses 16-bit addresses.
///
#ifndef _I2CEeprom_H_
#define _I2CEeprom_H_

#if ARDUINO >= 100
#include <Arduino.h>
#else
#include <wiring.h>
#include "pins_arduino.h"
#endif

//#ifndef I2CEEPROM_TWI_CLK
//#define I2CEEPROM_TWI_CLK 400000
//#endif

// EEPROM page size. Typically 64 (see data sheet for your EEPROM)
#ifndef I2CEEPROM_PAGE_SIZE
#define I2CEEPROM_PAGE_SIZE 64
#endif

// time out for write (ms)
#ifndef I2CEEPROM_WR_LIMIT
#define I2CEEPROM_WR_LIMIT 30
#endif

/** I2CEeprom class */
class I2CEeprom
{
public:

  I2CEeprom(uint8_t addr); //!< Constructor
  boolean initialize(); //!< setup
  uint8_t readByte(uint32_t addr); //!< read 1 byte from flash memory
  void readBytes(uint32_t addr, void* buf, uint16_t len); //!< read unlimited # of bytes
  void writeByte(uint32_t addr, uint8_t byt); //!< Write 1 byte to flash memory
  void writeBytes(uint32_t addr, const void* buf, uint16_t len); //!< write multiple bytes to flash memory (up to 64K), if define SPIFLASH_SST25TYPE is set AAI Word Programming will be used 
  boolean busy(); //!< check if the chip is busy erasing/writing
    
  // not needed for EEPROMs, but kept so SPI flash code compiles as is (functions are NOP)
  uint16_t readDeviceId() { return 0xDEAD; };
  void chipErase() {};
  void blockErase4K(uint32_t address) {};
  void blockErase32K(uint32_t address) {};
  void sleep() {};
  void wakeup() {};
  void end() {};
  
protected:

  uint8_t m_addr; //!< I2C address
};

#endif
