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
 *******************************
 *
 * DESCRIPTION
 *
 * Example for sending debug messages over the air (OTA).
 *
 */

// Enable debug
#define MY_DEBUG

// Enable OTA debugging to Node 0
#define MY_DEBUG_OTA (0)

// Allow sending logs without MY_DEBUG_OTA enabled
#define MY_OTA_LOG_SENDER_FEATURE

// Disable ACK for debug messages
//#define MY_DEBUG_OTA_DISABLE_ACK

// Enable and select radio type attached
#define MY_RADIO_RF24
//#define MY_RADIO_NRF5_ESB
//#define MY_RADIO_RFM69
//#define MY_RADIO_RFM95

#include <MySensors.h>

void setup()
{
}

void presentation()
{
	// Send the sketch version information to the gateway and Controller
	sendSketchInfo("DebugSensor", "1.0");
}

// Arduino loop
int c=0;
void loop()
{
	// Wait some time
	if (sleep(3000)==MY_SLEEP_NOT_POSSIBLE) {
		delay(3000);
	}

	// Count loops
	c++;

	// A debug message
	DEBUG_OUTPUT(PSTR("DEBUG\nc=%" PRId16 "\nmillis=%" PRId32 "\n"), c, hwMillis());

	// Send a log message with ACK to a node
	OTALog(0, true, PSTR("LOG\nc=%" PRId16 "\nmillis=%" PRId32 "\n"), c, hwMillis());
}