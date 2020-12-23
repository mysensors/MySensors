/*
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2020 Sensnology AB
 * Full contributor list: https://github.com/mysensors/MySensors/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * MultiTransport implementation created by Olivier Mauti 2020 <olivier@mysensors.org>
 */

#include "hal/transport/RF24/driver/RF24.h"

#if defined(MY_RF24_USE_INTERRUPTS) && !defined(MY_RF24_IRQ_PIN)
#error RF24 is using interrupts but MY_RF24_IRQ_PIN is not defined!
#endif

void RF24_transportSetAddress(const uint8_t address)
{
	RF24_setNodeAddress(address);
	RF24_startListening();
}

uint8_t RF24_transportGetAddress(void)
{
	return RF24_getNodeID();
}

bool RF24_transportSend(const uint8_t to, const void *data, const uint8_t len, const bool noACK)
{
#if defined(MY_RF24_ENABLE_ENCRYPTION)
	uint8_t tx_data[MAX_MESSAGE_SIZE];
	(void)memcpy((void *)tx_data, data, len);
	const uint8_t finalLength = transportEncryptionInsecureAESEncryption(tx_data, len);
#else
	const uint8_t finalLength = len;
	const void *tx_data = data;
#endif

	const bool result = RF24_sendMessage(to, tx_data, finalLength, noACK);
	return result;
}

bool RF24_transportDataAvailable(void)
{
	return RF24_isDataAvailable();
}

uint8_t RF24_transportReceive(void *data, const uint8_t maxBufSize)
{
	(void)maxBufSize; // size alreeady limited to 32 in RF24 driver
	const uint8_t len = RF24_readMessage(data);
#if defined(MY_RF24_ENABLE_ENCRYPTION)
	transportEncryptionInsecureAESDecryption(data, len);
#endif
	return len;
}

void RF24_transportTask(void)
{
#if defined(MY_TRANSPORT_RX_QUEUE)
#if defined(MY_RF24_USE_INTERRUPTS)
	if (!RF24_isDataAvailable()) { // IRQ flag set?
		return;
	}
#endif
	do {
		// get empty slot from HAL RX queue
		RXQueuedMessage_t *msgIn = transportHALGetQueueBuffer();
		if (RF24_isFIFOempty() || msgIn == NULL) {
			// Occasionally interrupt is triggered but no data is available or HAL RX queue is full
			// discard message (and clear IRQ)
			(void)RF24_readMessage(NULL);
			return;
		} else {
			msgIn->channel = TRANSPORT_RF24_CHANNEL_ID;
			msgIn->length = RF24_transportReceive((void *)&msgIn->data, sizeof(msgIn->data));
			(void)transportHALPushQueueBuffer(msgIn);
		}
	} while (!RF24_isFIFOempty());
#endif
}

bool RF24_transportSanityCheck(void)
{
	return RF24_sanityCheck();
}

bool RF24_transportInit(void)
{
	return RF24_initialize();
}

void RF24_transportSleep(void)
{
	RF24_sleep();
}

void RF24_transportStandBy(void)
{
	RF24_standBy();
}

void RF24_transportPowerDown(void)
{
	RF24_powerDown();
}

void RF24_transportPowerUp(void)
{
	RF24_powerUp();
}

int16_t RF24_transportGetSendingRSSI(void)
{
	return RF24_getSendingRSSI();
}

int16_t RF24_transportGetReceivingRSSI(void)
{
	// not available, only bool RPD
	return INVALID_RSSI;
}

int16_t RF24_transportGetSendingSNR(void)
{
	return INVALID_SNR;
}

int16_t RF24_transportGetReceivingSNR(void)
{
	return INVALID_SNR;
}

int16_t RF24_transportGetTxPowerPercent(void)
{
	return static_cast<int16_t>(RF24_getTxPowerPercent());
}

int16_t RF24_transportGetTxPowerLevel(void)
{
	return static_cast<int16_t>(RF24_getTxPowerLevel());
}

bool RF24_transportSetTxPowerPercent(const uint8_t powerPercent)
{
	return RF24_setTxPowerPercent(powerPercent);
}
