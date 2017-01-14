/*
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2017 Sensnology AB
 * Full contributor list: https://github.com/mysensors/Arduino/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

#include "RPi.h"
#include <stdlib.h>
#include "log.h"

const static int pin_to_gpio_rev1[41] = {-1, -1, -1, 0, -1, 1, -1, 4, 14, -1, 15, 17, 18, 21, -1, 22, 23, -1, 24, 10, -1, 9, 25, 11, 8, -1, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };
const static int pin_to_gpio_rev2[41] = {-1, -1, -1, 2, -1, 3, -1, 4, 14, -1, 15, 17, 18, 27, -1, 22, 23, -1, 24, 10, -1, 9, 25, 11, 8, -1, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };
const static int pin_to_gpio_rev3[41] = {-1, -1, -1, 2, -1, 3, -1, 4, 14, -1, 15, 17, 18, 27, -1, 22, 23, -1, 24, 10, -1, 9, 25, 11, 8, -1, 7, -1, -1, 5, -1, 6, 12, 13, -1, 19, 16, 26, 20, -1, 21 };

// Declare a single default instance
RPi rpi = RPi();

const int* RPi::pin_to_gpio = NULL;
rpi_info RPi::rpiinfo;

RPi::RPi()
{
	if (!bcm2835_init()) {
		logError("Failed to initialized bcm2835.\n");
		exit(1);
	}
}

RPi::~RPi()
{
	bcm2835_close();
}

void RPi::pinMode(uint8_t physPin, uint8_t mode)
{
	uint8_t gpioPin;

	if (physToGPIO(physPin, &gpioPin)) {
		logError("pinMode: invalid pin: %d\n", physPin);
		return;
	}

	bcm2835_gpio_fsel(gpioPin, mode);
}

void RPi::digitalWrite(uint8_t physPin, uint8_t value)
{
	uint8_t gpioPin;

	if (physToGPIO(physPin, &gpioPin)) {
		logError("digitalWrite: invalid pin: %d\n", physPin);
		return;
	}

	bcm2835_gpio_write(gpioPin, value);
	// Delay to allow any change in state to be reflected in the LEVn, register bit.
	delayMicroseconds(1);
}

uint8_t RPi::digitalRead(uint8_t physPin)
{
	uint8_t gpioPin;

	if (physToGPIO(physPin, &gpioPin)) {
		logError("digitalRead: invalid pin: %d\n", physPin);
		return 0;
	}

	return bcm2835_gpio_lev(gpioPin);
}

uint8_t RPi::digitalPinToInterrupt(uint8_t physPin)
{
	uint8_t gpioPin;

	if (physToGPIO(physPin, &gpioPin)) {
		logError("digitalPinToInterrupt: invalid pin: %d\n", physPin);
		return 0;
	}

	return gpioPin;
}

int RPi::physToGPIO(uint8_t physPin, uint8_t *gpio)
{
	if (pin_to_gpio == NULL) {
		// detect board revision and set up accordingly
		if (get_rpi_info(&rpiinfo)) {
			logError("This module can only be run on a Raspberry Pi!\n");
			exit(1);
		}

		if (rpiinfo.p1_revision == 1) {
			pin_to_gpio = &pin_to_gpio_rev1[0];
		} else if (rpiinfo.p1_revision == 2) {
			pin_to_gpio = &pin_to_gpio_rev2[0];
		} else { // assume model B+ or A+ or 2B
			pin_to_gpio = &pin_to_gpio_rev3[0];
		}
	}

	if ((rpiinfo.p1_revision != 3 && physPin > 26)
	        || (rpiinfo.p1_revision == 3 && physPin > 40)) {
		return -1;
	}

	if (*(pin_to_gpio+physPin) == -1) {
		return -1;
	} else {
		*gpio = *(pin_to_gpio+physPin);
	}

	return 0;
}
