#include "MyMessage.h"
#include <stdio.h>
#include <stdlib.h>


MyMessage::MyMessage() {
	destination = 0; // Gateway is default destination
}



/* Getters for payload converted to desired form */
void* MyMessage::getCustom() {
	return (void *)data;
}

/* NOTE: Calling this method might affect the payload by converting it to its string representation */
char* MyMessage::getString(char *buffer) {
	uint8_t payloadType = miGetPayloadType();

	if (payloadType == P_STRING) {
		data[miGetLength()] = '\0';
		return data;
	} else if (buffer != NULL) {
		switch(payloadType) {
			case P_BYTE:
				itoa(bValue, buffer, MAX_PAYLOAD);
				break;
			case P_INT16:
				itoa(iValue, buffer, MAX_PAYLOAD);
				break;
			case P_UINT16:
				ltoa(uiValue, buffer, MAX_PAYLOAD);
				break;
			case P_LONG32:
				ltoa(lValue, buffer, MAX_PAYLOAD);
				break;
			case P_ULONG32:
				ltoa(ulValue, buffer, MAX_PAYLOAD);
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
	switch(miGetPayloadType()) {
	case P_BYTE:
		return data[0];
	case P_STRING:
		return atoi(data);
	default:
		return 0;
	}
}

bool MyMessage::getBool() {
	return !(data[0]-'0'==0);
}

double MyMessage::getDouble() {
	return strtod(data, NULL);
}

long MyMessage::getLong() {
	switch(miGetPayloadType()) {
	case P_LONG32:
		return lValue;
	case P_STRING:
		return atol(data);
	default:
		return 0;
	}
}

unsigned long MyMessage::getULong() {
	switch(miGetPayloadType()) {
	case P_ULONG32:
		return ulValue;
	case P_STRING:
		return atol(data);
	default:
		return 0;
	}
}

int MyMessage::getInt() {
	switch(miGetPayloadType()) {
	case P_INT16:
		return iValue;
	case P_STRING:
		return atoi(data);
	default:
		return 0;
	}
}

unsigned int MyMessage::getUInt() {
	switch(miGetPayloadType()) {
	case P_UINT16:
		return uiValue;
	case P_STRING:
		return atoi(data);
	default:
		return 0;
	}

}


MyMessage MyMessage::setType(uint8_t _type) {
	type = _type;
	return *this;
}

MyMessage MyMessage::setSensor(uint8_t _sensor) {
	sensor = _sensor;
	return *this;
}

MyMessage MyMessage::setDestination(uint8_t _destination) {
	destination = _destination;
	return *this;
}

// Set payload
MyMessage MyMessage::set(void* value, uint8_t length) {
	miSetPayloadType(P_CUSTOM);
	miSetLength(length);
	memcpy(data, value, min(length, MAX_PAYLOAD));
	return *this;
}


MyMessage MyMessage::set(const char* value) {
	uint8_t length = strlen(value);
	miSetLength(length);
	miSetPayloadType(P_STRING);
	strncpy(data, value, min(length, MAX_PAYLOAD));
	return *this;
}

MyMessage MyMessage::set(uint8_t value) {
	miSetLength(1);
	miSetPayloadType(P_BYTE);
	data[0] = value;
	return *this;
}

MyMessage MyMessage::set(bool value) {
	miSetLength(1);
	miSetPayloadType(P_STRING);
	data[0] = value?'1':'0';
	return *this;
}

MyMessage MyMessage::set(double value, uint8_t decimals) {
	dtostrf(value,2,decimals,data);
	miSetLength(strlen(data));
	miSetPayloadType(P_STRING);
	return *this;
}

MyMessage MyMessage::set(unsigned long value) {
	miSetPayloadType(P_ULONG32);
	miSetLength(4);
	ulValue = value;
	return *this;
}

MyMessage MyMessage::set(long value) {
	miSetPayloadType(P_LONG32);
	miSetLength(4);
	lValue = value;
	return *this;
}

MyMessage MyMessage::set(unsigned int value) {
	miSetPayloadType(P_INT16);
	miSetLength(2);
	uiValue = value;
	return *this;
}

MyMessage MyMessage::set(int value) {
	miSetPayloadType(P_UINT16);
	miSetLength(2);
	iValue = value;
	return *this;
}


