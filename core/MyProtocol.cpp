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
 */

#include "MyConfig.h"
#include "MyTransport.h"
#include "MyProtocol.h"
#include "MyHelperFunctions.h"
#include <string.h>

char _fmtBuffer[MY_GATEWAY_MAX_SEND_LENGTH];
char _convBuffer[MAX_PAYLOAD * 2 + 1];

bool protocolSerial2MyMessage(MyMessage &message, char *inputString)
{
	char *str, *p;
	uint8_t index = 0;
	uint8_t command = 0;
	message.sender = GATEWAY_ADDRESS;
	message.last = GATEWAY_ADDRESS;
	mSetAck(message, false);

	// Extract command data coming on serial line
	for (str = strtok_r(inputString, ";", &p); // split using semicolon
	        str && index < 6; // loop while str is not null an max 5 times
	        str = strtok_r(NULL, ";", &p) // get subsequent tokens
	    ) {
		switch (index) {
		case 0: // Radio id (destination)
			message.destination = atoi(str);
			break;
		case 1: // Child id
			message.sensor = atoi(str);
			break;
		case 2: // Message type
			command = atoi(str);
			mSetCommand(message, command);
			break;
		case 3: // Should we request ack from destination?
			mSetRequestAck(message, atoi(str) ? 1 : 0);
			break;
		case 4: // Data type
			message.type = atoi(str);
			break;
		case 5: {// Variable value
			if (command == C_STREAM) {
				uint8_t bvalue[MAX_PAYLOAD];
				uint8_t blen = 0;
				while (*str) {
					uint8_t val;
					val = convertH2I(*str++) << 4;
					val += convertH2I(*str++);
					bvalue[blen] = val;
					blen++;
				}
				message.set(bvalue, blen);
			} else {
				char *value = str;
				// Remove trailing carriage return and newline character (if it exists)
				const uint8_t lastCharacter = strlen(value) - 1;
				if (value[lastCharacter] == '\r' || value[lastCharacter] == '\n') {
					value[lastCharacter] = '\0';
				}
				message.set(value);
			}
			break;
		}
		}
		index++;
	}
	// Return true if input valid
	return (index == 6);
}

char *protocolMyMessage2Serial(MyMessage &message)
{
	(void)snprintf_P(_fmtBuffer, MY_GATEWAY_MAX_SEND_LENGTH,
	                 PSTR("%" PRIu8 ";%" PRIu8 ";%" PRIu8 ";%" PRIu8 ";%" PRIu8 ";%s\n"), message.sender,
	                 message.sensor, mGetCommand(message), mGetAck(message), message.type,
	                 message.getString(_convBuffer));
	return _fmtBuffer;
}

char *protocolMyMessage2MQTT(const char *prefix, MyMessage &message)
{
	(void)snprintf_P(_fmtBuffer, MY_GATEWAY_MAX_SEND_LENGTH,
	                 PSTR("%s/%" PRIu8 "/%" PRIu8 "/%" PRIu8 "/%" PRIu8 "/%" PRIu8 ""), prefix,
	                 message.sender, message.sensor, mGetCommand(message), mGetAck(message), message.type);
	return _fmtBuffer;
}

bool protocolMQTT2MyMessage(MyMessage &message, char *topic, uint8_t *payload,
                            const unsigned int length)
{
	char *str, *p;
	uint8_t index = 0;
	message.sender = GATEWAY_ADDRESS;
	message.last = GATEWAY_ADDRESS;
	mSetAck(message, false);
	for (str = strtok_r(topic + strlen(MY_MQTT_SUBSCRIBE_TOPIC_PREFIX) + 1, "/", &p);
	        str && index < 5;
	        str = strtok_r(NULL, "/", &p)
	    ) {
		switch (index) {
		case 0:
			// Node id
			message.destination = atoi(str);
			break;
		case 1:
			// Sensor id
			message.sensor = atoi(str);
			break;
		case 2: {
			// Command type
			const uint8_t command = atoi(str);
			mSetCommand(message, command);
			// Add payload
			if (command == C_STREAM) {
				uint8_t bvalue[MAX_PAYLOAD];
				uint8_t blen = 0;
				while (*payload) {
					uint8_t val;
					val = convertH2I(*payload++) << 4;
					val += convertH2I(*payload++);
					bvalue[blen] = val;
					blen++;
				}
				message.set(bvalue, blen);
			} else {
				// terminate string
				char *value = (char *)payload;
				value[length] = '\0';
				message.set((const char*)payload);
			}
			break;
		}
		case 3:
			// Ack flag
			mSetRequestAck(message, atoi(str) ? 1 : 0);
			break;
		case 4:
			// Sub type
			message.type = atoi(str);
			break;
		}
		index++;
	}
	// Return true if input valid
	return (index == 5);
}
