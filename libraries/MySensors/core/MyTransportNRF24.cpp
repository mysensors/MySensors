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
#include <stdint.h>
#include "drivers/RF24/RF24.h"
#include "drivers/RF24/RF24_config.h"
#if defined(MY_RF24_ENABLE_ENCRYPTION)
#include "drivers/AES/AES.h"
#endif

#define TO_ADDR(x) (MY_RF24_BASE_RADIO_ID + x)

#define WRITE_PIPE ((uint8_t)0)
#define CURRENT_NODE_PIPE ((uint8_t)1)
#define BROADCAST_PIPE ((uint8_t)2)


RF24 _rf24(MY_RF24_CE_PIN, MY_RF24_CS_PIN);
uint8_t _address;
#if defined(MY_RF24_ENABLE_ENCRYPTION)
	AES _aes;
	uint8_t _dataenc[32] = {0};
	uint8_t _psk[] = { MY_RF24_ENCRYPTKEY };
#endif

bool transportInit() {
	// Start up the radio library
	_rf24.begin();

	if (!_rf24.isPVariant()) {
		return false;
	}
	_rf24.setAutoAck(1);
	_rf24.setAutoAck(BROADCAST_PIPE,false); // Turn off auto ack for broadcast
	_rf24.enableAckPayload();
	_rf24.setChannel(MY_RF24_CHANNEL);
	_rf24.setPALevel(MY_RF24_PA_LEVEL);
	if (!_rf24.setDataRate(MY_RF24_DATARATE)) {
		return false;
	}
	_rf24.setRetries(5,15);
	_rf24.setCRCLength(RF24_CRC_16);
	_rf24.enableDynamicPayloads();

	// All nodes listen to broadcast pipe (for FIND_PARENT_RESPONSE messages)
	_rf24.openReadingPipe(BROADCAST_PIPE, TO_ADDR(BROADCAST_ADDRESS));


	#if defined(MY_RF24_ENABLE_ENCRYPTION)
		_aes.set_key(_psk, 16); //set up AES-key
	#endif

	//_rf24.printDetails();

	return true;
}

void transportSetAddress(uint8_t address) {
	_address = address;
	_rf24.openReadingPipe(WRITE_PIPE, TO_ADDR(address));
	_rf24.openReadingPipe(CURRENT_NODE_PIPE, TO_ADDR(address));
	_rf24.startListening();
}

uint8_t transportGetAddress() {
	return _address;
}

bool transportSend(uint8_t to, const void* data, uint8_t len) {
	#if defined(MY_RF24_ENABLE_ENCRYPTION)
		memcpy(_dataenc,data,len); // copy input data because it is read-only

		_aes.set_IV(0);//not sure if necessary
		len = len > 16 ? 32 : 16;
		_aes.cbc_encrypt(_dataenc, _dataenc, len/16); //encrypt
	#endif

	// Make sure radio has powered up

	_rf24.powerUp();
	_rf24.stopListening();
	_rf24.openWritingPipe(TO_ADDR(to));
	#if defined(MY_RF24_ENABLE_ENCRYPTION)
		bool ok = _rf24.write(_dataenc, len, to == BROADCAST_ADDRESS);
	#else
		bool ok = _rf24.write(data, len, to == BROADCAST_ADDRESS);
	#endif
	_rf24.startListening();
	return ok;
}

bool transportAvailable(uint8_t *to) {
	uint8_t pipe = 255;
	boolean avail = _rf24.available(&pipe);

	(void)avail; //until somebody makes use of 'avail'
	if (pipe == CURRENT_NODE_PIPE)
		*to = _address;
	else if (pipe == BROADCAST_PIPE)
		*to = BROADCAST_ADDRESS;
	return (_rf24.available() && pipe < 6);
}

uint8_t transportReceive(void* data) {
	uint8_t len = _rf24.getDynamicPayloadSize();
	_rf24.read(data, len);
	#if defined(MY_RF24_ENABLE_ENCRYPTION)
		_aes.set_IV(0);//not sure if necessary
		_aes.cbc_decrypt((byte*)(data), (byte*)(data), len>16?2:1); // decrypt
	#endif
	return len;
}

void transportPowerDown() {
	_rf24.powerDown();
}
