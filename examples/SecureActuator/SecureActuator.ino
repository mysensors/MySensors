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
 */
/**
 * @ingroup MySigninggrp
 * @{
 * @file SecureActuator.ino
 * @brief Example sketch showing how to securely control locks.
 *
 * This example will remember lock state even after power failure.
 *
 * REVISION HISTORY
 *  - See git log (git log libraries/MySensors/examples/SecureActuator/SecureActuator.ino)
 */

/**
 * @example SecureActuator.ino
 * This example implements a secure actuator in the form of a IO controlled electrical lock.<br>
 * Multiple locks are supported as long as they are on subsequent IO pin indices. The first lock pin
 * is defined by @ref LOCK_1. The number of locks is controlled by @ref NOF_LOCKS .<br>
 * The sketch will require incoming messages to be signed and the use of signing backend is selected
 * by @ref MY_SIGNING_ATSHA204 or @ref MY_SIGNING_SOFT. Hard or soft ATSHA204 signing is supported.<br>
 * Whitelisting can be enabled through @ref MY_SIGNING_NODE_WHITELISTING in which case a single entry
 * is provided in this example which typically should map to the gateway of the network.
 */

#define MY_DEBUG //!< Enable debug prints to serial monitor
#define MY_DEBUG_VERBOSE_SIGNING //!< Enable signing related debug prints to serial monitor
#define MY_NODE_LOCK_FEATURE //!< Enable lockdown of node if suspicious activity is detected

// Enable and select radio type attached
#define MY_RADIO_RF24 //!< nRF24L01 radio driver
//#define MY_RADIO_NRF5_ESB //!< nRF5 radio driver (RF24 compatible)
//#define MY_RADIO_RFM69 //!< RFM69 radio driver
//#define MY_RADIO_RFM95 //!< RFM95 radio driver

// Select soft/hardware signing method
#define MY_SIGNING_SOFT //!< Software signing
//#define MY_SIGNING_ATSHA204 //!< Hardware signing using ATSHA204A

// Enable node whitelisting
//#define MY_SIGNING_NODE_WHITELISTING {{.nodeId = GATEWAY_ADDRESS,.serial = {0x09,0x08,0x07,0x06,0x05,0x04,0x03,0x02,0x01}}}
// Enable this if you want destination node to sign all messages sent to this node.
#define MY_SIGNING_REQUEST_SIGNATURES


// SETTINGS FOR MY_SIGNING_SOFT
#define MY_SIGNING_SOFT_RANDOMSEED_PIN 7 //!< Unconnected analog pin for random seed

// SETTINGS FOR MY_SIGNING_ATSHA204
#ifndef MY_SIGNING_ATSHA204_PIN
#define MY_SIGNING_ATSHA204_PIN 17 //!< A3 - pin where ATSHA204 is attached
#endif

#include <MySensors.h>


#define LOCK_1  3     //!< Arduino Digital I/O pin number for first lock (second on pin+1 etc)
#define NOF_LOCKS 1   //!< Total number of attached locks
#define LOCK_LOCK 1   //!< GPIO value to write to lock attached lock
#define LOCK_UNLOCK 0 //!< GPIO value to write to unlock attached lock

void setup()
{
	for (int lock=1, pin=LOCK_1; lock<=NOF_LOCKS; lock++, pin++) {
		// Set lock pins in output mode
		pinMode(pin, OUTPUT);
		// Set lock to last known state (using eeprom storage)
		digitalWrite(pin, loadState(lock)?LOCK_LOCK:LOCK_UNLOCK);
	}
}

void presentation()
{
	// Send the sketch version information to the gateway and Controller
	sendSketchInfo("Secure Lock", "1.0");

	// Fetch lock status
	for (int lock=1, pin=LOCK_1; lock<=NOF_LOCKS; lock++, pin++) {
		// Register all locks to gw (they will be created as child devices)
		present(lock, S_LOCK, "SecureActuator", false);
	}
}

/** @brief Sketch execution code */
void loop()
{
}

/**
 * @brief Incoming message handler
 *
 * @param message The message to handle.
 */
void receive(const MyMessage &message)
{
	// We only expect one type of message from controller. But we better check anyway.
	// And acks are not accepted as control messages
	if (message.type==V_LOCK_STATUS && message.sensor<=NOF_LOCKS && !mGetAck(message)) {
		// Change relay state
		digitalWrite(message.sensor-1+LOCK_1, message.getBool()?LOCK_LOCK:LOCK_UNLOCK);
		// Store state in eeprom
		saveState(message.sensor, message.getBool());
		// Write some debug info
		Serial.print("Incoming change for lock:");
		Serial.print(message.sensor);
		Serial.print(", New status: ");
		Serial.println(message.getBool());
	}
}
/** @}*/
