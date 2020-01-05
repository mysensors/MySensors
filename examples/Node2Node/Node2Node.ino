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
 * REVISION HISTORY
 * Version 1.0 - tekka 2018
 *
 * DESCRIPTION
 * Node to node communication example, destination node id = ACTUATOR_ID
 * Use SIGNING_ENABLED to enable soft/hard signing
 *
 */
#define MY_DEBUG
#define MY_RADIO_RF24
#define CHILD_SENSOR_ID 0
#define ACTUATOR_ID 200
//#define SIGNING_ENABLED // enable for secure actuator

#if defined(SIGNING_ENABLED)
// Signing, select soft/hardware signing method
#define MY_SIGNING_SOFT
//#define MY_SIGNING_ATSHA204
#define MY_DEBUG_VERBOSE_SIGNING
// Enable this if you want destination node to sign all messages sent to this node.
//#define MY_SIGNING_REQUEST_SIGNATURES
#endif

// triggering interval
static const uint32_t trigger_ms = 10000;

#include "MySensors.h"
MyMessage msgGeneral(CHILD_SENSOR_ID, V_STATUS);
uint32_t lastTrigger = 0;
bool actuatorStatus = false;

void presentation(void)
{
	sendSketchInfo("Node2Node", __DATE__);
	present(CHILD_SENSOR_ID, S_BINARY, "Remote");
}

void setup()
{
#if defined(SIGNING_ENABLED)
	SET_SIGN(ACTUATOR_ID);  // define signing requirements for remote node
#endif
}

void loop()
{
	if (millis() - lastTrigger > trigger_ms) {
		lastTrigger = millis();
		// set destination address
		msgGeneral.setDestination(ACTUATOR_ID);
		// send message to node
		send(msgGeneral.set(actuatorStatus));
		// invert status
		actuatorStatus ^= true;
	}
}
