// Copyright (c) 2013-2015 by Felix Rusu, LowPowerLab.com
// SPI Flash memory library for arduino/moteino.
// This works with 256byte/page SPI flash memory
// For instance a 4MBit (512Kbyte) flash chip will have 2048 pages: 256*2048 = 524288 bytes (512Kbytes)
// Minimal modifications should allow chips that have different page size but modifications
// DEPENDS ON: Arduino SPI library
// > Updated Jan. 5, 2015, TomWS1, modified writeBytes to allow blocks > 256 bytes and handle page misalignment.
// > Updated Feb. 26, 2015 TomWS1, added support for SPI Transactions (Arduino 1.5.8 and above)
// > Selective merge by Felix after testing in IDE 1.0.6, 1.6.4
// > Updated May 19, 2016 D-H-R, added support for SST25/Microchip Flash which does not support Page programming with OPCode 0x02,
// >                             use define MY_SPIFLASH_SST25TYPE for SST25 Type Flash Memory. Added / changed comments to better suit doxygen
// > Updated Sep 07, 2018 tekka, sync with https://github.com/LowPowerLab/SPIFlash
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
/// @file SPIFlash.h
///
/// @brief SPIFlash provides access to a SPI Flash IC for OTA update or storing data
///
/// IMPORTANT: NAND FLASH memory requires erase before write, because
///            it can only transition from 1s to 0s and only the erase command can reset all 0s to 1s
/// See http://en.wikipedia.org/wiki/Flash_memory
/// The smallest range that can be erased is a sector (4K, 32K, 64K); there is also a chip erase command
///
/// Standard SPI flash commands <BR>
/// Assuming the WP pin is pulled up (to disable hardware write protection).<BR>
/// To use any write commands the WEL bit in the status register must be set to 1.<BR>
/// This is accomplished by sending a 0x06 command before any such write/erase command.<BR>
/// The WEL bit in the status register resets to the logical ?0? state after a device power-up or reset.
/// In addition, the WEL bit will be reset to the logical ?0? state automatically under the following conditions:<BR>
/// - Write Disable operation completes successfully<BR>
/// - Write Status Register operation completes successfully or aborts<BR>
/// - Protect Sector operation completes successfully or aborts<BR>
/// - Unprotect Sector operation completes successfully or aborts<BR>
/// - Byte/Page Program operation completes successfully or aborts<BR>
/// - Sequential Program Mode reaches highest unprotected memory location<BR>
/// - Sequential Program Mode reaches the end of the memory array<BR>
/// - Sequential Program Mode aborts<BR>
/// - Block Erase operation completes successfully or aborts<BR>
/// - Chip Erase operation completes successfully or aborts<BR>
/// - Hold condition aborts
///

#ifndef _SPIFLASH_H_
#define _SPIFLASH_H_

#if ARDUINO >= 100
#include <Arduino.h>
#else
#include <wiring.h>
#include "pins_arduino.h"
#endif

#include <SPI.h>

#ifndef SPIFLASH_WRITEENABLE
#define SPIFLASH_WRITEENABLE      0x06        //!< write enable
#endif

#ifndef SPIFLASH_WRITEDISABLE
#define SPIFLASH_WRITEDISABLE     0x04        //!< write disable
#endif

#ifndef SPIFLASH_BLOCKERASE_4K
#define SPIFLASH_BLOCKERASE_4K    0x20        //!< erase one 4K block of flash memory
#endif

#ifndef SPIFLASH_BLOCKERASE_32K
#define SPIFLASH_BLOCKERASE_32K   0x52        //!< erase one 32K block of flash memory
#endif

#ifndef SPIFLASH_BLOCKERASE_64K
#define SPIFLASH_BLOCKERASE_64K   0xD8        //!< erase one 64K block of flash memory
#endif

#ifndef SPIFLASH_CHIPERASE
#define SPIFLASH_CHIPERASE        0x60        //!< @brief chip erase (may take several seconds depending on size)
#endif
//!< Chip is erased but not actually waited for completion (instead need to check the status register BUSY bit)

#ifndef SPIFLASH_STATUSREAD
#define SPIFLASH_STATUSREAD       0x05        //!< read status register
#endif

#ifndef SPIFLASH_STATUSWRITE
#define SPIFLASH_STATUSWRITE      0x01        //!< write status register
#endif

