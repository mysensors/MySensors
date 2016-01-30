/*
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

#include "MySigning.h"
#include "drivers/ATSHA204/sha256.h"

#define SIGNING_PRESENTATION_VERSION_1 1
#define SIGNING_PRESENTATION_REQUIRE_SIGNATURES   (1 << 0)
#define SIGNING_PRESENTATION_REQUIRE_WHITELISTING (1 << 1)

#if defined(MY_DEBUG_VERBOSE_SIGNING)
#define SIGN_DEBUG(x,...) debug(x, ##__VA_ARGS__)
#else
#define SIGN_DEBUG(x,...)
#endif

#if defined(MY_SIGNING_REQUEST_SIGNATURES) && (!defined(MY_SIGNING_ATSHA204) && !defined(MY_SIGNING_SOFT))
#error You have to pick either MY_SIGNING_ATSHA204 or MY_SIGNING_SOFT in order to require signatures!
#endif
#ifdef MY_SIGNING_FEATURE
uint8_t _doSign[32];      // Bitfield indicating which sensors require signed communication
uint8_t _doWhitelist[32]; // Bitfield indicating which sensors require serial salted signatures
MyMessage _msgSign;       // Buffer for message to sign.
uint8_t _signingNonceStatus;

#ifdef MY_NODE_LOCK_FEATURE
static uint8_t nof_nonce_requests = 0;
static uint8_t nof_failed_verifications = 0;
#endif

// Status when waiting for signing nonce in signerProcessInternal
enum { SIGN_WAITING_FOR_NONCE = 0, SIGN_OK = 1 };

// Macros for manipulating signing requirement table
#define DO_SIGN(node) (~_doSign[node>>3]&(1<<node%8))
#define SET_SIGN(node) (_doSign[node>>3]&=~(1<<node%8))
#define CLEAR_SIGN(node) (_doSign[node>>3]|=(1<<node%8))

/*
 * Stores signing identifier and a new nonce in provided message for signing operations.
 *
 * All space in message payload buffer is used for signing identifier and nonce.
 * Returns false if subsystem is busy processing an ongoing signing operation.
 * If successful, this marks the start of a signing operation at the receiving side so
 * implementation is expected to do any necessary initializations within this call.
 * This function is typically called as action when receiving a I_NONCE_REQUEST message.
 */
#if defined(MY_SIGNING_SOFT)
extern void signerAtsha204SoftInit(void);
extern bool signerAtsha204SoftCheckTimer(void);
extern bool signerAtsha204SoftGetNonce(MyMessage &msg);
extern void signerAtsha204SoftPutNonce(MyMessage &msg);
extern bool signerAtsha204SoftVerifyMsg(MyMessage &msg);
extern bool signerAtsha204SoftSignMsg(MyMessage &msg);
#endif
#if defined(MY_SIGNING_ATSHA204)
extern void signerAtsha204Init(void);
extern bool signerAtsha204CheckTimer(void);
extern bool signerAtsha204GetNonce(MyMessage &msg);
extern void signerAtsha204PutNonce(MyMessage &msg);
extern bool signerAtsha204VerifyMsg(MyMessage &msg);
extern bool signerAtsha204SignMsg(MyMessage &msg);
#endif

// Helper function to centralize signing/verification exceptions
static bool skipSign(MyMessage &msg) {
	if mGetAck(msg) {
		SIGN_DEBUG(PSTR("Skipping security for ACK on command %d type %d\n"), mGetCommand(msg), msg.type);
		return true;
	}	else if (mGetCommand(msg) == C_INTERNAL &&
		(msg.type == I_NONCE_REQUEST || msg.type == I_NONCE_RESPONSE       || msg.type == I_SIGNING_PRESENTATION ||
	   msg.type == I_ID_REQUEST    || msg.type == I_ID_RESPONSE          ||
	   msg.type == I_FIND_PARENT   || msg.type == I_FIND_PARENT_RESPONSE ||
	   msg.type == I_HEARTBEAT     || msg.type == I_HEARTBEAT_RESPONSE)) {
		SIGN_DEBUG(PSTR("Skipping security for command %d type %d\n"), mGetCommand(msg), msg.type);
		return true;
	} else if (mGetCommand(msg) == C_STREAM &&
		(msg.type == ST_FIRMWARE_REQUEST || msg.type == ST_FIRMWARE_RESPONSE ||
		 msg.type == ST_SOUND || msg.type == ST_IMAGE)) {
		SIGN_DEBUG(PSTR("Skipping security for command %d type %d\n"), mGetCommand(msg), msg.type);
		return true;
	} else {
		return false;
	}
}
#endif // MY_SIGNING_FEATURE

