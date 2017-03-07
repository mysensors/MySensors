/*
  VirtualPage.h - Flash page management
  Original Copyright (c) 2017 Frank Holtz.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
/**
 * @file VirtualPage.h
 * @brief Virtual page management on top of Flash
 * The managed pages are organized into VirtualPage.size() sized pages.
 * Some Bytes of a Flash page are reserved for a magic number and page management.
 *
 * @ingroup NVM
 * @details  Nonvolatile Memory Class
 * @{
 */
#pragma once

#include "Flash.h"
#include <Arduino.h>

/**
 * @class VirtualPageClass
 * @brief Virtual page management on top of Flash
 */
class VirtualPageClass
{
public:
	//----------------------------------------------------------------------------
	/** Constructor */
	VirtualPageClass() {};
	//----------------------------------------------------------------------------
	/** Initialize Virtual Pages */
	void begin() {};
	//----------------------------------------------------------------------------
	/** Deinitilize Virtual Pages */
	void end() {};
	//----------------------------------------------------------------------------
	/** Reports usable page size in bytes
	 * @return Number of bytes
	 */
	uint16_t size() const;
	//----------------------------------------------------------------------------
	/** Reports usable page size in 32 Bit words
	 * @return Number of words
	 */
	uint16_t length() const;
	//----------------------------------------------------------------------------
	/** Reports the maximum number of allocatable pages
	 * @return Number of pages
	 */
	uint16_t page_count() const;
	//----------------------------------------------------------------------------
	/** Calculates the rate of wear in percent*100.
	 * Values greater than 10000 indicates exceeding the chip specification.
	 * This value is only valid for controllers, never erased completely.
	 * @return calculated level
	 */
	uint32_t wear_level();
	//----------------------------------------------------------------------------
	/** Search for a page by given, unique magic number.
	 * Returns a pointer to (uint32_t *)~0 if there is no page. Don't write
	 * to this address.
	 * @param[in] magic A magic number to identify the page.
	 * @return Pointer to a page
	 */
	uint32_t *get(uint32_t magic);
	//----------------------------------------------------------------------------
	/** Returns an address to a blank page or (uint32_t *)~0 if no space
	 * available. Take care about RADIO, WDT and Interrupt timing! Calculate with
	 * 0-100ms until a page is available.
	 * @param[in] magic A magic number to identify the page.
	 * @return Pointer to a page
	 */
	uint32_t *allocate(uint32_t magic);
	//----------------------------------------------------------------------------
	/** Returns an address to a blank page or (uint32_t *)~0 if no space
	 * available. Take care about RADIO, WDT and Interrupt timing! Calculate with
	 * 0-100ms until a page is available.
	 * This function allows using a page multiple times without erasing on some platforms.
	 * @param[in] magic A magic number to identify the page.
	 * @param[in] max_writes The number of planned write operations in page lifecycle.
	 * @return Pointer to a page
	 */
	uint32_t *allocate(uint32_t magic, uint32_t max_writes);
	//----------------------------------------------------------------------------
	/** Start releasing a page. You have to allocate a new one and then release the
	 * old page.
	 * @param[in] *address A pointer to the page to release
	 */
	void release_prepare(uint32_t *address);
	//----------------------------------------------------------------------------
	/** End releasing a page.
	 * @param[in] *address A pointer to the page to release
	 */
	void release(uint32_t *address);
	//----------------------------------------------------------------------------
	/** Returns true if page is in release_prepare state
	 * @param[in] *address A pointer to a page
	 * @return Release state
	 */
	bool release_started(uint32_t *address);
	//----------------------------------------------------------------------------
	/** Mark a page as a defect page.
	 */
	void fail(uint32_t *address);
	//----------------------------------------------------------------------------
	/** Prepare released pages for faster reallocation. Plan with 0-100ms.
	 */
	void clean_up();
	//----------------------------------------------------------------------------
	/** Release all pages
	 */
	void format();

private:
	// convert page number 1..max_pages into an address
	uint32_t *get_page_address(uint16_t page);
	// build a page
	void build_page(uint32_t *address, uint32_t magic);
	// return number of erase cycles
	uint32_t get_page_erase_cycles(uint32_t *address);
};

extern VirtualPageClass VirtualPage;

/** @} */
