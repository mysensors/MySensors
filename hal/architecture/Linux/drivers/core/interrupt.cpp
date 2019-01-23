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
 * Based on wiringPi Copyright (c) 2012 Gordon Henderson.
 */

#include "interrupt.h"
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
#include <sched.h>
#include "log.h"

struct ThreadArgs {
	void (*func)();
	int gpioPin;
};

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

/*
 * Part of wiringPi: Simple way to get your program running at high priority
 * with realtime schedulling.
 */
int piHiPri(const int pri)
{
	struct sched_param sched ;

	memset (&sched, 0, sizeof(sched)) ;

	if (pri > sched_get_priority_max (SCHED_RR)) {
		sched.sched_priority = sched_get_priority_max (SCHED_RR) ;
	} else {
		sched.sched_priority = pri ;
	}

	return sched_setscheduler (0, SCHED_RR, &sched) ;
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
	polls.events = POLLPRI | POLLERR;

	while (1) {
		// Wait for it ...
		int ret = poll(&polls, 1, -1);
		if (ret < 0) {
			logError("Error waiting for interrupt: %s\n", strerror(errno));
			break;
		}
		// Do a dummy read to clear the interrupt
		//	A one character read appars to be enough.
		if (lseek (fd, 0, SEEK_SET) < 0) {
			logError("Interrupt handler error: %s\n", strerror(errno));
			break;
		}
		if (read (fd, &c, 1) < 0) {
			logError("Interrupt handler error: %s\n", strerror(errno));
			break;
		}
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

void attachInterrupt(uint8_t gpioPin, void (*func)(), uint8_t mode)
{
	FILE *fd;
	char fName[40];
	char c;
	int count, i;

	if (threadIds[gpioPin] == NULL) {
		threadIds[gpioPin] = new pthread_t;
	} else {
		// Cancel the existing thread for that pin
		pthread_cancel(*threadIds[gpioPin]);
		// Wait a bit
		usleep(1000);
	}

	// Export pin for interrupt
	if ((fd = fopen("/sys/class/gpio/export", "w")) == NULL) {
		logError("attachInterrupt: Unable to export pin %d for interrupt: %s\n", gpioPin, strerror(errno));
		exit(1);
	}
	fprintf(fd, "%d\n", gpioPin);
	fclose(fd);

	// Wait a bit the system to create /sys/class/gpio/gpio<GPIO number>
	usleep(1000);

	snprintf(fName, sizeof(fName), "/sys/class/gpio/gpio%d/direction", gpioPin) ;
	if ((fd = fopen (fName, "w")) == NULL) {
		logError("attachInterrupt: Unable to open GPIO direction interface for pin %d: %s\n",
		         gpioPin, strerror(errno));
		exit(1) ;
	}
	fprintf(fd, "in\n") ;
	fclose(fd) ;

	snprintf(fName, sizeof(fName), "/sys/class/gpio/gpio%d/edge", gpioPin) ;
	if ((fd = fopen(fName, "w")) == NULL) {
		logError("attachInterrupt: Unable to open GPIO edge interface for pin %d: %s\n", gpioPin,
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
			logError("Error reading pin %d: %s\n", gpioPin, strerror(errno));
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

void detachInterrupt(uint8_t gpioPin)
{
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

void interrupts()
{
	pthread_mutex_lock(&intMutex);
	interruptsEnabled = true;
	pthread_mutex_unlock(&intMutex);
}

void noInterrupts()
{
	pthread_mutex_lock(&intMutex);
	interruptsEnabled = false;
	pthread_mutex_unlock(&intMutex);
}
