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
	this->clear();
}

MyMessage::MyMessage(const uint8_t _sensorId, const mysensors_data_t _dataType)
{
	this->clear();
	(void)this->setSensor(_sensorId);
	(void)this->setType(static_cast<uint8_t>(_dataType));
}

void MyMessage::clear(void)
{
	this->last                 = 0u;
	this->sender               = 0u;
	this->destination          = GATEWAY_ADDRESS; // Gateway is default destination
	this->version_length       = 0u;
	this->command_echo_payload = 0u;
	this->type                 = 0u;
	this->sensor               = 0u;
	// clear data buffer
	(void)memset((void *)this->data, 0u, sizeof(this->data));

	// set message protocol version
	(void)this->setVersion();
}

uint8_t MyMessage::getHeaderSize(void) const
{
	return (uint8_t)HEADER_SIZE;
}

uint8_t MyMessage::getMaxPayloadSize(void) const
{
	return (uint8_t)MAX_PAYLOAD_SIZE;
}

uint8_t MyMessage::getExpectedMessageSize(void) const
{
	return this->getHeaderSize() + (this->getSigned() ? this->getMaxPayloadSize() : this->getLength());
}

bool MyMessage::isProtocolVersionValid(void) const
{
	return (this->getVersion() == V2_MYS_HEADER_PROTOCOL_VERSION);
}

uint8_t MyMessage::getType(void) const
{
	return this->type;
}

MyMessage& MyMessage::setType(const uint8_t messageType)
{
	this->type = messageType;
	return *this;
}

uint8_t MyMessage::getLast(void) const
{
	return this->last;
}

MyMessage& MyMessage::setLast(const uint8_t lastId)
{
	this->last = lastId;
	return *this;
}

uint8_t MyMessage::getSender(void) const
{
	return this->sender;
}

MyMessage& MyMessage::setSender(const uint8_t senderId)
{
	this->sender = senderId;
	return *this;
}

uint8_t MyMessage::getSensor(void) const
{
	return this->sensor;
}

MyMessage& MyMessage::setSensor(const uint8_t sensorId)
{
	this->sensor = sensorId;
	return *this;
}

uint8_t MyMessage::getDestination(void) const
{
	return this->destination;
}

MyMessage& MyMessage::setDestination(const uint8_t destinationId)
{
	this->destination = destinationId;
	return *this;
}

// TODO: Remove before v3 is released, use isEcho instead
bool MyMessage::isAck(void) const
{
	return this->isEcho();
}

bool MyMessage::isEcho(void) const
{
	return (bool)BF_GET(this->command_echo_payload, V2_MYS_HEADER_CEP_ECHO_POS,
	                    V2_MYS_HEADER_CEP_ECHO_SIZE);
}

MyMessage& MyMessage::setEcho(const bool echo)
{
	BF_SET(this->command_echo_payload, echo, V2_MYS_HEADER_CEP_ECHO_POS,
	       V2_MYS_HEADER_CEP_ECHO_SIZE);
	return *this;
}

bool MyMessage::getRequestEcho(void) const
{
	return (bool)BF_GET(this->command_echo_payload, V2_MYS_HEADER_CEP_ECHOREQUEST_POS,
	                    V2_MYS_HEADER_CEP_ECHOREQUEST_SIZE);
}

MyMessage& MyMessage::setRequestEcho(const bool requestEcho)
{
	BF_SET(this->command_echo_payload, requestEcho, V2_MYS_HEADER_CEP_ECHOREQUEST_POS,
	       V2_MYS_HEADER_CEP_ECHOREQUEST_SIZE);
	return *this;
}

uint8_t MyMessage::getVersion(void) const
{
	return (uint8_t)BF_GET(this->version_length, V2_MYS_HEADER_VSL_VERSION_POS,
	                       V2_MYS_HEADER_VSL_VERSION_SIZE);
}

MyMessage& MyMessage::setVersion(void)
{
	BF_SET(this->version_length, V2_MYS_HEADER_PROTOCOL_VERSION, V2_MYS_HEADER_VSL_VERSION_POS,
	       V2_MYS_HEADER_VSL_VERSION_SIZE);
	return *this;
}

