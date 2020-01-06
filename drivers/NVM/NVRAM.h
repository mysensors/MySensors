/*
 * NVRAM.h - Byte-wise storage for Virtual Pages.
 * Original Copyright (c) 2017 Frank Holtz.  All right reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/
/**
 * @file NVRAM.h
 * @brief Byte-wise storage for Virtual Pages.
 *
 * @defgroup NVM Nonvolatile Memory
 * @ingroup internals
 * @details  Nonvolatile Memory Class
 * @{
 */
#pragma once

#include "Flash.h"
#include "VirtualPage.h"
#include <Arduino.h>

/**
 * @class NVRAMClass
 * @brief Nonvolatile Memory
 */
class NVRAMClass
{
public:
	//----------------------------------------------------------------------------
	/** Constructor. */
	NVRAMClass() {};
	//----------------------------------------------------------------------------
	/** Initialize Class */
	void begin() {};
	//----------------------------------------------------------------------------
	/** Deinitialize Class */
	void end() {};
	//----------------------------------------------------------------------------
	/** NVM available space in bytes
	 * @return Number of bytes
	 */
	uint16_t length() const;
	//----------------------------------------------------------------------------
	/** Read a block of bytes
	 * @param[in] *dst Write bytes to given pointer
	 * @param[in] idx  First NVM address to read
	 * @param[in] n    Number of bytes to read
	 */
	void read_block(uint8_t *dst, uint16_t idx, uint16_t n);
	//----------------------------------------------------------------------------
	/** Read a byte
	 * @param[in] idx NVM Address to read
	 * @return Byte from given address
	 */
	uint8_t read(const uint16_t idx);
	//----------------------------------------------------------------------------
	/** Write a block
	 * @param[in] *src Read bytes from given pointer
	 * @param[in] idx  First NVM address to write
	 * @param[in] n    Number of bytes to write
	 * @return success
	 */
	bool write_block(uint8_t *src, uint16_t idx, uint16_t n);
	//----------------------------------------------------------------------------
	/** Write a byte
	 * @param[in] idx NVM Address to write
	 * @param[in] value Byte to write
	 * @return success
	 */
	bool write(const uint16_t idx, uint8_t value);
	//----------------------------------------------------------------------------
	/** Preserve the number of bytes in NVM log for time critical writes. Runtime
	 *  up to 5000ms!
	 * @param[in] number Bytes to preserve, can 0 to find out free log space
	 * @return Bytes available for writing
	 */
	int write_prepare(uint16_t number);
	//----------------------------------------------------------------------------
	/** lear log if full and prepare released pages for faster reallocation.
	 * Runtime up to 5000ms!
	 * @param[in] write_preserve Byte to preserve
	 */
	void clean_up(uint16_t write_preserve);

private:
	// Return a virtual page
	uint32_t *get_page();
	// Get actual log position
	uint16_t get_log_position(uint32_t *vpage);
	// Read a byte from page
	uint8_t get_byte_from_page(uint32_t *vpage, uint16_t log_start,
	                           uint16_t log_end, uint16_t idx);
	// switch a page
	uint32_t *switch_page(uint32_t *old_vpage, uint16_t *log_start,
	                      uint16_t *log_end);
};

/** Variable to access the NVRAMClass */
extern NVRAMClass NVRAM;

/** @} */
