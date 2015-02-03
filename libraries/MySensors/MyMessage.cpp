#include "MyMessage.h"
#include <stdio.h>
#include <stdlib.h>


MyMessage::MyMessage() {
	destination = 0; // Gateway is default destination
}

MyMessage::MyMessage(uint8_t _sensor, uint8_t _type) {
	destination = 0; // Gateway is default destination
	sensor = _sensor;
	type = _type;
}

bool MyMessage::isAck() const {
	return miGetAck();
}

/* Getters for payload converted to desired form */
void* MyMessage::getCustom() const {
	return (void *)data;
}

const char* MyMessage::getString() const {
	uint8_t payloadType = miGetPayloadType();
	if (payloadType == P_STRING) {
		return data;
	} else {
		return NULL;
	}
}

// handles single character hex (0 - 15)
char MyMessage::i2h(uint8_t i) const {
	uint8_t k = i & 0x0F;
	if (k <= 9)
		return '0' + k;
	else
		return 'A' + k - 10;
}

char* MyMessage::getCustomString(char *buffer) const {
	for (uint8_t i = 0; i < miGetLength(); i++)
	{
		buffer[i * 2] = i2h(data[i] >> 4);
		buffer[(i * 2) + 1] = i2h(data[i]);
	}
	buffer[miGetLength() * 2] = '\0';
	return buffer;
}

char* MyMessage::getStream(char *buffer) const {
	uint8_t cmd = miGetCommand();
	if ((cmd == C_STREAM) && (buffer != NULL)) {
		return getCustomString(buffer);
	} else {
		return NULL;
	}
}

char* MyMessage::getString(char *buffer) const {
	uint8_t payloadType = miGetPayloadType();
	if (payloadType == P_STRING) {
		strncpy(buffer, data, miGetLength());
		buffer[miGetLength()] = 0;
		return buffer;
	} else if (buffer != NULL) {
		if (payloadType == P_BYTE) {
			itoa(bValue, buffer, 10);
		} else if (payloadType == P_INT16) {
			itoa(iValue, buffer, 10);
		} else if (payloadType == P_UINT16) {
			utoa(uiValue, buffer, 10);
		} else if (payloadType == P_LONG32) {
			ltoa(lValue, buffer, 10);
		} else if (payloadType == P_ULONG32) {
			ultoa(ulValue, buffer, 10);
		} else if (payloadType == P_FLOAT32) {
			dtostrf(fValue,2,fPrecision,buffer);
		} else if (payloadType == P_CUSTOM) {
			return getCustomString(buffer);
		}
		return buffer;
	} else {
		return NULL;
	}
}

uint8_t MyMessage::getByte() const {
	if (miGetPayloadType() == P_BYTE) {
		return data[0];
	} else if (miGetPayloadType() == P_STRING) {
		return atoi(data);
	} else {
		return 0;
	}
}

bool MyMessage::getBool() const {
	return getInt();
}

float MyMessage::getFloat() const {
	if (miGetPayloadType() == P_FLOAT32) {
		return fValue;
	} else if (miGetPayloadType() == P_STRING) {
		return atof(data);
	} else {
		return 0;
	}
}

long MyMessage::getLong() const {
	if (miGetPayloadType() == P_LONG32) {
		return lValue;
	} else if (miGetPayloadType() == P_STRING) {
		return atol(data);
	} else {
		return 0;
	}
}

unsigned long MyMessage::getULong() const {
	if (miGetPayloadType() == P_ULONG32) {
		return ulValue;
	} else if (miGetPayloadType() == P_STRING) {
		return atol(data);
	} else {
		return 0;
	}
}

int MyMessage::getInt() const {
	if (miGetPayloadType() == P_INT16) { 
		return iValue;
	} else if (miGetPayloadType() == P_STRING) {
		return atoi(data);
	} else {
		return 0;
	}
}

unsigned int MyMessage::getUInt() const {
	if (miGetPayloadType() == P_UINT16) { 
		return uiValue;
	} else if (miGetPayloadType() == P_STRING) {
		return atoi(data);
	} else {
		return 0;
	}

}

MyMessage& MyMessage::setType(uint8_t _type) {
	type = _type;
	return *this;
}

MyMessage& MyMessage::setSensor(uint8_t _sensor) {
	sensor = _sensor;
	return *this;
}

MyMessage& MyMessage::setDestination(uint8_t _destination) {
	destination = _destination;
	return *this;
}

// Set payload
MyMessage& MyMessage::set(void* value, uint8_t length) {
	miSetPayloadType(P_CUSTOM);
	miSetLength(length);
	memcpy(data, value, min(length, MAX_PAYLOAD));
	return *this;
}

MyMessage& MyMessage::set(const char* value) {
	uint8_t length = min(strlen(value), MAX_PAYLOAD);
	miSetLength(length);
	miSetPayloadType(P_STRING);
	strncpy(data, value, length);
	return *this;
}

MyMessage& MyMessage::set(uint8_t value) {
	miSetLength(1);
	miSetPayloadType(P_BYTE);
	data[0] = value;
	return *this;
}

MyMessage& MyMessage::set(float value, uint8_t decimals) {
	miSetLength(5); // 32 bit float + persi
	miSetPayloadType(P_FLOAT32);
	fValue=value;
	fPrecision = decimals;
	return *this;
}

MyMessage& MyMessage::set(unsigned long value) {
	miSetPayloadType(P_ULONG32);
	miSetLength(4);
	ulValue = value;
	return *this;
}

MyMessage& MyMessage::set(long value) {
	miSetPayloadType(P_LONG32);
	miSetLength(4);
	lValue = value;
	return *this;
}

MyMessage& MyMessage::set(unsigned int value) {
	miSetPayloadType(P_UINT16);
	miSetLength(2);
	uiValue = value;
	return *this;
}

MyMessage& MyMessage::set(int value) {
	miSetPayloadType(P_INT16);
	miSetLength(2);
	iValue = value;
	return *this;
}
