/*
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2017 Sensnology AB
 * Full contributor list: https://github.com/mysensors/Arduino/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * Author of this file:
 *   til 9b630f7d46945e2de4171568428d4347dcae912e: above
 * until 9b630f7d46945e2de4171568428d4347dcae912e: Pasal Gollor <pascal@pgollor.de>
 *
 *
 * This script supports requestTime for the nodes.
 */

#include <iostream>
#include <cstdio>
#include <unistd.h>
#include <ctime> // std c time library

// For more options run ./configure --help

// How many clients should be able to connect to this gateway (default 1)
#define MY_GATEWAY_MAX_CLIENTS 10


#include <MySensors.h>

#define ARDUINO 100
// This space is intended to be used to include arduino libraries

#undef ARDUINO


void receive(const MyMessage &message)
{
	if (mGetCommand(message) == C_INTERNAL && message.destination == GATEWAY_ADDRESS && message.type == I_TIME)
	{
		std::time_t t = time(0);
		MyMessage timeMessage;

		// prepare message
		timeMessage.set(static_cast<uint32_t>(t));
		timeMessage.setDestination(message.sender);
		timeMessage.setType(I_TIME);
		mSetCommand(timeMessage, C_INTERNAL); // after set(.)

		DEBUG_OUTPUT("Reqeust time from node: %i - seconds since 1970: %d", message.sender, static_cast<uint32_t>(t));
		send(timeMessage);
	}
}

void setup()
{
	// Setup locally attached sensors
}

void presentation()
{
	// Present locally attached sensors here
}

void loop()
{
	// Send locally attached sensors data here
}
