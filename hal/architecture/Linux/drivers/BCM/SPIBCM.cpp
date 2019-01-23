/*
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2019 Sensnology AB
 * Full contributor list: https://github.com/mysensors/MySensors/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * Based on TMRh20 RF24 library, Copyright (c) 2015 Charles-Henri Hallard <tmrh20@gmail.com>
 */

#include "SPIBCM.h"
#include <pthread.h>
#include <stdlib.h>
#include "log.h"

static pthread_mutex_t spiMutex = PTHREAD_MUTEX_INITIALIZER;

// Declare a single default instance
SPIBCMClass SPIBCM = SPIBCMClass();

uint8_t SPIBCMClass::initialized = 0;

void SPIBCMClass::begin()
{
	if (!initialized) {
		if (!BCM.isInitialized()) {
			BCM.init();
		}
		if (!bcm2835_spi_begin()) {
			logError("You need root privilege to use SPI.\n");
			exit(1);
		}
	}

	initialized++; // reference count
}

void SPIBCMClass::end()
{
	if (initialized) {
		initialized--;
	}

	if (!initialized) {
		// End the SPI
		bcm2835_spi_end();
	}
}

void SPIBCMClass::setBitOrder(uint8_t bit_order)
{
	bcm2835_spi_setBitOrder(bit_order);
}

void SPIBCMClass::setDataMode(uint8_t data_mode)
{
	bcm2835_spi_setDataMode(data_mode);
}

void SPIBCMClass::setClockDivider(uint16_t divider)
{
	bcm2835_spi_setClockDivider(divider);
}

void SPIBCMClass::chipSelect(int csn_pin)
{
	if (csn_pin == RPI_GPIO_P1_26) {
		csn_pin = BCM2835_SPI_CS1;
	} else if (csn_pin == RPI_GPIO_P1_24) {
		csn_pin = BCM2835_SPI_CS0;
	} else {
		csn_pin = BCM2835_SPI_CS0;
	}
	bcm2835_spi_chipSelect(csn_pin);
	delayMicroseconds(5);
}

void SPIBCMClass::beginTransaction(SPISettings settings)
{
	pthread_mutex_lock(&spiMutex);
	setBitOrder(settings.border);
	setDataMode(settings.dmode);
	setClockDivider(settings.cdiv);
}

void SPIBCMClass::endTransaction()
{
	pthread_mutex_unlock(&spiMutex);
}

void SPIBCMClass::usingInterrupt(uint8_t interruptNumber)
{
	(void)interruptNumber;
}

void SPIBCMClass::notUsingInterrupt(uint8_t interruptNumber)
{
	(void)interruptNumber;
}