#ifndef SPIFLASH_ARRAYREAD
#define SPIFLASH_ARRAYREAD        0x0B        //!< read array (fast, need to add 1 dummy byte after 3 address bytes)
#endif

#ifndef SPIFLASH_ARRAYREADLOWFREQ
#define SPIFLASH_ARRAYREADLOWFREQ 0x03        //!< read array (low frequency)
#endif

#ifndef SPIFLASH_SLEEP
#define SPIFLASH_SLEEP            0xB9        //!< deep power down
#endif

#ifndef SPIFLASH_WAKE
#define SPIFLASH_WAKE             0xAB        //!< deep power wake up
#endif

#ifndef SPIFLASH_BYTEPAGEPROGRAM
#define SPIFLASH_BYTEPAGEPROGRAM  0x02        //!< write (1 to 256bytes). Writing more than one Byte is not supported on all devices (e.g. SST25 Series)
#endif

#ifndef SPIFLASH_AAIWORDPROGRAM
#define SPIFLASH_AAIWORDPROGRAM   0xAD        //!< @brief Auto Address Increment Programming on Microchip SST Family Devices which do not support page program. <BR>
#endif
//!< Use define #MY_SPIFLASH_SST25TYPE to use AAI prog instead of Bytepageprogram which does not work on SST Family Chips
//!< tested with SST25PF020B80 http://ww1.microchip.com/downloads/en/DeviceDoc/20005135B.pdf

#ifndef SPIFLASH_IDREAD
#define SPIFLASH_IDREAD           0x9F        //!< @brief read JEDEC manufacturer and device ID (2 bytes, specific bytes for each manufacturer and device)
#endif
//!< Example for Atmel-Adesto 4Mbit AT25DF041A: 0x1F44 (page 27: http://www.adestotech.com/sites/default/files/datasheets/doc3668.pdf)
//!< Example for Winbond 4Mbit W25X40CL: 0xEF30 (page 14: http://www.winbond.com/NR/rdonlyres/6E25084C-0BFE-4B25-903D-AE10221A0929/0/W25X40CL.pdf)

#ifndef SPIFLASH_MACREAD
#define SPIFLASH_MACREAD          0x4B        //!< read unique ID number (MAC)
#endif

///
/// @def MY_SPIFLASH_SST25TYPE
/// @brief If set AAI Word Programming is used to support SST25 Family SPI Flash.
///
/// SST25 Family Flash does not support programming multiple Bytes with opcode 0x02 #SPIFLASH_BYTEPAGEPROGRAM. <BR>
/// If SPIFLASH_SST25TYPE is set and writeBytes is called, it will use opcode 0xAD #SPIFLASH_AAIWORDPROGRAM and care for Byte alignment.<BR>
/// Note: AAI Wordprogramming is independent of Pages, so pagebreaking is not an issue when using AAI Wordprogramming.
///
#ifdef DOXYGEN //needed to tell doxygen not to ignore the define which is actually made somewhere else
#define MY_SPIFLASH_SST25TYPE
#endif

/** SPIFlash class */
class SPIFlash
{
public:
	static uint8_t UNIQUEID[8]; //!< Storage for unique identifier
	SPIFlash(uint8_t slaveSelectPin, uint16_t jedecID=0); //!< Constructor
	bool initialize(); //!< setup SPI, read device ID etc...
	void command(uint8_t cmd, bool isWrite=
	                 false); //!< Send a command to the flash chip, pass TRUE for isWrite when its a write command
	uint8_t readStatus(); //!< return the STATUS register
	uint8_t readByte(uint32_t addr); //!< read 1 byte from flash memory
	void readBytes(uint32_t addr, void* buf, uint16_t len); //!< read unlimited # of bytes
	void writeByte(uint32_t addr, uint8_t byt); //!< Write 1 byte to flash memory
	void writeBytes(uint32_t addr, const void* buf,
	                uint16_t len); //!< write multiple bytes to flash memory (up to 64K), if define SPIFLASH_SST25TYPE is set AAI Word Programming will be used
	bool busy(); //!< check if the chip is busy erasing/writing
	void chipErase(); //!< erase entire flash memory array
	void blockErase4K(uint32_t address); //!< erase a 4Kbyte block
	void blockErase32K(uint32_t address); //!< erase a 32Kbyte block
	void blockErase64K(uint32_t addr); //!< erase a 64Kbyte block
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
