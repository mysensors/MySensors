/*
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2025 Sensnology AB
 * Full contributor list: https://github.com/mysensors/MySensors/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

/**
 * @file MyMainSTM32.cpp
 * @brief Main entry point implementation for STM32
 *
 * This file provides the main() function that integrates MySensors with the
 * STM32duino Arduino core. It overrides the default Arduino main() to inject
 * MySensors _begin() and _process() calls around the user's sketch functions.
 */

#include "MyHwSTM32.h"

// Declare the sketch's setup() and loop() functions
__attribute__((weak)) void setup(void);
__attribute__((weak)) void loop(void);

// Override Arduino's main() function
int main(void)
{
	// Initialize Arduino core
	init();

#if defined(USBCON)
	// Initialize USB if available
	USBDevice.attach();
#endif

	_begin(); // Startup MySensors library

	for(;;) {
		_process();  // Process incoming data
		if (loop) {
			loop(); // Call sketch loop
		}
		// STM32duino doesn't use serialEventRun by default
	}

	return 0;
}
