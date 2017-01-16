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
#if defined(MY_SIGNING_GW_REQUEST_SIGNATURES_FROM_ALL) && !defined(MY_SIGNING_REQUEST_SIGNATURES)
#error You have to require signatures if you want to require signatures from all (also enable MY_SIGNING_REQUEST_SIGNATURES in your gateway)
#endif
#if defined(MY_SIGNING_SOFT) && defined(MY_SIGNING_ATSHA204)
#error You have to pick one and only one signing backend
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

// Status when waiting for signing nonce in signerSignMsg
enum { SIGN_WAITING_FOR_NONCE = 0, SIGN_OK = 1 };

// Macros for manipulating signing requirement tables
#define DO_SIGN(node) (~_doSign[node>>3]&(1<<node%8))
#define SET_SIGN(node) (_doSign[node>>3]&=~(1<<node%8))
#define CLEAR_SIGN(node) (_doSign[node>>3]|=(1<<node%8))
#define DO_WHITELIST(node) (~_doWhitelist[node>>3]&(1<<node%8))
#define SET_WHITELIST(node) (_doWhitelist[node>>3]&=~(1<<node%8))
#define CLEAR_WHITELIST(node) (_doWhitelist[node>>3]|=(1<<node%8))

#if defined(MY_SIGNING_SOFT)
extern void signerAtsha204SoftInit(void);
extern bool signerAtsha204SoftCheckTimer(void);
extern bool signerAtsha204SoftGetNonce(MyMessage &msg);
extern void signerAtsha204SoftPutNonce(MyMessage &msg);
extern bool signerAtsha204SoftVerifyMsg(MyMessage &msg);
extern bool signerAtsha204SoftSignMsg(MyMessage &msg);
#define signerBackendInit       signerAtsha204SoftInit
#define signerBackendCheckTimer signerAtsha204SoftCheckTimer
#define signerBackendGetNonce   signerAtsha204SoftGetNonce
#define signerBackendPutNonce   signerAtsha204SoftPutNonce
#define signerBackendVerifyMsg  signerAtsha204SoftVerifyMsg
#define signerBackendSignMsg    signerAtsha204SoftSignMsg
#elif defined(MY_SIGNING_ATSHA204)
extern void signerAtsha204Init(void);
extern bool signerAtsha204CheckTimer(void);
extern bool signerAtsha204GetNonce(MyMessage &msg);
extern void signerAtsha204PutNonce(MyMessage &msg);
extern bool signerAtsha204VerifyMsg(MyMessage &msg);
extern bool signerAtsha204SignMsg(MyMessage &msg);
#define signerBackendInit       signerAtsha204Init
#define signerBackendCheckTimer signerAtsha204CheckTimer
#define signerBackendGetNonce   signerAtsha204GetNonce
#define signerBackendPutNonce   signerAtsha204PutNonce
#define signerBackendVerifyMsg  signerAtsha204VerifyMsg
#define signerBackendSignMsg    signerAtsha204SignMsg
#endif
static bool skipSign(MyMessage &msg);
#else // not MY_SIGNING_FEATURE
#define signerBackendCheckTimer() true
#endif // MY_SIGNING_FEATURE
static void prepareSigningPresentation(MyMessage &msg, uint8_t destination);
static bool signerInternalProcessPresentation(MyMessage &msg);
static bool signerInternalProcessNonceRequest(MyMessage &msg);
static bool signerInternalProcessNonceResponse(MyMessage &msg);

void signerInit(void)
{
#if defined(MY_SIGNING_FEATURE)
	// Read out the signing requirements from EEPROM
	hwReadConfigBlock((void*)_doSign, (void*)EEPROM_SIGNING_REQUIREMENT_TABLE_ADDRESS,
	                  sizeof(_doSign));

	// Read out the whitelist requirements from EEPROM
	hwReadConfigBlock((void*)_doWhitelist, (void*)EEPROM_WHITELIST_REQUIREMENT_TABLE_ADDRESS,
	                  sizeof(_doWhitelist));

	signerBackendInit();
#endif
}

