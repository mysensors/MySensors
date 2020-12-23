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
 *
 * MultiTransport implementation created by Olivier Mauti 2020 <olivier@mysensors.org>
 */

#include "driver/RFM95_RFM69.h"

bool RFM95_transportInit(void)
{
	const bool result = RFM95_initialise(MY_RFM95_FREQUENCY);
#if !defined(MY_GATEWAY_FEATURE) && !defined(MY_RFM95_ATC_MODE_DISABLED)
	// only enable ATC mode in nodes
	RFM95_ATCmode(true, MY_RFM95_ATC_TARGET_RSSI);
#endif
	return result;
}

void RFM95_transportSetAddress(const uint8_t address)
{
	RFM95_setAddress(address);
}

uint8_t RFM95_transportGetAddress(void)
{
	return RFM95_getAddress();
}

bool RFM95_transportSend(const uint8_t to, const void *data, const uint8_t len, const bool noACK)
{
	return RFM95_sendWithRetry(to, data, len, noACK);
}

bool RFM95_transportDataAvailable(void)
{
	return RFM95_available();
}

void RFM95_transportTask(void)
{
	RFM95_handling();
#if defined(MY_TRANSPORT_RX_QUEUE)
	if (RFM95_available()) {
		RXQueuedMessage_t *msgIn = transportHALGetQueueBuffer();
		if (msgIn != NULL) {
			msgIn->channel = TRANSPORT_RFM95_CHANNEL_ID;
			msgIn->length = RFM95_receive((uint8_t *)&msgIn->data, MAX_MESSAGE_SIZE);
			(void)transportHALPushQueueBuffer(msgIn);
		}
	}
#endif
}

void RFM95_transportEncrypt(const char *key)
{
	// for RFM69 compatibility
	RFM95_encrypt(key);
}

bool RFM95_transportSanityCheck(void)
{
	return RFM95_sanityCheck();
}

uint8_t RFM95_transportReceive(void *data, const uint8_t maxBufSize)
{
	return RFM95_receive((uint8_t *)data, maxBufSize);
}

void RFM95_transportSleep(void)
{
	(void)RFM95_sleep();
}

void RFM95_transportStandBy(void)
{
	(void)RFM95_standBy();
}

void RFM95_transportPowerDown(void)
{
	RFM95_powerDown();
}

void RFM95_transportPowerUp(void)
{
	RFM95_powerUp();
}

void RFM95_transportToggleATCmode(const bool OnOff, const int16_t targetRSSI)
{
	RFM95_ATCmode(OnOff, targetRSSI);
}

int16_t RFM95_transportGetSendingRSSI(void)
{
	return RFM95_getSendingRSSI();
}

int16_t RFM95_transportGetReceivingRSSI(void)
{
	return RFM95_getReceivingRSSI();
}

int16_t RFM95_transportGetSendingSNR(void)
{
	return RFM95_getSendingSNR();
}

int16_t RFM95_transportGetReceivingSNR(void)
{
	return RFM95_getReceivingSNR();
}

int16_t RFM95_transportGetTxPowerPercent(void)
{
	return RFM95_getTxPowerPercent();
}

int16_t RFM95_transportGetTxPowerLevel(void)
{
	return RFM95_getTxPowerLevel();
}

bool RFM95_transportSetTxPowerPercent(const uint8_t powerPercent)
{
	return RFM95_setTxPowerPercent(powerPercent);
}

