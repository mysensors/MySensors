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

#include <time.h>
#include <sys/time.h>
#include <stdlib.h>
#include "Arduino.h"

// For millis()
static unsigned long millis_at_start = 0;

void yield(void) {}

unsigned long millis(void)
{
	timeval curTime;

	if (millis_at_start == 0) {
		gettimeofday(&curTime, NULL);
		millis_at_start = curTime.tv_sec;
	}

	gettimeofday(&curTime, NULL);
	return ((curTime.tv_sec - millis_at_start) * 1000) + (curTime.tv_usec / 1000);
}

unsigned long micros()
{
	timeval curTime;

	if (millis_at_start == 0) {
		gettimeofday(&curTime, NULL);
		millis_at_start = curTime.tv_sec;
	}

	gettimeofday(&curTime, NULL);
	return ((curTime.tv_sec - millis_at_start) * 1000000) + (curTime.tv_usec);
}

void _delay_milliseconds(unsigned int millis)
{
	struct timespec sleeper;

	sleeper.tv_sec  = (time_t)(millis / 1000);
	sleeper.tv_nsec = (long)(millis % 1000) * 1000000;
	nanosleep(&sleeper, NULL);
}

void _delay_microseconds(unsigned int micro)
{
	struct timespec sleeper;

	sleeper.tv_sec  = (time_t)(micro / 1000000);
	sleeper.tv_nsec = (long)(micro % 1000000) * 1000;
	nanosleep(&sleeper, NULL);
}

void randomSeed(unsigned long seed)
{
	if (seed != 0) {
		srand(seed);
	}
}

long randMax(long howbig)
{
	if (howbig == 0) {
		return 0;
	}
	return rand() % howbig;
}

long randMinMax(long howsmall, long howbig)
{
	if (howsmall >= howbig) {
		return howsmall;
	}
	long diff = howbig - howsmall;
	return randMax(diff) + howsmall;
}