void signerPresentation(MyMessage &msg, uint8_t destination)
{
	prepareSigningPresentation(msg, destination);

#if defined(MY_SIGNING_REQUEST_SIGNATURES)
	msg.data[1] |= SIGNING_PRESENTATION_REQUIRE_SIGNATURES;
	SIGN_DEBUG(PSTR("Signing required\n"));
#endif
#if defined(MY_SIGNING_NODE_WHITELISTING)
	msg.data[1] |= SIGNING_PRESENTATION_REQUIRE_WHITELISTING;
	SIGN_DEBUG(PSTR("Whitelisting required\n"));
#endif

	if (!_sendRoute(msg)) {
		SIGN_DEBUG(PSTR("Failed to transmit signing presentation!\n"));
	}

	if (destination == GATEWAY_ADDRESS) {
		SIGN_DEBUG(PSTR("Waiting for GW to send signing preferences...\n"));
		wait(2000, C_INTERNAL, I_SIGNING_PRESENTATION);
	}
}

bool signerProcessInternal(MyMessage &msg)
{
	bool ret;
	switch (msg.type) {
	case I_SIGNING_PRESENTATION:
		ret = signerInternalProcessPresentation(msg);
		break;
	case I_NONCE_REQUEST:
		ret = signerInternalProcessNonceRequest(msg);
		break;
	case I_NONCE_RESPONSE:
		ret = signerInternalProcessNonceResponse(msg);
		break;
	default:
		ret = false; // Let the transport process this message further as it is not related to signing
		break;
	}
	return ret;
}

bool signerCheckTimer(void)
{
	return signerBackendCheckTimer();
}

bool signerSignMsg(MyMessage &msg)
{
	bool ret;
#if defined(MY_SIGNING_FEATURE)
	// If destination is known to require signed messages and we are the sender,
	// sign this message unless it is identified as an exception
	if (DO_SIGN(msg.destination) && msg.sender == getNodeId()) {
		if (skipSign(msg)) {
			ret = true;
		} else {
			// Send nonce-request
			_signingNonceStatus=SIGN_WAITING_FOR_NONCE;
			if (!_sendRoute(build(_msgSign, msg.destination, msg.sensor, C_INTERNAL,
			                      I_NONCE_REQUEST).set(""))) {
				SIGN_DEBUG(PSTR("Failed to transmit nonce request!\n"));
				ret = false;
			} else {
				SIGN_DEBUG(PSTR("Nonce requested from %d. Waiting...\n"), msg.destination);
				// We have to wait for the nonce to arrive before we can sign our original message
				// Other messages could come in-between. We trust _process() takes care of them
				unsigned long enter = hwMillis();
				_msgSign = msg; // Copy the message to sign since message buffer might be touched in _process()
				while (hwMillis() - enter < MY_VERIFICATION_TIMEOUT_MS &&
				        _signingNonceStatus==SIGN_WAITING_FOR_NONCE) {
					_process();
				}
				if (hwMillis() - enter > MY_VERIFICATION_TIMEOUT_MS) {
					SIGN_DEBUG(PSTR("Timeout waiting for nonce!\n"));
					ret = false;
				} else {
					if (_signingNonceStatus == SIGN_OK) {
						// process() received a nonce and signerProcessInternal successfully signed the message
						msg = _msgSign; // Write the signed message back
						SIGN_DEBUG(PSTR("Message to send has been signed\n"));
						ret = true;
						// After this point, only the 'last' member of the message structure is allowed to be altered if the
						// message has been signed, or signature will become invalid and the message rejected by the receiver
					} else {
						SIGN_DEBUG(PSTR("Message to send could not be signed!\n"));
						ret = false;
					}
				}
			}
		}
	} else if (getNodeId() == msg.sender) {
		mSetSigned(msg, 0); // Message is not supposed to be signed, make sure it is marked unsigned
		SIGN_DEBUG(PSTR("Will not sign message for destination %d as it does not require it\n"),
		           msg.destination);
		ret = true;
	} else {
		SIGN_DEBUG(PSTR("Will not sign message since it was from %d and we are %d\n"), msg.sender,
		           getNodeId());
		ret = true;
	}
#else
	(void)msg;
	ret = true;
#endif // MY_SIGNING_FEATURE
	return ret;
}

