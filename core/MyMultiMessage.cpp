/*
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2022 Sensnology AB
 * Full contributor list: https://github.com/mysensors/MySensors/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * Multi message feature added by Constantin Petra <constantin.petra@gmail.com>
 * Copyright (C) 2022 Constantin Petra
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

#include "Version.h"
#include "MyConfig.h"
#include "MyEepromAddresses.h"
#include "MyMessage.h"
#include "MyMultiMessage.h"
#include "stddef.h"

#define SET_PAYLOAD_TYPE(u, type)  BF_SET(u, type, V2_MYS_HEADER_CEP_PAYLOADTYPE_POS, \
        V2_MYS_HEADER_CEP_PAYLOADTYPE_SIZE)
#define GET_PAYLOAD_TYPE(u)        BF_GET(u, V2_MYS_HEADER_CEP_PAYLOADTYPE_POS, \
        V2_MYS_HEADER_CEP_PAYLOADTYPE_SIZE)
#define SET_COMMAND(u, command)    BF_SET(u, command, V2_MYS_HEADER_CEP_COMMAND_POS, \
        V2_MYS_HEADER_CEP_COMMAND_SIZE)
#define GET_COMMAND(u)             BF_GET(u, V2_MYS_HEADER_CEP_COMMAND_POS, \
        V2_MYS_HEADER_CEP_COMMAND_SIZE)
#define SET_LENGTH(u, finalLength) BF_SET(u, finalLength, V2_MYS_HEADER_VSL_LENGTH_POS, \
        V2_MYS_HEADER_VSL_LENGTH_SIZE)
#define GET_LENGTH(u)              BF_GET(u, V2_MYS_HEADER_VSL_LENGTH_POS, \
        V2_MYS_HEADER_VSL_LENGTH_SIZE)
#define SET_VERSION(u)             BF_SET(u, V2_MYS_HEADER_PROTOCOL_VERSION, \
        V2_MYS_HEADER_VSL_VERSION_POS, V2_MYS_HEADER_VSL_VERSION_SIZE);

#define MAX_BLOB_SIZE (MAX_PAYLOAD_SIZE)

// replicates part of the MyMesgage structure related to a single sensor data.
typedef struct {
	uint8_t command_echo_payload;
	uint8_t type;
	uint8_t sensor;
	union {
		uint8_t bValue;
		uint16_t uiValue;
		int16_t iValue;
		uint32_t ulValue;
		int32_t lValue;
		struct {
			float fValue;
			uint8_t fPrecision;
		};
	} __attribute__((packed));
} __attribute__((packed)) blob;

MyMultiMessage::~MyMultiMessage()
{

}

MyMultiMessage::MyMultiMessage(MyMessage *msg)
{
	_msg = msg;
	_offset = 0;
	_msg->setPayloadType(P_CUSTOM);
}

void *MyMultiMessage::common(uint8_t messageType, uint8_t sensor, uint8_t ptype, uint8_t size,
                             uint8_t cmd)
{
	blob *b;
	if (_offset + size > MAX_BLOB_SIZE) {
		return NULL;
	}
	b = (blob *)(_msg->data + _offset);
	SET_PAYLOAD_TYPE(b->command_echo_payload, ptype);
	SET_COMMAND(b->command_echo_payload, cmd);
	b->type = messageType;
	b->sensor = sensor;
	_offset += size;
	SET_LENGTH(_msg->version_length, _offset);
	SET_VERSION(_msg->version_length);
	return b;
}

bool MyMultiMessage::set(uint8_t messageType, uint8_t sensor, uint8_t value)
{
	blob *b = (blob *)common(messageType, sensor, P_BYTE, 4, C_SET);
	if (b == NULL) {
		return false;
	}
	b->bValue = value;
	return true;
}

bool MyMultiMessage::set(uint8_t messageType, uint8_t sensor, uint16_t value)
{
	blob *b = (blob *)common(messageType, sensor, P_UINT16, 5, C_SET);
	if (b == NULL) {
		return false;
	}
	b->uiValue = value;
	return true;
}

bool MyMultiMessage::set(uint8_t messageType, uint8_t sensor, int16_t value)
{
	blob *b = (blob *)common(messageType, sensor, P_INT16, 5, C_SET);
	if (b == NULL) {
		return false;
	}
	b->uiValue = value;
	return true;
}

bool MyMultiMessage::set(uint8_t messageType, uint8_t sensor, uint32_t value)
{
	blob *b = (blob *)common(messageType, sensor, P_ULONG32, 7, C_SET);
	if (b == NULL) {
		return false;
	}
	b->ulValue = value;
	return true;
}

bool MyMultiMessage::set(uint8_t messageType, uint8_t sensor, int32_t value)
{
	blob *b = (blob *)common(messageType, sensor, P_LONG32, 7, C_SET);

	if (b == NULL) {
		return false;
	}
	b->lValue = value;
	return true;
}

bool MyMultiMessage::set(uint8_t messageType, uint8_t sensor, float value, uint8_t decimals)
{
	blob *b = (blob *)common(messageType, sensor, P_FLOAT32, 8, C_SET);

	if (b == NULL) {
		return false;
	}
	b->fValue = value;
	b->fPrecision = decimals;
	return true;
}

bool MyMultiMessage::setBattery(uint8_t value)
{
	blob *b = (blob *)common(I_BATTERY_LEVEL, NODE_SENSOR_ID, P_BYTE, 4, C_INTERNAL);

	if (b == NULL) {
		return false;
	}
	b->bValue = value;
	return true;
}


bool MyMultiMessage::getNext(MyMessage &m)
{
	uint8_t size = GET_LENGTH(_msg->version_length);
	uint8_t type;

	blob *b;
	if (_offset >= size) {
		return false;
	}

	m.last = _msg->last;
	m.sender = _msg->sender;
	m.destination = _msg->destination;
	m.version_length = _msg->version_length;

	b = (blob *)(_msg->data + _offset);
	m.type = b->type;
	m.sensor = b->sensor;

	type = GET_PAYLOAD_TYPE(b->command_echo_payload);
	m.command_echo_payload = b->command_echo_payload;

	if (type == P_BYTE) {
		m.bValue = b->bValue;
		_offset += 4;
		return true;
	} else if (type == P_UINT16) {
		m.uiValue = b->uiValue;
		_offset += 5;
		return true;
	} else if (type == P_INT16) {
		m.iValue = b->iValue;
		_offset += 5;
		return true;
	} else if (type == P_ULONG32) {
		m.ulValue = b->ulValue;
		_offset += 7;
		return true;
	} else if (type == P_LONG32) {
		m.lValue = b->lValue;
		_offset += 7;
		return true;
	} else if (type == P_FLOAT32) {
		m.fValue = b->fValue;
		m.fPrecision = b->fPrecision;
		_offset += 8;
		return true;
	}
	return false;
}

void MyMultiMessage::reset()
{
	_offset = 0;
	SET_LENGTH(_msg->version_length, _offset);
}
