/*
 Vera Arduino library is used to communicate sensor data to your Vera receiver sketch using RF24 library.
 The maximum amount of transferrable data is 336 characters from a sensor to receiver.

 Created by Henrik Ekblad <henrik.ekblad@gmail.com>
 Version 1.0

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.

 	modification by axillent@gmail.com to run on pure AVR
 */

#include "VeraAVR.h"
#include <avr/eeprom.h>
#include <util/delay.h>
#include <stdlib.h>

uint8_t ee_radio_id EEMEM = 255;

struct {
	uint16_t radioId;
	uint16_t relayId;
	message_s msg;  // Buffer for incoming messages.
	char convBuffer[20];
} _veraavr_data;

/*uint8_t VeraAVR_eeprom_radioid() {
	return eeprom_read_byte(&ee_radio_id);
}*/
/*static void VeraAVR_delay_ms(uint16_t delay) {
	for(; delay >= 50; delay -= 50) {
		_delay_ms(50);
		wdt_reset();
	}
	wdt_reset();
}*/

void VeraAVR_begin(uint16_t _radioId) {
	VeraAVR_begin2(_radioId, GATEWAY_ADDRESS);
}

void VeraAVR_begin2(uint16_t _radioId, uint16_t _relayId) {
	_veraavr_data.radioId = _radioId;
	_veraavr_data.relayId = _relayId;
	// Start up the radio library
	nrf24_begin();
	nrf24_enableDynamicPayloads();
	nrf24_setAutoAck(1) ;
	nrf24_setRetries(15, 15);
	nrf24_setChannel(VERA_CHANNEL);
	nrf24_setDataRate(RF24_1MBPS);
	nrf24_setCRCLength(RF24_CRC_16);
	if (_veraavr_data.radioId == GATEWAY_ADDRESS) {
		nrf24_openReadingPipe(1, BASE_RADIO_ID);
	} else {
		if (_veraavr_data.radioId == AUTO) {
			_veraavr_data.radioId = eeprom_read_byte(&ee_radio_id);
			if (_veraavr_data.radioId == 0xFFF || _veraavr_data.radioId == 0x00) {
				// No radio id has been fetched yet. EEPROM is unwritten.
				// Request new id from Vera. Use radio address 255 temporarily.
				//nrf24_openReadingPipe(0,0); // Don't listen to other nodes
				nrf24_openReadingPipe(1, BASE_RADIO_ID+_veraavr_data.radioId);
				nrf24_openWritingPipe(BASE_RADIO_ID);
				_veraavr_data.radioId = atoi(VeraAVR_getConfiguration(V_REQUEST_ID));
				// Write id to EEPROM
				if (_veraavr_data.radioId == AUTO) { // Vera will return 255 if all sensor id are taken
					// wait a minute before possible wdt reset
					for(uint8_t i=0; i < 120; i++) {
						wdt_reset();
						_delay_ms(500);
					}
					// wait for a while ot until wdt reset
					while (1) {}
				} else {
					eeprom_write_byte(&ee_radio_id, _veraavr_data.radioId);
				}
			}
		}
		//eeprom_write_byte(&ee_radio_id, _veraavr_data.radioId);
		nrf24_openReadingPipe(1, BASE_RADIO_ID+_veraavr_data.radioId);
	}
	nrf24_startListening();
	wdt_reset();
}

uint8_t VeraAVR_sendDataGW(uint8_t childId, messageType messageType, uint8_t type, const char *data) {
	return VeraAVR_sendData(_veraavr_data.radioId, _veraavr_data.relayId, GATEWAY_ADDRESS, childId, messageType, type, data);
}

uint8_t VeraAVR_sendData(uint16_t from, uint16_t next, uint16_t to, uint8_t childId, messageType messageType, uint8_t type, const char *data) {
	uint8_t ok = 0;
	if (strlen(data) < sizeof(_veraavr_data.msg.data)) {
		_veraavr_data.msg.header.version = PROTOCOL_VERSION;
		_veraavr_data.msg.header.binary = 0;
		_veraavr_data.msg.header.from = from;
		_veraavr_data.msg.header.to = to;
		_veraavr_data.msg.header.childId = childId;
		_veraavr_data.msg.header.messageType = messageType;
		_veraavr_data.msg.header.type = type;

		strncpy(_veraavr_data.msg.data, data, sizeof(_veraavr_data.msg.data)-1);
		_veraavr_data.msg.header.crc = VeraAVR_crc8Message(_veraavr_data.msg);


		nrf24_stopListening();
		nrf24_openWritingPipe(BASE_RADIO_ID + next);
		uint8_t retry = 5;
		do {
			ok = nrf24_write(&_veraavr_data.msg, min(MAX_MESSAGE_LENGTH, sizeof(_veraavr_data.msg.header) + strlen(_veraavr_data.msg.data) + 1));
			wdt_reset();
		}
		while ( !ok && --retry );
		nrf24_startListening();
	}
	return ok;
}

void VeraAVR_sendVariable_char(uint8_t childId, uint8_t variableType,
		const char *value) {
	VeraAVR_sendDataGW(childId, M_VARIABLE, variableType, value);
}

