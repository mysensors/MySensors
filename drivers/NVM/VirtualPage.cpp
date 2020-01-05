/*
  VirtualPage.cpp - Flash page management
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

#include "VirtualPage.h"

VirtualPageClass VirtualPage;

#ifndef NVM_VIRTUAL_PAGE_SIZE_BITS
#define NVM_VIRTUAL_PAGE_SIZE_BITS 12
#elif NVM_VIRTUAL_PAGE_SIZE_BITS < 12
#error "NVM_VIRTUAL_PAGE_SIZE_BITS must be >= 12"
#endif

// Calculate virtual page count, when mcuboot is present
#if defined(MCUBOOT_PRESENT) && !defined(NVM_VIRTUAL_PAGE_COUNT)
// mcuboot zephyr build via generated_dts_board.h
#include "generated_dts_board.h"
// Calculate number of free pages after scratch area
#define NVM_VIRTUAL_PAGE_COUNT (((CONFIG_FLASH_SIZE_0<<10)-(FLASH_AREA_IMAGE_SCRATCH_OFFSET_0+FLASH_AREA_IMAGE_SCRATCH_SIZE_0)) >> NVM_VIRTUAL_PAGE_SIZE_BITS)
#endif

// check page size
#ifndef NVM_VIRTUAL_PAGE_COUNT
#if FLASH_ERASE_CYCLES >= 20000
// use 16k of flash memory
#define NVM_VIRTUAL_PAGE_COUNT 4
#else
// use 32k of flash memory
#define NVM_VIRTUAL_PAGE_COUNT 8
#endif
#endif

/*
 * How many virtual pages are skipped from top of flash
 */
#ifndef NVM_VIRTUAL_PAGE_SKIP_FROM_TOP
#define NVM_VIRTUAL_PAGE_SKIP_FROM_TOP 0
#endif

/*
 * Calculate things around NVM_VIRTUAL_PAGE_SIZE
 */
#define NVM_VIRTUAL_PAGE_SIZE (1 << (NVM_VIRTUAL_PAGE_SIZE_BITS))
#define NVM_VIRTUAL_PAGE_ADDRESS_MASK (~(NVM_VIRTUAL_PAGE_SIZE - 1))
#define NVM_VIRTUAL_PAGE_ALIGN(address)                                        \
	{ address = (uint32_t *)((uint32_t)address & NVM_VIRTUAL_PAGE_ADDRESS_MASK); }

/*
 * Defines the position of status words in a page.
 * Offsets are defined in words!
 */
#ifdef FLASH_SUPPORTS_RANDOM_WRITE
// use first 8 byte for magic, erase counter and status
#define OFFSET_MAGIC 0
#define OFFSET_ERASE_COUNTER 1
#if FLASH_WRITES_PER_WORD > 2
// use first 12 bytes for magic, erase counter and status
#define MASK_ERASE_COUNTER 0x00FFFFFF
#define OFFSET_STATUS_RELEASE_PREPARE 1
#define OFFSET_STATUS_RELEASE_END 1
#define METADATA_SIZE 8
#define OFFSET_DATA 2
#elif FLASH_WRITES_PER_WORD == 2
#define MASK_ERASE_COUNTER 0x00FFFFFF
#define OFFSET_STATUS_RELEASE_PREPARE 2
#define OFFSET_STATUS_RELEASE_END 2
#define METADATA_SIZE 12
#define OFFSET_DATA 3
#else
// use first 12 bytes for erase counter, and magic
#define OFFSET_MAGIC 1
#define OFFSET_COUNTER 0
#define MASK_ERASE_COUNTER 0x00FFFFFF
#define OFFSET_STATUS_RELEASE_PREPARE NVM_VIRTUAL_PAGE_SIZE - 8
#define OFFSET_STATUS_RELEASE_END NVM_VIRTUAL_PAGE_SIZE - 4
#define METADATA_SIZE 16
#define OFFSET_DATA 4
#endif

#define BIT_STATUS_RELEASE_PREPARE (1 << 30)
#define BIT_STATUS_RELEASE_END (1 << 31)

#define NVM_VIRTUAL_PAGE_DATA_SIZE (NVM_VIRTUAL_PAGE_SIZE - METADATA_SIZE)
#else
// use first 8 byte for magic and erase counter and last 8 byte for page release
#define OFFSET_MAGIC 1
#define OFFSET_ERASE_COUNTER 0
#define OFFSET_DATA 2
#define OFFSET_STATUS_RELEASE_PREPARE                                          \
	((NVM_VIRTUAL_PAGE_SIZE - 8) / sizeof(uint32_t))
