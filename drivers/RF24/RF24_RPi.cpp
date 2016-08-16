/**
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Marcelo Aquino <marceloaqno@gmail.org>
 * Copyright (C) 2016 Marcelo Aquino
 * Full contributor list: https://github.com/mysensors/MySensors/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * Based on Olivier Mauti work(RF24.h), copyright (C) 2016 Olivier Mauti <olivier@mysensors.org>
 */

#include "RF24.h"

// verify RF24 IRQ defs
#if defined(MY_RX_MESSAGE_BUFFER_FEATURE)
	#if !defined(MY_RF24_IRQ_PIN)
		#error Message buffering feature requires MY_RF24_IRQ_PIN to be defined!
	#endif
	#ifndef SPI_HAS_TRANSACTION
		#error RF24 IRQ usage requires transactional SPI support
	#endif
#else
	#ifdef MY_RX_MESSAGE_BUFFER_SIZE
		#error Receive message buffering requires RF24 IRQ usage
	#endif
#endif

// pipes
#define BROADCAST_PIPE 1
#define NODE_PIPE 2

// debug 
#if defined(MY_DEBUG_VERBOSE_RF24)
	#define RF24_DEBUG(x,...) debug(x, ##__VA_ARGS__)
#else
	#define RF24_DEBUG(x,...)
#endif

#ifdef MY_RX_MESSAGE_BUFFER_FEATURE
	typedef void (*RF24_receiveCallbackType)(void);
	
	void (* RF24_receiveCallback)(void) = NULL;

	static pthread_mutex_t rf24_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

static uint8_t MY_RF24_BASE_ADDR[MY_RF24_ADDR_WIDTH] = { MY_RF24_BASE_RADIO_ID };
static uint8_t MY_RF24_NODE_ADDRESS = AUTO;

RF24 _rf24(MY_RF24_CE_PIN, MY_RF24_CS_PIN);

static void RF24_startListening(void) {
	RF24_DEBUG(PSTR("start listening\n"));

	_rf24.startListening();
}

static void RF24_powerDown(void) {
#ifdef MY_RX_MESSAGE_BUFFER_FEATURE
	pthread_mutex_lock(&rf24_mutex);
#endif
	_rf24.powerDown();
#ifdef MY_RX_MESSAGE_BUFFER_FEATURE
	pthread_mutex_unlock(&rf24_mutex);
#endif
}

static bool RF24_sendMessage( uint8_t recipient, const void* buf, uint8_t len ) {
#ifdef MY_RX_MESSAGE_BUFFER_FEATURE
	pthread_mutex_lock(&rf24_mutex);
#endif
	// Make sure radio has powered up
	_rf24.powerUp();
	_rf24.stopListening();

	RF24_DEBUG(PSTR("send message to %d, len=%d\n"), recipient,len);

	MY_RF24_BASE_ADDR[0] = recipient;
	_rf24.openWritingPipe(MY_RF24_BASE_ADDR);
	bool ok = _rf24.write(buf, len, recipient == BROADCAST_ADDRESS);
	_rf24.startListening();
#ifdef MY_RX_MESSAGE_BUFFER_FEATURE
	pthread_mutex_unlock(&rf24_mutex);
#endif
	return ok;
}

static bool RF24_isDataAvailable() {
#ifdef MY_RX_MESSAGE_BUFFER_FEATURE
	pthread_mutex_lock(&rf24_mutex);
#endif
	uint8_t pipe_num = 255;
	_rf24.available(&pipe_num);
#ifdef MY_RX_MESSAGE_BUFFER_FEATURE
	pthread_mutex_unlock(&rf24_mutex);
#endif
	return (pipe_num <= 5);
}

static uint8_t RF24_readMessage( void* buf) {
#ifdef MY_RX_MESSAGE_BUFFER_FEATURE
	pthread_mutex_lock(&rf24_mutex);
#endif
	uint8_t len = _rf24.getDynamicPayloadSize();
	_rf24.read(buf, len);
#ifdef MY_RX_MESSAGE_BUFFER_FEATURE
	pthread_mutex_unlock(&rf24_mutex);
#endif
	return len;
}

