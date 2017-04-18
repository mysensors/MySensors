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
#include <string.h>
#include "log.h"

const static int pin_to_gpio_rev1[41] = {-1, -1, -1, 0, -1, 1, -1, 4, 14, -1, 15, 17, 18, 21, -1, 22, 23, -1, 24, 10, -1, 9, 25, 11, 8, -1, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };
const static int pin_to_gpio_rev2[41] = {-1, -1, -1, 2, -1, 3, -1, 4, 14, -1, 15, 17, 18, 27, -1, 22, 23, -1, 24, 10, -1, 9, 25, 11, 8, -1, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1 };
const static int pin_to_gpio_rev3[41] = {-1, -1, -1, 2, -1, 3, -1, 4, 14, -1, 15, 17, 18, 27, -1, 22, 23, -1, 24, 10, -1, 9, 25, 11, 8, -1, 7, -1, -1, 5, -1, 6, 12, 13, -1, 19, 16, 26, 20, -1, 21 };

// Declare a single default instance
RPiClass RPi = RPiClass();

const int* RPiClass::pin_to_gpio = NULL;
#ifndef RPI_TYPE
#error Raspberry Pi type not set
#endif
const char* RPiClass::type = RPI_TYPE;

void RPiClass::pinMode(uint8_t physPin, uint8_t mode)
{
	uint8_t gpioPin;

	if (physToGPIO(physPin, &gpioPin) != 0) {
		logError("pinMode: invalid pin: %d\n", physPin);
		return;
	}

	BCM.pinMode(gpioPin, mode);
}

void RPiClass::digitalWrite(uint8_t physPin, uint8_t value)
{
	uint8_t gpioPin;

	if (physToGPIO(physPin, &gpioPin) != 0) {
		logError("digitalWrite: invalid pin: %d\n", physPin);
		return;
	}

	BCM.digitalWrite(gpioPin, value);
}

uint8_t RPiClass::digitalRead(uint8_t physPin)
{
	uint8_t gpioPin;

	if (physToGPIO(physPin, &gpioPin) != 0) {
		logError("digitalRead: invalid pin: %d\n", physPin);
		return 0;
	}

	return BCM.digitalRead(gpioPin);
}

uint8_t RPiClass::digitalPinToInterrupt(uint8_t physPin)
{
	uint8_t gpioPin;

	if (physToGPIO(physPin, &gpioPin) != 0) {
		logError("digitalPinToInterrupt: invalid pin: %d\n", physPin);
		return 0;
	}

	return gpioPin;
}

int RPiClass::physToGPIO(uint8_t physPin, uint8_t *gpio)
{
	if (pin_to_gpio == NULL) {
		if (strncmp(type,"rpi1", 4) == 0) {
			pin_to_gpio = &pin_to_gpio_rev1[0];
		} else if (strncmp(type,"rpi2", 4) == 0) {
			pin_to_gpio = &pin_to_gpio_rev2[0];
		} else if (strncmp(type,"rpi3", 4) == 0) {
			pin_to_gpio = &pin_to_gpio_rev3[0];
		} else {
			logError("Invalid Raspberry Pi type!\n");
			exit(1);
		}
	}

	if (gpio == NULL || physPin > 40) {
		return -1;
	}

	int pin = *(pin_to_gpio+physPin);
	if (pin == -1) {
		return -1;
	} else {
		*gpio = pin;
	}

	return 0;
}