#define OFFSET_STATUS_RELEASE_END                                              \
	((NVM_VIRTUAL_PAGE_SIZE - 4) / sizeof(uint32_t))

#define MASK_ERASE_COUNTER 0xFFFFFFFF

#define BIT_STATUS_RELEASE_PREPARE 1
#define BIT_STATUS_RELEASE_END 1

#define NVM_VIRTUAL_PAGE_DATA_SIZE (NVM_VIRTUAL_PAGE_SIZE - 16)

#endif

uint16_t VirtualPageClass::size() const
{
	return (NVM_VIRTUAL_PAGE_DATA_SIZE);
}

uint16_t VirtualPageClass::length() const
{
	return (NVM_VIRTUAL_PAGE_DATA_SIZE / 4);
}

uint16_t VirtualPageClass::page_count() const
{
	return (NVM_VIRTUAL_PAGE_COUNT - 1);
}

uint32_t VirtualPageClass::wear_level()
{
	uint32_t max_erase_cycles = 0;
	for (int i = 1; i <= NVM_VIRTUAL_PAGE_COUNT; i++) {
		uint32_t erase_cycles = get_page_erase_cycles(get_page_address(i));
		if (erase_cycles > max_erase_cycles) {
			max_erase_cycles = erase_cycles;
		}
	}
	return (uint32_t)((((uint64_t)max_erase_cycles * 10000)) /
	                  Flash.specified_erase_cycles());
}

uint32_t *VirtualPageClass::get(uint32_t magic)
{

	// Give back a page prepared for release and not closed
	for (int i = 1; i <= NVM_VIRTUAL_PAGE_COUNT; i++) {
		uint32_t *page = get_page_address(i);
		if (
		    // correct magic is set
		    (page[OFFSET_MAGIC] == magic) &&
		    // page is in release_prepare mode
		    ((page[OFFSET_STATUS_RELEASE_PREPARE] & BIT_STATUS_RELEASE_PREPARE) ==
		     0) &&
		    // page is not released
		    ((page[OFFSET_STATUS_RELEASE_END] & BIT_STATUS_RELEASE_END) > 0)) {
			// Return page in release process with priority
			return &page[OFFSET_DATA];
		}
	}

	// check if a unreleased page is available
	for (int i = 1; i <= NVM_VIRTUAL_PAGE_COUNT; i++) {
		uint32_t *page = get_page_address(i);
		if (
		    // correct magic is set
		    (page[OFFSET_MAGIC] == magic) &&
		    // page is not released
		    ((page[OFFSET_STATUS_RELEASE_END] & BIT_STATUS_RELEASE_END) > 0)) {
			// return page in normal operation
			return &page[OFFSET_DATA];
		}
	}

	return (uint32_t *)(~0);
}

uint32_t *VirtualPageClass::allocate(uint32_t magic)
{
	uint32_t *return_page = (uint32_t *)(~0);
	uint32_t max_erase_cycles = (uint32_t)~0;

	// Avoid duplicate allocation of pages, look for the less used page
	for (int i = 1; i <= NVM_VIRTUAL_PAGE_COUNT; i++) {
		uint32_t *page = get_page_address(i);

		// Delete duplicated pages
		if (
		    // same magic
		    (page[OFFSET_MAGIC] == magic) &&
		    // Not in release_end state
		    ((page[OFFSET_STATUS_RELEASE_END] & BIT_STATUS_RELEASE_END) > 0) &&
		    // Not in release_prepare state
		    (!release_started(page))) {
			// clear the page
			build_page(page, (uint32_t)~0);
		}

		uint32_t erase_cycles = get_page_erase_cycles(page);
		// When the page has less erase cycles and is not marked as failed
		if ((erase_cycles < max_erase_cycles) && (page[OFFSET_MAGIC] > 0) &&
		        (
		            // magic is empty
		            (page[OFFSET_MAGIC] == (uint32_t)~0) ||
		            // marked as released
		            ((page[OFFSET_STATUS_RELEASE_END] & BIT_STATUS_RELEASE_END) ==
		             0))) {
			max_erase_cycles = erase_cycles;
			return_page = page;
		}
	}

	// return if no page was found
	if (return_page == (uint32_t *)~0) {
		return return_page;
	}

	build_page(return_page, magic);
	return &return_page[OFFSET_DATA];
}

