// Copyright (c) 2013-2015 by Felix Rusu, LowPowerLab.com
// SPI Flash memory library for arduino/moteino.
// This works with 256byte/page SPI flash memory
// For instance a 4MBit (512Kbyte) flash chip will have 2048 pages: 256*2048 = 524288 bytes (512Kbytes)
// Minimal modifications should allow chips that have different page size but modifications
// DEPENDS ON: Arduino SPI library
// > Updated Jan. 5, 2015, TomWS1, modified writeBytes to allow blocks > 256 bytes and handle page misalignment.
// > Updated Feb. 26, 2015 TomWS1, added support for SPI Transactions (Arduino 1.5.8 and above)
// > Selective merge by Felix after testing in IDE 1.0.6, 1.6.4
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

#ifndef _SPIFLASH_H_
#define _SPIFLASH_H_

#if ARDUINO >= 100
#include <Arduino.h>
#else
#include <wiring.h>
#include "pins_arduino.h"
#endif

#include <SPI.h>

/// IMPORTANT: NAND FLASH memory requires erase before write, because
///            it can only transition from 1s to 0s and only the erase command can reset all 0s to 1s
/// See http://en.wikipedia.org/wiki/Flash_memory
/// The smallest range that can be erased is a sector (4K, 32K, 64K); there is also a chip erase command

/// Standard SPI flash commands
/// Assuming the WP pin is pulled up (to disable hardware write protection)
/// To use any write commands the WEL bit in the status register must be set to 1.
/// This is accomplished by sending a 0x06 command before any such write/erase command.
/// The WEL bit in the status register resets to the logical ?0? state after a
/// device power-up or reset. In addition, the WEL bit will be reset to the logical ?0? state automatically under the following conditions:
/// ? Write Disable operation completes successfully
/// ? Write Status Register operation completes successfully or aborts
/// ? Protect Sector operation completes successfully or aborts
/// ? Unprotect Sector operation completes successfully or aborts
/// ? Byte/Page Program operation completes successfully or aborts
/// ? Sequential Program Mode reaches highest unprotected memory location
/// ? Sequential Program Mode reaches the end of the memory array
/// ? Sequential Program Mode aborts
/// ? Block Erase operation completes successfully or aborts
/// ? Chip Erase operation completes successfully or aborts
/// ? Hold condition aborts
#define SPIFLASH_WRITEENABLE      0x06        // write enable
#define SPIFLASH_WRITEDISABLE     0x04        // write disable

#define SPIFLASH_BLOCKERASE_4K    0x20        // erase one 4K block of flash memory
#define SPIFLASH_BLOCKERASE_32K   0x52        // erase one 32K block of flash memory
#define SPIFLASH_BLOCKERASE_64K   0xD8        // erase one 64K block of flash memory
#define SPIFLASH_CHIPERASE        0x60        // chip erase (may take several seconds depending on size)
                                              // but no actual need to wait for completion (instead need to check the status register BUSY bit)
#define SPIFLASH_STATUSREAD       0x05        // read status register
#define SPIFLASH_STATUSWRITE      0x01        // write status register
#define SPIFLASH_ARRAYREAD        0x0B        // read array (fast, need to add 1 dummy byte after 3 address bytes)
#define SPIFLASH_ARRAYREADLOWFREQ 0x03        // read array (low frequency)

#define SPIFLASH_SLEEP            0xB9        // deep power down
#define SPIFLASH_WAKE             0xAB        // deep power wake up
#define SPIFLASH_BYTEPAGEPROGRAM  0x02        // write (1 to 256bytes)
#define SPIFLASH_IDREAD           0x9F        // read JEDEC manufacturer and device ID (2 bytes, specific bytes for each manufacturer and device)
                                              // Example for Atmel-Adesto 4Mbit AT25DF041A: 0x1F44 (page 27: http://www.adestotech.com/sites/default/files/datasheets/doc3668.pdf)
                                              // Example for Winbond 4Mbit W25X40CL: 0xEF30 (page 14: http://www.winbond.com/NR/rdonlyres/6E25084C-0BFE-4B25-903D-AE10221A0929/0/W25X40CL.pdf)
#define SPIFLASH_MACREAD          0x4B        // read unique ID number (MAC)
                                              
/** SPIFlash class */
class SPIFlash {
public:
  static uint8_t UNIQUEID[8]; //!< Storage for unique identifier
  SPIFlash(uint8_t slaveSelectPin, uint16_t jedecID=0); //!< Constructor
  boolean initialize(); //!< setup SPI, read device ID etc...
  void command(uint8_t cmd, boolean isWrite=false); //!< Send a command to the flash chip, pass TRUE for isWrite when its a write command
  uint8_t readStatus(); //!< return the STATUS register
  uint8_t readByte(uint32_t addr); //!< read 1 byte from flash memory
  void readBytes(uint32_t addr, void* buf, uint16_t len); //!< read unlimited # of bytes
  void writeByte(uint32_t addr, uint8_t byt); //!< Write 1 byte to flash memory
  void writeBytes(uint32_t addr, const void* buf, uint16_t len); //!< write multiple bytes to flash memory (up to 64K)
  boolean busy(); //!< check if the chip is busy erasing/writing
  void chipErase(); //!< erase entire flash memory array
  void blockErase4K(uint32_t address); //!< erase a 4Kbyte block
  void blockErase32K(uint32_t address); //!< erase a 32Kbyte block
  uint16_t readDeviceId(); //!< Get the manufacturer and device ID bytes (as a short word)
  uint8_t* readUniqueId(); //!< Get the 64 bit unique identifier, stores it in @ref UNIQUEID[8]
  
  void sleep(); //!< Put device to sleep
  void wakeup(); //!< Wake device
  void end(); //!< end
protected:
  void select(); //!< select
  void unselect(); //!< unselect
  uint8_t _slaveSelectPin; //!< Slave select pin
  uint16_t _jedecID; //!< JEDEC ID
  uint8_t _SPCR; //!< SPCR
  uint8_t _SPSR; //!< SPSR
#ifdef SPI_HAS_TRANSACTION
  SPISettings _settings;
#endif
};

#endif
