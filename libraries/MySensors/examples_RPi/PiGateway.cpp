/**
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Marcelo Aquino <marceloaqno@gmail.org>
 * Copyleft (c) 2016, Marcelo Aquino
 * Full contributor list: https://github.com/mysensors/Arduino/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */
 
#include <iostream>
#include <cstdio>
#include <unistd.h>

// Enable debug prints to serial monitor
#define MY_DEBUG 

// Enables and select radio type (if attached)
#define MY_RADIO_NRF24

#define MY_GATEWAY_RASPBERRYPI

// Enable MY_IP_ADDRESS here if you want to listen for incoming network
// connections on the specified IP address/hostname only
//#define MY_IP_ADDRESS 192,168,178,87

// The port to keep open on node server mode 
#define MY_PORT 5003      

// How many clients should be able to connect to this gateway (default 1)
#define MY_GATEWAY_MAX_CLIENTS 2

/* MQTT BUILD */
#ifdef MY_GATEWAY_MQTT_CLIENT
//Linux MQTT Defines - Uncomment and modify as needed.  Written as defaults.
//#define MQTT_IP "127.0.0.1"
//#define MQTT_PORT 1883
//#define MQTT_KEEPALIVE 60
// Enable these if your MQTT broker requires usenrame/password
//#define MY_MQTT_USER "username"
//#define MY_MQTT_PASSWORD "password"
//#define MY_MQTT_PUBLISH_TOPIC_PREFIX "mygateway1-out"
//#define MY_MQTT_SUBSCRIBE_TOPIC_PREFIX "mygateway1-in"
#endif

#include <MySensor.h>

using namespace std;

void setup() { 
}

void presentation() {
  // Present locally attached sensors here    
}

void loop() {
  // Send locally attached sensors data here
}
