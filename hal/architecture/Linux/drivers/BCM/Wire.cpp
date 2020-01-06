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

#include "Wire.h"
#include <stdlib.h>
#include <pthread.h>
#include "bcm2835.h"
#include "log.h"

static pthread_mutex_t i2cMutex = PTHREAD_MUTEX_INITIALIZER;

uint8_t TwoWire::rxBuffer[BUFFER_LENGTH];
uint8_t TwoWire::rxBufferIndex = 0;
uint8_t TwoWire::rxBufferLength = 0;

uint8_t TwoWire::txAddress = 0;
uint8_t TwoWire::txBuffer[BUFFER_LENGTH];
uint8_t TwoWire::txBufferIndex = 0;
uint8_t TwoWire::txBufferLength = 0;

uint8_t TwoWire::transmitting = 0;

void TwoWire::begin()
{
	if (!bcm2835_i2c_begin()) {
		logError("You need root privilege to use I2C.\n");
		exit(1);
	}
}

void TwoWire::begin(uint8_t address)
{
	begin();
	bcm2835_i2c_setSlaveAddress(address);
}

void TwoWire::begin(int address)
{
	begin(static_cast<uint8_t>(address));
}

void TwoWire::end()
{
	bcm2835_i2c_end();
}

void TwoWire::setClock(uint32_t clock)
{
	bcm2835_i2c_set_baudrate(clock);
}

void TwoWire::beginTransmission(uint8_t address)
{
	pthread_mutex_lock(&i2cMutex);
	// indicate that we are transmitting
	transmitting = 1;
	// set address of targeted slave
	txAddress = address;
	// reset tx buffer iterator vars
	txBufferIndex = 0;
	txBufferLength = 0;
}

void TwoWire::beginTransmission(int address)
{
	beginTransmission(static_cast<uint8_t>(address));
}

uint8_t TwoWire::endTransmission(void)
{
	// transmit buffer
	bcm2835_i2c_setSlaveAddress(txAddress);
	uint8_t ret = bcm2835_i2c_write(reinterpret_cast<const char *>(txBuffer), txBufferLength);

	// reset tx buffer iterator vars
	txBufferIndex = 0;
	txBufferLength = 0;
	// indicate that we are done transmitting
	transmitting = 0;

	pthread_mutex_unlock(&i2cMutex);

	if (ret == BCM2835_I2C_REASON_OK) {
		return 0;	// success
	} else if (ret == BCM2835_I2C_REASON_ERROR_NACK) {
		return 3;	// error: data send, nack received
	}
	return 4;	// other error
}

size_t TwoWire::requestFrom(uint8_t address, size_t quantity)
{
	// clamp to buffer length
	if (quantity > BUFFER_LENGTH) {
		quantity = BUFFER_LENGTH;
	}

	rxBufferIndex = 0;
	rxBufferLength = 0;

	bcm2835_i2c_setSlaveAddress(address);
	uint8_t ret = bcm2835_i2c_read(reinterpret_cast<char *>(rxBuffer), quantity);
	if (ret == BCM2835_I2C_REASON_OK) {
		rxBufferLength = quantity;
	}

	return rxBufferLength;
}

uint8_t TwoWire::requestFrom(uint8_t address, uint8_t quantity)
{
	return requestFrom(address, static_cast<size_t>(quantity));
}

uint8_t TwoWire::requestFrom(int address, int quantity)
{
	return requestFrom(static_cast<uint8_t>(address), static_cast<size_t>(quantity));
}

size_t TwoWire::write(uint8_t data)
{
	if (transmitting) {
		// in master transmitter mode
		// don't bother if buffer is full
		if (txBufferLength >= BUFFER_LENGTH) {
			setWriteError();
			return 0;
		}
		// put byte in tx buffer
		txBuffer[txBufferIndex] = data;
		++txBufferIndex;
		// update amount in buffer
		txBufferLength = txBufferIndex;

		return 1;
	} else {
		return write(&data, 1);
	}
}

size_t TwoWire::write(const uint8_t *data, size_t quantity)
{
	if (transmitting) {
		// in master transmitter mode
		for (size_t i = 0; i < quantity; ++i) {
			write(data[i]);
		}
	} else {
		uint8_t rc = bcm2835_i2c_write(reinterpret_cast<const char *>(data), quantity);
		if (rc != BCM2835_I2C_REASON_OK) {
			return 0;
		}
	}

	return quantity;
}

int TwoWire::available()
{
	return rxBufferLength - rxBufferIndex;
}

int TwoWire::read()
{
	int value = -1;

	if (rxBufferIndex < rxBufferLength) {
		value = rxBuffer[rxBufferIndex];
		++rxBufferIndex;
	}

	return value;
}

int TwoWire::peek()
{
	if (rxBufferIndex < rxBufferLength) {
		return rxBuffer[rxBufferIndex];
	}

	return -1;
}

void TwoWire::flush()
{
	rxBufferIndex = 0;
	rxBufferLength = 0;
	txBufferIndex = 0;
	txBufferLength = 0;
}

TwoWire Wire = TwoWire();
