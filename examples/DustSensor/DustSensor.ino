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
 * Converted to 1.4 by Henrik Ekblad
 *
 * DESCRIPTION
 * Arduino Dust Sensort
 *
 * connect the sensor as follows :
 *
 *   VCC       >>> 5V
 *   A         >>> A0
 *   GND       >>> GND
 *
 * Based on: http://www.dfrobot.com/wiki/index.php/Sharp_GP2Y1010AU
 * Authors: Cyrille Médard de Chardon (serialC), Christophe Trefois (Trefex)
 *
 * http://www.mysensors.org/build/dust
 *
 */

// Enable debug prints
#define MY_DEBUG

// Enable and select radio type attached
#define MY_RADIO_RF24
//#define MY_RADIO_NRF5_ESB
//#define MY_RADIO_RFM69
//#define MY_RADIO_RFM95

#include <MySensors.h>

#define CHILD_ID_DUST 0
#define DUST_SENSOR_ANALOG_PIN 1

uint32_t SLEEP_TIME = 30*1000; // Sleep time between reads (in milliseconds)
//VARIABLES
int val = 0;           // variable to store the value coming from the sensor
float valDUST =0.0;
float lastDUST =0.0;
int samplingTime = 280;
int deltaTime = 40;
int sleepTime = 9680;
float voMeasured = 0;
float calcVoltage = 0;
float dustDensity = 0;

MyMessage dustMsg(CHILD_ID_DUST, V_LEVEL);

void presentation()
{
	// Send the sketch version information to the gateway and Controller
	sendSketchInfo("Dust Sensor", "1.1");

	// Register all sensors to gateway (they will be created as child devices)
	present(CHILD_ID_DUST, S_DUST);
}

void loop()
{
	uint16_t voMeasured = analogRead(DUST_SENSOR_ANALOG_PIN);// Get DUST value

	// 0 - 5V mapped to 0 - 1023 integer values
	// recover voltage
	calcVoltage = voMeasured * (5.0 / 1024.0);

	// linear equation taken from http://www.howmuchsnow.com/arduino/airquality/
	// Chris Nafis (c) 2012
	dustDensity = (0.17 * calcVoltage - 0.1)*1000;

	Serial.print("Raw Signal Value (0-1023): ");
	Serial.print(voMeasured);

	Serial.print(" - Voltage: ");
	Serial.print(calcVoltage);

	Serial.print(" - Dust Density: ");
	Serial.println(dustDensity); // unit: ug/m3

	if (ceil(dustDensity) != lastDUST) {
		send(dustMsg.set((int16_t)ceil(dustDensity)));
		lastDUST = ceil(dustDensity);
	}

	sleep(SLEEP_TIME);
}