mysensors_command_t MyMessage::getCommand(void) const
{
	return static_cast<mysensors_command_t>(BF_GET(this->command_echo_payload,
	                                        V2_MYS_HEADER_CEP_COMMAND_POS, V2_MYS_HEADER_CEP_COMMAND_SIZE));
}

MyMessage& MyMessage::setCommand(const mysensors_command_t command)
{
	BF_SET(this->command_echo_payload, static_cast<uint8_t>(command), V2_MYS_HEADER_CEP_COMMAND_POS,
	       V2_MYS_HEADER_CEP_COMMAND_SIZE);
	return *this;
}

mysensors_payload_t MyMessage::getPayloadType(void) const
{
	return static_cast<mysensors_payload_t>(BF_GET(this->command_echo_payload,
	                                        V2_MYS_HEADER_CEP_PAYLOADTYPE_POS, V2_MYS_HEADER_CEP_PAYLOADTYPE_SIZE));
}

MyMessage& MyMessage::setPayloadType(const mysensors_payload_t payloadType)
{
	BF_SET(this->command_echo_payload, static_cast<uint8_t>(payloadType),
	       V2_MYS_HEADER_CEP_PAYLOADTYPE_POS, V2_MYS_HEADER_CEP_PAYLOADTYPE_SIZE);
	return *this;
}

bool MyMessage::getSigned(void) const
{
	return (bool)BF_GET(this->version_length, V2_MYS_HEADER_VSL_SIGNED_POS,
	                    V2_MYS_HEADER_VSL_SIGNED_SIZE);
}

MyMessage& MyMessage::setSigned(const bool signedFlag)
{
	BF_SET(this->version_length, signedFlag, V2_MYS_HEADER_VSL_SIGNED_POS,
	       V2_MYS_HEADER_VSL_SIGNED_SIZE);
	return *this;
}

uint8_t MyMessage::getLength(void) const
{
	uint8_t length = BF_GET(this->version_length, V2_MYS_HEADER_VSL_LENGTH_POS,
	                        V2_MYS_HEADER_VSL_LENGTH_SIZE);
	// limit length
	if (length > MAX_PAYLOAD_SIZE) {
		length = MAX_PAYLOAD_SIZE;
	}
	return length;
}

MyMessage& MyMessage::setLength(const uint8_t length)
{
	uint8_t finalLength = length;
	// limit length
	if (finalLength > MAX_PAYLOAD_SIZE) {
		finalLength = MAX_PAYLOAD_SIZE;
	}

	BF_SET(this->version_length, finalLength, V2_MYS_HEADER_VSL_LENGTH_POS,
	       V2_MYS_HEADER_VSL_LENGTH_SIZE);
	return *this;
}

/* Getters for payload converted to desired form */
void* MyMessage::getCustom(void) const
{
	return (void *)this->data;
}

const char* MyMessage::getString(void) const
{
	if (this->getPayloadType() == P_STRING) {
		return this->data;
	} else {
		return NULL;
	}
}

char* MyMessage::getCustomString(char *buffer) const
{
	if (buffer != NULL) {
		for (uint8_t i = 0; i < this->getLength(); i++) {
			buffer[i * 2] = convertI2H(this->data[i] >> 4);
			buffer[(i * 2) + 1] = convertI2H(this->data[i]);
		}
		buffer[this->getLength() * 2] = '\0';
		return buffer;
	} else {
		return NULL;
	}
}

char* MyMessage::getStream(char *buffer) const
{
	if (buffer != NULL) {
		if (this->getCommand() == C_STREAM) {
			return this->getCustomString(buffer);
		}
		return buffer;
	} else {
		return NULL;
	}
}

