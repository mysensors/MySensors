/*
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
#include "drivers/RF24/RF24.h"

#if defined(MY_RF24_ENABLE_ENCRYPTION)
	#include "drivers/AES/AES.h"
#endif

#if defined(MY_RF24_ENABLE_ENCRYPTION)
	AES _aes;
	uint8_t _dataenc[32] = {0};
	uint8_t _psk[16];
#endif

bool transportInit() {
	
	#if defined(MY_RF24_ENABLE_ENCRYPTION)
		hwReadConfigBlock((void*)_psk, (void*)EEPROM_RF_ENCRYPTION_AES_KEY_ADDRESS, 16);
		//set up AES-key
		_aes.set_key(_psk, 16);
		// Make sure it is purged from memory when set
		memset(_psk, 0, 16);
	#endif
	
	return RF24_initialize();
}

void transportSetAddress(uint8_t address) {
	RF24_setNodeAddress(address);
	RF24_startListening();
}

uint8_t transportGetAddress() {
	return RF24_getNodeID();
}

bool transportSend(uint8_t recipient, const void* data, uint8_t len) {
	#if defined(MY_RF24_ENABLE_ENCRYPTION)
		// copy input data because it is read-only
		memcpy(_dataenc,data,len); 
		// has to be adjusted, WIP!
		_aes.set_IV(0);
		len = len > 16 ? 32 : 16;
		//encrypt data
		_aes.cbc_encrypt(_dataenc, _dataenc, len/16); 
		bool status = RF24_sendMessage( recipient, _dataenc, len );
	#else
		bool status = RF24_sendMessage( recipient, data, len );
	#endif
	
	return status;
}

bool transportAvailable() {
	bool avail = RF24_isDataAvailable();
	return avail;
}

bool transportSanityCheck() {
	return RF24_sanityCheck();
}

uint8_t transportReceive(void* data) {
	uint8_t len = RF24_readMessage(data);
	#if defined(MY_RF24_ENABLE_ENCRYPTION)
		// has to be adjusted, WIP!
		_aes.set_IV(0);
		// decrypt data
		_aes.cbc_decrypt((byte*)(data), (byte*)(data), len>16?2:1);
	#endif
	return len;
}

void transportPowerDown() {
	RF24_powerDown();
}
