/*
  NVRAM.cpp - Byte wise storage for Virtual Pages.
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
#include "NVRAM.h"

// VirtualPage magic
#define NVRAM_MAGIC (0x7710fdb9)
// Number of emulated cells
#define NVRAM_LENGTH 3072
// Log configuration: Address index in bits
#define NVRAM_ADDR_POS 20
// Log configuration: Mask for comparsion (4k space)
#define NVRAM_ADDR_MASK 0xfff00000
// Log configuration: Bit position of used address bitmap
#define NVRAM_BITMAP_POS 8
// Log configuration: used address bitmap calulation
#define NVRAM_BITMAP_ADDR_SHIFT 8
// Log configuration: Mask for bitmap extraction
#define NVRAM_BITMAP_MASK 0x000fff00
#define ADDR2BIT(index)                                                        \
	((1 << (index >> NVRAM_BITMAP_ADDR_SHIFT)) << NVRAM_BITMAP_POS)

NVRAMClass NVRAM;

uint16_t NVRAMClass::length() const
{
	return (NVRAM_LENGTH);
}

void NVRAMClass::read_block(uint8_t *dst, uint16_t idx, uint16_t n)
{
	uint32_t *vpage;
	uint16_t log_start, log_end;

	// find correct page
	vpage = get_page();

	// copy 0xff to dst when no page is available
	if (vpage == (uint32_t *)~0) {
		for (uint32_t i = 0; i < n; i++) {
			((uint8_t *)dst)[i] = 0xff;
		}
		return;
	}

	// calculate actual log position
	log_end = get_log_position(vpage);
	if (log_end == 0) {
		log_start = 1;
	} else {
		log_start = vpage[0] + 1;
	}
	/*
	  Serial.print("\r\nread_block idx=");
	  Serial.print(idx);
	  Serial.print(" n=");
	  Serial.print(n);
	  Serial.print("("); */
	while (n > 0) {
		// Read cell
		*dst = get_byte_from_page(vpage, log_start, log_end, idx);
		// Serial.print(*dst, HEX);
		// calculate next address
		n--;
		dst++;
		idx++;
	}
	// Serial.println(")");
}

uint8_t NVRAMClass::read(const uint16_t idx)
{
	uint8_t ret;
	read_block(&ret, idx, 1);
	return ret;
}

bool NVRAMClass::write_block(uint8_t *src, uint16_t idx, uint16_t n)
{
	uint32_t *vpage;
	uint32_t bitmap;
	uint16_t log_start, log_end;

	// find correct page
	vpage = get_page();

	// return on invalid page
	if (vpage == (uint32_t *)~0) {
		return false;
	}

	// calculate actual log position
	log_start = vpage[0] + 1;
	log_end = get_log_position(vpage);
	if (log_end > log_start) {
		bitmap = vpage[log_end - 1] & NVRAM_BITMAP_MASK;
	} else {
		bitmap = 0;
	}

	while (n > 0) {
		// Read cell
		uint8_t old_value = get_byte_from_page(vpage, log_start, log_end, idx);
		uint8_t new_value = *src;

		// Have to write into log?
		if (new_value != old_value) {

			// need to calculate a new page?
			if (log_end >= VirtualPage.length()) {
				vpage = switch_page(vpage, &log_start, &log_end);
				if (vpage == (uint32_t *)~0) {
					// do nothing if no page is available
					return false;
				}
				bitmap = 0;
			}

			// Add Entry into log
			Flash.write(&vpage[log_end], (idx << NVRAM_ADDR_POS) | bitmap |
			            ADDR2BIT(idx) | (uint32_t)new_value);
			log_end++;
		}

		// calculate next address
		n--;
		src++;
		idx++;
	}
	return true;
}

bool NVRAMClass::write(uint16_t idx, uint8_t value)
{
	return (write_block(&value, idx, 1));
}

int NVRAMClass::write_prepare(uint16_t number)
{
	// find correct page
	uint32_t *vpage = get_page();
	// Want to write to much or into an invalid page?
	if ((vpage == (uint32_t *)~0) || (number > length())) {
		return -1;
	}

	// calculate actual log position
	uint16_t log_end = get_log_position(vpage);

	// Calculate number of free bytes in log
	int free_bytes = ((VirtualPage.length() - 1) - log_end);

	// switch page when
	if (free_bytes < number) {
		uint16_t log_start = vpage[0] + 1;
		vpage = switch_page(vpage, &log_start, &log_end);
		if (vpage == (uint32_t *)~0) {
			// do nothing if no page is available
			return -1;
		}
		log_end = get_log_position(vpage);
		free_bytes = ((VirtualPage.length() - 1) - log_end);
	}
	return free_bytes;
}

