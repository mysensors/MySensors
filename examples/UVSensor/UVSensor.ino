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
 * Version 1.0 - epierre
 * Contribution: bulldoglowell, gizmocuz
 *
 * DESCRIPTION
 * Arduino UVM-30A
 * Index table taken from: http://www.elecrow.com/sensors-c-111/environment-c-111_112/uv-sensor-moduleuvm30a-p-716.html
 * Because this table is pretty lineair, we can calculate a UVI with one decimal
 *
 * Connect sensor:
 *
 *   +   >>> 5V
 *   -   >>> GND
 *   out >>> A0
 *
 * License: Attribution-NonCommercial-ShareAlike 3.0 Unported (CC BY-NC-SA 3.0)
 */

// Enable debug prints to serial monitor
#define MY_DEBUG

// Enable and select radio type attached
#define MY_RADIO_RF24
//#define MY_RADIO_NRF5_ESB
//#define MY_RADIO_RFM69
//#define MY_RADIO_RFM95

#include <MySensors.h>

#define UV_SENSOR_ANALOG_PIN 0

#define CHILD_ID_UV 0

uint32_t SLEEP_TIME = 30*1000; // Sleep time between reads (in milliseconds)

MyMessage uvMsg(CHILD_ID_UV, V_UV);

uint32_t lastSend =0;
float uvIndex;
float lastUV = -1;
uint16_t uvIndexValue [12] = { 50, 227, 318, 408, 503, 606, 696, 795, 881, 976, 1079, 1170};


void presentation()
{
	// Send the sketch version information to the gateway and Controller
	sendSketchInfo("UV Sensor", "1.2");

	// Register all sensors to gateway (they will be created as child devices)
	present(CHILD_ID_UV, S_UV);
}

void loop()
{
	uint32_t currentTime = millis();

	uint16_t uv = analogRead(UV_SENSOR_ANALOG_PIN);// Get UV value
	if (uv>1170) {
		uv=1170;
	}

	//Serial.print("UV Analog reading: ");
	//Serial.println(uv);

	int i;
	for (i = 0; i < 12; i++) {
		if (uv <= uvIndexValue[i]) {
			uvIndex = i;
			break;
		}
	}

	//calculate 1 decimal if possible
	if (i>0) {
		float vRange=uvIndexValue[i]-uvIndexValue[i-1];
		float vCalc=uv-uvIndexValue[i-1];
		uvIndex+=(1.0/vRange)*vCalc-1.0;
	}

	//Serial.print("UVI: ");
	//Serial.println(uvIndex,2);

	//Send value to gateway if changed, or at least every 5 minutes
	if ((uvIndex != lastUV)||(currentTime-lastSend >= 5UL*60UL*1000UL)) {
		lastSend=currentTime;
		send(uvMsg.set(uvIndex,2));
		lastUV = uvIndex;
	}

	sleep(SLEEP_TIME);
}
