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

#include "GPIO.h"
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include "log.h"

// Declare a single default instance
GPIOClass GPIO = GPIOClass();

GPIOClass::GPIOClass()
{
	FILE *f;
	DIR* dp;
	char file[64];

	dp = opendir("/sys/class/gpio");
	if (dp == NULL) {
		logError("Could not open /sys/class/gpio directory");
		exit(1);
	}

	lastPinNum = 0;

	while (true) {
		dirent *de = readdir(dp);
		if (de == NULL) {
			break;
		}

		if (strncmp("gpiochip", de->d_name, 8) == 0) {
			sprintf(file, "/sys/class/gpio/%s/base", de->d_name);
			f = fopen(file, "r");
			int base;
			if (fscanf(f, "%d", &base) == EOF) {
				logError("Failed to open %s\n", file);
				base = 0;
			}
			fclose(f);

			sprintf(file, "/sys/class/gpio/%s/ngpio", de->d_name);
			f = fopen(file, "r");
			int ngpio;
			if (fscanf(f, "%d", &ngpio) == EOF) {
				logError("Failed to open %s\n", file);
				ngpio = 0;
			}
			fclose(f);

			int max = ngpio + base - 1;
			if (lastPinNum < max) {
				lastPinNum = max;
			}
		}
	}
	closedir(dp);

	exportedPins = new uint8_t[lastPinNum + 1];

	for (int i = 0; i < lastPinNum + 1; ++i) {
		exportedPins[i] = 0;
	}
}

GPIOClass::GPIOClass(const GPIOClass& other)
{
	lastPinNum = other.lastPinNum;

	exportedPins = new uint8_t[lastPinNum + 1];
	for (int i = 0; i < lastPinNum + 1; ++i) {
		exportedPins[i] = other.exportedPins[i];
	}
}

GPIOClass::~GPIOClass()
{
	FILE *f;

	for (int i = 0; i < lastPinNum + 1; ++i) {
		if (exportedPins[i]) {
			f = fopen("/sys/class/gpio/unexport", "w");
			fprintf(f, "%d\n", i);
			fclose(f);
		}
	}

	delete [] exportedPins;
}

void GPIOClass::pinMode(uint8_t pin, uint8_t mode)
{
	FILE *f;

	if (pin > lastPinNum) {
		return;
	}

	f = fopen("/sys/class/gpio/export", "w");
	fprintf(f, "%d\n", pin);
	fclose(f);

	int counter = 0;
	char file[128];
	sprintf(file, "/sys/class/gpio/gpio%d/direction", pin);

	while ((f = fopen(file,"w")) == NULL) {
		// Wait 10 seconds for the file to be accessible if not open on first attempt
		sleep(1);
		counter++;
		if (counter > 10) {
			logError("Could not open /sys/class/gpio/gpio%u/direction", pin);
			exit(1);
		}
	}
	if (mode == INPUT) {
		fprintf(f, "in\n");
	} else {
		fprintf(f, "out\n");
	}

	exportedPins[pin] = 1;

	fclose(f);
}

void GPIOClass::digitalWrite(uint8_t pin, uint8_t value)
{
	FILE *f;
	char file[128];

	if (pin > lastPinNum) {
		return;
	}
	if (0 == exportedPins[pin]) {
		pinMode(pin, OUTPUT);
	}

	sprintf(file, "/sys/class/gpio/gpio%d/value", pin);
	f = fopen(file, "w");

	if (value == 0)	{
		fprintf(f, "0\n");
	} else {
		fprintf(f, "1\n");
	}

	fclose(f);
}

uint8_t GPIOClass::digitalRead(uint8_t pin)
{
	FILE *f;
	char file[128];

	if (pin > lastPinNum) {
		return 0;
	}
	if (0 == exportedPins[pin]) {
		pinMode(pin, INPUT);
	}

	sprintf(file, "/sys/class/gpio/gpio%d/value", pin);
	f = fopen(file, "r");

	int i;
	if (fscanf(f, "%d", &i) == EOF) {
		logError("digitalRead: failed to read pin %u\n", pin);
		i = 0;
	}
	fclose(f);
	return i;
}

uint8_t GPIOClass::digitalPinToInterrupt(uint8_t pin)
{
	return pin;
}

GPIOClass& GPIOClass::operator=(const GPIOClass& other)
{
	if (this != &other) {
		lastPinNum = other.lastPinNum;

		exportedPins = new uint8_t[lastPinNum + 1];
		for (int i = 0; i < lastPinNum + 1; ++i) {
			exportedPins[i] = other.exportedPins[i];
		}
	}
	return *this;
}
