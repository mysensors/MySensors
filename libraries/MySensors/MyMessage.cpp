#include "MyMessage.h"
#include <stdio.h>
#include <stdlib.h>


MyMessage::MyMessage() {
	setVersion(PROTOCOL_VERSION);
}

MyMessage::MyMessage(uint8_t _sensor, uint8_t _type, uint8_t _destination) {
	sensor = _sensor;
	type = _type;
	destination = _destination;
	setVersion(PROTOCOL_VERSION);
	setCommand(C_SET_VARIABLE);
}

MyMessage MyMessage::build(uint8_t _sender, uint8_t _destination, uint8_t _sensor, uint8_t _command, uint8_t _type) {
	sender = _sender;
	destination = _destination;
	sensor = _sensor;
	type = _type;
	setCommand(_command);
	setLength(0);
	setPayloadType(P_STRING);
	return *this;
}

/*MyMessage MyMessage::failed() {
	return getCommand() == C_FAILED
}*/

// Header getters

uint8_t MyMessage::getCRC() {
	return crc;
}

uint8_t MyMessage::getVersion() {
	return BF_GET(version_length, 0, 3);
}

uint8_t MyMessage::getLength() {
	return BF_GET(version_length, 3, 5);
}

uint8_t MyMessage::getCommand() {
	return BF_GET(command_payload, 0, 4);
}

uint8_t MyMessage::getPayloadType() {
	return BF_GET(command_payload, 4, 4);
}

uint8_t MyMessage::getType() {
	return type;
}

uint8_t MyMessage::getSensor() {
	return sensor;
}

uint8_t MyMessage::getSender() {
	return sender;
}

uint8_t MyMessage::getLast() {
	return last;
}

uint8_t MyMessage::getDestination() {
	return destination;
}


/* Getters for payload converted to desired form */
void* MyMessage::getCustom() {
	return (void *)data;
}

/* NOTE: Calling this method might affect the payload by converting it to its string representation */
char* MyMessage::getString(char *buffer) {
	uint8_t payloadType = getPayloadType();

	if (payloadType == P_STRING) {
		data[getLength()] = '\0';
		return data;
	} else if (buffer != NULL) {
		switch(payloadType) {
			case P_BYTE:
				itoa(bValue, buffer, MAX_PAYLOAD);
				break;
			case P_INT16:
				itoa(iValue, data, MAX_PAYLOAD);
				break;
			case P_UINT16:
				ltoa(uiValue, data, MAX_PAYLOAD);
				break;
			case P_LONG32:
				ltoa(lValue, data, MAX_PAYLOAD);
				break;
			case P_ULONG32:
				ltoa(ulValue, data, MAX_PAYLOAD);
				break;
			case P_CUSTOM:
				// TODO: Ok, what do we do here? We should probably convert this to hex
				// Mostly gateway interested in this so we do the special handling of this
				// over there.

				break;
		}
		return buffer;
	} else {
		return NULL;
	}
}

uint8_t MyMessage::getByte() {
	return data[0];
}
bool MyMessage::getBool() {
	return !(data[0]-'0'==0);
}

double MyMessage::getDouble() {
	return strtod(data, NULL);
}
long MyMessage::getLong() {
	return lValue;
}
unsigned long MyMessage::getULong() {
	return ulValue;
}
int MyMessage::getInt() {
	return iValue;
}
unsigned int MyMessage::getUInt() {
	return uiValue;
}

// Header setters
MyMessage MyMessage::setCRC(uint8_t _crc) {
	crc = _crc;
	return *this;
}

MyMessage MyMessage::setVersion(uint8_t version) {
	BF_SET(version_length, version, 0, 3);
	return *this;
}

MyMessage MyMessage::setLength(uint8_t length) {
	BF_SET(version_length, length, 3, 5);
	return *this;
}

MyMessage MyMessage::setCommand(uint8_t command) {
	BF_SET(command_payload, command, 0, 4);
	return *this;
}

MyMessage MyMessage::setPayloadType(uint8_t pt) {
	BF_SET(command_payload, pt, 4, 4);
	return *this;
}

MyMessage MyMessage::setType(uint8_t _type) {
	type = _type;
	return *this;
}

MyMessage MyMessage::setSensor(uint8_t _sensor) {
	sensor = _sensor;
	return *this;
}

MyMessage MyMessage::setSender(uint8_t _sender) {
	sender = _sender;
	return *this;
}

MyMessage MyMessage::setLast(uint8_t _last) {
	last=_last;
	return *this;
}

MyMessage MyMessage::setDestination(uint8_t _destination) {
	destination = _destination;
	return *this;
}


// Set payload
MyMessage MyMessage::set(void* value, uint8_t length) {
	setPayloadType(P_CUSTOM);
	setLength(length);
	memcpy(data, value, min(length, MAX_PAYLOAD));
	return *this;
}


MyMessage MyMessage::set(const char* value) {
	uint8_t length = strlen(value);
	setLength(length);
	setPayloadType(P_STRING);
	strncpy(data, value, min(length, MAX_PAYLOAD));
	return *this;
}

MyMessage MyMessage::set(uint8_t value) {
	setLength(1);
	setPayloadType(P_BYTE);
	data[0] = value;
	return *this;
}

MyMessage MyMessage::set(bool value) {
	setLength(1);
	setPayloadType(P_STRING);
	data[0] = value?'1':'0';
	return *this;
}

MyMessage MyMessage::set(double value, uint8_t decimals) {
	dtostrf(value,2,decimals,data);
	setLength(strlen(data));
	setPayloadType(P_STRING);
	return *this;
}

MyMessage MyMessage::set(unsigned long value) {
	setPayloadType(P_ULONG32);
	setLength(4);
	ulValue = value;
	return *this;
}

MyMessage MyMessage::set(long value) {
	setPayloadType(P_LONG32);
	setLength(4);
	lValue = value;
	return *this;
}

MyMessage MyMessage::set(unsigned int value) {
	setPayloadType(P_INT16);
	setLength(2);
	uiValue = value;
	return *this;
}

MyMessage MyMessage::set(int value) {
	setPayloadType(P_UINT16);
	setLength(2);
	iValue = value;
	return *this;
}


