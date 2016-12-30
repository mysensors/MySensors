/*
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Marcelo Aquino <marceloaqno@gmail.org>
 * Copyright (C) 2016 Marcelo Aquino
 * Full contributor list: https://github.com/mysensors/MySensors/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * Based on wiringPi Copyright (c) 2012 Gordon Henderson.
 */

#include "rpi_util.h"
#include <pthread.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <unistd.h>
#include <poll.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stropts.h>
#include <errno.h>
#include "SPI.h"
#include "log.h"
#include "cpuinfo.h"

extern "C" {
	int piHiPri(const int pri);
}

struct ThreadArgs {
	void (*func)();
	int gpioPin;
};

static const int *pin_to_gpio = 0;
static rpi_info rpiinfo;

volatile bool interruptsEnabled = true;
static pthread_mutex_t intMutex = PTHREAD_MUTEX_INITIALIZER;

static pthread_t *threadIds[64] = {NULL};

// sysFds:
//	Map a file descriptor from the /sys/class/gpio/gpioX/value
static int sysFds[64] = {
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
};

int get_gpio_number(uint8_t physPin, uint8_t *gpio)
{
	if (pin_to_gpio == 0) {
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

void *interruptHandler(void *args)
{
	int fd;
	struct pollfd polls;
	char c;
	struct ThreadArgs *arguments = (struct ThreadArgs *)args;
	int gpioPin = arguments->gpioPin;
	void (*func)() = arguments->func;
	delete arguments;

	(void)piHiPri(55);	// Only effective if we run as root

	if ((fd = sysFds[gpioPin]) == -1) {
		logError("Failed to attach interrupt for pin %d\n", gpioPin);
		return NULL;
	}

	// Setup poll structure
	polls.fd     = fd;
	polls.events = POLLPRI;

	while (1) {
		// Wait for it ...
		int ret = poll(&polls, 1, -1);
		if (ret < 0) {
			logError("Error waiting for interrupt: %s\n", strerror(errno));
			break;
		}
		// Do a dummy read to clear the interrupt
		//	A one character read appars to be enough.
		//	Followed by a seek to reset it.
		(void)read (fd, &c, 1) ;
		lseek (fd, 0, SEEK_SET) ;
		// Call user function.
		pthread_mutex_lock(&intMutex);
		if (interruptsEnabled) {
			pthread_mutex_unlock(&intMutex);
			func();
		} else {
			pthread_mutex_unlock(&intMutex);
		}
	}

	close(fd);

	return NULL;
}

void rpi_util::pinMode(uint8_t physPin, uint8_t mode)
{
	uint8_t gpioPin;

	if (get_gpio_number(physPin, &gpioPin)) {
		logError("pinMode: invalid pin: %d\n", physPin);
		return;
	}

	// Check if SPI is in use and target pin is related to SPI
	if (SPIClass::is_initialized() && gpioPin >= RPI_GPIO_P1_26 && gpioPin <= RPI_GPIO_P1_23) {
		return;
	} else {
		bcm2835_gpio_fsel(gpioPin, mode);
	}
}

void rpi_util::digitalWrite(uint8_t physPin, uint8_t value)
{
	uint8_t gpioPin;

	if (get_gpio_number(physPin, &gpioPin)) {
		logError("digitalWrite: invalid pin: %d\n", physPin);
		return;
	}

	// Check if SPI is in use and target pin is related to SPI
	if (SPIClass::is_initialized() && gpioPin >= RPI_GPIO_P1_26 && gpioPin <= RPI_GPIO_P1_23) {
		if (value == LOW && (gpioPin == RPI_GPIO_P1_24 || gpioPin == RPI_GPIO_P1_26)) {
			SPI.chipSelect(gpioPin);
		}
	} else {
		bcm2835_gpio_write(gpioPin, value);
		// Delay to allow any change in state to be reflected in the LEVn, register bit.
		delayMicroseconds(1);
	}
}

uint8_t rpi_util::digitalRead(uint8_t physPin)
{
	uint8_t gpioPin;

	if (get_gpio_number(physPin, &gpioPin)) {
		logError("digitalRead: invalid pin: %d\n", physPin);
		return 0;
	}

	// Check if SPI is in use and target pin is related to SPI
	if (SPIClass::is_initialized() && gpioPin >= RPI_GPIO_P1_26 && gpioPin <= RPI_GPIO_P1_23) {
		return 0;
	} else {
		return bcm2835_gpio_lev(gpioPin);
	}
}

void rpi_util::attachInterrupt(uint8_t physPin, void (*func)(), uint8_t mode)
{
	FILE *fd;
	char fName[40];
	char c;
	int count, i;
	uint8_t gpioPin;

	if (get_gpio_number(physPin, &gpioPin)) {
		logError("attachInterrupt: invalid pin: %d\n", physPin);
		return;
	}

	if (threadIds[gpioPin] == NULL) {
		threadIds[gpioPin] = new pthread_t;
	} else {
		// Cancel the existing thread for that pin
		pthread_cancel(*threadIds[gpioPin]);
		// Wait a bit
		delay(1L);
	}

	// Export pin for interrupt
	if ((fd = fopen("/sys/class/gpio/export", "w")) == NULL) {
		logError("attachInterrupt: Unable to export pin %d for interrupt: %s\n", physPin, strerror(errno));
		exit(1);
	}
	fprintf(fd, "%d\n", gpioPin);
	fclose(fd);

	// Wait a bit the system to create /sys/class/gpio/gpio<GPIO number>
	delay(1L);

	snprintf(fName, sizeof(fName), "/sys/class/gpio/gpio%d/direction", gpioPin) ;
	if ((fd = fopen (fName, "w")) == NULL) {
		logError("attachInterrupt: Unable to open GPIO direction interface for pin %d: %s\n",
		         physPin, strerror(errno));
		exit(1) ;
	}
	fprintf(fd, "in\n") ;
	fclose(fd) ;

	snprintf(fName, sizeof(fName), "/sys/class/gpio/gpio%d/edge", gpioPin) ;
	if ((fd = fopen(fName, "w")) == NULL) {
		logError("attachInterrupt: Unable to open GPIO edge interface for pin %d: %s\n", physPin,
		         strerror(errno));
		exit(1) ;
	}
	switch (mode) {
	case CHANGE:
		fprintf(fd, "both\n");
		break;
	case FALLING:
		fprintf(fd, "falling\n");
		break;
	case RISING:
		fprintf(fd, "rising\n");
		break;
	case NONE:
		fprintf(fd, "none\n");
		break;
	default:
		logError("attachInterrupt: Invalid mode\n");
		fclose(fd);
		return;
	}
	fclose(fd);

	if (sysFds[gpioPin] == -1) {
		snprintf(fName, sizeof(fName), "/sys/class/gpio/gpio%d/value", gpioPin);
		if ((sysFds[gpioPin] = open(fName, O_RDWR)) < 0) {
			logError("Error reading pin %d: %s\n", physPin, strerror(errno));
			exit(1);
		}
	}

	// Clear any initial pending interrupt
	ioctl(sysFds[gpioPin], FIONREAD, &count);
	for (i = 0; i < count; ++i) {
		if (read(sysFds[gpioPin], &c, 1) == -1) {
			logError("attachInterrupt: failed to read pin status: %s\n", strerror(errno));
		}
	}

	struct ThreadArgs *threadArgs = new struct ThreadArgs;
	threadArgs->func = func;
	threadArgs->gpioPin = gpioPin;

	// Create a thread passing the pin and function
	pthread_create(threadIds[gpioPin], NULL, interruptHandler, (void *)threadArgs);
}

void rpi_util::detachInterrupt(uint8_t physPin)
{
	uint8_t gpioPin;

	if (get_gpio_number(physPin, &gpioPin)) {
		logError("detachInterrupt: invalid pin: %d\n", physPin);
		return;
	}

	// Cancel the thread
	if (threadIds[gpioPin] != NULL) {
		pthread_cancel(*threadIds[gpioPin]);
		delete threadIds[gpioPin];
		threadIds[gpioPin] = NULL;
	}

	// Close filehandle
	if (sysFds[gpioPin] != -1) {
		close(sysFds[gpioPin]);
		sysFds[gpioPin] = -1;
	}

	FILE *fp = fopen("/sys/class/gpio/unexport", "w");
	if (fp == NULL) {
		logError("Unable to unexport pin %d for interrupt\n", gpioPin);
		exit(1);
	}
	fprintf(fp, "%d", gpioPin);
	fclose(fp);
}

uint8_t rpi_util::digitalPinToInterrupt(uint8_t physPin)
{
	// No need to convert the pin to gpio, we do it in attachInterrupt().
	return physPin;
}

void rpi_util::interrupts()
{
	pthread_mutex_lock(&intMutex);
	interruptsEnabled = true;
	pthread_mutex_unlock(&intMutex);
}

void rpi_util::noInterrupts()
{
	pthread_mutex_lock(&intMutex);
	interruptsEnabled = false;
	pthread_mutex_unlock(&intMutex);
}