uint32_t *VirtualPageClass::allocate(uint32_t magic, uint32_t max_writes)
{
	// max_writes is not implemented yet -> page is erased with every allocate
	(void)max_writes;
	return allocate(magic);
}

void VirtualPageClass::release_prepare(uint32_t *address)
{
	// move pointer to beginning of the page
	NVM_VIRTUAL_PAGE_ALIGN(address);

	// Nothing to do at a empty page
	if (address[OFFSET_MAGIC] == (uint32_t)~0) {
		return;
	}

	if (release_started(address) == false) {
		// Clear bit BIT_PAGE_RELEASED
		Flash.write(&address[OFFSET_STATUS_RELEASE_PREPARE],
		            address[OFFSET_STATUS_RELEASE_PREPARE] &
		            ~BIT_STATUS_RELEASE_PREPARE);
	}
	return;
}

void VirtualPageClass::release(uint32_t *address)
{
	// move pointer to beginning of the page
	NVM_VIRTUAL_PAGE_ALIGN(address);

	// Nothing to do at a empty page
	if (address[OFFSET_MAGIC] == (uint32_t)~0) {
		return;
	}

	// Check if status bit already cleared
	if ((address[OFFSET_STATUS_RELEASE_END] & BIT_STATUS_RELEASE_END) > 0) {
		// Clear bit BIT_PAGE_RELEASED
		Flash.write(&address[OFFSET_STATUS_RELEASE_END],
		            address[OFFSET_STATUS_RELEASE_END] & ~BIT_STATUS_RELEASE_END);
	}
	return;
}

bool VirtualPageClass::release_started(uint32_t *address)
{
	// move pointer to beginning of the page
	NVM_VIRTUAL_PAGE_ALIGN(address);

	return (address[OFFSET_STATUS_RELEASE_PREPARE] &
	        BIT_STATUS_RELEASE_PREPARE) == 0;
}

void VirtualPageClass::fail(uint32_t *address)
{
	// move pointer to beginning of the page
	NVM_VIRTUAL_PAGE_ALIGN(address);

	build_page(address, 0x00000000);
	return;
}

void VirtualPageClass::clean_up()
{
	// No page found -> try to give back a page prepared for release
	for (int i = 1; i <= NVM_VIRTUAL_PAGE_COUNT; i++) {
		uint32_t *page = get_page_address(i);
		if ((page[OFFSET_STATUS_RELEASE_END] & BIT_STATUS_RELEASE_END) == 0) {
			build_page(get_page_address(i), ~0);
			return; // a maximum of a page is cleaned -> return
		}
	}
}

void VirtualPageClass::format()
{
	for (int i = 1; i <= NVM_VIRTUAL_PAGE_COUNT; i++) {
		uint32_t *address = get_page_address(i);
		build_page(address, (uint32_t)~0);
	}
}

uint32_t *VirtualPageClass::get_page_address(uint16_t page)
{
	return (uint32_t *)(Flash.top_app_page_address() -
	                    ((page + NVM_VIRTUAL_PAGE_SKIP_FROM_TOP)
	                     << NVM_VIRTUAL_PAGE_SIZE_BITS));
}

void VirtualPageClass::build_page(uint32_t *address, uint32_t magic)
{
	// move pointer to beginning of the page
	NVM_VIRTUAL_PAGE_ALIGN(address);
	// get erase counter
	uint32_t erase_counter = get_page_erase_cycles(address);

	// Check if a magic is set
	if (address[OFFSET_MAGIC] != (uint32_t)~0) {
		Flash.erase(address, NVM_VIRTUAL_PAGE_SIZE);
	} else {
		// check if page is empty
		for (int i = OFFSET_DATA; i < (NVM_VIRTUAL_PAGE_SIZE / 4); i++) {
			if (address[i] != (uint32_t)~0) {
				Flash.erase(address, NVM_VIRTUAL_PAGE_SIZE);
				break;
			}
		}
	}

	// write a new page
	Flash.write(&address[OFFSET_MAGIC], magic);
	if (address[OFFSET_ERASE_COUNTER] == (uint32_t)~0) {
		Flash.write(&address[OFFSET_ERASE_COUNTER],
		            erase_counter | ~MASK_ERASE_COUNTER);
	}
}

uint32_t VirtualPageClass::get_page_erase_cycles(uint32_t *address)
{
	// Return number of cycles
	return ((((uint32_t)address[OFFSET_ERASE_COUNTER])+1) &
	        (uint32_t)MASK_ERASE_COUNTER);
}
