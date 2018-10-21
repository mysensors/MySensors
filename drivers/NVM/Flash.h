/*
 *  Flash.h - Flash library
 *  Original Copyright (c) 2017 Frank Holtz.  All right reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */
/**
 * @file Flash.h
 * @brief Flash abstraction layer
 *
 * @ingroup NVM
 * @details  Nonvolatile Memory Class
 * @{
 */

#pragma once

#include <Arduino.h>
#include <stdio.h> // for size_t

/*
 * Define characteristics of Flash
 *
 * @def FLASH_ERASE_CYCLES
 * @brief Specified number of erase cycles
 *
 * @def FLASH_PAGE_SIZE
 * @brief Used/supported Flash page size
 *
 * @def FLASH_ERASE_PAGE_TIME
 * @brief Time in ms to delete a page
 *
 * @def FLASH_WRITES_PER_WORD
 * @brief How often a dataword (32 bit) can be written
 *
 * @def FLASH_WRITES_PER_PAGE
 * @brief How many writes are allowed into a page
 *
 * @def FLASH_SUPPORTS_RANDOM_WRITE
 * @brief Set this if it is allowed to write to a page in random order.
 */

#if defined(NRF51)
#define FLASH_ERASE_CYCLES 20000
#define FLASH_PAGE_SIZE 1024
#define FLASH_ERASE_PAGE_TIME 23
#define FLASH_SUPPORTS_RANDOM_WRITE true
#define FLASH_WRITES_PER_WORD 2
#define FLASH_WRITES_PER_PAGE 512
#elif defined(NRF52)
#define FLASH_ERASE_CYCLES 10000
#define FLASH_PAGE_SIZE 4096
#define FLASH_ERASE_PAGE_TIME 90
#define FLASH_SUPPORTS_RANDOM_WRITE true
#define FLASH_WRITES_PER_WORD 32
#define FLASH_WRITES_PER_PAGE 181
#elif defined(NRF52840)
#define FLASH_ERASE_CYCLES 10000
#define FLASH_PAGE_SIZE 4096
#define FLASH_ERASE_PAGE_TIME 90
#define FLASH_SUPPORTS_RANDOM_WRITE true
#define FLASH_WRITES_PER_WORD 2
#define FLASH_WRITES_PER_PAGE 403
#else
#define FLASH_ERASE_CYCLES 10000
#define FLASH_PAGE_SIZE 4096
#define FLASH_ERASE_PAGE_TIME 100
//#define FLASH_SUPPORTS_RANDOM_WRITE true
#define FLASH_WRITES_PER_WORD 1
#warning "Unknown platform. Please check the code."
#endif

/**
 * @class FlashClass
 * @brief This class provides low-level access to internal Flash memory.
 */
class FlashClass
{
public:
	//----------------------------------------------------------------------------
	/** Constructor */
	FlashClass() {};
	//----------------------------------------------------------------------------
	/** Initialize Flash */
	void begin() {};
	/** Deinitialize Flash */
	void end() {};
	//----------------------------------------------------------------------------
	/*
	 * Physical flash geometry
	 */
	//----------------------------------------------------------------------------
	/** Page size in bytes
	 * @return Number of bytes
	 */
	uint32_t page_size() const;
	//----------------------------------------------------------------------------
	/** Page address width in bits. Page size is 2^x
	 * @return Number of bits
	 */
	uint8_t page_size_bits() const;
	//----------------------------------------------------------------------------
	/** Number of managed flash pages
	 * @return Number of pages
	 */
	uint32_t page_count() const;
	//----------------------------------------------------------------------------
	/** Number of page erase cycles
	 * @return Number of page erase cycles
	 */
	uint32_t specified_erase_cycles() const;
	//----------------------------------------------------------------------------
	/** Get a address of a page
	 * @param[in] page Page number, starting at 0
	 * @return address of given page
	 */
	uint32_t *page_address(size_t page);
	/** Get top of available flash for application data
	 * @return Last available address + 1
	 */
	uint32_t *top_app_page_address();
	//----------------------------------------------------------------------------
	/*
	 * Accessing flash memory
	 */
	//----------------------------------------------------------------------------
	/** Erase a page of given size. Size must be page_size aligned!
	 *  Take care about RADIO, WDT and Interrupt timing!
	 * @param[in] *address Pointer to page
	 * @param[in] size number of page aligned bytes to erase
	 */
	void erase(uint32_t *address, size_t size);
	//----------------------------------------------------------------------------
	/** Erase the complete MCU. This can brick your device!
	 */
	void erase_all();
	//----------------------------------------------------------------------------
	/** write a aligned 32 bit word to flash.
	 * @param[in] *address 32 bit aligned pointer to destination word
	 * @param[in] value Data word to write
	 */
	void write(uint32_t *address, uint32_t value);
	//----------------------------------------------------------------------------
	/** write a aligned block to flash.
	 * @param[in] *dst_address 32 bit aligned pointer to destination
	 * @param[in] *src_address 32 bit aligned pointer to source
	 * @param[in] word_count Number of words to write
	 */
	void write_block(uint32_t *dst_address, uint32_t *src_address,
	                 uint16_t word_count);

private:
	// Wait until flash is ready
	void wait_for_ready();
};

extern FlashClass Flash;

/** Load Hardwarespecific files */
#ifdef NRF5
#include "hal/architecture/NRF5/drivers/Flash.cpp"
#else
#error "Unsupported platform."
#endif

/** @} */
