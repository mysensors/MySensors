/*
   Flash.h - Flash library
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

#pragma once

#include <Arduino.h>
#include <stdio.h> // for size_t

#ifdef __RFduino__
#include <chip.h>
#else
#include <nrf.h>
#endif

/*
 * Define characteristics of Flash
 *
 * FLASH_ERASE_CYCLES         : Specified number of erase cycles
 * FLASH_PAGE_SIZE            : Used/supported Flash page size
 * FLASH_ERASE_PAGE_TIME      : Time in ms to delete a page
 * FLASH_WRITES_PER_WORD      : How often a dataword (32 bit) can written
 * FLASH_WRITES_PER_PAGE      : How many writes are allowed into a page
 * FLASH_SUPPORTS_RANDOM_WRITE: Set this if it is allowed to write to a page in
 *                              random order.
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

class FlashClass {
public:
  FlashClass(){};
  void begin(){};
  void end(){};

  /*
   * Physical flash geometry
   */

  // Page size in bytes
  uint32_t page_size() const;

  // Page size in bits
  uint8_t page_size_bits() const;

  // Number of pages
  uint32_t page_count() const;

  // Number of erase cycles
  uint32_t specified_erase_cycles() const;

  // Get a address of a page
  uint32_t *page_address(size_t page);

  /*
   * Accessing flash memory
   */

  // Erase a page of given size. Size must be page_size aligned!
  // Take care about RADIO, WDT and Interrupt timing!
  void erase(uint32_t *address, size_t size);

  // Erase the complete MCU. This can brick your device!
  void erase_all();

  // write a aligned 32 bit word to flash. Depends on write_enable!
  void write(uint32_t *address, uint32_t value);

  // write a aligned 32 bit word to flash. Depends on write_enable!
  void write_block(uint32_t *dst_address, uint32_t *src_address,
                   uint16_t word_count);

private:
  // Wait until flash is ready
  void wait_for_ready();
};

extern FlashClass Flash;
