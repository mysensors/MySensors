// Copyright (C) 2016 Krister W. <kisse66@hobbylabs.org>
//
// Original SPI flash driver this is based on:
// Copyright (c) 2013-2015 by Felix Rusu, LowPowerLab.com
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

#include <Wire.h>
#include "I2CEeprom.h"

I2CEeprom::I2CEeprom(uint8_t addr) : extEEPROM(I2CEEPROM_CHIP_SIZE, 1, I2CEEPROM_PAGE_SIZE)
{
	m_addr = addr;    // we only need this for busy()
}

/// setup
bool I2CEeprom::initialize()
{
	return extEEPROM::begin(I2CEEPROM_TWI_CLK) ? false : true;
}

/// read 1 byte
uint8_t I2CEeprom::readByte(uint32_t addr)
{
	uint8_t val;
	readBytes(addr, &val, 1);
	return val;
}

/// read multiple bytes
void I2CEeprom::readBytes(uint32_t addr, void* buf, uint16_t len)
{
	extEEPROM::read((unsigned long)addr, (byte *)buf, (unsigned int) len);
}

/// check if the chip is busy
bool I2CEeprom::busy()
{
	Wire.beginTransmission(m_addr);
	Wire.write(0);
	Wire.write(0);
	if (Wire.endTransmission() == 0) {
		return false;
	} else {
		return true;    // busy
	}
}

/// Write 1 byte
void I2CEeprom::writeByte(uint32_t addr, uint8_t byt)
{
	writeBytes(addr, &byt, 1);
}

/// write multiple bytes
void I2CEeprom::writeBytes(uint32_t addr, const void* buf, uint16_t len)
{
	extEEPROM::write((unsigned long) addr, (byte *)buf, (unsigned int) len);
}
