/*
 The MySensors library adds a new layer on top of the RF24 library.
 It handles radio network routing, relaying and ids.

 Created by Henrik Ekblad <henrik.ekblad@gmail.com>

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.
 */

#ifndef MyMessage_h
#define MyMessage_h

#include <SPI.h>

#define PROTOCOL_VERSION 2
#define MAX_MESSAGE_LENGTH 32
#define HEADER_SIZE 8
#define MAX_PAYLOAD (MAX_MESSAGE_LENGTH - HEADER_SIZE)
#define GATEWAY_ADDRESS ((uint8_t)0)

// Message types
typedef enum {
	C_PRESENTATION = 0,
	C_SET_VARIABLE = 1,
	C_REQ_VARIABLE = 2,
	C_SET_WITH_ACK = 3,
	C_INTERNAL = 4,
	C_STREAM = 5, // For Firmaware and other larger chunks of data that need to be divided into pieces.
	C_FAILED = 6
} command;

// Type of sensor data (for set/req/ack messages)
typedef enum {
	V_TEMP,V_HUM, V_LIGHT, V_DIMMER, V_PRESSURE, V_FORECAST, V_RAIN,
	V_RAINRATE, V_WIND, V_GUST, V_DIRECTION, V_UV, V_WEIGHT, V_DISTANCE,
	V_IMPEDANCE, V_ARMED, V_TRIPPED, V_WATT, V_KWH, V_SCENE_ON, V_SCENE_OFF,
	V_HEATER, V_HEATER_SW, V_LIGHT_LEVEL, V_VAR1, V_VAR2, V_VAR3, V_VAR4, V_VAR5,
	V_UP, V_DOWN, V_STOP, V_IR_SEND, V_IR_RECEIVE, V_FLOW, V_VOLUME, V_LOCK_STATUS
} data;

// Type of internal messages (for internal messages)
typedef enum {
	I_BATTERY_LEVEL, I_TIME, I_VERSION, I_REQUEST_ID,
	I_INCLUSION_MODE, I_RELAY_NODE, I_PING, I_PING_ACK,
	I_LOG_MESSAGE, I_CHILDREN, I_UNIT, I_SKETCH_NAME, I_SKETCH_VERSION
} internal;

// Type of sensor  (for presentation message)
typedef enum {
	S_DOOR, S_MOTION, S_SMOKE, S_LIGHT, S_DIMMER, S_COVER, S_TEMP, S_HUM, S_BARO, S_WIND,
	S_RAIN, S_UV, S_WEIGHT, S_POWER, S_HEATER, S_DISTANCE, S_LIGHT_LEVEL, S_ARDUINO_NODE,
	S_ARDUINO_RELAY, S_LOCK, S_IR, S_WATER
} sensor;

// Type of data stream  (for streamed message)
typedef enum {
	ST_FIRMWARE, ST_SOUND, ST_IMAGE
} stream;

typedef enum {
	P_STRING, P_BYTE, P_INT16, P_UINT16, P_LONG32, P_ULONG32, P_CUSTOM
} payload;

// Possible return values by validate() when doing crc check of received message.
typedef enum {
	VALIDATE_OK, VALIDATE_BAD_CRC, VALIDATE_BAD_VERSION
} validationResult;



#define BIT(n)                  ( 1<<(n) )
// Create a bitmask of length len.
#define BIT_MASK(len)           ( BIT(len)-1 )
// Create a bitfield mask of length starting at bit 'start'.
#define BF_MASK(start, len)     ( BIT_MASK(len)<<(start) )
// Prepare a bitmask for insertion or combining.
#define BF_PREP(x, start, len)  ( ((x)&BIT_MASK(len)) << (start) )
// Extract a bitfield of length len starting at bit 'start' from y.
#define BF_GET(y, start, len)   ( ((y)>>(start)) & BIT_MASK(len) )
// Insert a new bitfield value x into y.
#define BF_SET(y, x, start, len)    ( y= ((y) &~ BF_MASK(start, len)) | BF_PREP(x, start, len) )



class MyMessage
{
public:
	// Constructors
	MyMessage(uint8_t sensor, uint8_t type, uint8_t destination=GATEWAY_ADDRESS);
	MyMessage();

	/* Check this after callback to see if request failed */
//	bool failed();


	// Getters for header
	uint8_t getCRC();
	uint8_t getVersion();
	uint8_t getLength();
	uint8_t getCommand();
	uint8_t getPayloadType();
	uint8_t getType();
	uint8_t getSensor();
	uint8_t getSender();
	uint8_t getLast();
	uint8_t getDestination();

	// Getters for payload


	/**
	 * If payload is something else than P_STRING you can have payload the value converted
	 * to string representation by supplying a buffer with the minimum size of
	 * 2*MAX_PAYLOAD+1. This is to be able to fit hex-conversion of a full binary payload.
	 */
	char* getString(char *buffer = NULL);

	/**
	 * Returns payload untouched
	 */
	void* getCustom();

	uint8_t getByte();
	bool getBool();
	double getDouble();
	long getLong();
	unsigned long getULong();
	int getInt();
	unsigned int getUInt();

	// Builder helper
	MyMessage build(uint8_t sender, uint8_t destination, uint8_t sensor, uint8_t command, uint8_t type);

	// Setters for header
	MyMessage setCRC(uint8_t crc);
	MyMessage setVersion(uint8_t version);
	MyMessage setLength(uint8_t length);
	MyMessage setCommand(uint8_t command);
	MyMessage setPayloadType(uint8_t pt);
	MyMessage setType(uint8_t type);
	MyMessage setSensor(uint8_t sensor);
	MyMessage setSender(uint8_t sender);
	MyMessage setLast(uint8_t last);
	MyMessage setDestination(uint8_t destination);


	// Setters for payload
	MyMessage set(void* payload, uint8_t length);
	MyMessage set(const char* value);
	MyMessage set(uint8_t value);
	MyMessage set(bool value);
	MyMessage set(double value, uint8_t decimals);
	MyMessage set(unsigned long value);
	MyMessage set(long value);
	MyMessage set(unsigned int value);
	MyMessage set(int value);


	uint8_t version_length;  // 3 bit - Protocol version + 5 bit - Length of payload
	uint8_t command_payload; // 4 bit - Command type + 4 bit - Payload data type
	uint8_t type;            // 8 bit - Type varies depending on command
	uint8_t sensor;          // 8 bit - Id of sensor that this message concerns.
	uint8_t sender;          // 8 bit - Id of sender node
	uint8_t last;            // 8 bit - Id of last node this message passed
	uint8_t destination;     // 8 bit - Id of destination node
	uint8_t crc;             // 8 bit - Message checksum
	// Each message can transfer a payload. We add one extra byte for string
	// terminator \0 to be "printable" this is not transferred OTA
	// This union is used to simplify the construction of the binary transferred int/long values.
	union {
		uint8_t bValue;
		unsigned long ulValue;
		long lValue;
		unsigned int uiValue;
		int iValue;
		char data[MAX_PAYLOAD + 1];
	};


};

#endif
