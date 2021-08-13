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

inline void _my_sensors_loop()
{
	// Process incoming data
	_process();
	// Call of loop() in the Arduino sketch
	loop();
}

/*
 * Use preprocessor defines for injection of the MySensors calls
 * to _begin() and _process() in file core_esp8266_main.cpp.
 * These functions implement the "magic" how the MySensors stack
 * is setup and executed in background without need
 * for explicit calls from the Arduino sketch.
 */

// Start up MySensors library including call of setup() in the Arduino sketch
#define setup _begin
// Helper function to _process() and call of loop() in the Arduino sketch
#define loop _my_sensors_loop

#include <core_esp8266_main.cpp>

// Tidy up injection defines
#undef loop
#undef setup
