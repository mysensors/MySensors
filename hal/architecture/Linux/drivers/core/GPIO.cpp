/*
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2026 Sensnology AB
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
#include <cerrno>

#include "interrupt.h"
#include "log.h"

// Declare a single default instance
GPIOClass GPIO = GPIOClass();

GPIOClass::GPIOClass()
{
	char chip_path[] = "/dev/gpiochip0";
	this->chip = gpiod_chip_open(chip_path);
	if (this->chip == NULL) {
		logError("Failed to open GPIO chip\n");
		exit(1);
	}
}

GPIOClass::~GPIOClass()
{
	for (int i=0; i<MAX_PIN; i++) {
		if (this->threadIds[i] != nullptr) {
			pthread_cancel(*this->threadIds[i]);
			delete threadIds[i];
			this->threadIds[i] = nullptr;
		}
	}
	for (int i=0; i<MAX_PIN; i++) {
		if (this->requests[i] != nullptr) {
			gpiod_line_request_release(this->requests[i]);
			this->requests[i] = nullptr;
		}
	}
	if (this->chip != nullptr) {
		gpiod_chip_close(this->chip);
		this->chip = nullptr;
	}
}

void GPIOClass::pinMode(uint8_t pin, uint8_t mode)
{
	struct gpiod_line_settings *settings;
	struct gpiod_line_config *line_cfg;

	if (pin >= MAX_PIN) {
		logError("pin number too high");
		exit(1);
	}

	// line settings
	settings = gpiod_line_settings_new();
	if (!settings) {
		logError("Could not allocate settings\n");
		exit(1);
	}
	auto dir = GPIOD_LINE_DIRECTION_OUTPUT;
	if (mode == INPUT) {
		dir = GPIOD_LINE_DIRECTION_INPUT;
	}
	if (gpiod_line_settings_set_direction(settings, dir) != 0) {
		logError("Could not set direction\n");
		exit(1);
	}

	// goes into a line config
	line_cfg = gpiod_line_config_new();
	if (!line_cfg) {
		logError("Could not allocate line config\n");
		exit(1);
	}
	const unsigned int offset[] = {pin};
	if (gpiod_line_config_add_line_settings(line_cfg, offset, 1,settings) != 0) {
		logError("Could not add line settings\n");
		exit(1);
	}


	if (this->requests[pin] != nullptr) {
		if (gpiod_line_request_reconfigure_lines(this->requests[pin], line_cfg) != 0) {
			logError("Could not set reconfigure lines\n");
			exit(1);
		}
	} else {
		// request config
		struct gpiod_request_config *request_config = gpiod_request_config_new();
		if (request_config == nullptr) {
			logError("Failed to allocate request config\n");
			exit(1);
		}
		gpiod_request_config_set_consumer(request_config, "MySensors");

		this->requests[pin] = gpiod_chip_request_lines(chip, request_config, line_cfg);
		if (this->requests[pin] == nullptr) {
			logError("Failed to request GPIO line\n");
			exit(1);
		}
		gpiod_request_config_free(request_config);
	}

	gpiod_line_config_free(line_cfg);
	gpiod_line_settings_free(settings);
}

void GPIOClass::digitalWrite(uint8_t pin, uint8_t value)
{
	if (pin >= MAX_PIN) {
		logError("pin number too high");
		exit(1);
	}
	if (this->requests[pin] != nullptr) {
		this->pinMode(pin, OUTPUT);
	}
	if (gpiod_line_request_set_value(this->requests[pin], pin, value==0?GPIOD_LINE_VALUE_INACTIVE:GPIOD_LINE_VALUE_ACTIVE) != 0) {
		logError("Could not set value for GPIO line\n");
		exit(1);
	}
}

uint8_t GPIOClass::digitalRead(uint8_t pin)
{
	if (pin >= MAX_PIN) {
		logError("pin number too high");
		exit(1);
	}
	if (this->requests[pin] != nullptr) {
		this->pinMode(pin, INPUT);
	}
	auto ret = gpiod_line_request_get_value(this->requests[pin], pin);
	if (ret < 0) {
		logError("Could not get value for GPIO line\n");
		exit(1);
	}
	return ret;
}

uint8_t GPIOClass::digitalPinToInterrupt(uint8_t pin)
{
	return pin;
}

// attachInterrupt stuff
/*
 * Part of wiringPi: Simple way to get your program running at high priority
 * with realtime schedulling.
 */
