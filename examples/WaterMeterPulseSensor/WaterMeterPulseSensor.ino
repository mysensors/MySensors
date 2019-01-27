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
 * Version 1.1 - GizMoCuz
 *
 * DESCRIPTION
 * Use this sensor to measure volume and flow of your house water meter.
 * You need to set the correct pulsefactor of your meter (pulses per m3).
 * The sensor starts by fetching current volume reading from gateway (VAR 1).
 * Reports both volume and flow back to gateway.
 *
 * Unfortunately millis() won't increment when the Arduino is in
 * sleepmode. So we cannot make this sensor sleep if we also want
 * to calculate/report flow.
 * http://www.mysensors.org/build/pulse_water
 */

// Enable debug prints to serial monitor
#define MY_DEBUG

// Enable and select radio type attached
#define MY_RADIO_RF24
//#define MY_RADIO_NRF5_ESB
//#define MY_RADIO_RFM69
//#define MY_RADIO_RFM95

#include <MySensors.h>

#define DIGITAL_INPUT_SENSOR 3                  // The digital input you attached your sensor.  (Only 2 and 3 generates interrupt!)

#define PULSE_FACTOR 1000                       // Number of blinks per m3 of your meter (One rotation/liter)

#define SLEEP_MODE false                        // flowvalue can only be reported when sleep mode is false.

#define MAX_FLOW 40                             // Max flow (l/min) value to report. This filters outliers.

#define CHILD_ID 1                              // Id of the sensor child

uint32_t SEND_FREQUENCY =
    30000;           // Minimum time between send (in milliseconds). We don't want to spam the gateway.

MyMessage flowMsg(CHILD_ID,V_FLOW);
MyMessage volumeMsg(CHILD_ID,V_VOLUME);
MyMessage lastCounterMsg(CHILD_ID,V_VAR1);

double ppl = ((double)PULSE_FACTOR)/1000;        // Pulses per liter

volatile uint32_t pulseCount = 0;
volatile uint32_t lastBlink = 0;
volatile double flow = 0;
bool pcReceived = false;
uint32_t oldPulseCount = 0;
uint32_t newBlink = 0;
double oldflow = 0;
double volume =0;
double oldvolume =0;
uint32_t lastSend =0;
uint32_t lastPulse =0;

void setup()
{
	// initialize our digital pins internal pullup resistor so one pulse switches from high to low (less distortion)
	pinMode(DIGITAL_INPUT_SENSOR, INPUT_PULLUP);

	pulseCount = oldPulseCount = 0;

	// Fetch last known pulse count value from gw
	request(CHILD_ID, V_VAR1);

	lastSend = lastPulse = millis();

	attachInterrupt(digitalPinToInterrupt(DIGITAL_INPUT_SENSOR), onPulse, FALLING);
}

void presentation()
{
	// Send the sketch version information to the gateway and Controller
	sendSketchInfo("Water Meter", "1.1");

	// Register this device as Water flow sensor
	present(CHILD_ID, S_WATER);
}

void loop()
{
	uint32_t currentTime = millis();

	// Only send values at a maximum frequency or woken up from sleep
	if (SLEEP_MODE || (currentTime - lastSend > SEND_FREQUENCY)) {
		lastSend=currentTime;

		if (!pcReceived) {
			//Last Pulsecount not yet received from controller, request it again
			request(CHILD_ID, V_VAR1);
			return;
		}

		if (!SLEEP_MODE && flow != oldflow) {
			oldflow = flow;

			Serial.print("l/min:");
			Serial.println(flow);

			// Check that we don't get unreasonable large flow value.
			// could happen when long wraps or false interrupt triggered
			if (flow<((uint32_t)MAX_FLOW)) {
				send(flowMsg.set(flow, 2));                   // Send flow value to gw
			}
		}

		// No Pulse count received in 2min
		if(currentTime - lastPulse > 120000) {
			flow = 0;
		}

		// Pulse count has changed
		if ((pulseCount != oldPulseCount)||(!SLEEP_MODE)) {
			oldPulseCount = pulseCount;

			Serial.print("pulsecount:");
			Serial.println(pulseCount);

			send(lastCounterMsg.set(pulseCount));                  // Send  pulsecount value to gw in VAR1

			double volume = ((double)pulseCount/((double)PULSE_FACTOR));
			if ((volume != oldvolume)||(!SLEEP_MODE)) {
				oldvolume = volume;

				Serial.print("volume:");
				Serial.println(volume, 3);

				send(volumeMsg.set(volume, 3));               // Send volume value to gw
			}
		}
	}
	if (SLEEP_MODE) {
		sleep(SEND_FREQUENCY);
	}
}

void receive(const MyMessage &message)
{
	if (message.type==V_VAR1) {
		uint32_t gwPulseCount=message.getULong();
		pulseCount += gwPulseCount;
		flow=oldflow=0;
		Serial.print("Received last pulse count from gw:");
		Serial.println(pulseCount);
		pcReceived = true;
	}
}

void onPulse()
{
	if (!SLEEP_MODE) {
		uint32_t newBlink = micros();
		uint32_t interval = newBlink-lastBlink;

		if (interval!=0) {
			lastPulse = millis();
			if (interval<500000L) {
				// Sometimes we get interrupt on RISING,  500000 = 0.5 second debounce ( max 120 l/min)
				return;
			}
			flow = (60000000.0 /interval) / ppl;
		}
		lastBlink = newBlink;
	}
	pulseCount++;
}
