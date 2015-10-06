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
 *
 *******************************
 *
 * DESCRIPTION
 * Signing support created by Patrick "Anticimex" Fallberg <patrick@fallberg.net>
 * The MySigning driver provides a generic API for various signing backends to offer
 * signing of MySensors messages.
 *
 */
#ifndef MySigning_h
#define MySigning_h

#include "MyConfig.h"
#include <stdint.h>
#include "MyMessage.h"
#include "drivers/ATSHA204/ATSHA204.h"

#ifdef MY_SIGNING_NODE_WHITELISTING
typedef struct {
	uint8_t nodeId;
	uint8_t serial[SHA204_SERIAL_SZ];
} whitelist_entry_t;
#endif

// Macros for manipulating signing requirement table
#define DO_SIGN(node) (node == 0 ? (~_doSign[0]&1) : (~_doSign[node>>4]&(node%16)))
#define SET_SIGN(node) (node == 0 ? (_doSign[0]&=~1) : (_doSign[node>>4]&=~(node%16)))
#define CLEAR_SIGN(node) (node == 0 ? (_doSign[0]|=1) : (_doSign[node>>4]|=(node%16)))
#define NUM_OF(x) (sizeof(x)/sizeof(x[0]))

// Stores signing identifier and a new nonce in provided message for signing operations.
// All space in message payload buffer is used for signing identifier and nonce.
// Returns false if subsystem is busy processing an ongoing signing operation.
// If successful, this marks the start of a signing operation at the receiving side so
// implementation is expected to do any necessary initializations within this call.
bool signerGetNonce(MyMessage &msg);

// Check timeout of verification session.
// Nonce will be purged if it takes too long for a signed message to be sent to the receiver.
// This function should be called on regular intervals.
bool signerCheckTimer(void);

// Get nonce from provided message and store for signing operations.
// Returns false if subsystem is busy processing an ongoing signing operation.
// Returns false if signing identifier found in message is not supported by the used backend.
// If successful, this marks the start of a signing operation at the sending side so
// implementation is expected to do any necessary initializations within this call.
bool signerPutNonce(MyMessage &msg);

// Signs provided message. All remaining space in message payload buffer is used for signing
// identifier and signature.
// Nonce used for signature calculation is the one generated previously using getNonce().
// Nonce will be cleared when this function is called to prevent re-use of nonce.
// Returns false if subsystem is busy processing an ongoing signing operation.
// Returns false if not two bytes or more of free payload space is left in provided message.
// This ends a signing operation at the sending side so implementation is expected to do any
// deinitializations and enter a power saving state within this call.
bool signerSignMsg(MyMessage &msg);

// Verifies signature in provided message.
// Nonce used for verification is the one previously set using putNonce().
// Nonce will be cleared when this function is called to prevent re-use of nonce.
// Returns false if subsystem is busy processing an ongoing signing operation.
// Returns false if signing identifier found in message is not supported by the used backend.
// This ends a signing operation at the receiving side so implementation is expected to do any
// deinitializations and enter a power saving state within this call.
bool signerVerifyMsg(MyMessage &msg);

#endif
