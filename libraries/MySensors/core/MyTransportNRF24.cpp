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
#include "drivers/RF24/RF24.h"
#include "drivers/CircularBuffer/CircularBuffer.h"

#if defined(MY_RF24_ENABLE_ENCRYPTION)
	#include "drivers/AES/AES.h"
#endif

#if (MY_RF24_IRQ_PIN != MY_RF24_IRQ_PIN_UNUSED)
typedef struct _queuedMessage
{
    uint8_t   m_to;                         /** Destination address of this message. */
    uint8_t   m_len;                        /** Length of the data. */
    uint8_t   m_data[MAX_MESSAGE_LENGTH];   /** The raw data. */
} queuedMessage;

/** Buffer to store queued messages in. */
static queuedMessage queueStorage[MY_RF24_MESSAGE_BUFFER_SIZE];
/** Circular buffer, which uses the queueStorage and administers stored messages. */
static CircularBuffer<queuedMessage> receiveQueue(queueStorage, MY_RF24_MESSAGE_BUFFER_SIZE); 

static volatile uint8_t lostMessageCount = 0;

static void rxCallback(const uint8_t to)
{
    if (!receiveQueue.full())
    {
        queuedMessage* msg = receiveQueue.getFront();
        msg->m_len = readMessage(msg->m_data);
        (void)receiveQueue.pushFront(msg);
    } else {
        // TODO: Message is not really lost, as it is still in the nRF FIFO. But once the FIFO is also full messages will get lost. 
        ++lostMessageCount;
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

    #if (MY_RF24_IRQ_PIN != MY_RF24_IRQ_PIN_UNUSED)
        registerReceiveCallback( rxCallback );
    #endif
	return initializeRF24();
}

void transportSetAddress(uint8_t address) {
	setNodeAddress(address);
	startListening();
}

uint8_t transportGetAddress() {
	return getNodeID();
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
		bool status = sendMessage( recipient, _dataenc, len );
	#else
		bool status = sendMessage( recipient, data, len );
	#endif
	
	return status;
}

bool transportAvailable(uint8_t *to) {
    bool avail = false;
    #if (MY_RF24_IRQ_PIN != MY_RF24_IRQ_PIN_UNUSED)
        queuedMessage* msg = receiveQueue.getBack();
        if (msg)
        {
            avail = true;
            *to = msg->m_to;
        } else {
            // TODO: The nRF FIFO could still hold a message
        }
    #else
        avail = IsDataAvailable(to);
    #endif
    return avail;
}

uint8_t transportReceive(void* data) {
    uint8_t len; 
    #if (MY_RF24_IRQ_PIN != MY_RF24_IRQ_PIN_UNUSED)
//        debug(PSTR("lost:%d\n"), lostMessageCount);
        queuedMessage* msg = receiveQueue.getBack();
        if (msg)
        {
            len = msg->m_len;
            (void)memcpy(data, msg->m_data, len);
            (void)receiveQueue.popBack();
        } else {
            // TODO: The nRF FIFO could still hold a message
        }
    #else
        len = readMessage(data);
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
	powerDown();
}