static void RF24_setNodeAddress(uint8_t address) {
#ifdef MY_RX_MESSAGE_BUFFER_FEATURE
	pthread_mutex_lock(&rf24_mutex);
#endif
	if (address != AUTO){
		MY_RF24_NODE_ADDRESS = address;
		// enable node pipe
		MY_RF24_BASE_ADDR[0] = MY_RF24_NODE_ADDRESS;
		_rf24.openReadingPipe(NODE_PIPE, MY_RF24_BASE_ADDR);
		// enable autoACK on node pipe
		_rf24.setAutoAck(NODE_PIPE, true);
	}
#ifdef MY_RX_MESSAGE_BUFFER_FEATURE
	pthread_mutex_unlock(&rf24_mutex);
#endif
}

static uint8_t RF24_getNodeID(void) {
	return MY_RF24_NODE_ADDRESS;
}

bool RF24_sanityCheck(void) {
	bool ok = true;
#ifdef MY_RX_MESSAGE_BUFFER_FEATURE
	pthread_mutex_lock(&rf24_mutex);
#endif
	// detect HW defect ot interrupted SPI line, CE disconnect cannot be detected	
	if (_rf24.getPALevel() != MY_RF24_PA_LEVEL || _rf24.getDataRate() != MY_RF24_DATARATE ||
			_rf24.getChannel() != MY_RF24_CHANNEL) {
		ok = false;
	}
#ifdef MY_RX_MESSAGE_BUFFER_FEATURE
	pthread_mutex_unlock(&rf24_mutex);
#endif
	return ok;
}

#ifdef MY_RX_MESSAGE_BUFFER_FEATURE
void RF24_irqHandler(void) {
	if (RF24_receiveCallback) {
		while (RF24_isDataAvailable()) {
			RF24_receiveCallback();
		} 
	} else {
		// clear RX interrupt
		bool tx_ok,tx_fail,rx;
		_rf24.whatHappened(tx_ok,tx_fail,rx);
	}
}

void RF24_registerReceiveCallback(RF24_receiveCallbackType cb) {
	RF24_receiveCallback = cb;
}
#endif

static bool RF24_initialize(void) {
	// start up the radio library
	_rf24.begin();
	// determine whether the hardware is an nRF24L01+ or not
	if (!_rf24.isPVariant()) {
		RF24_DEBUG(PSTR("radio hardware not compatible"));
		return false;
	}
	// set CRC
	_rf24.setCRCLength(RF24_CRC_16);
	// set address width
	_rf24.setAddressWidth(MY_RF24_ADDR_WIDTH);
	// auto retransmit delay 1500us, auto retransmit count 15 
	_rf24.setRetries(5, 15);
	// channel
	_rf24.setChannel(MY_RF24_CHANNEL);
	// PA level
	_rf24.setPALevel(MY_RF24_PA_LEVEL);
	// data rate
	_rf24.setDataRate(MY_RF24_DATARATE);
	// sanity check
	#if defined(MY_RF24_SANITY_CHECK)
		if (!RF24_sanityCheck()) {
			RF24_DEBUG(PSTR("RF24:Sanity check failed: configuration mismatch! Check wiring, replace module or non-P version\n"));
			return false;
		}
	#endif
	// enable Dynamic payload
	_rf24.enableDynamicPayloads();
	// enable ACK payload
	_rf24.enableAckPayload();
	// disable AA on all pipes, activate when node pipe set
	_rf24.setAutoAck(false);
	// all nodes listen to broadcast pipe (for FIND_PARENT_RESPONSE messages)
	MY_RF24_BASE_ADDR[0] = BROADCAST_ADDRESS;
	_rf24.openReadingPipe(BROADCAST_PIPE, MY_RF24_BASE_ADDR);

	//_rf24.printDetails();
	
	#ifdef MY_RX_MESSAGE_BUFFER_FEATURE
		// Mask all interrupts except the receive interrupt
		_rf24.maskIRQ(1,1,0);
		
		attachInterrupt(MY_RF24_IRQ_PIN, INT_EDGE_FALLING, RF24_irqHandler);
	#endif

	return true;
}
