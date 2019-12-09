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

#include "hal/transport/RF24/driver/RF24.h"

#if defined(MY_RX_MESSAGE_BUFFER_FEATURE)
#include "drivers/CircularBuffer/CircularBuffer.h"

typedef struct _transportQueuedMessage {
	uint8_t m_len;                        // Length of the data
	uint8_t m_data[MAX_MESSAGE_SIZE];   // The raw data
} transportQueuedMessage;

/** Buffer to store queued messages in. */
static transportQueuedMessage transportRxQueueStorage[MY_RX_MESSAGE_BUFFER_SIZE];
/** Circular buffer, which uses the transportRxQueueStorage and administers stored messages. */
static CircularBuffer<transportQueuedMessage> transportRxQueue(transportRxQueueStorage,
        MY_RX_MESSAGE_BUFFER_SIZE);

static volatile uint8_t transportLostMessageCount = 0;

static void transportRxCallback(void)
{
	// Called for each message received by radio, from interrupt context.
	// This function _must_ call RF24_readMessage() to de-assert interrupt line!
	if (!transportRxQueue.full()) {
		transportQueuedMessage* msg = transportRxQueue.getFront();
		msg->m_len = RF24_readMessage(msg->m_data);		// Read payload & clear RX_DR
		(void)transportRxQueue.pushFront(msg);
	} else {
		// Queue is full. Discard message.
		(void)RF24_readMessage(NULL);		// Read payload & clear RX_DR
		// Keep track of messages lost. Max 255, prevent wrapping.
		if (transportLostMessageCount < 255) {
			++transportLostMessageCount;
		}
	}
}
#endif

bool transportInit(void)
{
#if defined(MY_RX_MESSAGE_BUFFER_FEATURE)
	RF24_registerReceiveCallback( transportRxCallback );
#endif
	return RF24_initialize();
}

void transportSetAddress(const uint8_t address)
{
	RF24_setNodeAddress(address);
	RF24_startListening();
}

uint8_t transportGetAddress(void)
{
	return RF24_getNodeID();
}

bool transportSend(const uint8_t to, const void *data, const uint8_t len, const bool noACK)
{
	return RF24_sendMessage(to, data, len, noACK);
}

bool transportDataAvailable(void)
{
#if defined(MY_RX_MESSAGE_BUFFER_FEATURE)
	(void)RF24_isDataAvailable;				// Prevent 'defined but not used' warning
	return !transportRxQueue.empty();
#else
	return RF24_isDataAvailable();
#endif
}

bool transportSanityCheck(void)
{
	return RF24_sanityCheck();
}

uint8_t transportReceive(void *data)
{
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
	return len;
}

void transportSleep(void)
{
	RF24_sleep();
}

void transportStandBy(void)
{
	RF24_standBy();
}

void transportPowerDown(void)
{
	RF24_powerDown();
}

void transportPowerUp(void)
{
	RF24_powerUp();
}

int16_t transportGetSendingRSSI(void)
{
	return RF24_getSendingRSSI();
}

int16_t transportGetReceivingRSSI(void)
{
	// not available, only bool RPD
	return INVALID_RSSI;
}

int16_t transportGetSendingSNR(void)
{
	return INVALID_SNR;
}

int16_t transportGetReceivingSNR(void)
{
	return INVALID_SNR;
}

int16_t transportGetTxPowerPercent(void)
{
	return static_cast<int16_t>(RF24_getTxPowerPercent());
}

int16_t transportGetTxPowerLevel(void)
{
	return static_cast<int16_t>(RF24_getTxPowerLevel());
}

bool transportSetTxPowerPercent(const uint8_t powerPercent)
{
	return RF24_setTxPowerPercent(powerPercent);
}
