/*
	Vera Arduino library is used to communicate sensor data to your Vera receiver sketch using RF24 library.

	Created by Henrik Ekblad <henrik.ekblad@gmail.com>
	Version 1.0

	This program is free software; you can redistribute it and/or
	modify it under the terms of the GNU General Public License
	version 2 as published by the Free Software Foundation.

	modification by axillent@gmail.com to run on pure AVR

	before running this you need to configure properly RF24-avrlib
*/

#ifndef Vera_h
#define Vera_h

#include <RF24/nRF24L01.h>
#include <RF24/RF24.h>
#include <RF24/RF24_config.h>
#include <crc8/crc8.h>

#define LIBRARY_VERSION "1.2+"
#define PROTOCOL_VERSION 1
#define BAUD_RATE 115200
#define AUTO 0xFFF // Id 4095 is reserved for auto initialization of radioId.
#define NODE_CHILD_ID 0xFF // Node child id is always created for when a new sensor is detected

#define VERA_CHANNEL	76

// This is the radioId for Vera receiver sketch (where all sensors should send their data).
// This is also act as base value for sensor radioId
#define BASE_RADIO_ID 0xABCDABC000LL
#define GATEWAY_ADDRESS ((uint16_t)0)

#define MAX_MESSAGE_LENGTH 32

#define CRC8INIT  0x00
#define CRC8POLY  0x18              //0X18 = X^8+X^5+X^4+X^0

typedef enum { M_PRESENTATION = 0, M_VARIABLE, M_STATUS, M_CUSTOM, M_GATEWAY_MESSAGE, M_REQUEST_STATUS_RESPONSE } messageType;

// Adding new variable- and device types should be done at the end of enum
typedef enum { V_TEMP,V_HUM, V_LIGHT, V_DIMMER, V_PRESSURE, V_FORECAST, V_RAIN, V_RAINRATE, V_WIND, V_GUST,
			   V_DIRECTION, V_UV, V_WEIGHT, V_DISTANCE, V_IMPEDANCE, V_BATTERY_LEVEL, V_BATTERY_DATE,
			   V_ARMED, V_TRIPPED, V_LAST_TRIP, V_WATT, V_KWH, V_SCENE_ON, V_SCENE_OFF, V_HEATER,
			   V_HEATER_SW, V_LIGHT_LEVEL, V_VAR1, V_VAR2, V_VAR3, V_VAR4, V_VAR5, V_TIME, V_VERSION,
			   V_REQUEST_ID, V_INCLUSION_MODE, V_INCLUSION_COUNT, V_INCLUSION_RESULT, V_NEIGHBORS,
			   V_RELAY_MODE, V_LAST_UPDATE} variable;

typedef enum { S_DOOR, S_MOTION, S_SMOKE, S_LIGHT, S_DIMMER, S_COVER, S_TEMP, S_HUM, S_BARO, S_WIND,
			   S_RAIN, S_UV, S_WEIGHT, S_POWER, S_HEATER, S_DISTANCE, S_LIGHT_LEVEL, S_ARDUINO_NODE} senosr;

// result of validate()
enum { VALIDATE_OK=0, VALIDATE_BAD_CRC, VALIDATE_BAD_VERSION };


typedef struct {
  uint8_t crc       	 	 : 8;    // 8 bits crc
  uint8_t version       	 : 3;    // 3 bits protocol version
  uint8_t binary			 : 1; 	 // 1 bit. Data is binary and should be encoded when sent to vera
  uint16_t from				 : 12;	 // 12 bits. RadioId of sender node
  uint16_t to      	         : 12;   // 12 bits. RadioId of destination node
  uint8_t childId            : 8;	 // 1 byte. Up to MAX_CHILD_DEVICES child sensors per radioId
  uint8_t messageType        : 4;    // 4 bits. Type of message. See messageType
  uint8_t type               : 8;	 // 8 bits. variableType or deviceType depending on messageType
} header_s;

typedef struct {
  header_s header;
  char data[MAX_MESSAGE_LENGTH - sizeof(header_s) + 1];  // Each message can transfer a payload. Add one extra byte for \0
} message_s;



/**
* Begin operation of the vera library
*
* Call this in setup(), before calling any other vera library methods.
*/
void VeraAVR_begin(uint16_t _radioId);
void VeraAVR_begin2(uint16_t _radioId, uint16_t _relayId);

/**
* Sends a variable change to vera
*
* @param childId  The child id for which to update variable. Value can be 0-127.
* @param variableType The variableType to update
* @param value  New value of the variable
*/
void VeraAVR_sendVariable_char (uint8_t childId, uint8_t variableType, const char *value);
void VeraAVR_sendVariable_float (uint8_t childId, uint8_t variableType, float value, int decimals);
void VeraAVR_sendVariable_long(uint8_t childId, uint8_t variableType, long value);
void VeraAVR_sendVariable_int(uint8_t childId, uint8_t variableType, int value);

/**
* Requests status for a vera variable (sent from an actuator)
*
* @param childId  The unique child id for the different sensors connected to this arduino. 0-127.
* @param variableType The variableType to update
*/
void VeraAVR_requestStatus (uint8_t childId, uint8_t variableType);

/**
* Requests configuration parameter
*
* @param variableType The variableType to fetch
*/
void VeraAVR_requestConfiguration(uint8_t variableType);
char * VeraAVR_getConfiguration(uint8_t variableType);
/**
 * Fetches time from vera
 */
unsigned long VeraAVR_getTime();
/*
* The arduino must send a presentation of all the sensors connected before any variable changes will be registrered on Vera side.
* Usually it's good to present all sensors when Arduino starts up (setup).
* Waits until all data has been transmitted and acknowledged by Vera receiver (retries RESEND_PRESENTATION times).
*
* @param childId The unique child id for the different sensors connected to this arduino. 0-254.
* @param sensorType Sensor types to create. They will be numbered from 0 up to 127.
*/
void VeraAVR_sendSensorPresentation (uint8_t childId, uint8_t sensorType);

/**
* Registers this arduino device as a relay. This requires this node to call
* api methods regularly to relay messages that could be passing this node and
* answer to any neighbor-discovery messages.
* Basically all api methods relays messages (waitForMessage, messageAvailable, ..)
* When this method is called vera will be informed that this node acts as relay.
*/
void VeraAVR_registerNodeAsRelay();
/**
* Busy waits until there is a message available to be read (used for actuators, like relays)
*/
message_s VeraAVR_waitForVeraMessage(void);
/**
* Returns true if there is a message addressed for this node is available to be read (used by VeraGateway)
*/
uint8_t VeraAVR_messageAvailable();
	/**
	* Returns the last received message
	*/
message_s VeraAVR_getMessage();
uint8_t sendData(uint16_t from, uint16_t next, uint16_t to, uint8_t childId, messageType messageType, uint8_t type, const char *data);
uint8_t VeraAVR_validate();
uint8_t VeraAVR_crc8Message(message_s);
uint8_t VeraAVR_checkCRCMessage(message_s);
uint8_t VeraAVR_checkCRC();
message_s VeraAVR_readMessage(void);
uint8_t VeraAVR_sendDataGW(uint8_t childId, messageType messageType, uint8_t type, const char *data);
uint8_t VeraAVR_sendData(uint16_t from, uint16_t next, uint16_t to, uint8_t childId, messageType messageType, uint8_t type, const char *data);

#include "VeraAVR.c"

#endif