int GPIOClass::_piHiPri(const int pri)
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

void GPIOClass::_interruptHandlerDetachInterrupt(void *arg) {
	struct gpiod_line_settings *line_settings = gpiod_line_settings_new();
	if (gpiod_line_settings_set_edge_detection(line_settings, GPIOD_LINE_EDGE_NONE) != 0) {
		logError("Failed to set pin edge detection mode\n");
		exit(1);
	}
	struct gpiod_line_config *line_config = gpiod_line_config_new();
	if (line_config == nullptr) {
		logError("Failed to allocate line config\n");
		exit(1);
	}
	const unsigned int offset[] = {*static_cast<unsigned int *>(arg)};
	if (gpiod_line_config_add_line_settings(line_config, offset, 1, line_settings) != 0) {
		logError("Failed to add line settings\n");
		exit(1);
	}
	if (GPIO.requests[offset[0]] != nullptr) {
		gpiod_line_request_reconfigure_lines(GPIO.requests[offset[0]], line_config);
	}
	gpiod_line_settings_free(line_settings);
	gpiod_line_config_free(line_config);
}

void GPIOClass::_interruptHandlerEventBufferRelease(void *arg) {
	struct gpiod_edge_event_buffer *buf = static_cast<struct gpiod_edge_event_buffer *>(arg);
	gpiod_edge_event_buffer_free(buf);
}

void *GPIOClass::_interruptHandler(void *args)
{
	struct ThreadArgs *arguments = static_cast<struct ThreadArgs *>(args);
	struct gpiod_line_request *request = arguments->gpiod_line_request;
	void (*func)() = arguments->func;
	unsigned int pin = arguments->pin;
	delete arguments;

	pthread_cleanup_push(GPIOClass::_interruptHandlerDetachInterrupt, static_cast<void*>(&pin));
	(void)GPIOClass::_piHiPri(55);	// Only effective if we run as root

	struct gpiod_edge_event_buffer *event_buffer = gpiod_edge_event_buffer_new(1);
	if (event_buffer == nullptr) {
		logError("Failed to allocate event buffer\n");
		exit(1);
	}
	pthread_cleanup_push(GPIOClass::_interruptHandlerEventBufferRelease, event_buffer);

	while (true) {
		// Wait for it ...
		int ret = gpiod_line_request_wait_edge_events(request, -1);
		if (ret < 0) {
			logError("Error waiting for interrupt: %s\n", strerror(errno));
			break;
		}
		if (gpiod_line_request_read_edge_events(request, event_buffer, 1) < 0) {
			logError("Error reading edge event: %s\n", strerror(errno));
			break;
		}
		// get event so libgpiod knows we have processed it
		struct gpiod_edge_event *event = gpiod_edge_event_buffer_get_event(event_buffer, 0);
		(void)event;
		// in the future we might check the event type actually matches before calling user function
		// checking would work like this:
		// gpiod_edge_event_get_event_type(event) == GPIOD_EDGE_EVENT_RISING_EDGE

		// Call user function. Logging disabled for performance.
		// logError("Calling user function\n");

		pthread_mutex_lock(&GPIO.intMutex);
		if (GPIO.interruptsEnabled) {
			pthread_mutex_unlock(&GPIO.intMutex);
			func();
		} else {
			pthread_mutex_unlock(&GPIO.intMutex);
		}
	}

	pthread_cleanup_pop(1);
	pthread_cleanup_pop(1);

	return nullptr;
}

