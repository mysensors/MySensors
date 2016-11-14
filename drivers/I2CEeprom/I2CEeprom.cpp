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

I2CEeprom::I2CEeprom(uint8_t addr)
{
  m_addr = addr;
}

// setup Wire
boolean I2CEeprom::initialize()
{
	Wire.begin();
	#ifdef I2CEEPROM_TWI_CLK
		Wire.setClock(I2CEEPROM_TWI_CLK);
	#endif

	delay(1); // let the bus settle

	// try to access
	return busy() ? false : true; 
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
    uint8_t *data = (uint8_t *)buf;
	
    while (len > 0) {
        uint16_t nump = I2CEEPROM_PAGE_SIZE - ( addr & (I2CEEPROM_PAGE_SIZE - 1) ); // bytes at current page
        uint16_t num = len < nump ? len : nump;
		if(num > BUFFER_LENGTH)	// too large for Wire?
			num = BUFFER_LENGTH;
			
        Wire.beginTransmission(m_addr);		// initiate addr write
        Wire.write((uint8_t)(addr >> 8));	// MSB
        Wire.write((uint8_t)addr);			// LSB
        if (Wire.endTransmission() != 0){
			return;
        }

        Wire.requestFrom(m_addr, num);
        for (uint8_t i=0; i<num; i++)
			data[i] = Wire.read();

        // next page
		addr += num;
        data += num;
        len -= num;
    }
}

/// check if the chip is busy
boolean I2CEeprom::busy()
{
	Wire.beginTransmission(m_addr);
    Wire.write(0);
    Wire.write(0);
    if (Wire.endTransmission() == 0)
		return false;
	else
		return true;	// busy
}

/// Write 1 byte
void I2CEeprom::writeByte(uint32_t addr, uint8_t byt)
{  
  writeBytes(addr, &byt, 1);
}

/// write multiple bytes
void I2CEeprom::writeBytes(uint32_t addr, const void* buf, uint16_t len)
{
    uint16_t num;
	uint8_t *data = (uint8_t *)buf;
    uint16_t nump;

	while (len > 0) {
        nump = I2CEEPROM_PAGE_SIZE - (addr & (I2CEEPROM_PAGE_SIZE - 1)); // bytes at current page
        num = len < nump ? len : nump;
		if(num > (BUFFER_LENGTH-2))	// too large for Wire buffer?
			num = BUFFER_LENGTH-2;
        
        Wire.beginTransmission(m_addr);	// initiate write
        Wire.write(addr >> 8);			// MSB
        Wire.write(addr);				// LSB
        Wire.write(data, num);			// data
        if (Wire.endTransmission() != 0)
			return;

        // wait for write to complete
        for (uint8_t t=I2CEEPROM_WR_LIMIT; t; t--) {
            delay(1);
            if(!busy())
				return;
        }

        // next page
		addr += num;
        data += num;
        len -= num;
    }
}
