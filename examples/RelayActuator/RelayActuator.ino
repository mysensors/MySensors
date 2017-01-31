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
 * Version 1.0 - Henrik Ekblad
 * Version 1.2 - Joris Meijerink
 *
 * DESCRIPTION
 * Example sketch showing how to control physical relays.
 * This example will remember relay state after power failure.
 * http://www.mysensors.org/build/relay
 *
 * Version 1.2
 * Changed the way the pins are assigned, now it is possible to use 8 channel relays
 */

// Enable debug prints to serial monitor
#define MY_DEBUG

// Enable and select radio type attached
#define MY_RADIO_NRF24
//#define MY_RADIO_RFM69

// Enable repeater functionality for this node
#define MY_REPEATER_FEATURE

#include <MySensors.h>

const int PINS[] = {3, 4, 5, 6, 7, 8, 14, 15}; // I/O pins 3, 4, 5, 6, 7, 8, A0, A1 for the relays 
#define NUMBER_OF_RELAYS 8 // Total number of attached relays
#define RELAY_ON 0  // GPIO value to write to turn on attached relay
#define RELAY_OFF 1 // GPIO value to write to turn off attached relay


void before()
{
    for (int sensor=1; sensor<=NUMBER_OF_RELAYS; sensor++) {
        // Then set relay pins in output mode
        pinMode(PINS[sensor-1], OUTPUT);
        // Set relay to last known state (using eeprom storage)
        digitalWrite(PINS[sensor-1], loadState(sensor)?RELAY_ON:RELAY_OFF);
        
    }
}

void setup()
{

}

void presentation()
{
    // Send the sketch version information to the gateway and Controller
    sendSketchInfo("Relay", "1.0");

    for (int sensor=1; sensor<=NUMBER_OF_RELAYS; sensor++) {
        // Register all sensors to gw (they will be created as child devices)
        present(sensor, S_BINARY);
    }
}


void loop()
{

}

void receive(const MyMessage &message)
{
    // We only expect one type of message from controller. But we better check anyway.
    if (message.type==V_STATUS) {
        // Change relay state
        digitalWrite(PINS[message.sensor-1], message.getBool()?RELAY_ON:RELAY_OFF);
        // Store state in eeprom
        saveState(message.sensor, message.getBool());
        // Write some debug info
        Serial.print("Incoming change for sensor:");
        Serial.print(message.sensor);
        Serial.print(", New status: ");
        Serial.println(message.getBool());
    }
}
