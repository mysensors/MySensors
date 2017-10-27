/*
  TwoWire.h - TWI/I2C library for Arduino & Wiring
  Copyright (c) 2006 Nicholas Zambetti.  All right reserved.

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

  Modified 2012 by Todd Krein (todd@krein.org) to implement repeated starts
  Modified December 2014 by Ivan Grokhotkov (ivan@esp8266.com) - esp8266 support
  Modified April 2015 by Hrsto Gochkov (ficeto@ficeto.com) - alternative esp8266 support
  Modified October 2016 by Marcelo Aquino <marceloaqno@gmail.org> for Raspberry Pi
*/

#ifndef Wire_h
#define Wire_h

#if !DOXYGEN
#include <stdint.h>
#include "Stream.h"
#include "BCM.h"

#define BUFFER_LENGTH 32

class TwoWire : public Stream
{

private:
	static uint8_t rxBuffer[];
	static uint8_t rxBufferIndex;
	static uint8_t rxBufferLength;

	static uint8_t txAddress;
	static uint8_t txBuffer[];
	static uint8_t txBufferIndex;
	static uint8_t txBufferLength;

	static uint8_t transmitting;

public:
	void begin();
	void begin(uint8_t address);
	void begin(int address);
	void end();
	void setClock(uint32_t clock);

	void beginTransmission(uint8_t address);
	void beginTransmission(int address);
	uint8_t endTransmission(void);

	size_t requestFrom(uint8_t address, size_t size);
	uint8_t requestFrom(uint8_t address, uint8_t quantity);
	uint8_t requestFrom(int address, int quantity);

	size_t write(uint8_t data);
	size_t write(const uint8_t *data, size_t quantity);
	int available();
	int read();
	int peek();
	void flush();

	inline size_t write(unsigned long n)
	{
		return write((uint8_t)n);
	}
	inline size_t write(long n)
	{
		return write((uint8_t)n);
	}
	inline size_t write(unsigned int n)
	{
		return write((uint8_t)n);
	}
	inline size_t write(int n)
	{
		return write((uint8_t)n);
	}
	using Print::write;
};

extern TwoWire Wire;

#endif
#endif
