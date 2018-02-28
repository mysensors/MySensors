/*
  Flash.cpp - Flash library
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
#include "drivers/NVM/Flash.h"
#include <nrf.h>

FlashClass Flash;

uint32_t FlashClass::page_size() const
{
	return (size_t)NRF_FICR->CODEPAGESIZE;
}

uint8_t FlashClass::page_size_bits() const
{
#if defined(NRF51)
	return 10;
#elif defined(NRF52)
	return 12;
#endif
}

uint32_t FlashClass::page_count() const
{
	return (uint32_t)NRF_FICR->CODESIZE;
}

uint32_t FlashClass::specified_erase_cycles() const
{
	return FLASH_ERASE_CYCLES;
}

uint32_t *FlashClass::page_address(size_t page)
{
	return (uint32_t *)(page << page_size_bits());
}

uint32_t *FlashClass::top_app_page_address()
{
#if !defined(MCUBOOT_PRESENT)
	// Bootcode at the top of the flash memory?
	// https://infocenter.nordicsemi.com/index.jsp?topic=%2Fcom.nordic.infocenter.sdk5.v12.0.0%2Flib_bootloader.html
	if (NRF_UICR->NRFFW[0]<0xFFFFFFFF) {
		// Return pointer calculated by SoftDevice/bootloader
		return (uint32_t *)NRF_UICR->NRFFW[0];
	}
#endif

	// Return flash length
	return (uint32_t *)(Flash.page_count() << Flash.page_size_bits());
}

void FlashClass::erase(uint32_t *address, size_t size)
{
	size_t end_address = (size_t)address + size;

	// align address
	address =
	    (uint32_t *)((size_t)address & (size_t)((size_t)(~0) - FLASH_PAGE_SIZE));

	// Wrong parameters?
	if ((size_t)address >= end_address) {
		return;
	}

	// get old nvm controller state
	uint32_t old_config = NRF_NVMC->CONFIG;

	// Enable erasing flash
	NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Een << NVMC_CONFIG_WEN_Pos;

	// Erase page(s)
	while ((size_t)address < end_address) {
		wait_for_ready();
		// Erase one 1k/4k page
		NRF_NVMC->ERASEPAGE = (size_t)(address);
		address = (uint32_t *)((size_t)address + FLASH_PAGE_SIZE);
	}

	// Disable erasing
	wait_for_ready();
	NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos;

	// Restore old state
	wait_for_ready();
	NRF_NVMC->CONFIG = old_config;

	// Go back if controller is ready
	wait_for_ready();
}

void FlashClass::erase_all()
{
	// Enable erasing flash
	NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Een << NVMC_CONFIG_WEN_Pos;
	wait_for_ready();

	// Erase Flash and UICR
	NRF_NVMC->ERASEALL = 1;
	wait_for_ready();

	// Disable erasing
	NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos;
	wait_for_ready();
}

void FlashClass::write(uint32_t *address, uint32_t value)
{
	// Compare word
	if (*address != value) {
		// Enable write
		NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Wen << NVMC_CONFIG_WEN_Pos;
		wait_for_ready();
		// Write word
		*address = value;
		// Disable write
		wait_for_ready();
		NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos;
		wait_for_ready();
	}
}

void FlashClass::write_block(uint32_t *dst_address, uint32_t *src_address,
                             uint16_t word_count)
{
	// Enable write
	NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Wen << NVMC_CONFIG_WEN_Pos;
	wait_for_ready();

	while (word_count > 0) {
		if (*dst_address != *src_address) {
			*dst_address = *src_address;
		}
		word_count--;
		dst_address++;
		src_address++;
	}

	// Disable write
	wait_for_ready();
	NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos;
	wait_for_ready();
}

void FlashClass::wait_for_ready()
{
	while (NRF_NVMC->READY == NVMC_READY_READY_Busy) {
	};
}
