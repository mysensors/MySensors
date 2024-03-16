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
#include <errno.h>
#include <sched.h>
#include "log.h"

struct ThreadArgs {
	void (*func)();
	int gpioPin;
	struct gpiod_line *line;
};

volatile bool interruptsEnabled = true;
static pthread_mutex_t intMutex = PTHREAD_MUTEX_INITIALIZER;

static pthread_t *threadIds[256] = {NULL};

// sysFds:
//	Map a file descriptor from the /sys/class/gpio/gpioX/value
static int sysFds[256] = {
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
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
	char c;
	struct ThreadArgs *arguments = (struct ThreadArgs *)args;
	int gpioPin = arguments->gpioPin;
	struct gpiod_line *line = arguments->line;
	void (*func)() = arguments->func;
	delete arguments;

	(void)piHiPri(55);	// Only effective if we run as root

	if ((fd = sysFds[gpioPin]) == -1) {
		logError("Failed to attach interrupt for pin %d\n", gpioPin);
		return NULL;
	}

	while (1) {
		// Wait for it ...
		// New version
		int ret = gpiod_line_event_wait(line, NULL);
		if (ret < 0) {
			logError("Error waiting for interrupt: %s\n", strerror(errno));
			break;
		}
		struct gpiod_line_event event;
		if (gpiod_line_event_read(line, &event) == 0) {
			if (event.event_type == GPIOD_LINE_EVENT_RISING_EDGE) {
				logInfo("RISING Edge\n");
			} else {
				logInfo("FALLING Edge\n");
			}
		}

		// Call user function.
		logError("Calling user function\n");

		pthread_mutex_lock(&intMutex);
		if (interruptsEnabled) {
			pthread_mutex_unlock(&intMutex);
			func();
		} else {
			pthread_mutex_unlock(&intMutex);
		}
	}
	// Adding gpiod closing instructions
	gpiod_line_release(line);

	close(fd);

	return NULL;
}

void attachInterrupt(uint8_t gpioPin, void (*func)(), uint8_t mode)
{
	FILE *fd;
	char fName[40];
	char c;
	int count;

	if (threadIds[gpioPin] == NULL) {
		threadIds[gpioPin] = new pthread_t;
	} else {
		// Cancel the existing thread for that pin
		pthread_cancel(*threadIds[gpioPin]);
		// Wait a bit
		usleep(1000);
	}

	char *chipname = "gpiochip0";
	unsigned int line_num = gpioPin;
	struct gpiod_line_event event;
	struct gpiod_chip *chip;
	struct gpiod_line *line;


	chip = gpiod_chip_open_by_name(chipname);
	if (!chip) {
		logError("Open chip failed\n");
		exit(1);
	}

	line = gpiod_chip_get_line(chip, line_num);
	if (!line) {
		logError("Get line failed\n");
		exit(1);
	}

	switch (mode) {
	case CHANGE:
		gpiod_line_request_both_edges_events(line, "gpiointerrupt");
		break;
	case FALLING:
		gpiod_line_request_falling_edge_events(line, "gpiointerrupt");
		break;
	case RISING:
		gpiod_line_request_rising_edge_events(line, "gpiointerrupt");
		break;
	case NONE:
		break;
	default:
		logError("attachInterrupt: Invalid mode\n");
		return;
	}

	if (sysFds[gpioPin] == -1) {
		if ((sysFds[gpioPin] = gpiod_line_event_get_fd(line)) < 0) {
			logError("Error reading pin %d: %s\n", gpioPin, strerror(errno));
			exit(1);
		}
	}

	struct ThreadArgs *threadArgs = new struct ThreadArgs;
	threadArgs->func = func;
	threadArgs->gpioPin = gpioPin;
	threadArgs->line = line;

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