void NVRAMClass::clean_up(uint16_t write_preserve)
{
	VirtualPage.clean_up();
	if (write_preserve > 0) {
		write_prepare(write_preserve);
	}
}

uint32_t *NVRAMClass::switch_page(uint32_t *old_vpage, uint16_t *log_start,
                                  uint16_t *log_end)
{
	// Mark old page as in release
	VirtualPage.release_prepare(old_vpage);

	// Get a blank page
	uint32_t *new_vpage = VirtualPage.allocate(NVRAM_MAGIC, VirtualPage.length());
	if (new_vpage == (uint32_t *)~0) {
		// failed
		return new_vpage;
	}

	// Store four bytes for map creation
	uint32_t value;

	// Length of new map
	uint16_t map_length = 0;

	// Build map
#ifdef FLASH_SUPPORTS_RANDOM_WRITE
	// Copy current values
	for (uint16_t i = 0; i < (NVRAM_LENGTH >> 2); i++) {
		read_block((uint8_t *)&value, i << 2, 4);
		if (value != (uint32_t)~0) {
			// Value found
			map_length = i + 1;
			Flash.write(&new_vpage[i + 1], value);
		}
	}
	// Store map length
	Flash.write(new_vpage, map_length);
#else
	// find map length
	for (uint16_t i = (NVRAM_LENGTH >> 2); i > 0; i--) {
		read_block((uint8_t *)&value, i << 2, 4);
		if (value != (uint32_t)~0) {
			// Value found
			map_length = i;
			break;
		}
	}
	map_length++;

	// Store map length
	Flash.write(new_vpage, map_length);

	// Copy current values
	for (uint16_t i = 0; i <= map_length; i++) {
		read_block((uint8_t *)&value, i << 2, 4);
		if (value != (uint32_t)~0) {
			// Value found
			map_length = i;
			Flash.write(&new_vpage[i + 1], value);
		}
	}
#endif

	// Release old page
	VirtualPage.release(old_vpage);

	// Set log position
	*log_start = map_length + 1;
	*log_end = *log_start;

	return new_vpage;
}

uint32_t *NVRAMClass::get_page()
{
	uint32_t *vpage = VirtualPage.get(NVRAM_MAGIC);
	// Invalid page?
	if (vpage == (uint32_t *)~0) {
		// Allocate a new page
		vpage = VirtualPage.allocate(NVRAM_MAGIC, VirtualPage.length());
		// Set map length to 0
		Flash.write(&vpage[0], 0x0);
	}
	return vpage;
}

uint16_t NVRAMClass::get_log_position(uint32_t *vpage)
{
	uint16_t position_min = vpage[0] + 1;
	uint16_t position_max = VirtualPage.length();

	// Return if page is not filled
	if ((vpage[0] == (uint32_t)~0) || (position_min >= position_max)) {
		return 0;
	}

	// loop until postition_min != position_max-1
	while (position_min != position_max - 1) {
		// Calculate middle between min and max
		uint16_t mid = position_min + ((position_max - position_min) >> 1);
		// Set max or min to current position
		if (vpage[mid] == (uint32_t)~0) {
			position_max = mid;
		} else {
			position_min = mid;
		}
	}

	return position_max;
}

uint8_t NVRAMClass::get_byte_from_page(uint32_t *vpage, uint16_t log_start,
                                       uint16_t log_end, uint16_t idx)
{
	// mask matching a bit signaling wich address range is in log
	uint32_t address_mask = ADDR2BIT(idx);
	// mask matching the index address
	uint32_t address_match = idx << NVRAM_ADDR_POS;

	// Check the log backwards
	while (log_end > log_start) {
		log_end--;
		uint32_t value = vpage[log_end];
		// end here if address map bit is not set
		if ((value & address_mask) == 0) {
			break;
		}
		// check address match -> update found -> return
		if ((value & NVRAM_ADDR_MASK) == address_match) {
			return (uint8_t)value;
		}
	}

	// Calculate address in the eeprom map at the beginning of a vpage
	uint16_t map_address = (idx >> 2);
	map_address++; // jump over log offset field

	// look at map if calculated addess before log start position
	if (map_address < log_start) {
		switch (idx % 4) {
		case 3:
			return (uint8_t)(vpage[map_address] >> 24);
			break;
		case 2:
			return (uint8_t)(vpage[map_address] >> 16);
			break;
		case 1:
			return (uint8_t)(vpage[map_address] >> 8);
			break;
		default:
			return (uint8_t)(vpage[map_address]);
			break;
		}
	}

	// empty cell
	return 0xff;
}
