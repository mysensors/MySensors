/*
   The MySensors Arduino library handles the wireless radio link and protocol
   between your home built sensors/actuators and HA controller of choice.
   The sensors forms a self healing radio network with optional repeaters. Each
   repeater and gateway builds a routing tables in EEPROM which keeps track of the
   network topology allowing messages to be routed to nodes.

   Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
   Copyright (C) 2013-2022 Sensnology AB
   Full contributor list: https://github.com/mysensors/MySensors/graphs/contributors

   Documentation: http://www.mysensors.org
   Support Forum: http://forum.mysensors.org

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as published by the Free Software Foundation.

 *******************************

   DESCRIPTION

   This is an example that demonstrates how to report the battery level for a sensor
   using the internal measurement method.
   Instructions for measuring battery capacity are available here:
   http://www.mysensors.org/build/battery

*/

// Enable debug prints to serial monitor
#define MY_DEBUG

// Enable and select radio type attached
#define MY_RADIO_RF24
//#define MY_RADIO_NRF5_ESB
//#define MY_RADIO_RFM69
//#define MY_RADIO_RFM95

#include <MySensors.h>

uint32_t SLEEP_TIME = 900000;  // sleep time between reads (seconds * 1000 milliseconds)
int oldBatteryPcnt = 0;
#define FULL_BATTERY 3 // 3V for 2xAA alkaline. Adjust if you use a different battery setup.

void setup()
{
}

void presentation()
{
	// Send the sketch version information to the gateway and Controller
	sendSketchInfo("Battery Meter", "1.0");
}

void loop()
{
	// get the battery Voltage
	long batteryMillivolts = hwCPUVoltage();
	int batteryPcnt = batteryMillivolts / FULL_BATTERY / 1000.0 * 100 + 0.5;
#ifdef MY_DEBUG
	Serial.print("Battery voltage: ");
	Serial.print(batteryMillivolts / 1000.0);
	Serial.println("V");
	Serial.print("Battery percent: ");
	Serial.print(batteryPcnt);
	Serial.println(" %");
#endif

	if (oldBatteryPcnt != batteryPcnt) {
		sendBatteryLevel(batteryPcnt);
		oldBatteryPcnt = batteryPcnt;
	}
	sleep(SLEEP_TIME);
}
