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

// Enable debug prints to monitor
#ifndef MY_DEBUG
	#define MY_DEBUG
#endif

// Config file
#ifndef MY_LINUX_CONFIG_FILE
	#define MY_LINUX_CONFIG_FILE "/etc/MySensorGateway.cfg"
#endif

// Enables and select radio type (if attached)
#ifndef MY_RADIO_NRF24
	#define MY_RADIO_NRF24
#endif

#define MY_GATEWAY_RASPBERRYPI   

// How many clients should be able to connect to this gateway (default 1)
#define MY_GATEWAY_MAX_CLIENTS 2

#ifdef MY_GATEWAY_MQTT_CLIENT
	// Set MQTT client id
	#ifndef MY_MQTT_CLIENT_ID
		#define MY_MQTT_CLIENT_ID "mysensors-1"
	#endif
	// MQTT broker ip address.
	#ifndef MY_CONTROLLER_IP_ADDRESS
		#define MY_CONTROLLER_IP_ADDRESS 192, 168, 178, 68
	#endif
	// The MQTT broker port to to open 
	#ifndef MY_PORT
		#define MY_PORT 1883
	#endif

	// Enable these if your MQTT broker requires usenrame/password
	//#define MY_MQTT_USER "username"
	//#define MY_MQTT_PASSWORD "password"
	
	// Set this node's subscribe and publish topic prefix
	#ifndef MY_MQTT_PUBLISH_TOPIC_PREFIX
		#define MY_MQTT_PUBLISH_TOPIC_PREFIX "mygateway1-out"
	#endif
	#ifndef MY_MQTT_SUBSCRIBE_TOPIC_PREFIX
		#define MY_MQTT_SUBSCRIBE_TOPIC_PREFIX "mygateway1-in"
	#endif
#else
	// Enable MY_IP_ADDRESS here if you want to listen for incoming network
	// connections on the specified IP address/hostname only
	//#define MY_IP_ADDRESS 192,168,178,87

	// The port to keep open on node server mode 
	#ifndef MY_PORT
		#define MY_PORT 5003
	#endif
#endif

#include <MySensors.h>

void setup() { 
}

void presentation() {
  // Present locally attached sensors here    
}

void loop() {
  // Send locally attached sensors data here
}
