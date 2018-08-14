/**
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2015 Sensnology AB
 * Full contributor list: https://github.com/mysensors/Arduino/graphs/contributors
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
 * Version 1.0 - Michael Rahr
 *
 * DESCRIPTION
 * This sketch is for testing the HC12 Radio, it is based
 * on relay sketch from the example foler, but radio
 * is changed to HC12
 * http://www.mysensors.org/build/relay
 */

// Enable debug prints to serial monitor
#define MY_DEBUG
#define MY_BAUD_RATE 115200
// Enable and select radio type attached
// Default pins for HC12 on UNO,NANO,PRO MINI are pin 10 and 11
// Baudrate is 9600 to ensure a stable connection via software serial
#define MY_RADIO_HC12
//#define MY_RADIO_NRF24
//#define MY_RADIO_NRF5_ESB
//#define MY_RADIO_RFM69
//#define MY_RADIO_RFM95

// Enable repeater functionality for this node
//#define MY_REPEATER_FEATURE

// Defineing the set pin, this is use to configure the HC12
#define MY_HC12_SET_PIN 5

#include <MySensors.h>

#define RELAY_PIN 4  // Arduino Digital I/O pin number for first relay (second on pin+1 etc)
#define NUMBER_OF_RELAYS 1 // Total number of attached relays
#define RELAY_ON 1  // GPIO value to write to turn on attached relay
#define RELAY_OFF 0 // GPIO value to write to turn off attached relay
#define CHILD_ID 15


MyMessage msg(CHILD_ID,V_STATUS);
bool initial_state_sent = false;
void before()
{
}

void setup()
{

}

void presentation()
{
	// Send the sketch version information to the gateway and Controller
	sendSketchInfo("Relae switch", "1.0");
	present(CHILD_ID, S_BINARY,"Switch relay",false);

}


void loop()
{
	if (!initial_state_sent) {
		send(msg.set(0));
		Serial.println("SENDING INIT STATE");
		initial_state_sent = true;
	}
}

void receive(const MyMessage &message)
{

	Serial.println("We got data");
	// We only expect one type of message from controller. But we better check anyway.
	if (message.type==V_STATUS) {
		// Change relay state
		//  digitalWrite(message.sensor-1+RELAY_PIN, message.getBool()?RELAY_ON:RELAY_OFF);
		// Store state in eeprom
		//saveState(message.sensor, message.getBool());
		send(msg.set(message.getBool()));
		// Write some debug info
		Serial.print("Incoming change for sensor:");
		Serial.print(message.sensor);
		Serial.print(", New status: ");
		Serial.println(message.getBool());
	}
}