void GPIOClass::attachInterrupt(uint8_t pin, void (*func)(), uint8_t mode) {
	if (pin >= MAX_PIN) {
		logError("pin number too high");
		exit(1);
	}

	if (this->threadIds[pin] == nullptr) {
		threadIds[pin] = new pthread_t;
	} else {
		// Cancel the existing thread for that pin
		pthread_cancel(*threadIds[pin]);
		// Wait a bit
		usleep(1000);
	}

	// line settings
	struct gpiod_line_settings *line_settings = gpiod_line_settings_new();
	if (gpiod_line_settings_set_direction(line_settings, GPIOD_LINE_DIRECTION_INPUT) != 0) {
		logError("Failed to set pin direction\n");
		exit(1);
	}
	auto edge = GPIOD_LINE_EDGE_NONE;
	switch (mode) {
		case CHANGE:
			edge = GPIOD_LINE_EDGE_BOTH;
			break;
		case FALLING:
			edge = GPIOD_LINE_EDGE_FALLING;
			break;
		case RISING:
			edge = GPIOD_LINE_EDGE_RISING;
			break;
		case NONE:
		default:
			logError("attachInterrupt: Invalid mode\n");
			exit(1);
	}
	if (gpiod_line_settings_set_edge_detection(line_settings, edge) != 0) {
		logError("Failed to set pin edge detection mode\n");
		exit(1);
	}
	if (gpiod_line_settings_set_bias(line_settings, GPIOD_LINE_BIAS_AS_IS) != 0) {
		logError("Failed to set pin bias mode\n");
		exit(1);
	}


	// put it into a line config
	struct gpiod_line_config *line_config = gpiod_line_config_new();
	if (line_config == NULL) {
		logError("Failed to allocate line config\n");
		exit(1);
	}
	const unsigned int offset[] = {pin};
	if (gpiod_line_config_add_line_settings(line_config, offset, 1, line_settings) != 0) {
		logError("Failed to add line settings\n");
		exit(1);
	}

	if (this->requests[pin] != nullptr) {
		if (gpiod_line_request_reconfigure_lines(this->requests[pin], line_config) != 0) {
			logError("Could not set reconfigure lines\n");
			exit(1);
		}
	} else {
		// request config
		struct gpiod_request_config *request_config = gpiod_request_config_new();
		if (request_config == NULL) {
			logError("Failed to allocate request config\n");
			exit(1);
		}
		gpiod_request_config_set_consumer(request_config, "MySensors Interrupt");
		this->requests[pin] = gpiod_chip_request_lines(chip, request_config, line_config);
		if (this->requests[pin] == NULL) {
			logError("Failed to request GPIO line\n");
			exit(1);
		}
		gpiod_request_config_free(request_config);
	}

	gpiod_line_settings_free(line_settings);
	gpiod_line_config_free(line_config);

	struct ThreadArgs *threadArgs = new struct ThreadArgs;
	threadArgs->func = func;
	threadArgs->pin = pin;
	threadArgs->gpiod_line_request = this->requests[pin];

	// Create a thread passing the pin and function
	pthread_create(this->threadIds[pin], nullptr, GPIOClass::_interruptHandler, (void *)threadArgs);
}
void GPIOClass::detachInterrupt(uint8_t pin) {
	if (pin >= MAX_PIN) {
		logError("pin number too high");
		exit(1);
	}
	// Cancel the thread
	if (this->threadIds[pin] != nullptr) {
		pthread_cancel(*this->threadIds[pin]);
		delete threadIds[pin];
		this->threadIds[pin] = nullptr;
	}
}

void GPIOClass::interrupts() {
	pthread_mutex_lock(&intMutex);
	interruptsEnabled = true;
	pthread_mutex_unlock(&intMutex);
}
void GPIOClass::noInterrupts() {
	pthread_mutex_lock(&intMutex);
	interruptsEnabled = false;
	pthread_mutex_unlock(&intMutex);
}