bool signerVerifyMsg(MyMessage &msg)
{
	bool verificationResult = true;
	// Before processing message, reject unsigned messages if signing is required and check signature
	// (if it is signed and addressed to us)
	// Note that we do not care at all about any signature found if we do not require signing
#if defined(MY_SIGNING_FEATURE) && defined(MY_SIGNING_REQUEST_SIGNATURES)
	// If we are a node, or we are a gateway and the sender require signatures (or just a strict gw)
	// and we are the destination...
#if defined(MY_SIGNING_GW_REQUEST_SIGNATURES_FROM_ALL)
	if (msg.destination == getNodeId()) {
#else
	if ((!MY_IS_GATEWAY || DO_SIGN(msg.sender)) && msg.destination == getNodeId()) {
#endif
		// Internal messages of certain types are not verified
		if (skipSign(msg)) {
			verificationResult = true;
		} else if (!mGetSigned(msg)) {
			// Got unsigned message that should have been signed
			SIGN_DEBUG(PSTR("Message is not signed, but it should have been!\n"));
			verificationResult = false;
		} else {
			if (!signerBackendVerifyMsg(msg)) {
				SIGN_DEBUG(PSTR("Signature verification failed!\n"));
				verificationResult = false;
			}
#if defined(MY_NODE_LOCK_FEATURE)
			if (verificationResult) {
				// On successful verification, clear lock counters
				nof_nonce_requests = 0;
				nof_failed_verifications = 0;
			} else {
				nof_failed_verifications++;
				SIGN_DEBUG(PSTR("Failed verification attempts left until lockdown: %d\n"),
				           MY_NODE_LOCK_COUNTER_MAX-nof_failed_verifications);
				if (nof_failed_verifications >= MY_NODE_LOCK_COUNTER_MAX) {
					_nodeLock("TMFV"); //Too many failed verifications
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

void signerSha256Init(void)
{
	memset(sha256_hash, 0, 32);
	_soft_sha256.init();
}

void signerSha256Update(const uint8_t* data, size_t sz)
{
	for (size_t i = 0; i < sz; i++) {
		_soft_sha256.write(data[i]);
	}
}

uint8_t* signerSha256Final(void)
{
	memcpy(sha256_hash, _soft_sha256.result(), 32);
	return sha256_hash;
}

int signerMemcmp(const void* a, const void* b, size_t sz)
{
	int retVal;
	size_t i;
	int done = 0;
	const uint8_t* ptrA = (const uint8_t*)a;
	const uint8_t* ptrB = (const uint8_t*)b;
	for (i=0; i < sz; i++) {
		if (ptrA[i] == ptrB[i]) {
			if (done > 0) {
				done = 1;
			} else {
				done = 0;
			}
		}	else {
			if (done > 0) {
				done = 2;
			} else {
				done = 3;
			}
		}
	}
	if (done > 0) {
		retVal = -1;
	} else {
		retVal = 0;
	}
	return retVal;
}

#if defined(MY_SIGNING_FEATURE)
// Helper function to centralize signing/verification exceptions
static bool skipSign(MyMessage &msg)
{
	bool ret;
	if (mGetAck(msg)) {
		SIGN_DEBUG(PSTR("Skipping security for ACK on command %d type %d\n"), mGetCommand(msg), msg.type);
		ret = true;
	}	else if (mGetCommand(msg) == C_INTERNAL &&
	             (msg.type == I_NONCE_REQUEST					|| msg.type == I_NONCE_RESPONSE				||
	              msg.type == I_SIGNING_PRESENTATION	||
	              msg.type == I_ID_REQUEST						|| msg.type == I_ID_RESPONSE					||
	              msg.type == I_FIND_PARENT_REQUEST		|| msg.type == I_FIND_PARENT_RESPONSE	||
	              msg.type == I_HEARTBEAT_REQUEST			|| msg.type == I_HEARTBEAT_RESPONSE		||
	              msg.type == I_PING									|| msg.type == I_PONG									||
	              msg.type == I_REGISTRATION_REQUEST	|| msg.type == I_DISCOVER_REQUEST	||
	              msg.type == I_DISCOVER_RESPONSE )) {
		SIGN_DEBUG(PSTR("Skipping security for command %d type %d\n"), mGetCommand(msg), msg.type);
		ret = true;
	} else if (mGetCommand(msg) == C_STREAM &&
	           (msg.type == ST_FIRMWARE_REQUEST || msg.type == ST_FIRMWARE_RESPONSE ||
	            msg.type == ST_SOUND || msg.type == ST_IMAGE)) {
		SIGN_DEBUG(PSTR("Skipping security for command %d type %d\n"), mGetCommand(msg), msg.type);
		ret = true;
	} else {
		ret = false;
	}
	return ret;
}
#endif

// Helper to prepare a signing presentation message
static void prepareSigningPresentation(MyMessage &msg, uint8_t destination)
{
	// Only supports version 1 for now
	(void)build(msg, destination, NODE_SENSOR_ID, C_INTERNAL, I_SIGNING_PRESENTATION).set("");
	mSetLength(msg, 2);
	mSetPayloadType(msg, P_CUSTOM);		// displayed as hex
	msg.data[0] = SIGNING_PRESENTATION_VERSION_1;
	msg.data[1] = 0;
}

// Helper to process presentation mesages
static bool signerInternalProcessPresentation(MyMessage &msg)
{
	const uint8_t sender = msg.sender;
#if defined(MY_SIGNING_FEATURE)
	if (msg.data[0] != SIGNING_PRESENTATION_VERSION_1) {
		SIGN_DEBUG(PSTR("Unsupported signing presentation version (%d)!\n"), msg.data[0]);
		return true; // Just drop this presentation message
	}
	// We only handle version 1 here...
	if (msg.data[1] & SIGNING_PRESENTATION_REQUIRE_SIGNATURES) {
		// We received an indicator that the sender require us to sign all messages we send to it
		SIGN_DEBUG(PSTR("Mark node %d as one that require signed messages\n"), sender);
		SET_SIGN(sender);
	} else {
		// We received an indicator that the sender does not require us to sign all messages we send to it
		SIGN_DEBUG(PSTR("Mark node %d as one that do not require signed messages\n"), sender);
		CLEAR_SIGN(sender);
	}
	if (msg.data[1] & SIGNING_PRESENTATION_REQUIRE_WHITELISTING) {
		// We received an indicator that the sender require us to salt signatures with serial
		SIGN_DEBUG(PSTR("Mark node %d as one that require whitelisting\n"), sender);
		SET_WHITELIST(sender);
	} else {
		// We received an indicator that the sender does not require us to sign all messages we send to it
		SIGN_DEBUG(PSTR("Mark node %d as one that do not require whitelisting\n"), sender);
		CLEAR_WHITELIST(sender);
	}

	// Save updated tables
	hwWriteConfigBlock((void*)_doSign, (void*)EEPROM_SIGNING_REQUIREMENT_TABLE_ADDRESS,
	                   sizeof(_doSign));
	hwWriteConfigBlock((void*)_doWhitelist, (void*)EEPROM_WHITELIST_REQUIREMENT_TABLE_ADDRESS,
	                   sizeof(_doWhitelist));

	// Inform sender about our preference if we are a gateway, but only require signing if the sender
	// required signing unless we explicitly configure it to
#if defined(MY_GATEWAY_FEATURE)
	prepareSigningPresentation(msg, sender);
#if defined(MY_SIGNING_REQUEST_SIGNATURES)
#if defined(MY_SIGNING_GW_REQUEST_SIGNATURES_FROM_ALL)
	msg.data[1] |= SIGNING_PRESENTATION_REQUIRE_SIGNATURES;
#else
	if (DO_SIGN(sender)) {
		msg.data[1] |= SIGNING_PRESENTATION_REQUIRE_SIGNATURES;
	}
#endif
#endif // MY_SIGNING_REQUEST_SIGNATURES
#if defined(MY_SIGNING_NODE_WHITELISTING)
#if defined(MY_SIGNING_GW_REQUEST_SIGNATURES_FROM_ALL)
	msg.data[1] |= SIGNING_PRESENTATION_REQUIRE_WHITELISTING;
#else
	if (DO_WHITELIST(sender)) {
		msg.data[1] |= SIGNING_PRESENTATION_REQUIRE_WHITELISTING;
	}
#endif
#endif // MY_SIGNING_NODE_WHITELISTING
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
	if (!_sendRoute(msg)) {
		SIGN_DEBUG(PSTR("Failed to transmit signing presentation!\n"));
	}
#endif // MY_GATEWAY_FEATURE
#else // not MY_SIGNING_FEATURE
#if defined(MY_GATEWAY_FEATURE)
	// If we act as gateway and do not have the signing feature and receive a signing request we still
	// need to do make sure the requester does not believe the gateway still require signatures
	prepareSigningPresentation(msg, sender);
	SIGN_DEBUG(
	    PSTR("Informing node %d that we do not require signatures because we do not support it\n"),
	    sender);
	if (!_sendRoute(msg)) {
		SIGN_DEBUG(PSTR("Failed to transmit signing presentation!\n"));
	}
#else // not MY_GATEWAY_FEATURE
	// If we act as a node and do not have the signing feature then we just silently drop any signing
	// presentation messages received
	(void)msg;
	(void)sender;
	SIGN_DEBUG(PSTR("Received signing presentation, but signing is not supported (message ignored)\n"));
#endif // not MY_GATEWAY_FEATURE
#endif // not MY_SIGNING_FEATURE 
	return true; // No need to further process I_SIGNING_PRESENTATION
}

// Helper to process nonce request mesages
static bool signerInternalProcessNonceRequest(MyMessage &msg)
{
#if defined(MY_SIGNING_FEATURE)
#if defined(MY_NODE_LOCK_FEATURE)
	nof_nonce_requests++;
	SIGN_DEBUG(PSTR("Nonce requests left until lockdown: %d\n"),
	           MY_NODE_LOCK_COUNTER_MAX-nof_nonce_requests);
	if (nof_nonce_requests >= MY_NODE_LOCK_COUNTER_MAX) {
		_nodeLock("TMNR"); //Too many nonces requested
	}
#endif // MY_NODE_LOCK_FEATURE
	if (signerBackendGetNonce(msg)) {
		if (!_sendRoute(build(msg, msg.sender, NODE_SENSOR_ID, C_INTERNAL, I_NONCE_RESPONSE))) {
			SIGN_DEBUG(PSTR("Failed to transmit nonce!\n"));
		} else {
			SIGN_DEBUG(PSTR("Transmitted nonce\n"));
		}
	} else {
		SIGN_DEBUG(PSTR("Failed to generate nonce!\n"));
	}
#else // not MY_SIGNING_FEATURE
	(void)msg;
	SIGN_DEBUG(PSTR("Received nonce request, but signing is not supported (message ignored)\n"));
#endif // MY_SIGNING_FEATURE
	return true; // No need to further process I_NONCE_REQUEST
}

// Helper to process nonce response mesages
static bool signerInternalProcessNonceResponse(MyMessage &msg)
{
#if defined(MY_SIGNING_FEATURE)
	// Proceed with signing if nonce has been received
	SIGN_DEBUG(PSTR("Nonce received from %d.\n"), msg.sender);
	if (msg.sender != _msgSign.destination) {
		SIGN_DEBUG(PSTR("Nonce did not come from the destination (%d) of the message to be signed! "
		                "It came from %d.\n"), _msgSign.destination, msg.sender);
		SIGN_DEBUG(PSTR("Silently discarding this nonce\n"));
	} else {
		SIGN_DEBUG(PSTR("Proceeding with signing...\n"));
		signerBackendPutNonce(msg);
		if (!signerBackendSignMsg(_msgSign)) {
			SIGN_DEBUG(PSTR("Failed to sign message!\n"));
		} else {
			SIGN_DEBUG(PSTR("Message signed\n"));
			_signingNonceStatus = SIGN_OK; // _msgSign now contains the signed message pending transmission
		}
	}
#else
	(void)msg;
	SIGN_DEBUG(PSTR("Received nonce response, but signing is not supported (message ignored)\n"));
#endif
	return true; // No need to further process I_NONCE_RESPONSE
}
