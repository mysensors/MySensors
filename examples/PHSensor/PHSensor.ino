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
 * Version 1.0 - mboyer85
 *
 * DESCRIPTION
 * Example sketch showing how to send PH readings back to the controller
 */

// Enable debug prints to serial monitor
//#define MY_DEBUG

// Enable and select radio type attached
#define MY_RADIO_RF24
//#define MY_RADIO_NRF5_ESB
//#define MY_RADIO_RFM69
//#define MY_RADIO_RFM95

#include <MySensors.h>

#define COMPARE_PH 1 // Send PH only if changed? 1 = Yes 0 = No

uint32_t SLEEP_TIME = 60000; // Sleep time between reads (in milliseconds)
float lastPH;
bool receivedConfig = false;
bool metric = true;
// Initialize PH message
MyMessage msg(0, V_PH);

void setup()
{
	//Setup your PH sensor here (I2C,Serial,Phidget...)
}

float getPH()
{
	//query your PH sensor here (I2C,Serial,Phidget...)
	float dummy = 7;
	return dummy;
}

void presentation()
{
	// Send the sketch version information to the gateway and Controller
	sendSketchInfo("PH Sensor", "1.1");
	present(0, S_WATER_QUALITY);

}

void loop()
{
	float ph = getPH();

#if COMPARE_PH == 1
	if (lastPH != ph) {
#endif

		// Send in the new PH value
		send(msg.set(ph, 1));
		// Save new PH value for next compare
		lastPH = ph;

#if COMPARE_PH == 1
	}
#endif
	sleep(SLEEP_TIME);
}