// Helper to prepare a signing presentation message
static void prepareSigningPresentation(MyMessage &msg, uint8_t destination) {
	// Only supports version 1 for now
	build(msg, _nc.nodeId, destination, NODE_SENSOR_ID, C_INTERNAL, I_SIGNING_PRESENTATION, false).set("");
	mSetLength(msg, 2);
	msg.data[0] = SIGNING_PRESENTATION_VERSION_1;
	msg.data[1] = 0;
}

void signerInit(void) {
#if defined(MY_SIGNING_FEATURE)
	// Read out the signing requirements from EEPROM
	hwReadConfigBlock((void*)_doSign, (void*)EEPROM_SIGNING_REQUIREMENT_TABLE_ADDRESS,
		sizeof(_doSign));

	// Read out the whitelist requirements from EEPROM
	hwReadConfigBlock((void*)_doWhitelist, (void*)EEPROM_WHITELIST_REQUIREMENT_TABLE_ADDRESS,
		sizeof(_doWhitelist));

#if defined(MY_SIGNING_SOFT)
	signerAtsha204SoftInit();
#endif
#if defined(MY_SIGNING_ATSHA204)
	signerAtsha204Init();
#endif
#endif
}

void signerPresentation(MyMessage &msg) {
	prepareSigningPresentation(msg, GATEWAY_ADDRESS);

#if defined(MY_SIGNING_REQUEST_SIGNATURES)
	msg.data[1] |= SIGNING_PRESENTATION_REQUIRE_SIGNATURES;
	SIGN_DEBUG(PSTR("Signing required\n"));
#endif
#if defined(MY_SIGNING_NODE_WHITELISTING)
	msg.data[1] |= SIGNING_PRESENTATION_REQUIRE_WHITELISTING;
	SIGN_DEBUG(PSTR("Whitelisting required\n"));
#endif

	_sendRoute(msg);

#if defined(MY_SIGNING_FEATURE)
	// If we do support signing, wait for the gateway to tell us how it prefer us to transmit our messages
	SIGN_DEBUG(PSTR("Waiting for GW to send signing preferences...\n"));
	wait(2000);
#endif
}

