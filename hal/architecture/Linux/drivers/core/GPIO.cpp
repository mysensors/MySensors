/*
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2022 Sensnology AB
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
#include <gpiod.h>
#include "log.h"
// Chip 0 on older Pi models, chip 4 on Pi 5.
#define CHIP "/dev/gpiochip0"
// Declare a single default instance
GPIOClass GPIO = GPIOClass();

GPIOClass::GPIOClass()
{
	struct gpiochip_info chip_info;
	struct gpiod_line_request handle_request;


	int file_descriptor = open(CHIP, O_RDONLY);

	if (file_descriptor < 0) {
		printf("Failed opening GPIO chip.\n");
		return 1;
	}

	res = ioctl(file_descriptor, GPIO_GET_CHIPINFO_IOCTL, &chip_info);

	if (res < 0) {
		printf("Failed getting chip information.\n");
		close(file_descriptor);
		return 1;
	}


	printf("GPIO chip information:\n");
	printf("name: %s\n",chip_info.name);
	printf("label: %s\n",chip_info.label);
	printf("lines: %i\n", chip_info.lines);

	for (int i = 0; i < chip_info.lines; i++) {

		struct gpioline_info line_info;

		line_info.line_offset = i;

		if (ioctl(file_descriptor, GPIO_GET_LINEINFO_IOCTL, &line_info) < 0) {
			printf("Failed getting line %i info.\n", i);
		} else {
			printf("%d %s\n",i,line_info.name);
		}
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

// destructor
GPIOClass::~GPIOClass()
{
	close(handle_request.fd);

	// Tell the thread to finish
	// TERM = 1;

	// Give it time
	sleep(1);

	// Close resources
	close(file_descriptor);

	// Rejoin main thread with finished thread
	pthread_join(reader_thread,NULL);

	return 0;

}

void GPIOClass::pinMode(uint8_t pin, uint8_t mode)
{
	// Request handle on writing line. Many can be requested instead of one.

	handle_request.lineoffsets[0] = pin;
	handle_request.flags =
	    (mode==INPUT?GPIOHANDLE_REQUEST_INPUT:GPIOHANDLE_REQUEST_OUTPUT;
	     //| GPIOHANDLE_REQUEST_BIAS_PULL_DOWN;
	     handle_request.lines = 1;

	     res = ioctl(file_descriptor, GPIO_GET_LINEHANDLE_IOCTL, &handle_request);

	if (res < 0) {
	printf("Failed requesting write handle.\n");
		close(file_descriptor);
		return 1;
	}
}

void GPIOClass::digitalWrite(uint8_t pin, uint8_t value)
{

	// Request handle on writing line. Many can be requested instead of one.

	handle_request.lineoffsets[0] = WRITE_GPIO;
	handle_request.flags = GPIOHANDLE_REQUEST_OUTPUT | GPIOHANDLE_REQUEST_BIAS_PULL_DOWN;
	handle_request.lines = 1;

	gpiod_line_value line_value = (value==0?GPIOD_LINE_VALUE_INACTIVE:GPIOD_LINE_VALUE_ACTIVE);

	res = gpio_line_request_set_value(handle_request,0,line_value);

	if (res < 0) {
		printf("Failed requesting write handle.\n");
		return 1;
	}
}

uint8_t GPIOClass::digitalRead(uint8_t pin)
{

	// Request handle on line. Many can be requested instead of one.

	handle_request.lineoffsets[0] = pin;
	handle_request.flags = GPIOHANDLE_REQUEST_INPUT;
	handle_request.lines = 1;

	res = gpiod_line_request_get_value(handle_request,0);

	if (res < 0) {
		printf("Failed requesting write handle.\n");
		return 1;
	}
	return res;
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
