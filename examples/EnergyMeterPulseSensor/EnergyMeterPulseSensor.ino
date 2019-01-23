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
 * Version 1.0 - Henrik Ekblad
 *
 * DESCRIPTION
 * This sketch provides an example how to implement a LM393 PCB
 * Use this sensor to measure kWh and Watt of your house meter
 * You need to set the correct pulsefactor of your meter (blinks per kWh).
 * The sensor starts by fetching current kWh value from gateway.
 * Reports both kWh and Watt back to gateway.
 *
 * Unfortunately millis() won't increment when the Arduino is in
 * sleepmode. So we cannot make this sensor sleep if we also want
 * to calculate/report watt value.
 * http://www.mysensors.org/build/pulse_power
 */

// Enable debug prints
#define MY_DEBUG

// Enable and select radio type attached
#define MY_RADIO_RF24
//#define MY_RADIO_NRF5_ESB
//#define MY_RADIO_RFM69
//#define MY_RADIO_RFM95

#include <MySensors.h>

#define DIGITAL_INPUT_SENSOR 3  // The digital input you attached your light sensor.  (Only 2 and 3 generates interrupt!)
#define PULSE_FACTOR 1000       // Number of blinks per of your meter
#define SLEEP_MODE false        // Watt value can only be reported when sleep mode is false.
#define MAX_WATT 10000          // Max watt value to report. This filters outliers.
#define CHILD_ID 1              // Id of the sensor child

uint32_t SEND_FREQUENCY =
    20000; // Minimum time between send (in milliseconds). We don't want to spam the gateway.
double ppwh = ((double)PULSE_FACTOR)/1000; // Pulses per watt hour
bool pcReceived = false;
volatile uint32_t pulseCount = 0;
volatile uint32_t lastBlink = 0;
volatile uint32_t watt = 0;
uint32_t oldPulseCount = 0;
uint32_t oldWatt = 0;
double oldkWh;
uint32_t lastSend;
MyMessage wattMsg(CHILD_ID,V_WATT);
MyMessage kWhMsg(CHILD_ID,V_KWH);
MyMessage pcMsg(CHILD_ID,V_VAR1);


void setup()
{
	// Fetch last known pulse count value from gw
	request(CHILD_ID, V_VAR1);

	// Use the internal pullup to be able to hook up this sketch directly to an energy meter with S0 output
	// If no pullup is used, the reported usage will be too high because of the floating pin
	pinMode(DIGITAL_INPUT_SENSOR,INPUT_PULLUP);

	attachInterrupt(digitalPinToInterrupt(DIGITAL_INPUT_SENSOR), onPulse, RISING);
	lastSend=millis();
}

void presentation()
{
	// Send the sketch version information to the gateway and Controller
	sendSketchInfo("Energy Meter", "1.0");

	// Register this device as power sensor
	present(CHILD_ID, S_POWER);
}

void loop()
{
	uint32_t now = millis();
	// Only send values at a maximum frequency or woken up from sleep
	bool sendTime = now - lastSend > SEND_FREQUENCY;
	if (pcReceived && (SLEEP_MODE || sendTime)) {
		// New watt value has been calculated
		if (!SLEEP_MODE && watt != oldWatt) {
			// Check that we don't get unreasonable large watt value.
			// could happen when long wraps or false interrupt triggered
			if (watt<((uint32_t)MAX_WATT)) {
				send(wattMsg.set(watt));  // Send watt value to gw
			}
			Serial.print("Watt:");
			Serial.println(watt);
			oldWatt = watt;
		}

		// Pulse count value has changed
		if (pulseCount != oldPulseCount) {
			send(pcMsg.set(pulseCount));  // Send pulse count value to gw
			double kWh = ((double)pulseCount/((double)PULSE_FACTOR));
			oldPulseCount = pulseCount;
			if (kWh != oldkWh) {
				send(kWhMsg.set(kWh, 4));  // Send kWh value to gw
				oldkWh = kWh;
			}
		}
		lastSend = now;
	} else if (sendTime && !pcReceived) {
		// No pulse count value received. Try requesting it again
		request(CHILD_ID, V_VAR1);
		lastSend=now;
	}

	if (SLEEP_MODE) {
		sleep(SEND_FREQUENCY);
	}
}

void receive(const MyMessage &message)
{
	if (message.type==V_VAR1) {
		pulseCount = oldPulseCount = message.getLong();
		Serial.print("Received last pulse count value from gw:");
		Serial.println(pulseCount);
		pcReceived = true;
	}
}

void onPulse()
{
	if (!SLEEP_MODE) {
		uint32_t newBlink = micros();
		uint32_t interval = newBlink-lastBlink;
		if (interval<10000L) { // Sometimes we get interrupt on RISING
			return;
		}
		watt = (3600000000.0 /interval) / ppwh;
		lastBlink = newBlink;
	}
	pulseCount++;
}
