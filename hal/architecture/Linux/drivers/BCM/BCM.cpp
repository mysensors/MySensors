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
 */

#include "BCM.h"
#include <stdlib.h>
#include "log.h"

// Declare a single default instance
BCMClass BCM = BCMClass();

uint8_t BCMClass::initialized = 0;

BCMClass::~BCMClass()
{
	if (initialized) {
		bcm2835_close();
		initialized = 0;
	}
}

uint8_t BCMClass::init()
{
	if (!bcm2835_init()) {
		logError("Failed to initialized bcm2835.\n");
		exit(1);
	}
	initialized = 1;

	return 1;
}

void BCMClass::pinMode(uint8_t gpio, uint8_t mode)
{
	if (!initialized) {
		init();
	}

	bcm2835_gpio_fsel(gpio, mode);
}

void BCMClass::digitalWrite(uint8_t gpio, uint8_t value)
{
	if (!initialized) {
		init();
	}

	bcm2835_gpio_write(gpio, value);
	// Delay to allow any change in state to be reflected in the LEVn, register bit.
	delayMicroseconds(1);
}

uint8_t BCMClass::digitalRead(uint8_t gpio)
{
	if (!initialized) {
		init();
	}

	return bcm2835_gpio_lev(gpio);
}

uint8_t BCMClass::isInitialized()
{
	return initialized;
}
