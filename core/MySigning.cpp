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
 */

#include "MySigning.h"

#define SIGNING_PRESENTATION_VERSION_1 1
#define SIGNING_PRESENTATION_REQUIRE_SIGNATURES   (1 << 0)
#define SIGNING_PRESENTATION_REQUIRE_WHITELISTING (1 << 1)

#if defined(MY_DEBUG_VERBOSE_SIGNING)
#define SIGN_DEBUG(x,...) DEBUG_OUTPUT(x, ##__VA_ARGS__)
#else
#define SIGN_DEBUG(x,...)
#endif

#if defined(MY_SIGNING_REQUEST_SIGNATURES) &&\
    (!defined(MY_SIGNING_ATSHA204) && !defined(MY_SIGNING_SOFT))
#error You have to pick either MY_SIGNING_ATSHA204 or MY_SIGNING_SOFT to reqire signatures!
#endif
#if defined(MY_SIGNING_SOFT) && defined(MY_SIGNING_ATSHA204)
#error You have to pick one and only one signing backend
#endif
#ifdef MY_SIGNING_FEATURE
static uint8_t _doSign[32];      // Bitfield indicating which sensors require signed communication
static uint8_t _doWhitelist[32]; // Bitfield indicating which sensors require salted signatures
static MyMessage _msgSign;       // Buffer for message to sign.
static uint8_t _signingNonceStatus;
static bool stateValid = false;

#ifdef MY_NODE_LOCK_FEATURE
static uint8_t nof_nonce_requests = 0;
static uint8_t nof_failed_verifications = 0;
#endif

// Status when waiting for signing nonce in signerSignMsg
enum { SIGN_WAITING_FOR_NONCE = 0, SIGN_OK = 1 };

// Macros for manipulating signing requirement tables
#define DO_SIGN(node) (~_doSign[node>>3]&(1<<node%8))
#define SET_SIGN(node) (_doSign[node>>3]&=~(1<<node%8))
#if defined(MY_SIGNING_WEAK_SECURITY)
#define CLEAR_SIGN(node) (_doSign[node>>3]|=(1<<node%8))
#endif
#define DO_WHITELIST(node) (~_doWhitelist[node>>3]&(1<<node%8))
#define SET_WHITELIST(node) (_doWhitelist[node>>3]&=~(1<<node%8))
#if defined(MY_SIGNING_WEAK_SECURITY)
#define CLEAR_WHITELIST(node) (_doWhitelist[node>>3]|=(1<<node%8))
#endif

#if defined(MY_SIGNING_SOFT)
extern bool signerAtsha204SoftInit(void);
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
extern bool signerAtsha204Init(void);
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
#if (defined(MY_ENCRYPTION_FEATURE) || defined(MY_SIGNING_FEATURE)) &&\
    !defined(MY_SIGNING_SIMPLE_PASSWD)
static bool signerInternalValidatePersonalization(void);
#endif

void signerInit(void)
{
#if defined(MY_SIGNING_FEATURE)
	stateValid = true;
#endif
#if (defined (MY_ENCRYPTION_FEATURE) || defined (MY_SIGNING_FEATURE)) &&\
    !defined (MY_SIGNING_SIMPLE_PASSWD)
	// Suppress this warning since it is only fixed on Linux builds and this keeps the code more tidy
	// cppcheck-suppress knownConditionTrueFalse
	if (!signerInternalValidatePersonalization()) {
		SIGN_DEBUG(PSTR("!SGN:PER:TAMPERED\n"));
#if defined(MY_SIGNING_FEATURE)
		stateValid = false;
#endif
	} else {
		SIGN_DEBUG(PSTR("SGN:PER:OK\n"));
	}
#endif
#if defined(MY_SIGNING_FEATURE)
	// Read out the signing requirements from EEPROM
	hwReadConfigBlock((void*)_doSign, (void*)EEPROM_SIGNING_REQUIREMENT_TABLE_ADDRESS,
	                  sizeof(_doSign));

	// Read out the whitelist requirements from EEPROM
	hwReadConfigBlock((void*)_doWhitelist, (void*)EEPROM_WHITELIST_REQUIREMENT_TABLE_ADDRESS,
	                  sizeof(_doWhitelist));

	if (!signerBackendInit()) {
		SIGN_DEBUG(PSTR("!SGN:INI:BND FAIL\n"));
	} else {
		SIGN_DEBUG(PSTR("SGN:INI:BND OK\n"));
	}
#endif
}

