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

#ifndef MyTransportQueue_h
#define MyTransportQueue_h

#include "hal/transport/MyTransportHAL.h"
#include "drivers/CircularBuffer/CircularBuffer.h"

#define RX_QUEUE_MAX_MSG_LENGTH 32
#define RX_QUEUE_BUFFER_SIZE 4

/**
* @brief RXQueuedMessage_t
*/
typedef struct {
	transportChannelID_t channel;             //!< channel of origin
	uint8_t length;                           //!< length of data
	uint8_t data[RX_QUEUE_MAX_MSG_LENGTH];    //!< raw data
} RXQueuedMessage_t;

#endif // MyTransportQueue_h
