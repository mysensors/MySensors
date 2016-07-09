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
 */

#include "MyConfig.h"
#include "MyTransport.h"
#include "MyProtocol.h"

uint8_t protocolH2i(char c);

char _fmtBuffer[MY_GATEWAY_MAX_SEND_LENGTH];
char _convBuffer[MAX_PAYLOAD*2+1];

bool protocolParse(MyMessage &message, char *inputString) {
	char *str, *p, *value=NULL;
	uint8_t bvalue[MAX_PAYLOAD];
	uint8_t blen = 0;
	int i = 0;
	uint8_t command = 0;
	uint8_t ack = 0;

	// Extract command data coming on serial line
	for (str = strtok_r(inputString, ";", &p); // split using semicolon
		str && i < 6; // loop while str is not null an max 5 times
		str = strtok_r(NULL, ";", &p) // get subsequent tokens
			) {
		switch (i) {
			case 0: // Radioid (destination)
				message.destination = atoi(str);
				break;
			case 1: // Childid
				message.sensor = atoi(str);
				break;
			case 2: // Message type
				command = atoi(str);
				mSetCommand(message, command);
				break;
			case 3: // Should we request ack from destination?
				ack = atoi(str);
				break;
			case 4: // Data type
				message.type = atoi(str);
				break;
			case 5: // Variable value
				if (command == C_STREAM) {
					blen = 0;
					uint8_t val;
					while (*str) {
						val = protocolH2i(*str++) << 4;
						val += protocolH2i(*str++);
						bvalue[blen] = val;
						blen++;
					}
				} else {
					value = str;
					// Remove trailing carriage return and newline character (if it exists)
					uint8_t lastCharacter = strlen(value)-1;
					if (value[lastCharacter] == '\r')
						value[lastCharacter] = 0;
					if (value[lastCharacter] == '\n')
						value[lastCharacter] = 0;
				}
				break;
		}
		i++;
	}
	//debug(PSTR("Received %d"), i);
	// Check for invalid input
	if (i < 5)
		return false;

	message.sender = GATEWAY_ADDRESS;
	message.last = GATEWAY_ADDRESS;
    mSetRequestAck(message, ack?1:0);
    mSetAck(message, false);
	if (command == C_STREAM)
		message.set(bvalue, blen);
	else
		message.set(value);
	return true;
}

char * protocolFormat(MyMessage &message) {
	snprintf_P(_fmtBuffer, MY_GATEWAY_MAX_SEND_LENGTH, PSTR("%d;%d;%d;%d;%d;%s\n"), message.sender, message.sensor, (uint8_t)mGetCommand(message), (uint8_t)mGetAck(message), message.type, message.getString(_convBuffer));
	return _fmtBuffer;
}

uint8_t protocolH2i(char c) {
	uint8_t i = 0;
	if (c <= '9')
		i += c - '0';
	else if (c >= 'a')
		i += c - 'a' + 10;
	else
		i += c - 'A' + 10;
	return i;
}


