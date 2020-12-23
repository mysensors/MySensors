/*
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2020 Sensnology AB
 * Full contributor list: https://github.com/mysensors/Arduino/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

#include "MyTransportQueue.h"

// debug
#if defined(MY_DEBUG_VERBOSE_TRANSPORT_QUEUE)
#define TRANSPORT_QUEUE_DEBUG(x,...) DEBUG_OUTPUT(x, ##__VA_ARGS__)	//!< debug
#else
#define TRANSPORT_QUEUE_DEBUG(x,...)	//!< debug NULL
#endif

static RXQueuedMessage_t transportRxQueueStorage[RX_QUEUE_BUFFER_SIZE];
static CircularBuffer<RXQueuedMessage_t> transportRxQueue(transportRxQueueStorage,
        RX_QUEUE_BUFFER_SIZE);

#define SIZE_CHANNEL_ROUTE 256
static uint8_t channelRoute[SIZE_CHANNEL_ROUTE];

transportChannelID_t transportGetChannel(const uint8_t nodeId)
{
	if(nodeId == BROADCAST_ADDRESS) {
		return TRANSPORT_ALL_CHANNEL_ID;
	}
	uint8_t channel = channelRoute[nodeId];
	TRANSPORT_QUEUE_DEBUG(PSTR("MTQ:GCH:GET N=%d,CH=%d\n"), nodeId, channel);
	return static_cast<transportChannelID_t>(channel);
}

void transportUpdateChannel(const uint8_t nodeId, const transportChannelID_t channel)
{
	if (nodeId != BROADCAST_ADDRESS) {
		channelRoute[nodeId] = static_cast<uint8_t>(channel);
		TRANSPORT_QUEUE_DEBUG(PSTR("MTQ:UCH:SET N=%d,CH=%d\n"), nodeId, channel);
	}
}

// reset channel routes
void transportResetChannels(void)
{
	for (uint16_t i = 0; i < SIZE_CHANNEL_ROUTE; i++) {
		transportUpdateChannel(i, TRANSPORT_ALL_CHANNEL_ID);
	}
}

