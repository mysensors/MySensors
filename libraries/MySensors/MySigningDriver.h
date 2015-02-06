/*
 The MySigning driver provides a generic API for various signing backends to offer
 signing of MySensors messages.
 
 Created by Patrick "Anticimex" Fallberg <patrick@fallberg.net>
*/
#ifndef MySigningDriver_h
#define MySigningDriver_h

#include <stdint.h>
#include "MyMessage.h"

class MySigningDriver
{ 
public:
	// MySigningDriver constructor.
	// Different parameters would be needed depending on signing backend (e.g. pins connected etc.).
	// Keeping these parameters as #define's in MyConfig to streamline the driver interface.
	MySigningDriver();

	// Stores signing identifier and a new nonce in provided message for signing operations.
	// All space in message payload buffer is used for signing identifier and nonce.
	// Returns false if subsystem is busy processing an ongoing signing operation.
	// If successful, this marks the start of a signing operation at the receiving side so
	// implementation is expected to do any necessary initializations within this call.
	virtual bool getNonce(MyMessage &msg) = 0;

	// Check timeout of verification session.
	// Nonce will be purged if it takes too long for a signed message to be sent to the receiver.
	// This function should be called on regular intervals.
	virtual bool checkTimer(void) = 0;

	// Get nonce from provided message and store for signing operations.
	// Returns false if subsystem is busy processing an ongoing signing operation.
	// Returns false if signing identifier found in message is not supported by the used backend.
	// If successful, this marks the start of a signing operation at the sending side so
	// implementation is expected to do any necessary initializations within this call.
	virtual bool putNonce(MyMessage &msg) = 0;

	// Signs provided message. All remaining space in message payload buffer is used for signing
	// identifier and signature.
	// Nonce used for signature calculation is the one generated previously using getNonce().
	// Nonce will be cleared when this function is called to prevent re-use of nonce.
	// Returns false if subsystem is busy processing an ongoing signing operation.
	// Returns false if not two bytes or more of free payload space is left in provided message.
	// This ends a signing operation at the sending side so implementation is expected to do any
	// deinitializations and enter a power saving state within this call.
	virtual bool signMsg(MyMessage &msg) = 0;

	// Verifies signature in provided message.
	// Nonce used for verification is the one previously set using putNonce().
	// Nonce will be cleared when this function is called to prevent re-use of nonce.
	// Returns false if subsystem is busy processing an ongoing signing operation.
	// Returns false if signing identifier found in message is not supported by the used backend.
	// This ends a signing operation at the receiving side so implementation is expected to do any
	// deinitializations and enter a power saving state within this call.
	virtual bool verifyMsg(MyMessage &msg) = 0;
};

#endif
