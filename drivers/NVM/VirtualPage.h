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

/*
 * Virtual page management:
 * The managed pages are organized into VirtualPage.size() sized pages.
 * The first and last 8 Bytes are reserved for magic number and page marking.
 *
 */

#pragma once

#include "Flash.h"
#include <Arduino.h>

class VirtualPageClass {
public:
  VirtualPageClass(){};
  void begin(){};
  void end(){};

  // Usable page size in bytes
  uint16_t size() const;

  // Usable page size in data words
  uint16_t length() const;

  // Maximum number of allocatable pages
  uint16_t page_count() const;

  // Calculates rate of wear in percent*100.
  // Values grater than 10000 indicates exeeding the chip specification
  // this value is only valid for fresh controllers.
  uint32_t wear_level();

  // Search for a page by given, uniqe magic number.
  // Returns a pointer to (uint32_t *)~0 if there is no page. Don't write to
  // this address.
  uint32_t *get(uint32_t magic);

  // Returns an address to an blank page or (uint32_t *)~0 if no space
  // available.
  // Take care about RADIO, WDT and Interrupt timing! Plan with 0-100ms
  uint32_t *allocate(uint32_t magic);

  // Search for a blank page from top of flash to do max_writes write operations
  // Take care about RADIO, WDT and Interrupt timing! Plan with 0-100ms
  uint32_t *allocate(uint32_t magic, uint32_t max_writes);

  // Start releasing a page. You have to allocate a new one and than release the
  // old page
  void release_prepare(uint32_t *address);

  // Page is marked as available for next allocation
  void release(uint32_t *address);

  // Returns true if page is in release_prepare state
  bool release_started(uint32_t *address);

  // mark a page as defect
  void fail(uint32_t *address);

  // Prepare released pages for faster reallocation. Plan with 0-100ms.
  void clean_up();

  // Release all pages
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