bool signerProcessInternal(MyMessage &msg) {
	uint8_t sender = msg.sender;
	(void)sender;
	if (mGetCommand(msg) == C_INTERNAL) {
#if !defined(MY_SIGNING_FEATURE) && defined(MY_GATEWAY_FEATURE)
		// If we act as gateway and do not have the signing feature and receive a signing request we still
		// need to do make sure the requester does not believe the gateway still require signatures
		if (msg.type == I_SIGNING_PRESENTATION) {
			prepareSigningPresentation(msg, sender);
			SIGN_DEBUG(PSTR("Informing node %d that we do not require signatures because we do not support it\n"),
				sender);
			_sendRoute(msg);
			return true; // No need to further process I_SIGNING_PRESENTATION in this case
		}
#elif defined(MY_SIGNING_FEATURE)
		if (msg.type == I_NONCE_REQUEST) {
#if defined(MY_NODE_LOCK_FEATURE)
			nof_nonce_requests++;
			SIGN_DEBUG(PSTR("Nonce requests left until lockdown: %d\n"), MY_NODE_LOCK_COUNTER_MAX-nof_nonce_requests);
			if (nof_nonce_requests >= MY_NODE_LOCK_COUNTER_MAX) {
				nodeLock("TMNR"); //Too many nonces requested
			}
#endif
#if defined(MY_SIGNING_SOFT)
			if (signerAtsha204SoftGetNonce(msg)) {
#endif
#if defined(MY_SIGNING_ATSHA204)
			if (signerAtsha204GetNonce(msg)) {
#endif
				SIGN_DEBUG(PSTR("Transmittng nonce\n"));
				_sendRoute(build(msg, _nc.nodeId, msg.sender, NODE_SENSOR_ID,
					C_INTERNAL, I_NONCE_RESPONSE, false));
			} else {
				SIGN_DEBUG(PSTR("Failed to generate nonce!\n"));
			}
			return true; // No need to further process I_NONCE_REQUEST
		} else if (msg.type == I_SIGNING_PRESENTATION) {
			if (msg.data[0] != SIGNING_PRESENTATION_VERSION_1) {
				SIGN_DEBUG(PSTR("Unsupported signing presentation version (%d)!\n"), msg.data[0]);
				return true; // Just drop this presentation message
			}

			// We only handle version 1 here...
			if (msg.data[1] & SIGNING_PRESENTATION_REQUIRE_SIGNATURES) {
				// We received an indicator that the sender require us to sign all messages we send to it
				SIGN_DEBUG(PSTR("Mark node %d as one that require signed messages\n"), msg.sender);
				SET_SIGN(msg.sender);
			} else {
				// We received an indicator that the sender does not require us to sign all messages we send to it
				SIGN_DEBUG(PSTR("Mark node %d as one that do not require signed messages\n"), msg.sender);
				CLEAR_SIGN(msg.sender);
			}

			if (msg.data[1] & SIGNING_PRESENTATION_REQUIRE_WHITELISTING) {
				// We received an indicator that the sender require us to salt signatures with serial
				SIGN_DEBUG(PSTR("Mark node %d as one that require whitelisting\n"), msg.sender);
				SET_WHITELIST(msg.sender);
			} else {
				// We received an indicator that the sender does not require us to sign all messages we send to it
				SIGN_DEBUG(PSTR("Mark node %d as one that do not require whitelisting\n"), msg.sender);
				CLEAR_WHITELIST(msg.sender);
			}

			// Save updated tables
			hwWriteConfigBlock((void*)_doSign, (void*)EEPROM_SIGNING_REQUIREMENT_TABLE_ADDRESS,
				sizeof(_doSign));
			hwWriteConfigBlock((void*)_doWhitelist, (void*)EEPROM_WHITELIST_REQUIREMENT_TABLE_ADDRESS,
				sizeof(_doWhitelist));

			// Inform sender about our preference if we are a gateway, but only require signing if the sender
			// required signing
			// We do not want a gateway to require signing from all nodes in a network just because it wants one node
			// to sign it's messages
#if defined(MY_GATEWAY_FEATURE)
			prepareSigningPresentation(msg, sender);
#if defined(MY_SIGNING_REQUEST_SIGNATURES)
			if (DO_SIGN(sender)) {
				msg.data[1] |= SIGNING_PRESENTATION_REQUIRE_SIGNATURES;
			}
#endif
#if defined(MY_SIGNING_NODE_WHITELISTING)
			if (DO_WHITELIST(sender)) {
				msg.data[1] |= SIGNING_PRESENTATION_REQUIRE_WHITELISTING;
			}
#endif
			if (msg.data[1] & SIGNING_PRESENTATION_REQUIRE_SIGNATURES) {
				SIGN_DEBUG(PSTR("Informing node %d that we require signatures\n"), sender);
			} else {
				SIGN_DEBUG(PSTR("Informing node %d that we do not require signatures\n"), sender);
			}
			if (msg.data[1] & SIGNING_PRESENTATION_REQUIRE_WHITELISTING) {
				SIGN_DEBUG(PSTR("Informing node %d that we require whitelisting\n"), sender);
			} else {
				SIGN_DEBUG(PSTR("Informing node %d that we do not require whitelisting\n"), sender);
			}
			_sendRoute(msg);
#endif // MY_GATEWAY_FEATURE
			return true; // No need to further process I_SIGNING_PRESENTATION
		} else if (msg.type == I_NONCE_RESPONSE) {
			// Proceed with signing if nonce has been received
			SIGN_DEBUG(PSTR("Nonce received from %d. Proceeding with signing...\n"), sender);
			if (sender != _msgSign.destination) {
				SIGN_DEBUG(PSTR("Nonce did not come from the destination (%d) of the message to be signed! "
					"It came from %d.\n"), _msgSign.destination, sender);
				SIGN_DEBUG(PSTR("Silently discarding this nonce\n"));
				return true; // No need to further process I_NONCE_RESPONSE
			}
#if defined(MY_SIGNING_SOFT)
			signerAtsha204SoftPutNonce(msg);
#endif
#if defined(MY_SIGNING_ATSHA204)
			signerAtsha204PutNonce(msg);
#endif
#if defined(MY_SIGNING_SOFT)
			if (!signerAtsha204SoftSignMsg(_msgSign)) {
#endif
#if defined(MY_SIGNING_ATSHA204)
			if (!signerAtsha204SignMsg(_msgSign)) {
#endif
				SIGN_DEBUG(PSTR("Failed to sign message!\n"));
			} else {
				SIGN_DEBUG(PSTR("Message signed\n"));
				_signingNonceStatus = SIGN_OK; // _msgSign now contains the signed message pending transmission
			}
			return true; // No need to further process I_NONCE_RESPONSE
		}
#endif // MY_SIGNING_FEATURE
	}
	return false;
}

bool signerCheckTimer(void) {
#if defined(MY_SIGNING_SOFT)
	return signerAtsha204SoftCheckTimer();
#elif defined(MY_SIGNING_ATSHA204)
	return signerAtsha204CheckTimer();
#else
	return true; // Without a configured backend, we always give "positive" results
#endif
}

bool signerSignMsg(MyMessage &msg) {
#if defined(MY_SIGNING_FEATURE)
	// If destination is known to require signed messages and we are the sender,
	// sign this message unless it is a handshake message
	if (DO_SIGN(msg.destination) && msg.sender == _nc.nodeId) {
		if (skipSign(msg)) {
			return true;
		} else {
			// Send nonce-request
			_signingNonceStatus=SIGN_WAITING_FOR_NONCE;
			if (!_sendRoute(build(_msgTmp, _nc.nodeId, msg.destination, msg.sensor,
				C_INTERNAL, I_NONCE_REQUEST, false).set(""))) {
				SIGN_DEBUG(PSTR("Failed to transmit nonce request!\n"));
				return false;
			}
			SIGN_DEBUG(PSTR("Nonce requested from %d. Waiting...\n"), msg.destination);
			// We have to wait for the nonce to arrive before we can sign our original message
			// Other messages could come in-between. We trust _process() takes care of them
			unsigned long enter = hwMillis();
			_msgSign = msg; // Copy the message to sign since message buffer might be touched in _process()
			while (hwMillis() - enter < MY_VERIFICATION_TIMEOUT_MS && _signingNonceStatus==SIGN_WAITING_FOR_NONCE) {
				_process();
			}
			if (hwMillis() - enter > MY_VERIFICATION_TIMEOUT_MS) {
				SIGN_DEBUG(PSTR("Timeout waiting for nonce!\n"));
				return false;
			}
			if (_signingNonceStatus == SIGN_OK) {
				// process() received a nonce and signerProcessInternal successfully signed the message
				msg = _msgSign; // Write the signed message back
				SIGN_DEBUG(PSTR("Message to send has been signed\n"));
			} else {
				SIGN_DEBUG(PSTR("Message to send could not be signed!\n"));
				return false;
			}
			// After this point, only the 'last' member of the message structure is allowed to be altered if the
			// message has been signed, or signature will become invalid and the message rejected by the receiver
		}
	} else if (_nc.nodeId == msg.sender) {
		mSetSigned(msg, 0); // Message is not supposed to be signed, make sure it is marked unsigned
	}
#else
	(void)msg;
#endif // MY_SIGNING_FEATURE
	return true;
}

bool signerVerifyMsg(MyMessage &msg) {
	bool verificationResult = true;
	// Before processing message, reject unsigned messages if signing is required and check signature
	// (if it is signed and addressed to us)
	// Note that we do not care at all about any signature found if we do not require signing
#if defined(MY_SIGNING_FEATURE) && defined(MY_SIGNING_REQUEST_SIGNATURES)
	// If we are a node, or we are a gateway and the sender require signatures
	// and we are the destination...
	if ((!MY_IS_GATEWAY || DO_SIGN(msg.sender)) && msg.destination == _nc.nodeId) {
		// Internal messages of certain types are not verified
		if (skipSign(msg)) {
			verificationResult = true;
		}
		else if (!mGetSigned(msg)) {
			// Got unsigned message that should have been signed
			SIGN_DEBUG(PSTR("Message is not signed, but it should have been!\n"));
			verificationResult = false;
		} else {
#if defined(MY_SIGNING_SOFT)
			verificationResult = signerAtsha204SoftVerifyMsg(msg);
#endif
#if defined(MY_SIGNING_ATSHA204)
			verificationResult = signerAtsha204VerifyMsg(msg);
#endif
			if (!verificationResult) {
				SIGN_DEBUG(PSTR("Signature verification failed!\n"));
			}
#if defined(MY_NODE_LOCK_FEATURE)
			if (verificationResult) {
				// On successful verification, clear lock counters
				nof_nonce_requests = 0;
				nof_failed_verifications = 0;
			} else {
				nof_failed_verifications++;
				SIGN_DEBUG(PSTR("Failed verification attempts left until lockdown: %d\n"), MY_NODE_LOCK_COUNTER_MAX-nof_failed_verifications);
				if (nof_failed_verifications >= MY_NODE_LOCK_COUNTER_MAX) {
					nodeLock("TMFV"); //Too many failed verifications
				}
			}
#endif
			mSetSigned(msg,0); // Clear the sign-flag now as verification is completed
		}
	}
#else
	(void)msg;
#endif // MY_SIGNING_REQUEST_SIGNATURES
	return verificationResult;
}

static uint8_t sha256_hash[32];
Sha256Class _soft_sha256;

void signerSha256Init(void) {
	memset(sha256_hash, 0, 32);
	_soft_sha256.init();
}

void signerSha256Update(const uint8_t* data, size_t sz) {
	for (size_t i = 0; i < sz; i++) {
		_soft_sha256.write(data[i]);
	}
}

uint8_t* signerSha256Final(void) {
	memcpy(sha256_hash, _soft_sha256.result(), 32);
	return sha256_hash;
}