char* MyMessage::getString(char *buffer) const
{
	if (buffer != NULL) {
		const uint8_t payloadType = this->getPayloadType();
		if (payloadType == P_STRING) {
			(void)strncpy(buffer, this->data, this->getLength());
			buffer[this->getLength()] = 0;
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
			(void)dtostrf(fValue, 2, min(fPrecision, (uint8_t)8u), buffer);
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
	return (bool)this->getByte();
}

uint8_t MyMessage::getByte(void) const
{
	if (this->getPayloadType() == P_BYTE) {
		return (uint8_t)this->data[0];
	} else if (this->getPayloadType() == P_STRING) {
		return (uint8_t)atoi(this->data);
	} else {
		return 0;
	}
}


float MyMessage::getFloat(void) const
{
	if (this->getPayloadType() == P_FLOAT32) {
		return this->fValue;
	} else if (this->getPayloadType() == P_STRING) {
		return (float)atof(this->data);
	} else {
		return 0;
	}
}

int32_t MyMessage::getLong(void) const
{
	if (this->getPayloadType() == P_LONG32) {
		return this->lValue;
	} else if (this->getPayloadType() == P_STRING) {
		return (int32_t)atol(this->data);
	} else {
		return 0;
	}
}

uint32_t MyMessage::getULong(void) const
{
	if (this->getPayloadType() == P_ULONG32) {
		return this->ulValue;
	} else if (this->getPayloadType() == P_STRING) {
		return (uint32_t)atol(this->data);
	} else {
		return 0;
	}
}

int16_t MyMessage::getInt(void) const
{
	if (this->getPayloadType() == P_INT16) {
		return this->iValue;
	} else if (this->getPayloadType() == P_STRING) {
		return (int16_t)atoi(this->data);
	} else {
		return 0;
	}
}

uint16_t MyMessage::getUInt(void) const
{
	if (this->getPayloadType() == P_UINT16) {
		return this->uiValue;
	} else if (this->getPayloadType() == P_STRING) {
		return (uint16_t)atoi(this->data);
	} else {
		return 0;
	}
}

MyMessage& MyMessage::set(const void* value, const size_t _length)
{
	(void)this->setLength((value != NULL) ? _length : 0);
	(void)this->setPayloadType(P_CUSTOM);
	(void)memcpy((void *)this->data, value, this->getLength());
	return *this;
}

MyMessage& MyMessage::set(const char* value)
{
	(void)this->setLength((value != NULL) ? strlen(value) : 0);
	(void)this->setPayloadType(P_STRING);
	(void)strncpy(this->data, value, this->getLength());
	// null terminate string
	this->data[this->getLength()] = 0;
	return *this;
}

#if !defined(__linux__)
MyMessage& MyMessage::set(const __FlashStringHelper* value)
{
	(void)this->setLength((value != NULL) ? strlen_P(reinterpret_cast<const char *>(value)) : 0);
	(void)this->setPayloadType(P_STRING);
	(void)strncpy_P(this->data, reinterpret_cast<const char *>(value), this->getLength());
	// null terminate string
	this->data[this->getLength()] = 0;
	return *this;
}
#endif


MyMessage& MyMessage::set(const bool value)
{
	return this->set((uint8_t)value);
}

MyMessage& MyMessage::set(const uint8_t value)
{
	(void)this->setLength(1u);
	(void)this->setPayloadType(P_BYTE);
	this->bValue = value;
	return *this;
}

MyMessage& MyMessage::set(const float value, const uint8_t decimals)
{
	(void)this->setLength(5u); // 32 bit float + persi
	(void)this->setPayloadType(P_FLOAT32);
	this->fValue = value;
	this->fPrecision = decimals;
	return *this;
}

MyMessage& MyMessage::set(const uint32_t value)
{
	(void)this->setLength(4u);
	(void)this->setPayloadType(P_ULONG32);
	this->ulValue = value;
	return *this;
}

MyMessage& MyMessage::set(const int32_t value)
{
	(void)this->setLength(4u);
	(void)this->setPayloadType(P_LONG32);
	this->lValue = value;
	return *this;
}

MyMessage& MyMessage::set(const uint16_t value)
{
	(void)this->setLength(2u);
	(void)this->setPayloadType(P_UINT16);
	this->uiValue = value;
	return *this;
}

MyMessage& MyMessage::set(const int16_t value)
{
	(void)this->setLength(2u);
	(void)this->setPayloadType(P_INT16);
	this->iValue = value;
	return *this;
}
