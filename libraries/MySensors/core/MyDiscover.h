/*
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2016 Sensnology AB
 * Full contributor list: https://github.com/mysensors/Arduino/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

#ifndef MyDiscover_h
#define MyDiscover_h

#include "MyTransport.h"	// NodeFirmwareConfig def


#define MY_DISCOVER_HEADER_VERSION 1
// total
#define MY_DISCOVER_TOTAL_PAGES 5
// page types
#define MY_DISCOVER_TYPEID_PARENT 0
#define MY_DISCOVER_TYPEID_GENERAL 1
#define MY_DISCOVER_TYPEID_ARCHITECTURE 2
#define MY_DISCOVER_TYPEID_BOOTLOADER 3
#define MY_DISCOVER_TYPEID_TRANSPORT 4
#define MY_DISCOVER_TYPEID_PERIPHERY 5
// pages
#define MY_DISCOVER_PAGEID_PARENT 0
#define MY_DISCOVER_PAGEID_GENERAL 1
#define MY_DISCOVER_PAGEID_ARCHITECTURE 2
#define MY_DISCOVER_PAGEID_BOOTLOADER 3
#define MY_DISCOVER_PAGEID_TRANSPORT_UPLINK 4
// data rates
#define MY_DISCOVER_BAUD_UNKNOWN 0
#define MY_DISCOVER_BAUD_9600 1
#define MY_DISCOVER_BAUD_19200 2
#define MY_DISCOVER_BAUD_38400 3
#define MY_DISCOVER_BAUD_57600 4
#define MY_DISCOVER_BAUD_115200 5
// hardware ids
#define MY_DISCOVER_HWID_UNKNOWN 0
#define MY_DISCOVER_HWID_AVR 1
#define MY_DISCOVER_HWID_ESP8266 2
#define MY_DISCOVER_HWID_SAMD 3
// transport types
#define MY_DISCOVER_TRANSPORT_TYPE_UNKNOWN 0
#define MY_DISCOVER_TRANSPORT_TYPE_RF24 1
#define MY_DISCOVER_TRANSPORT_TYPE_RFM69 2
#define MY_DISCOVER_TRANSPORT_TYPE_RS232 3
#define MY_DISCOVER_TRANSPORT_TYPE_RS485 4
#define MY_DISCOVER_TRANSPORT_TYPE_TCP 5


// MY_HWID
#if defined(ARDUINO_ARCH_AVR)
	// to read fuse settings
	#include <avr/boot.h>
	#define MY_DISCOVER_HARDWARE_ID MY_DISCOVER_HWID_AVR
#elif defined (ARDUINO_ARCH_ESP8266)
	#define MY_DISCOVER_HARDWARE_ID MY_DISCOVER_HWID_ESP8266
#elif defined (ARDUINO_ARCH_SAMD)
	#define MY_DISCOVER_HARDWARE_ID MY_DISCOVER_HWID_SAMD
#else
	#define MY_DISCOVER_HARDWARE_ID MY_DISCOVER_HWID_UNKNOWN
#endif
// transport count
#if defined(MY_GATEWAY_SERIAL) 
	#define MY_DISCOVER_SERIAL 1
#else
	#define MY_DISCOVER_SERIAL 0
#endif

#if defined(MY_GATEWAY_W5100) || defined(MY_GATEWAY_ENC28J60) || defined(MY_GATEWAY_ESP8266) || defined(MY_GATEWAY_MQTT_CLIENT) 
	#define MY_DISCOVER_TCPIP 1
#else
	#define MY_DISCOVER_TCPIP 0
#endif

#if defined(MY_RADIO_NRF24)
	#define MY_DISCOVER_NRF24 1
#else
	#define MY_DISCOVER_NRF24 0
#endif

#if defined(MY_RADIO_RFM69)
	#define MY_DISCOVER_RFM69 1
#else
	#define MY_DISCOVER_RFM69 0
#endif

#if defined(MY_RS485)
	#define MY_DISCOVER_MY_RS485 1
#else
	#define MY_DISCOVER_MY_RS485 0
#endif
#define MY_TRANSPORT_COUNT (MY_DISCOVER_SERIAL + MY_DISCOVER_TCPIP + MY_DISCOVER_NRF24 + MY_DISCOVER_RFM69 + MY_DISCOVER_MY_RS485)

// features
#if defined(MY_REPEATER_FEATURE)
	#define MY_DISCOVER_REPEATER 1
#else
	#define MY_DISCOVER_REPEATER 0
#endif

#if defined(MY_GATEWAY_FEATURE)
	#define MY_DISCOVER_GATEWAY 1
#else
	#define MY_DISCOVER_GATEWAY 0
#endif
// no way of discovering if sensors attached, i.e. gateway + sensor excluded
#if !defined(MY_GATEWAY_FEATURE) || defined(MY_SENSOR_NODE)
	#define MY_DISCOVER_SENSORS 1
#else
	#define MY_DISCOVER_SENSORS 0
#endif

// node power
#if defined(MY_POWER_PSU)
	#define MY_DISCOVER_PSU 1
#else
	#define MY_DISCOVER_PSU 0
#endif
#if defined(MY_POWER_BATTERY)
	#define MY_DISCOVER_BATTERY 1
#else
	#define MY_DISCOVER_BATTERY 0
#endif
#if defined(MY_POWER_SOLAR)
	#define MY_DISCOVER_SOLAR 1
#else
	#define MY_DISCOVER_SOLAR 0
#endif
#define MY_DISCOVER_NODE_TYPE (MY_DISCOVER_SOLAR<<6) |(MY_DISCOVER_BATTERY<<5) |(MY_DISCOVER_PSU<<4) | (MY_DISCOVER_REPEATER<<2) | (MY_DISCOVER_REPEATER<<1) | MY_DISCOVER_SENSORS

// node features
#if defined(MY_OTA_FIRMWARE_FEATURE)
	#define MY_DISCOVER_OTA_FIRMWARE_FEATURE 1
#else
	#define MY_DISCOVER_OTA_FIRMWARE_FEATURE 0
#endif
#if defined(MY_DISABLE_REMOTE_RESET)
	#define MY_DISCOVER_REMOTE_RESET 1
#else
	#define MY_DISCOVER_REMOTE_RESET 0
#endif
#if defined(MY_SLEEPING_NODE)
	#define MY_DISCOVER_SLEEPING_NODE 1
#else
	#define MY_DISCOVER_SLEEPING_NODE 0
#endif
#define MY_DISCOVER_NODE_FEATURES (MY_DISCOVER_SLEEPING_NODE<<2) |(MY_DISCOVER_REMOTE_RESET<<1) | MY_DISCOVER_OTA_FIRMWARE_FEATURE

// uplink transport
#if defined(MY_GATEWAY_SERIAL)
	#define MY_DISCOVER_TRANSPORT_PARENT_TYPE MY_DISCOVER_TRANSPORT_TYPE_RS232
	#define MY_DISCOVER_PARENT_POWER_LEVEL 0
	#define MY_DISCOVER_PARENT_CHANNEL (uint16_t)0
	
	#if (MY_BAUD_RATE==9600)
		#define MY_DISCOVER_PARENT_DATA_RATE MY_DISCOVER_BAUD_9600
	#elif (MY_BAUD_RATE==19200)
		#define MY_DISCOVER_PARENT_DATA_RATE MY_DISCOVER_BAUD_19200
	#elif (MY_BAUD_RATE==38400)
		#define MY_DISCOVER_PARENT_DATA_RATE MY_DISCOVER_BAUD_38400
	#elif (MY_BAUD_RATE==57600)
		#define MY_DISCOVER_PARENT_DATA_RATE MY_DISCOVER_BAUD_57600
	#elif (MY_BAUD_RATE==115200)
		#define MY_DISCOVER_PARENT_DATA_RATE MY_DISCOVER_BAUD_115200
	#else
		#define MY_DISCOVER_PARENT_DATA_RATE MY_DISCOVER_BAUD_UNKNOWN
	#endif
			
#elif defined(MY_GATEWAY_W5100) || defined(MY_GATEWAY_ENC28J60) || defined(MY_GATEWAY_ESP8266) || defined(MY_GATEWAY_MQTT_CLIENT)
	#define MY_DISCOVER_TRANSPORT_PARENT_TYPE MY_DISCOVER_TRANSPORT_TYPE_TCP
	#define MY_DISCOVER_PARENT_DATA_RATE 0
	#define MY_DISCOVER_PARENT_POWER_LEVEL 0
	#define MY_DISCOVER_PARENT_CHANNEL (uint16_t)MY_PORT

#elif defined(MY_RS485)
	#define MY_DISCOVER_TRANSPORT_PARENT_TYPE MY_DISCOVER_TRANSPORT_TYPE_RS485
	#define MY_DISCOVER_PARENT_POWER_LEVEL 0
	#define MY_DISCOVER_PARENT_CHANNEL (uint16_t)0
	
	#if (MY_BAUD_RATE==9600)
		#define MY_DISCOVER_PARENT_DATA_RATE MY_DISCOVER_BAUD_9600
	#elif (MY_BAUD_RATE==19200)
		#define MY_DISCOVER_PARENT_DATA_RATE MY_DISCOVER_BAUD_19200
	#elif (MY_BAUD_RATE==38400)
		#define MY_DISCOVER_PARENT_DATA_RATE MY_DISCOVER_BAUD_38400
	#elif (MY_BAUD_RATE==57600)
		#define MY_DISCOVER_PARENT_DATA_RATE MY_DISCOVER_BAUD_57600
	#elif (MY_BAUD_RATE==115200)
		#define MY_DISCOVER_PARENT_DATA_RATE MY_DISCOVER_BAUD_115200
	#else
		#define MY_DISCOVER_PARENT_DATA_RATE MY_DISCOVER_BAUD_UNKNOWN
	#endif
	
#elif defined(MY_RADIO_NRF24)
	#define MY_DISCOVER_TRANSPORT_PARENT_TYPE MY_DISCOVER_TRANSPORT_TYPE_RF24
	#define MY_DISCOVER_PARENT_CHANNEL (uint16_t)MY_RF24_CHANNEL
	// data rate
	#if (MY_RF24_DATARATE==RF24_250KBPS)
		#define MY_DISCOVER_PARENT_DATA_RATE 1
	#elif (MY_RF24_DATARATE==RF24_1MBPS)
		#define MY_DISCOVER_PARENT_DATA_RATE 2
	#elif (MY_RF24_DATARATE==RF24_2MBPS)
		#define MY_DISCOVER_PARENT_DATA_RATE 3
	#else
		#define MY_DISCOVER_PARENT_DATA_RATE 0
	#endif
	// tx level
	#if (MY_RF24_PA_LEVEL==RF24_PA_MIN)
		#define MY_DISCOVER_PARENT_POWER_LEVEL 1
	#elif (MY_RF24_PA_LEVEL==RF24_PA_LOW)
		#define MY_DISCOVER_PARENT_POWER_LEVEL 2
	#elif (MY_RF24_PA_LEVEL==RF24_PA_HIGH)
		#define MY_DISCOVER_PARENT_POWER_LEVEL 3
	#elif (MY_RF24_PA_LEVEL==RF24_PA_MAX)
		#define MY_DISCOVER_PARENT_POWER_LEVEL 4
	#else
		#define MY_DISCOVER_PARENT_POWER_LEVEL 0
	#endif


#elif defined(MY_RADIO_RFM69)
	#define MY_DISCOVER_TRANSPORT_PARENT_TYPE MY_DISCOVER_TRANSPORT_TYPE_RFM69
	#define MY_DISCOVER_PARENT_DATA_RATE 0
	#define MY_DISCOVER_PARENT_POWER_LEVEL 0
	#define MY_DISCOVER_PARENT_CHANNEL (uint16_t)MY_RFM69_FREQUENCY
#else
	#define MY_DISCOVER_TRANSPORT_PARENT_TYPE MY_DISCOVER_TRANSPORT_TYPE_UNKNOWN
	#define MY_DISCOVER_PARENT_DATA_RATE 0
	#define MY_DISCOVER_PARENT_POWER_LEVEL 0
	#define MY_DISCOVER_PARENT_CHANNEL (uint16_t)0
#endif

#define MY_DISCOVER_TRANSPORT_PARENT (0 << 4) | MY_DISCOVER_TRANSPORT_PARENT_TYPE

// signing
#if defined(MY_SIGNING_FEATURE)
	#define MY_DISCOVER_SIGNING 1
#else
	#define MY_DISCOVER_SIGNING 0
#endif

#if defined(MY_SIGNING_REQUEST_SIGNATURES)
	#define MY_DISCOVER_REQUEST_SIGNATURES 1
#else
	#define MY_DISCOVER_REQUEST_SIGNATURES 0
#endif

#if defined(MY_SIGNING_NODE_WHITELISTING)
	#define MY_DISCOVER_NODE_WHITELISTING 1
#else
	#define MY_DISCOVER_NODE_WHITELISTING 0
#endif

#define MY_DISCOVER_SIGNATURES (MY_DISCOVER_NODE_WHITELISTING << 2) | (MY_DISCOVER_REQUEST_SIGNATURES << 1) | (MY_DISCOVER_SIGNING) 

// encryption
#if defined(MY_RF24_ENABLE_ENCRYPTION) || defined(MY_RFM69_ENABLE_ENCRYPTION)
	#define MY_DISCOVER_ENCRYPTION_AES 1
#else
	#define MY_DISCOVER_ENCRYPTION_AES 0
#endif

#define MY_DISCOVER_ENCRYPTION MY_DISCOVER_ENCRYPTION_AES

/// @brief MyDiscover data structure
typedef struct {
	uint8_t PAGE_ID:5; //!< Page identifier
	uint8_t Revision:3; //!< Header revision
	uint8_t PAGE_TYPE_ID:4; //!< page type identifier
	uint8_t MY_HWID:4; //!< Hardware ID 
	uint8_t data[MAX_PAYLOAD - 2]; //!< Payload
} __attribute__((packed)) MyDiscover;



/**
 * Send Disover info / tbd
 *
 */
void generateDiscoverResponse(uint8_t page, uint8_t* buffer, uint8_t* len);

#endif // #ifdef MyDiscover_h
