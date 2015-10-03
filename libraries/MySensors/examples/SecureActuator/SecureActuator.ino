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
 * Version 1.0 - Patrick "Anticimex" Fallberg <patrick@fallberg.net>
 * 
 * DESCRIPTION
 * Example sketch showing how to securely control locks. 
 * This example will remember lock state even after power failure.
 */

// Enable debug prints to serial monitor
#define MY_DEBUG 

// Enable and select radio type attached
#define MY_RADIO_NRF24
//#define MY_RADIO_RFM69
 
// Select soft/hardware signing method
#define MY_SIGNING_SOFT // Software signing enabled
//#define MY_SIGNING_ATSHA204 // Hardware signing using ATSHA204A

// Enable node whitelisting
//#define MY_SIGNING_NODE_WHITELISTING {{.nodeId = GATEWAY_ADDRESS,.serial = {0x09,0x08,0x07,0x06,0x05,0x04,0x03,0x02,0x01}}}
// Enable this if you want destination node to sign all messages sent to this node. Default is not to require signing. 
//#define MY_SIGNING_REQUEST_SIGNATURES


// SETTINGS FOR MY_SIGNING_SOFT
// Set the soft_serial value to an arbitrary value for proper security (9 bytes)
#define MY_SIGNING_SOFT_SERIAL 0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09 
// Key to use for HMAC calculation in soft signing (32 bytes)
#define MY_SIGNING_SOFT_HMAC_KEY 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00

#define MY_SIGNING_SOFT_RANDOMSEED_PIN 7

// SETTINGS FOR MY_SIGNING_ATSHA204
#define MY_SIGNING_ATSHA204_PIN 17 // A3 - pin where ATSHA204 is attached

#include <SPI.h>
#include <MySensor.h>


#define LOCK_1  3  // Arduino Digital I/O pin number for first lock (second on pin+1 etc)
#define NOF_LOCKS 1 // Total number of attached locks
#define LOCK_LOCK 1  // GPIO value to write to lock attached lock
#define LOCK_UNLOCK 0 // GPIO value to write to unlock attached lock

void setup() {
  for (int lock=1, pin=LOCK_1; lock<=NOF_LOCKS;lock++, pin++) {
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
  for (int lock=1, pin=LOCK_1; lock<=NOF_LOCKS;lock++, pin++) {
    // Register all locks to gw (they will be created as child devices)
    present(lock, S_LOCK, "SecureActuator", false);
  }
}

void loop() 
{
}

void receive(const MyMessage &message) {
  // We only expect one type of message from controller. But we better check anyway.
  if (message.type==V_LOCK_STATUS && message.sensor<=NOF_LOCKS) {
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