void VeraAVR_sendVariable_float(uint8_t childId, uint8_t variableType, 	float value, int decimals) {
    VeraAVR_sendVariable_char(childId, variableType, dtostrf(value,2,decimals,_veraavr_data.convBuffer));
}

void VeraAVR_sendVariable_int(uint8_t childId, uint8_t variableType, 	int value) {
    VeraAVR_sendVariable_char(childId, variableType, itoa(value, _veraavr_data.convBuffer, 10));
}
void VeraAVR_sendVariable_long(uint8_t childId, uint8_t variableType, 	long value) {
    VeraAVR_sendVariable_char(childId, variableType, ltoa(value, _veraavr_data.convBuffer, 10));
}

void VeraAVR_requestStatus(uint8_t childId, uint8_t variableType) {
	VeraAVR_sendDataGW(childId, M_STATUS, variableType, "");
}

char* VeraAVR_getStatus(uint8_t childId, uint8_t variableType) {
	while (1) {
		VeraAVR_requestStatus(childId, variableType);
		uint8_t i = 0;
		while (i < 100) {  // 5 seconds timeout before re-sending status request
			while (VeraAVR_messageAvailable()) {
				// Check that it is right type of message and not a routing message
				if ((_veraavr_data.msg.header.messageType == M_REQUEST_STATUS_RESPONSE) &&
						(_veraavr_data.msg.header.type == variableType) &&
						(_veraavr_data.msg.header.to == childId))  {
					return _veraavr_data.msg.data;
				}
			}
			_delay_ms(50);
			wdt_reset();
			i++;
		}
	}
	return NULL;
}

void VeraAVR_requestConfiguration(uint8_t variableType) {
	VeraAVR_requestStatus(NODE_CHILD_ID, variableType);
}

char* VeraAVR_getConfiguration(uint8_t variableType) {
	return VeraAVR_getStatus(NODE_CHILD_ID, variableType);
}

unsigned long VeraAVR_getTime() {
	return atol(VeraAVR_getConfiguration(V_TIME));
}

void VeraAVR_sendSensorPresentation(uint8_t childId, uint8_t sensorType) {
	VeraAVR_sendDataGW(childId, M_PRESENTATION, sensorType, LIBRARY_VERSION);
}


uint8_t VeraAVR_messageAvailable() {
	while (nrf24_available()) {
		VeraAVR_readMessage();
		// Check that message was addressed for me. Could be a message from some other sensor
		// as we«re automatically listening to pipe 0 (our writing pipe) where all sensors
		// send their data
		if (_veraavr_data.msg.header.to == _veraavr_data.radioId && VeraAVR_validate() == VALIDATE_OK)
			return 1;
		wdt_reset();
	}
	return 0;
}


message_s VeraAVR_waitForMessage() {
	while (1) {
		if (VeraAVR_messageAvailable()) {
			return _veraavr_data.msg;
		}
		wdt_reset();
	}
}

message_s VeraAVR_getMessage() {
	return _veraavr_data.msg;
}


message_s VeraAVR_readMessage() {
	uint8_t len = nrf24_getDynamicPayloadSize();
	//uint8_t done =
	nrf24_read(&_veraavr_data.msg, len);
	// Make sure string gets terminated ok.
	_veraavr_data.msg.data[sizeof(_veraavr_data.msg.data)-1] = '\0';
	return _veraavr_data.msg;
}

/*
 * calculate CRC8 on message_s data taking care of data structure and protocol version
 */
uint8_t VeraAVR_crc8Message(message_s var_msg) {
	struct {
		message_s msg;
		uint8_t protocol_version;
	} crc_data;
	memcpy(&crc_data, &var_msg, sizeof(var_msg));
	crc_data.protocol_version = PROTOCOL_VERSION;
	// some clean up needed for repeated result
	crc_data.msg.header.crc = 0;
	// fill unused space by zeroes for string data only
	if(!crc_data.msg.header.binary) {
		uint8_t len = strlen(crc_data.msg.data);
		if(len < sizeof(crc_data.msg.data)-1) {
			memset(&crc_data.msg.data[len], 0, sizeof(crc_data.msg.data) - 1 - len);
		}
	}
	return crc8((uint8_t*)&crc_data, (uint8_t)sizeof(crc_data));
}
/*
 * true if message is consistent *
 */
uint8_t VeraAVR_checkCRCMessage(message_s var_msg) {
	return (var_msg.header.crc == VeraAVR_crc8Message(var_msg))?1:0;
}
uint8_t VeraAVR_checkCRC() {
	return VeraAVR_checkCRCMessage(_veraavr_data.msg);
}

uint8_t VeraAVR_validate() {
	uint8_t crc_check = VeraAVR_checkCRC();
	uint8_t version_check = (_veraavr_data.msg.header.version == PROTOCOL_VERSION);
	if(crc_check && version_check) return VALIDATE_OK;
	if(!crc_check) return VALIDATE_BAD_CRC;
	return VALIDATE_BAD_VERSION;
}
