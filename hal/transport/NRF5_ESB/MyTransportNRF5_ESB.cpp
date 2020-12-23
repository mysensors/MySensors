/*
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2020 Sensnology AB
 * Copyright (C) 2017 Frank Holtz
 * Full contributor list:
 * https://github.com/mysensors/MySensors/graphs/contributors
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

#include "hal/transport/NRF5_ESB/driver/Radio.h"
#include "hal/transport/NRF5_ESB/driver/Radio_ESB.h"

bool NRF5_ESB_transportInit(void)
{
	return NRF5_ESB_initialize();
}

void NRF5_ESB_transportSetAddress(const uint8_t address)
{
	NRF5_ESB_setNodeAddress(address);
	NRF5_ESB_startListening();
}

uint8_t NRF5_ESB_transportGetAddress(void)
{
	return NRF5_ESB_getNodeID();
}

bool NRF5_ESB_transportSend(const uint8_t to, const void *data, const uint8_t len, const bool noACK)
{
	return NRF5_ESB_sendMessage(to, data, len, noACK);
}

bool NRF5_ESB_transportDataAvailable(void)
{
	return NRF5_ESB_isDataAvailable();
}

bool NRF5_ESB_transportSanityCheck(void)
{
	return NRF5_ESB_sanityCheck();
}

uint8_t NRF5_ESB_transportReceive(void *data, const uint8_t maxBufSize)
{
	uint8_t len = 0;
	len = NRF5_ESB_readMessage(data, maxBufSize);
	return len;
}

void NRF5_ESB_transportTask(void)
{
#if defined(MY_TRANSPORT_RX_QUEUE)
	while (NRF5_ESB_isDataAvailable()) {
		RXQueuedMessage_t *msgIn = transportHALGetQueueBuffer();
		if (msgIn != NULL) {
			msgIn->channel = TRANSPORT_NRF5_ESB_CHANNEL_ID;
			msgIn->length = NRF5_ESB_readMessage((void *)&msgIn->data, sizeof(msgIn->data));
			(void)transportHALPushQueueBuffer(msgIn);
		} else {
			// queue full, discard message
			(void)NRF5_ESB_readMessage(NULL, MAX_MESSAGE_SIZE);
			return;
		}
	}
#endif
}

void NRF5_ESB_transportPowerDown(void)
{
	NRF5_ESB_powerDown();
}

void NRF5_ESB_transportPowerUp(void)
{
	NRF5_ESB_powerUp();
}

void NRF5_ESB_transportSleep(void)
{
	NRF5_ESB_sleep();
}

void NRF5_ESB_transportStandBy(void)
{
	NRF5_ESB_standBy();
}

int16_t NRF5_ESB_transportGetSendingRSSI(void)
{
	return NRF5_ESB_getSendingRSSI();
}

int16_t NRF5_ESB_transportGetReceivingRSSI(void)
{
	return NRF5_ESB_getReceivingRSSI();
}

int16_t NRF5_ESB_transportGetSendingSNR(void)
{
	return INVALID_SNR;
}

int16_t NRF5_ESB_transportGetReceivingSNR(void)
{
	return INVALID_SNR;
}

int16_t NRF5_ESB_transportGetTxPowerPercent(void)
{
	return NRF5_getTxPowerPercent();
}

int16_t NRF5_ESB_transportGetTxPowerLevel(void)
{
	return static_cast<int16_t>(NRF5_getTxPowerLevel());
}

bool NRF5_ESB_transportSetTxPowerPercent(const uint8_t powerPercent)
{
	return NRF5_setTxPowerPercent(powerPercent);
}
