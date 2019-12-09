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

#include "RPi.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "log.h"

const static int phys_to_gpio_rev1[41] = {-1, -1, -1, 0, -1, 1, -1, 4, 14, -1, 15, 17, 18, 21, -1, 22, 23, -1, 24, 10, -1, 9, 25, 11, 8, -1, 7, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
const static int phys_to_gpio_rev2[41] = {-1, -1, -1, 2, -1, 3, -1, 4, 14, -1, 15, 17, 18, 27, -1, 22, 23, -1, 24, 10, -1, 9, 25, 11, 8, -1, 7, -1, -1, 5, -1, 6, 12, 13, -1, 19, 16, 26, 20, -1, 21};

// Declare a single default instance
RPiClass RPi = RPiClass();

const int* RPiClass::phys_to_gpio = NULL;

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

int RPiClass::rpiGpioLayout()
{
	/*
	* Based on wiringPi Copyright (c) 2012 Gordon Henderson.
	*/
	FILE *fd;
	char line[120];
	char *c;

	if ((fd = fopen("/proc/cpuinfo", "r")) == NULL) {
		return -1;
	}

	while (fgets(line, 120, fd) != NULL) {
		if (strncmp(line, "Revision", 8) == 0) {
			fclose(fd);
			// Chop trailing CR/NL
			for (c = &line[strlen(line) - 1]; (*c == '\n') || (*c == '\r'); --c) {
				*c = 0;
			}
			// Scan to the first character of the revision number
			for (c = line; *c; ++c) {
				if (*c == ':') {
					// Chop spaces
					++c;
					while (isspace(*c)) {
						++c;
					}

					// Check hex digit at start
					if (!isxdigit(*c)) {
						return -1;
					}

					// Check bogus revision line (too small)
					if (strlen(c) < 4) {
						return -1;
					}

					// Isolate last 4 characters: (in-case of overvolting or new encoding scheme)
					c = c + strlen(c) - 4;

					if ((strcmp(c, "0002") == 0) || (strcmp(c, "0003") == 0) ||
					        (strcmp(c, "0004") == 0) || (strcmp(c, "0005") == 0) ||
					        (strcmp(c, "0006") == 0) || (strcmp(c, "0007") == 0) ||
					        (strcmp(c, "0008") == 0) || (strcmp(c, "0009") == 0) ||
					        (strcmp(c, "000d") == 0) || (strcmp(c, "000e") == 0) ||
					        (strcmp(c, "000f") == 0)) {
						return 1;
					} else {
						return 2;
					}
				}
			}
		}
	}
	fclose(fd);
	return -1;
}

int RPiClass::physToGPIO(uint8_t physPin, uint8_t *gpio)
{
	if (phys_to_gpio == NULL) {
		if (rpiGpioLayout() == 1) {
			// A, B, Rev 1, 1.1
			phys_to_gpio = &phys_to_gpio_rev1[0];
		} else {
			// A2, B2, A+, B+, CM, Pi2, Pi3, Zero
			phys_to_gpio = &phys_to_gpio_rev2[0];
		}
	}

	if (gpio == NULL || physPin > 40) {
		return -1;
	}

	int pin = *(phys_to_gpio+physPin);
	if (pin == -1) {
		return -1;
	} else {
		*gpio = pin;
	}

	return 0;
}