void signerPresentation(MyMessage &msg, uint8_t destination)
{
	prepareSigningPresentation(msg, destination);

#if defined(MY_SIGNING_REQUEST_SIGNATURES)
	msg.data[1] |= SIGNING_PRESENTATION_REQUIRE_SIGNATURES;
	SIGN_DEBUG(PSTR("SGN:PRE:SGN REQ\n")); // Signing required
#else
	SIGN_DEBUG(PSTR("SGN:PRE:SGN NREQ\n")); // Signing not required
#endif
#if defined(MY_SIGNING_NODE_WHITELISTING)
	msg.data[1] |= SIGNING_PRESENTATION_REQUIRE_WHITELISTING;
	SIGN_DEBUG(PSTR("SGN:PRE:WHI REQ\n")); // Whitelisting required
#else
	SIGN_DEBUG(PSTR("SGN:PRE:WHI NREQ\n")); // Whitelisting not required
#endif

	if (!_sendRoute(msg)) {
		SIGN_DEBUG(PSTR("!SGN:PRE:XMT,TO=%" PRIu8 " FAIL\n"),
		           destination); // Failed to transmit presentation!
	} else {
		SIGN_DEBUG(PSTR("SGN:PRE:XMT,TO=%" PRIu8 "\n"), destination); // Transmitted signing presentation!
	}

	if (destination == GATEWAY_ADDRESS) {
		SIGN_DEBUG(PSTR("SGN:PRE:WAIT GW\n")); // Waiting for GW to send signing preferences...
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
			// Before starting, validate that our state is good, or signing will fail
			if (!stateValid) {
				SIGN_DEBUG(PSTR("!SGN:SGN:STATE\n")); // Signing system is not in a valid state
				ret = false;
			} else {
				// Send nonce-request
				_signingNonceStatus=SIGN_WAITING_FOR_NONCE;
				if (!_sendRoute(build(_msgSign, msg.destination, msg.sensor, C_INTERNAL,
				                      I_NONCE_REQUEST).set(""))) {
					SIGN_DEBUG(PSTR("!SGN:SGN:NCE REQ,TO=%" PRIu8 " FAIL\n"),
					           msg.destination); // Failed to transmit nonce request!
					ret = false;
				} else {
					SIGN_DEBUG(PSTR("SGN:SGN:NCE REQ,TO=%" PRIu8 "\n"), msg.destination); // Nonce requested
					// We have to wait for the nonce to arrive before we can sign our original message
					// Other messages could come in-between. We trust _process() takes care of them
					unsigned long enter = hwMillis();
					_msgSign = msg; // Copy the message to sign since buffer might be touched in _process()
					while (hwMillis() - enter < MY_VERIFICATION_TIMEOUT_MS &&
					        _signingNonceStatus==SIGN_WAITING_FOR_NONCE) {
						_process();
					}
					if (hwMillis() - enter > MY_VERIFICATION_TIMEOUT_MS) {
						SIGN_DEBUG(PSTR("!SGN:SGN:NCE TMO\n")); // Timeout waiting for nonce!
						ret = false;
					} else {
						if (_signingNonceStatus == SIGN_OK) {
							// process() received a nonce and signerProcessInternal successfully signed the message
							msg = _msgSign; // Write the signed message back
							SIGN_DEBUG(PSTR("SGN:SGN:SGN\n")); // Message to send has been signed
							ret = true;
							// After this point, only the 'last' member of the message structure is allowed to be
							// altered if the message has been signed, or signature will become invalid and the
							// message rejected by the receiver
						} else {
							SIGN_DEBUG(PSTR("!SGN:SGN:SGN FAIL\n")); // Message to send could not be signed!
							ret = false;
						}
					}
				}
			}
		}
	} else if (getNodeId() == msg.sender) {
		mSetSigned(msg, 0); // Message is not supposed to be signed, make sure it is marked unsigned
		SIGN_DEBUG(PSTR("SGN:SGN:NREQ=%" PRIu8 "\n"),
		           msg.destination); // Do not sign message as it is not req
		ret = true;
	} else {
		SIGN_DEBUG(PSTR("SGN:SGN:%" PRIu8 "!=%" PRIu8 " NUS\n"), msg.sender,
		           getNodeId()); // Will not sign message since it was from someone else
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
#if defined(MY_SIGNING_WEAK_SECURITY)
	if ((!MY_IS_GATEWAY || DO_SIGN(msg.sender)) && msg.destination == getNodeId()) {
#else
	if (msg.destination == getNodeId()) {
#endif
		// Internal messages of certain types are not verified
		if (skipSign(msg)) {
			verificationResult = true;
		} else if (!mGetSigned(msg)) {
			// Got unsigned message that should have been signed
			SIGN_DEBUG(PSTR("!SGN:VER:NSG\n")); // Message is not signed, but it should have been!
			verificationResult = false;
		} else {
			// Before starting, validate that our state is good, or signing will fail
			if (!stateValid) {
				SIGN_DEBUG(PSTR("!SGN:VER:STATE\n")); // Signing system is not in a valid state
				verificationResult = false;
			} else {
				if (!signerBackendVerifyMsg(msg)) {
					SIGN_DEBUG(PSTR("!SGN:VER:FAIL\n")); // Signature verification failed!
					verificationResult = false;
				} else {
					SIGN_DEBUG(PSTR("SGN:VER:OK\n"));
				}
			}
#if defined(MY_NODE_LOCK_FEATURE)
			if (verificationResult) {
				// On successful verification, clear lock counters
				nof_nonce_requests = 0;
				nof_failed_verifications = 0;
			} else {
				nof_failed_verifications++;
				SIGN_DEBUG(PSTR("SGN:VER:LEFT=%" PRIu8 "\n"), MY_NODE_LOCK_COUNTER_MAX-nof_failed_verifications);
				if (nof_failed_verifications >= MY_NODE_LOCK_COUNTER_MAX) {
					_nodeLock("TMFV"); // Too many failed verifications
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
	bool ret = false;
	if (mGetAck(msg)) {
		ret = true;
	}	else if (mGetCommand(msg) == C_INTERNAL &&
	             (msg.type == I_SIGNING_PRESENTATION	||
	              msg.type == I_REGISTRATION_REQUEST	||
	              msg.type == I_NONCE_REQUEST					|| msg.type == I_NONCE_RESPONSE				||
	              msg.type == I_ID_REQUEST						|| msg.type == I_ID_RESPONSE					||
	              msg.type == I_FIND_PARENT_REQUEST		|| msg.type == I_FIND_PARENT_RESPONSE	||
	              msg.type == I_HEARTBEAT_REQUEST			|| msg.type == I_HEARTBEAT_RESPONSE		||
	              msg.type == I_PING									|| msg.type == I_PONG									||
	              msg.type == I_DISCOVER_REQUEST	    || msg.type == I_DISCOVER_RESPONSE    ||
	              msg.type == I_LOG_MESSAGE)) {
		ret = true;
	} else if (mGetCommand(msg) == C_STREAM &&
	           (msg.type == ST_SOUND            ||
	            msg.type == ST_IMAGE            ||
	            msg.type == ST_FIRMWARE_REQUEST || msg.type == ST_FIRMWARE_RESPONSE )) {
		ret = true;
	}
	if (ret) {
		SIGN_DEBUG(PSTR("SGN:SKP:%s CMD=%" PRIu8 ",TYPE=%" PRIu8 "\n"), mGetAck(msg) ? "ACK" : "MSG",
		           mGetCommand(msg),
		           msg.type); //Skip signing/verification of this message
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

// Helper to process presentation messages
static bool signerInternalProcessPresentation(MyMessage &msg)
{
	const uint8_t sender = msg.sender;
#if defined(MY_SIGNING_FEATURE)
	if (msg.data[0] != SIGNING_PRESENTATION_VERSION_1) {
		SIGN_DEBUG(PSTR("!SGN:PRE:VER=%" PRIu8 "\n"),
		           msg.data[0]); // Unsupported signing presentation version
		return true; // Just drop this presentation message
	}
	// We only handle version 1 here...
	if (msg.data[1] & SIGNING_PRESENTATION_REQUIRE_SIGNATURES) {
		// We received an indicator that the sender require us to sign all messages we send to it
		SIGN_DEBUG(PSTR("SGN:PRE:SGN REQ,FROM=%" PRIu8 "\n"), sender); // Node require signatures
		SET_SIGN(sender);
	} else {
#if defined(MY_SIGNING_WEAK_SECURITY)
		// We received an indicator that the sender does not require us to sign messages we send to it
		SIGN_DEBUG(PSTR("SGN:PRE:SGN NREQ,FROM=%" PRIu8 "\n"), sender); // Node does not require signatures
		CLEAR_SIGN(sender);
#else
		if (DO_SIGN(sender)) {
			SIGN_DEBUG(PSTR("!SGN:PRE:SGN NREQ,FROM=%" PRIu8 " REJ\n"),
			           sender); // Node does not require signatures but used to do so
		}
#endif
	}
	if (msg.data[1] & SIGNING_PRESENTATION_REQUIRE_WHITELISTING) {
		// We received an indicator that the sender require us to salt signatures with serial
		SIGN_DEBUG(PSTR("SGN:PRE:WHI REQ,FROM=%" PRIu8 "\n"), sender); // Node require whitelisting
		SET_WHITELIST(sender);
	} else {
#if defined(MY_SIGNING_WEAK_SECURITY)
		// We received an indicator that the sender does not require us to sign messages we send to it
		SIGN_DEBUG(PSTR("SGN:PRE:WHI NREQ,FROM=%" PRIu8 "\n"),
		           sender); // Node does not require whitelisting
		CLEAR_WHITELIST(sender);
#else
		if (DO_WHITELIST(sender)) {
			SIGN_DEBUG(PSTR("!SGN:PRE:WHI NREQ,FROM=%" PRIu8 " REJ\n"),
			           sender); // Node does not require whitelisting but used to do so
		}
#endif
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
#if defined(MY_SIGNING_WEAK_SECURITY)
	if (DO_SIGN(sender)) {
		msg.data[1] |= SIGNING_PRESENTATION_REQUIRE_SIGNATURES;
	}
#else
	msg.data[1] |= SIGNING_PRESENTATION_REQUIRE_SIGNATURES;
#endif
#endif // MY_SIGNING_REQUEST_SIGNATURES
#if defined(MY_SIGNING_NODE_WHITELISTING)
	msg.data[1] |= SIGNING_PRESENTATION_REQUIRE_WHITELISTING;
#endif // MY_SIGNING_NODE_WHITELISTING
	if (msg.data[1] & SIGNING_PRESENTATION_REQUIRE_SIGNATURES) {
		SIGN_DEBUG(PSTR("SGN:PRE:SGN REQ,TO=%" PRIu8 "\n"),
		           sender); // Inform node that we require signatures
	} else {
		SIGN_DEBUG(PSTR("SGN:PRE:SGN NREQ,TO=%" PRIu8 "\n"),
		           sender); // Inform node that we do not require signatures
	}
	if (msg.data[1] & SIGNING_PRESENTATION_REQUIRE_WHITELISTING) {
		SIGN_DEBUG(PSTR("SGN:PRE:WHI REQ,TO=%" PRIu8 "\n"),
		           sender); // Inform node that we require whitelisting
	} else {
		SIGN_DEBUG(PSTR("SGN:PRE:WHI NREQ,TO=%" PRIu8 "\n"),
		           sender); // Inform node that we do not require whitelisting
	}
	if (!_sendRoute(msg)) {
		SIGN_DEBUG(PSTR("!SGN:PRE:XMT,TO=%" PRIu8 " FAIL\n"),
		           sender); // Failed to transmit signing presentation!
	} else {
		SIGN_DEBUG(PSTR("SGN:PRE:XMT,TO=%" PRIu8 "\n"), sender);
	}
#endif // MY_GATEWAY_FEATURE
#else // not MY_SIGNING_FEATURE
#if defined(MY_GATEWAY_FEATURE)
	// If we act as gateway and do not have the signing feature and receive a signing request we still
	// need to do make sure the requester does not believe the gateway still require signatures
	prepareSigningPresentation(msg, sender);
	SIGN_DEBUG(PSTR("SGN:PRE:NSUP,TO=%" PRIu8 "\n"),
	           sender); // Informing node that we do not support signing
	if (!_sendRoute(msg)) {
		SIGN_DEBUG(PSTR("!SGN:PRE:XMT,TO=%" PRIu8 " FAIL\n"),
		           sender); // Failed to transmit signing presentation!
	} else {
		SIGN_DEBUG(PSTR("SGN:PRE:XMT,TO=%" PRIu8 "\n"), sender);
	}
#else // not MY_GATEWAY_FEATURE
	// If we act as a node and do not have the signing feature then we just silently drop any signing
	// presentation messages received
	(void)msg;
	(void)sender;
	// Received signing presentation, but signing is not supported (message ignored)
	SIGN_DEBUG(
	    PSTR("SGN:PRE:NSUP\n"));
#endif // not MY_GATEWAY_FEATURE
#endif // not MY_SIGNING_FEATURE 
	return true; // No need to further process I_SIGNING_PRESENTATION
}

// Helper to process nonce request messages
static bool signerInternalProcessNonceRequest(MyMessage &msg)
{
#if defined(MY_SIGNING_FEATURE)
#if defined(MY_NODE_LOCK_FEATURE)
	nof_nonce_requests++;
	SIGN_DEBUG(PSTR("SGN:NCE:LEFT=%" PRIu8 "\n"),
	           MY_NODE_LOCK_COUNTER_MAX-nof_nonce_requests); // Nonce requests left until lockdown
	if (nof_nonce_requests >= MY_NODE_LOCK_COUNTER_MAX) {
		_nodeLock("TMNR"); // Too many nonces requested
	}
#endif // MY_NODE_LOCK_FEATURE
	if (signerBackendGetNonce(msg)) {
		if (!_sendRoute(build(msg, msg.sender, NODE_SENSOR_ID, C_INTERNAL, I_NONCE_RESPONSE))) {
			SIGN_DEBUG(PSTR("!SGN:NCE:XMT,TO=%" PRIu8 " FAIL\n"), msg.sender); // Failed to transmit nonce!
		} else {
			SIGN_DEBUG(PSTR("SGN:NCE:XMT,TO=%" PRIu8 "\n"), msg.sender);
		}
	} else {
		SIGN_DEBUG(PSTR("!SGN:NCE:GEN\n")); // Failed to generate nonce!
	}
#else // not MY_SIGNING_FEATURE
	(void)msg;
	SIGN_DEBUG(
	    PSTR("SGN:NCE:NSUP (DROPPED)\n")); // Received nonce request/response without signing support
#endif // MY_SIGNING_FEATURE
	return true; // No need to further process I_NONCE_REQUEST
}

// Helper to process nonce response messages
static bool signerInternalProcessNonceResponse(MyMessage &msg)
{
#if defined(MY_SIGNING_FEATURE)
	// Proceed with signing if nonce has been received
	SIGN_DEBUG(PSTR("SGN:NCE:FROM=%" PRIu8 "\n"), msg.sender);
	if (msg.sender != _msgSign.destination) {
		SIGN_DEBUG(PSTR("SGN:NCE:%" PRIu8 "!=%" PRIu8 " (DROPPED)\n"), _msgSign.destination, msg.sender);
	} else {
		signerBackendPutNonce(msg);
		if (signerBackendSignMsg(_msgSign)) {
			// _msgSign now contains the signed message pending transmission
			_signingNonceStatus = SIGN_OK;
		}
	}
#else
	(void)msg;
	SIGN_DEBUG(
	    PSTR("SGN:NCE:NSUP (DROPPED)\n")); // Received nonce request/response without signing support
#endif
	return true; // No need to further process I_NONCE_RESPONSE
}

#if (defined(MY_ENCRYPTION_FEATURE) || defined(MY_SIGNING_FEATURE)) &&\
    !defined(MY_SIGNING_SIMPLE_PASSWD)
static bool signerInternalValidatePersonalization(void)
{
#ifdef __linux__
	// Personalization on linux is a bit more crude
	return true;
#else
	uint8_t buffer[SIZE_SIGNING_SOFT_HMAC_KEY + SIZE_RF_ENCRYPTION_AES_KEY + SIZE_SIGNING_SOFT_SERIAL];
	uint8_t hash[32];
	uint8_t checksum;
	hwReadConfigBlock((void*)buffer, (void*)EEPROM_SIGNING_SOFT_HMAC_KEY_ADDRESS,
	                  SIZE_SIGNING_SOFT_HMAC_KEY);
	hwReadConfigBlock((void*)&buffer[SIZE_SIGNING_SOFT_HMAC_KEY],
	                  (void*)EEPROM_RF_ENCRYPTION_AES_KEY_ADDRESS, SIZE_RF_ENCRYPTION_AES_KEY);
	hwReadConfigBlock((void*)&buffer[SIZE_SIGNING_SOFT_HMAC_KEY + SIZE_RF_ENCRYPTION_AES_KEY],
	                  (void*)EEPROM_SIGNING_SOFT_SERIAL_ADDRESS, SIZE_SIGNING_SOFT_SERIAL);
	hwReadConfigBlock((void*)&checksum, (void*)EEPROM_PERSONALIZATION_CHECKSUM_ADDRESS,
	                  SIZE_PERSONALIZATION_CHECKSUM);

	SHA256(hash, buffer, sizeof(buffer));
	return (checksum == hash[0]);
#endif
}
#endif
