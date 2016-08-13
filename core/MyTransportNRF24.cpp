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
#include "drivers/CircularBuffer/CircularBuffer.h"

#if defined(MY_RF24_ENABLE_ENCRYPTION)
	#include "drivers/AES/AES.h"
#endif

#if defined(MY_RX_MESSAGE_BUFFER_FEATURE)
typedef struct _transportQueuedMessage
{
	uint8_t m_len;                        // Length of the data
	uint8_t m_data[MAX_MESSAGE_LENGTH];   // The raw data
} transportQueuedMessage;

/** Buffer to store queued messages in. */
static transportQueuedMessage transportRxQueueStorage[MY_RX_MESSAGE_BUFFER_SIZE];
/** Circular buffer, which uses the transportRxQueueStorage and administers stored messages. */
static CircularBuffer<transportQueuedMessage> transportRxQueue(transportRxQueueStorage, MY_RX_MESSAGE_BUFFER_SIZE); 

static volatile uint8_t transportLostMessageCount = 0;

static void transportRxCallback(void)
{
	// Called for each message received by radio, from interrupt context.
	// This function _must_ call RF24_readMessage() to de-assert interrupt line!
	if (!transportRxQueue.full())
	{
		transportQueuedMessage* msg = transportRxQueue.getFront();
		msg->m_len = RF24_readMessage(msg->m_data);		// Read payload & clear RX_DR
		(void)transportRxQueue.pushFront(msg);
	} else {
		// Queue is full. Discard message.
		(void)RF24_readMessage(NULL);		// Read payload & clear RX_DR
		// Keep track of messages lost. Max 255, prevent wrapping.
		if (transportLostMessageCount < 255)
		{
			++transportLostMessageCount;
		}
	}
}
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
	
	#if defined(MY_RX_MESSAGE_BUFFER_FEATURE)
		RF24_registerReceiveCallback( transportRxCallback );
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
	#if defined(MY_RX_MESSAGE_BUFFER_FEATURE)
		(void)RF24_isDataAvailable;				// Prevent 'defined but not used' warning
		return !transportRxQueue.empty();
	#else
		return RF24_isDataAvailable();
	#endif
}

bool transportSanityCheck() {
	return RF24_sanityCheck();
}

uint8_t transportReceive(void* data) {
	uint8_t len = 0; 
	#if defined(MY_RX_MESSAGE_BUFFER_FEATURE)
		transportQueuedMessage* msg = transportRxQueue.getBack();
		if (msg) {
			len = msg->m_len;
			(void)memcpy(data, msg->m_data, len);
			(void)transportRxQueue.popBack();
		}
	#else
		len = RF24_readMessage(data);
	#endif
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
