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


#include "MyMessage.h"
#include "MyHelperFunctions.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

MyMessage::MyMessage(void)
{
	clear();
}

MyMessage::MyMessage(const uint8_t _sensor, const uint8_t _type)
{
	clear();
	sensor = _sensor;
	type   = _type;
}

void MyMessage::clear(void)
{
	last                = 0u;
	sender              = 0u;
	destination         = 0u; // Gateway is default destination
	version_length      = 0u;
	command_ack_payload = 0u;
	type                = 0u;
	sensor              = 0u;
	(void)memset((void *)data, 0u, sizeof(data));

	// set message protocol version
	miSetVersion(PROTOCOL_VERSION);
}

bool MyMessage::isAck(void) const
{
	return miGetAck();
}

uint8_t MyMessage::getCommand(void) const
{
	return miGetCommand();
}

/* Getters for payload converted to desired form */
void* MyMessage::getCustom(void) const
{
	return (void *)data;
}

const char* MyMessage::getString(void) const
{
	uint8_t payloadType = miGetPayloadType();
	if (payloadType == P_STRING) {
		return data;
	} else {
		return NULL;
	}
}

char* MyMessage::getCustomString(char *buffer) const
{
	for (uint8_t i = 0; i < miGetLength(); i++) {
		buffer[i * 2] = convertI2H(data[i] >> 4);
		buffer[(i * 2) + 1] = convertI2H(data[i]);
	}
	buffer[miGetLength() * 2] = '\0';
	return buffer;
}

char* MyMessage::getStream(char *buffer) const
{
	uint8_t cmd = miGetCommand();
	if ((cmd == C_STREAM) && (buffer != NULL)) {
		return getCustomString(buffer);
	} else {
		return NULL;
	}
}

char* MyMessage::getString(char *buffer) const
{
	uint8_t payloadType = miGetPayloadType();
	if (buffer != NULL) {
		if (payloadType == P_STRING) {
			(void)strncpy(buffer, data, miGetLength());
			buffer[miGetLength()] = 0;
		} else if (payloadType == P_BYTE) {
			(void)itoa(bValue, buffer, 10);
		} else if (payloadType == P_INT16) {
			(void)itoa(iValue, buffer, 10);
		} else if (payloadType == P_UINT16) {
			(void)utoa(uiValue, buffer, 10);
		} else if (payloadType == P_LONG32) {
			(void)ltoa(lValue, buffer, 10);
		} else if (payloadType == P_ULONG32) {
			(void)ultoa(ulValue, buffer, 10);
		} else if (payloadType == P_FLOAT32) {
			(void)dtostrf(fValue, 2, min(fPrecision, (uint8_t)8), buffer);
		} else if (payloadType == P_CUSTOM) {
			return getCustomString(buffer);
		}
		return buffer;
	} else {
		return NULL;
	}
}

bool MyMessage::getBool(void) const
{
	return getByte();
}

uint8_t MyMessage::getByte(void) const
{
	if (miGetPayloadType() == P_BYTE) {
		return data[0];
	} else if (miGetPayloadType() == P_STRING) {
		return atoi(data);
	} else {
		return 0;
	}
}


float MyMessage::getFloat(void) const
{
	if (miGetPayloadType() == P_FLOAT32) {
		return fValue;
	} else if (miGetPayloadType() == P_STRING) {
		return atof(data);
	} else {
		return 0;
	}
}

int32_t MyMessage::getLong(void) const
{
	if (miGetPayloadType() == P_LONG32) {
		return lValue;
	} else if (miGetPayloadType() == P_STRING) {
		return atol(data);
	} else {
		return 0;
	}
}

uint32_t MyMessage::getULong(void) const
{
	if (miGetPayloadType() == P_ULONG32) {
		return ulValue;
	} else if (miGetPayloadType() == P_STRING) {
		return atol(data);
	} else {
		return 0;
	}
}

int16_t MyMessage::getInt(void) const
{
	if (miGetPayloadType() == P_INT16) {
		return iValue;
	} else if (miGetPayloadType() == P_STRING) {
		return atoi(data);
	} else {
		return 0;
	}
}

uint16_t MyMessage::getUInt(void) const
{
	if (miGetPayloadType() == P_UINT16) {
		return uiValue;
	} else if (miGetPayloadType() == P_STRING) {
		return atoi(data);
	} else {
		return 0;
	}
}

MyMessage& MyMessage::setType(const uint8_t _type)
{
	type = _type;
	return *this;
}

MyMessage& MyMessage::setSensor(const uint8_t _sensor)
{
	sensor = _sensor;
	return *this;
}

MyMessage& MyMessage::setDestination(const uint8_t _destination)
{
	destination = _destination;
	return *this;
}

// Set payload
MyMessage& MyMessage::set(const void* value, const size_t length)
{
	const size_t payloadLength = value == NULL ? 0 : min(length, (size_t)MAX_PAYLOAD);
	miSetLength(payloadLength);
	miSetPayloadType(P_CUSTOM);
	(void)memcpy(data, value, payloadLength);
	return *this;
}

MyMessage& MyMessage::set(const char* value)
{
	const size_t payloadLength = value == NULL ? 0 : min(strlen(value), (size_t)MAX_PAYLOAD);
	miSetLength(payloadLength);
	miSetPayloadType(P_STRING);
	if (payloadLength) {
		(void)strncpy(data, value, payloadLength);
	}
	// null terminate string
	data[payloadLength] = 0;
	return *this;
}

#if !defined(__linux__)
MyMessage& MyMessage::set(const __FlashStringHelper* value)
{
	const size_t payloadLength = value == NULL ? 0
	                             : min(strlen_P(reinterpret_cast<const char *>(value)), (size_t)MAX_PAYLOAD);
	miSetLength(payloadLength);
	miSetPayloadType(P_STRING);
	if (payloadLength) {
		(void)strncpy_P(data, reinterpret_cast<const char *>(value), payloadLength);
	}
	// null terminate string
	data[payloadLength] = 0;
	return *this;
}
#endif


MyMessage& MyMessage::set(const bool value)
{
	miSetLength(1);
	miSetPayloadType(P_BYTE);
	data[0] = value;
	return *this;
}

MyMessage& MyMessage::set(const uint8_t value)
{
	miSetLength(1);
	miSetPayloadType(P_BYTE);
	data[0] = value;
	return *this;
}

MyMessage& MyMessage::set(const float value, const uint8_t decimals)
{
	miSetLength(5); // 32 bit float + persi
	miSetPayloadType(P_FLOAT32);
	fValue=value;
	fPrecision = decimals;
	return *this;
}

MyMessage& MyMessage::set(const uint32_t value)
{
	miSetPayloadType(P_ULONG32);
	miSetLength(4);
	ulValue = value;
	return *this;
}

MyMessage& MyMessage::set(const int32_t value)
{
	miSetPayloadType(P_LONG32);
	miSetLength(4);
	lValue = value;
	return *this;
}

MyMessage& MyMessage::set(const uint16_t value)
{
	miSetPayloadType(P_UINT16);
	miSetLength(2);
	uiValue = value;
	return *this;
}

MyMessage& MyMessage::set(const int16_t value)
{
	miSetPayloadType(P_INT16);
	miSetLength(2);
	iValue = value;
	return *this;
}
