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
 */


#include "MySensor.h"

#define DISTANCE_INVALID (0xFF)

#ifdef MY_SIGNING_FEATURE
// Macros for manipulating signing requirement table
#define DO_SIGN(node) (~doSign[node>>3]&(1<<(node%8)))
#define SET_SIGN(node) (doSign[node>>3]&=~(1<<(node%8)))
#define CLEAR_SIGN(node) (doSign[node>>3]|=(1<<(node%8)))
#endif

// Inline function and macros
static inline MyMessage& build (MyMessage &msg, uint8_t sender, uint8_t destination, uint8_t sensor, uint8_t command, uint8_t type, bool enableAck) {
	msg.sender = sender;
	msg.destination = destination;
	msg.sensor = sensor;
	msg.type = type;
	mSetCommand(msg,command);
	mSetRequestAck(msg,enableAck);
	mSetAck(msg,false);
	return msg;
}

static inline bool isValidParent( const uint8_t parent ) {
	return parent != AUTO;
}
static inline bool isValidDistance( const uint8_t distance ) {
	return distance != DISTANCE_INVALID;
}


MySensor::MySensor(MyTransport &_radio, MyHw &_hw
#ifdef MY_SIGNING_FEATURE
	, MySigning &_signer
#endif
#ifdef WITH_LEDS_BLINKING
		, uint8_t _rx, uint8_t _tx, uint8_t _er, unsigned long _blink_period
#endif
	)
	:
	radio(_radio),
#ifdef MY_SIGNING_FEATURE
	signer(_signer),
#endif
#ifdef WITH_LEDS_BLINKING
	pinRx(_rx), pinTx(_tx), pinEr(_er), ledBlinkPeriod(_blink_period),
#endif
#ifdef MY_OTA_FIRMWARE_FEATURE
 	flash(MY_OTA_FLASH_SS, MY_OTA_FLASH_JDECID),
#endif
	hw(_hw)
{
}


#ifdef MY_OTA_FIRMWARE_FEATURE

// do a crc16 on the whole received firmware
bool MySensor::isValidFirmware() {		
	// init crc
	uint16_t crc = ~0;
	for (uint16_t i = 0; i < fc.blocks * FIRMWARE_BLOCK_SIZE; ++i) {
		crc ^= flash.readByte(i + FIRMWARE_START_OFFSET);
	    for (int8_t j = 0; j < 8; ++j) {
	        if (crc & 1)
	            crc = (crc >> 1) ^ 0xA001;
	        else
	            crc = (crc >> 1);
	    }
	}	
	return crc == fc.crc; 
}

#endif

#ifdef WITH_LEDS_BLINKING
void MySensor::handleLedsBlinking() {
	// Just return if it is not the time...
	// http://playground.arduino.cc/Code/TimingRollover
	if ((long)(hw_millis() - blink_next_time) < 0)
		return;
	else
		blink_next_time = hw_millis() + ledBlinkPeriod;

	// do the actual blinking
	if(countRx && countRx != 255) {
		// switch led on
		hw_digitalWrite(pinRx, LED_ON);
	}
	else if(!countRx) {
		// switching off
		hw_digitalWrite(pinRx, LED_OFF);
	}
	if(countRx != 255)
		--countRx;

	if(countTx && countTx != 255) {
		// switch led on
		hw_digitalWrite(pinTx, LED_ON);
	}
	else if(!countTx) {
		// switching off
		hw_digitalWrite(pinTx, LED_OFF);
	}
	if(countTx != 255)
		--countTx;

	if(countErr && countErr != 255) {
		// switch led on
		hw_digitalWrite(pinEr, LED_ON);
	}
	else if(!countErr) {
		// switching off
		hw_digitalWrite(pinEr, LED_OFF);
	}
	if(countErr != 255)
		--countErr;
}

void MySensor::rxBlink(uint8_t cnt) {
  if(countRx == 255) { countRx = cnt; }
}

void MySensor::txBlink(uint8_t cnt) {
  if(countTx == 255) { countTx = cnt; }
}

void MySensor::errBlink(uint8_t cnt) {
  if(countErr == 255) { countErr = cnt; }
}
#endif

void MySensor::begin(void (*_msgCallback)(const MyMessage &), uint8_t _nodeId, boolean _repeaterMode, uint8_t _parentNodeId) {
    #ifdef ENABLED_SERIAL
	    hw_init();
    #endif
	repeaterMode = _repeaterMode;
	msgCallback = _msgCallback;
	failedTransmissions = 0;

	// Only gateway should use node id 0!
	isGateway = _nodeId == GATEWAY_ADDRESS;

	// Setup radio
	if (!radio.init()) {
		debug(PSTR("radio init fail\n"));
		while(1); // Nothing more we can do
	}

#ifdef MY_SIGNING_FEATURE
	// Read out the signing requirements from EEPROM
	hw_readConfigBlock((void*)doSign, (void*)EEPROM_SIGNING_REQUIREMENT_TABLE_ADDRESS, sizeof(doSign));
#endif

#ifdef WITH_LEDS_BLINKING
	// Setup led pins
	pinMode(pinRx, OUTPUT);
	pinMode(pinTx, OUTPUT);
	pinMode(pinEr, OUTPUT);

	// Set initial state of leds
	hw_digitalWrite(pinRx, LED_OFF);
	hw_digitalWrite(pinTx, LED_OFF);
	hw_digitalWrite(pinEr, LED_OFF);

	// initialize counters
	countRx = 0;
	countTx = 0;
	countErr = 0;
#endif

	if (isGateway) {
		// Set configuration for gateway
		nc.parentNodeId = GATEWAY_ADDRESS;
		nc.distance = 0;
		nc.nodeId = GATEWAY_ADDRESS;
	} else {
		// Read settings from eeprom
		hw_readConfigBlock((void*)&nc, (void*)EEPROM_NODE_ID_ADDRESS, sizeof(NodeConfig));
		// Read latest received controller configuration from EEPROM
		hw_readConfigBlock((void*)&cc, (void*)EEPROM_CONTROLLER_CONFIG_ADDRESS, sizeof(ControllerConfig));
#ifdef MY_OTA_FIRMWARE_FEATURE
		// Read firmware config from EEPROM, i.e. type, version, CRC, blocks
		hw_readConfigBlock((void*)&fc, (void*)EEPROM_FIRMWARE_TYPE_ADDRESS, sizeof(NodeFirmwareConfig));
#endif

		if (cc.isMetric == 0xff) {
			// Eeprom empty, set default to metric
			cc.isMetric = 0x01;
		}

		autoFindParent = _parentNodeId == AUTO;
		if (!autoFindParent) {
			nc.parentNodeId = _parentNodeId;
			// Save static parent id in eeprom (used by bootloader)
			hw_writeConfig(EEPROM_PARENT_NODE_ID_ADDRESS, _parentNodeId);
			// We don't actually know the distance to gw here. Let's pretend it is 1.
			// If the current node is also repeater, be aware of this.
			nc.distance = 1;
		} else if (!isValidParent(nc.parentNodeId)) {
			// Auto find parent, but parent in eeprom is invalid. Try find one.
			findParentNode();
		}

		if (_nodeId != AUTO) {
			// Set static id
			nc.nodeId = _nodeId;
			// Save static id in eeprom
			hw_writeConfig(EEPROM_NODE_ID_ADDRESS, _nodeId);
		} else if (nc.nodeId == AUTO && isValidParent(nc.parentNodeId)) {
			// Try to fetch node-id from gateway
			requestNodeId();
		}
	}

	setupNode();
	debug(PSTR("%s started, id=%d, parent=%d, distance=%d\n"), isGateway?"gateway":(repeaterMode?"repeater":"sensor"), nc.nodeId, nc.parentNodeId, nc.distance);
}


uint8_t MySensor::getNodeId() {
	return nc.nodeId;
}

ControllerConfig MySensor::getConfig() {
	return cc;
}

void MySensor::requestNodeId() {
	debug(PSTR("req id\n"));
	radio.setAddress(nc.nodeId);
	build(msg, nc.nodeId, GATEWAY_ADDRESS, NODE_SENSOR_ID, C_INTERNAL, I_ID_REQUEST, false).set("");
	sendWrite(nc.parentNodeId, msg);
	wait(2000);
}

void MySensor::setupNode() {
	// Open reading pipe for messages directed to this node (set write pipe to same)
	radio.setAddress(nc.nodeId);

	// Present node and request config
	if (!isGateway && nc.nodeId != AUTO) {
#ifdef MY_SIGNING_FEATURE
		// Notify gateway (and possibly controller) about the signing preferences of this node
		sendRoute(build(msg, nc.nodeId, GATEWAY_ADDRESS, NODE_SENSOR_ID, C_INTERNAL, I_REQUEST_SIGNING, false).set(signer.requestSignatures()));

		// If we do require signing, wait for the gateway to tell us how it prefer us to transmit our messages
		if (signer.requestSignatures()) {
			wait(2000);
		}
#else
		// We do not support signing, make sure gateway knows this
		sendRoute(build(msg, nc.nodeId, GATEWAY_ADDRESS, NODE_SENSOR_ID, C_INTERNAL, I_REQUEST_SIGNING, false).set(false));
#endif

		// Send presentation for this radio node (attach
		present(NODE_SENSOR_ID, repeaterMode? S_ARDUINO_REPEATER_NODE : S_ARDUINO_NODE);

		// Send a configuration exchange request to controller
		// Node sends parent node. Controller answers with latest node configuration
		// which is picked up in process()
		sendRoute(build(msg, nc.nodeId, GATEWAY_ADDRESS, NODE_SENSOR_ID, C_INTERNAL, I_CONFIG, false).set(nc.parentNodeId));

		// Wait configuration reply.
		wait(2000);

#ifdef MY_OTA_FIRMWARE_FEATURE
		RequestFirmwareConfig *reqFWConfig = (RequestFirmwareConfig *)msg.data;
		mSetLength(msg, sizeof(RequestFirmwareConfig));
		mSetCommand(msg, C_STREAM);
		mSetPayloadType(msg,P_CUSTOM);
		// copy node settings to reqFWConfig
		memcpy(reqFWConfig,&fc,sizeof(NodeFirmwareConfig));
		// add bootloader information
		reqFWConfig->BLVersion = MY_OTA_BOOTLOADER_VERSION;
		fwUpdateOngoing = false;
		sendRoute(build(msg, nc.nodeId, GATEWAY_ADDRESS, NODE_SENSOR_ID, C_STREAM, ST_FIRMWARE_CONFIG_REQUEST, false));
#endif


	}
}

void MySensor::findParentNode() {
	static boolean findingParentNode = false;

	if (findingParentNode)
		return;
	findingParentNode = true;

	failedTransmissions = 0;

	// Set distance to max
	nc.distance = 255;

	// Send ping message to BROADCAST_ADDRESS (to which all relaying nodes and gateway listens and should reply to)
	debug(PSTR("find parent\n"));

	build(msg, nc.nodeId, BROADCAST_ADDRESS, NODE_SENSOR_ID, C_INTERNAL, I_FIND_PARENT, false).set("");
	// Write msg, but suppress recursive parent search
	sendWrite(BROADCAST_ADDRESS, msg);

	// Wait for ping response.
	wait(2000);
	findingParentNode = false;
}

boolean MySensor::sendRoute(MyMessage &message) {
	uint8_t sender = message.sender;
	uint8_t dest = message.destination;
	uint8_t last = message.last;
	bool ok;

	// If we still don't have any parent id, re-request and skip this message.
	if (nc.parentNodeId == AUTO) {
		findParentNode();
#ifdef WITH_LEDS_BLINKING
		errBlink(1);
#endif
		return false;
	}

	// If we still don't have any node id, re-request and skip this message.
	if (nc.nodeId == AUTO) {
		requestNodeId();
#ifdef WITH_LEDS_BLINKING
		errBlink(1);
#endif
		return false;
	}

	mSetVersion(message, PROTOCOL_VERSION);

#ifdef MY_SIGNING_FEATURE
	// If destination is known to require signed messages and we are the sender, sign this message unless it is an ACK or a handshake message
	if (DO_SIGN(message.destination) && message.sender == nc.nodeId && !mGetAck(message) && mGetLength(message) &&
		(mGetCommand(message) != C_INTERNAL ||
		 (message.type != I_GET_NONCE && message.type != I_GET_NONCE_RESPONSE && message.type != I_REQUEST_SIGNING &&
		  message.type != I_ID_REQUEST && message.type != I_ID_RESPONSE &&
		  message.type != I_FIND_PARENT && message.type != I_FIND_PARENT_RESPONSE))) {
		bool signOk = false;
		// Send nonce-request
		if (!sendRoute(build(tmpMsg, nc.nodeId, message.destination, message.sensor, C_INTERNAL, I_GET_NONCE, false).set(""))) {
			debug(PSTR("nonce tr err\n"));
			return false;
		}
		// We have to wait for the nonce to arrive before we can sign our original message
		// Other messages could come in-between. We trust process() takes care of them
		unsigned long enter = hw_millis();
		msgSign = message; // Copy the message to sign since message buffer might be touched in process()
		while (hw_millis() - enter < MY_VERIFICATION_TIMEOUT_MS) {
			if (process()) {
				if (mGetCommand(getLastMessage()) == C_INTERNAL && getLastMessage().type == I_GET_NONCE_RESPONSE) {
					// Proceed with signing if nonce has been received
					if (signer.putNonce(getLastMessage()) && signer.signMsg(msgSign)) {
						message = msgSign; // Write the signed message back
						signOk = true;
					}
					break;
				}
			}
		}
		if (hw_millis() - enter > MY_VERIFICATION_TIMEOUT_MS) {
			debug(PSTR("nonce tmo\n"));
#ifdef WITH_LEDS_BLINKING
			errBlink(1);
#endif
			return false;
		}
		if (!signOk) {
			debug(PSTR("sign fail\n"));
#ifdef WITH_LEDS_BLINKING
			errBlink(1);
#endif
			return false;
		}
		// After this point, only the 'last' member of the message structure is allowed to be altered if the message has been signed,
		// or signature will become invalid and the message rejected by the receiver
	} else if (nc.nodeId == message.sender) {
		mSetSigned(message, 0); // Message is not supposed to be signed, make sure it is marked unsigned
	}
#endif

	if (dest == GATEWAY_ADDRESS || !repeaterMode) {
		// Store this address in routing table (if repeater)
		if (repeaterMode) {
			hw_writeConfig(EEPROM_ROUTES_ADDRESS+sender, last);
		}
		// If destination is the gateway or if we aren't a repeater, let
		// our parent take care of the message
		ok = sendWrite(nc.parentNodeId, message);
	} else {
		// Relay the message
		uint8_t route = hw_readConfig(EEPROM_ROUTES_ADDRESS+dest);
		if (route > GATEWAY_ADDRESS && route < BROADCAST_ADDRESS) {
			// This message should be forwarded to a child node. If we send message
			// to this nodes pipe then all children will receive it because the are
			// all listening to this nodes pipe.
			//
			//    +----B
			//  -A
			//    +----C------D
			//
			//  We're node C, Message comes from A and has destination D
			//
			// Message destination is not gateway and is in routing table for this node.
			// Send it downstream
			return sendWrite(route, message);
		} else if (sender == GATEWAY_ADDRESS && dest == BROADCAST_ADDRESS) {
			// Node has not yet received any id. We need to send it
			// by doing a broadcast sending,
			return sendWrite(BROADCAST_ADDRESS, message);
		} else if (isGateway) {
			// Destination isn't in our routing table and isn't a broadcast address
			// Nothing to do here
			return false;
		} else  {
			// A message comes from a child node and we have no
			// route for it.
			//
			//    +----B
			//  -A
			//    +----C------D    <-- Message comes from D
			//
			//     We're node C
			//
			// Message should be passed to node A (this nodes relay)

			// This message should be routed back towards sensor net gateway
			ok = sendWrite(nc.parentNodeId, message);
			// Add this child to our "routing table" if it not already exist
			hw_writeConfig(EEPROM_ROUTES_ADDRESS+sender, last);
		}
	}

	if (!ok) {
		// Failure when sending to parent node. The parent node might be down and we
		// need to find another route to gateway.
#ifdef WITH_LEDS_BLINKING
		errBlink(1);
#endif
		failedTransmissions++;
		if (autoFindParent && failedTransmissions > SEARCH_FAILURES) {
			findParentNode();
		}
	} else {
		failedTransmissions = 0;
	}
	return ok;
}

boolean MySensor::sendWrite(uint8_t to, MyMessage &message) {
	mSetVersion(message, PROTOCOL_VERSION);
	uint8_t length = mGetSigned(message) ? MAX_MESSAGE_LENGTH : mGetLength(message);
	message.last = nc.nodeId;
#ifdef WITH_LEDS_BLINKING
	txBlink(1);
#endif
	bool ok = radio.send(to, &message, min(MAX_MESSAGE_LENGTH, HEADER_SIZE + length));

	debug(PSTR("send: %d-%d-%d-%d s=%d,c=%d,t=%d,pt=%d,l=%d,sg=%d,st=%s:%s\n"),
			message.sender,message.last, to, message.destination, message.sensor, mGetCommand(message), message.type,
			mGetPayloadType(message), mGetLength(message), mGetSigned(message), to==BROADCAST_ADDRESS ? "bc" : (ok ? "ok":"fail"), message.getString(convBuf));

	return ok;
}

bool MySensor::send(MyMessage &message, bool enableAck) {
	message.sender = nc.nodeId;
	mSetCommand(message,C_SET);
    mSetRequestAck(message,enableAck);
	return sendRoute(message);
}

void MySensor::sendBatteryLevel(uint8_t value, bool enableAck) {
	sendRoute(build(msg, nc.nodeId, GATEWAY_ADDRESS, NODE_SENSOR_ID, C_INTERNAL, I_BATTERY_LEVEL, enableAck).set(value));
}

void MySensor::present(uint8_t childSensorId, uint8_t sensorType, const char *description, bool enableAck) {
	sendRoute(build(msg, nc.nodeId, GATEWAY_ADDRESS, childSensorId, C_PRESENTATION, sensorType, enableAck).set(childSensorId==NODE_SENSOR_ID?LIBRARY_VERSION:description));
}

void MySensor::sendSketchInfo(const char *name, const char *version, bool enableAck) {
	if (name != NULL) {
		sendRoute(build(msg, nc.nodeId, GATEWAY_ADDRESS, NODE_SENSOR_ID, C_INTERNAL, I_SKETCH_NAME, enableAck).set(name));
	}
    if (version != NULL) {
    	sendRoute(build(msg, nc.nodeId, GATEWAY_ADDRESS, NODE_SENSOR_ID, C_INTERNAL, I_SKETCH_VERSION, enableAck).set(version));
    }
}

void MySensor::request(uint8_t childSensorId, uint8_t variableType, uint8_t destination) {
	sendRoute(build(msg, nc.nodeId, destination, childSensorId, C_REQ, variableType, false).set(""));
}

void MySensor::requestTime(void (* _timeCallback)(unsigned long)) {
	timeCallback = _timeCallback;
	sendRoute(build(msg, nc.nodeId, GATEWAY_ADDRESS, NODE_SENSOR_ID, C_INTERNAL, I_TIME, false).set(""));
}

boolean MySensor::process() {
	hw_watchdogReset();

#ifdef WITH_LEDS_BLINKING
	handleLedsBlinking();
#endif

	uint8_t to = 0;
	if (!radio.available(&to))
	{
#ifdef MY_OTA_FIRMWARE_FEATURE
		unsigned long enter = hw_millis();
		if (fwUpdateOngoing && (enter - fwLastRequestTime > MY_OTA_RETRY_DELAY)) {
			if (!fwRetry) {
				debug(PSTR("fw upd fail\n"));
				// Give up. We have requested MY_OTA_RETRY times without any packet in return.
				fwUpdateOngoing = false;
#ifdef WITH_LEDS_BLINKING
				errBlink(1);
#endif
				return false;
			}
			fwRetry--;
			fwLastRequestTime = enter;
			// Time to (re-)request firmware block from controller
			RequestFWBlock *firmwareRequest = (RequestFWBlock *)msg.data;
			mSetLength(msg, sizeof(RequestFWBlock));
			firmwareRequest->type = fc.type;
			firmwareRequest->version = fc.version;
			firmwareRequest->block = (fwBlock - 1);
			sendRoute(build(msg, nc.nodeId, GATEWAY_ADDRESS, NODE_SENSOR_ID, C_STREAM, ST_FIRMWARE_REQUEST, false));
		}
#endif
		return false;
	}

#ifdef MY_SIGNING_FEATURE
	(void)signer.checkTimer(); // Manage signing timeout
#endif

	uint8_t len = radio.receive((uint8_t *)&msg);
	(void)len; //until somebody makes use of 'len'
#ifdef WITH_LEDS_BLINKING
	rxBlink(1);
#endif

#ifdef MY_SIGNING_FEATURE
	// Before processing message, reject unsigned messages if signing is required and check signature (if it is signed and addressed to us)
	// Note that we do not care at all about any signature found if we do not require signing, nor do we care about ACKs (they are never signed)
	if (signer.requestSignatures() && msg.destination == nc.nodeId && mGetLength(msg) && !mGetAck(msg) &&
		(mGetCommand(msg) != C_INTERNAL ||
		 (msg.type != I_GET_NONCE_RESPONSE && msg.type != I_GET_NONCE && msg.type != I_REQUEST_SIGNING &&
		  msg.type != I_ID_REQUEST && msg.type != I_ID_RESPONSE &&
		  msg.type != I_FIND_PARENT && msg.type != I_FIND_PARENT_RESPONSE))) {
		if (!mGetSigned(msg)) {
			// Got unsigned message that should have been signed
			debug(PSTR("no sign\n"));
#ifdef WITH_LEDS_BLINKING
			errBlink(1);
#endif
			return false;
		}
		else if (!signer.verifyMsg(msg)) {
			debug(PSTR("verify fail\n"));
#ifdef WITH_LEDS_BLINKING
			errBlink(1);
#endif
			return false; // This signed message has been tampered with!
		}
	}
#endif

	if (msg.destination == nc.nodeId) {
		debug(PSTR("read: %d-%d-%d s=%d,c=%d,t=%d,pt=%d,l=%d,sg=%d:%s\n"),
			msg.sender, msg.last, msg.destination, msg.sensor, mGetCommand(msg), msg.type, mGetPayloadType(msg), mGetLength(msg), mGetSigned(msg), msg.getString(convBuf));
	} else {
		if (repeaterMode && nc.nodeId != AUTO) {
			debug(PSTR("read and forward: %d-%d-%d s=%d,c=%d,t=%d,pt=%d,l=%d,sg=%d:%s\n"),
				msg.sender, msg.last, msg.destination, msg.sensor, mGetCommand(msg), msg.type, mGetPayloadType(msg), mGetLength(msg), mGetSigned(msg), msg.getString(convBuf));
		} else {
			debug(PSTR("read and drop: %d-%d-%d s=%d,c=%d,t=%d,pt=%d,l=%d,sg=%d:%s\n"),
				msg.sender, msg.last, msg.destination, msg.sensor, mGetCommand(msg), msg.type, mGetPayloadType(msg), mGetLength(msg), mGetSigned(msg), msg.getString(convBuf));
		}
	}

	if(!(mGetVersion(msg) == PROTOCOL_VERSION)) {
		debug(PSTR("ver mismatch\n"));
#ifdef WITH_LEDS_BLINKING
		errBlink(1);
#endif
		return false;
	}

	uint8_t command = mGetCommand(msg);
	uint8_t type = msg.type;
	uint8_t sender = msg.sender;
	uint8_t last = msg.last;
	uint8_t destination = msg.destination;

	if (destination == nc.nodeId) {
		// This message is addressed to this node
		mSetSigned(msg,0);
		msg.data[mGetLength(msg)] = '\0'; // Add NULL termination so that "getters" works as expected

		if (repeaterMode && last != nc.parentNodeId) {
			// Message is from one of the child nodes. Add it to routing table.
			hw_writeConfig(EEPROM_ROUTES_ADDRESS+sender, last);
		}

		// Check if sender requests an ack back.
		if (mGetRequestAck(msg)) {
			// Copy message
			tmpMsg = msg;
			mSetRequestAck(tmpMsg,false); // Reply without ack flag (otherwise we would end up in an eternal loop)
			mSetAck(tmpMsg,true);
			tmpMsg.sender = nc.nodeId;
			tmpMsg.destination = msg.sender;
			sendRoute(tmpMsg);
		}

		if (command == C_INTERNAL) {
			if (type == I_FIND_PARENT_RESPONSE) {
				if (autoFindParent) {
					// We've received a reply to a FIND_PARENT message. Check if the distance is
					// shorter than we already have.
					uint8_t distance = msg.getByte();
					if (isValidDistance(distance))
					{
						// Distance to gateway is one more for us w.r.t. parent
						distance++;
						if (isValidDistance(distance) && (distance < nc.distance)) {
							// Found a neighbor closer to GW than previously found
							nc.distance = distance;
							nc.parentNodeId = msg.sender;
							hw_writeConfig(EEPROM_PARENT_NODE_ID_ADDRESS, nc.parentNodeId);
							hw_writeConfig(EEPROM_DISTANCE_ADDRESS, nc.distance);
							debug(PSTR("parent=%d, d=%d\n"), nc.parentNodeId, nc.distance);
						}
					}
				}
				return false;
#ifdef MY_SIGNING_FEATURE
			} else if (type == I_GET_NONCE) {
				if (signer.getNonce(msg)) {
					sendRoute(build(msg, nc.nodeId, msg.sender, NODE_SENSOR_ID, C_INTERNAL, I_GET_NONCE_RESPONSE, false));
				}
				return false; // Nonce exchange is an internal MySensor protocol message, no need to inform caller about this
			} else if (type == I_REQUEST_SIGNING) {
				if (msg.getBool()) {
					// We received an indicator that the sender require us to sign all messages we send to it
					SET_SIGN(msg.sender);
				} else {
					// We received an indicator that the sender does not require us to sign all messages we send to it
					CLEAR_SIGN(msg.sender);
				}
				// Save updated table
				hw_writeConfigBlock((void*)doSign, (void*)EEPROM_SIGNING_REQUIREMENT_TABLE_ADDRESS, sizeof(doSign));

				// Inform sender about our preference if we are a gateway, but only require signing if the sender required signing
				// We do not currently want a gateway to require signing from all nodes in a network just because it wants one node
				// to sign it's messages
				if (isGateway) {
					if (signer.requestSignatures() && DO_SIGN(msg.sender))
						sendRoute(build(msg, nc.nodeId, msg.sender, NODE_SENSOR_ID, C_INTERNAL, I_REQUEST_SIGNING, false).set(true));
					else
						sendRoute(build(msg, nc.nodeId, msg.sender, NODE_SENSOR_ID, C_INTERNAL, I_REQUEST_SIGNING, false).set(false));
				}
				return false; // Signing request is an internal MySensor protocol message, no need to inform caller about this
			} else if (type == I_GET_NONCE_RESPONSE) {
				return true; // Just pass along nonce silently (no need to call callback for these)
#endif
			} else if (sender == GATEWAY_ADDRESS) {
				bool isMetric;

				if (type == I_REBOOT) {
					// Requires MySensors or other bootloader with watchdogs enabled
					hw_reboot();
				} else if (type == I_ID_RESPONSE) {
					if (nc.nodeId == AUTO) {
						nc.nodeId = msg.getByte();
						if (nc.nodeId == AUTO) {
							// sensor net gateway will return max id if all sensor id are taken
							debug(PSTR("full\n"));
							while (1); // Wait here. Nothing else we can do...
						}
						setupNode();
						// Write id to EEPROM
						hw_writeConfig(EEPROM_NODE_ID_ADDRESS, nc.nodeId);
						debug(PSTR("id=%d\n"), nc.nodeId);
					}
				} else if (type == I_CONFIG) {
					// Pick up configuration from controller (currently only metric/imperial)
					// and store it in eeprom if changed
					isMetric = msg.getString()[0] == 'M' ;
					cc.isMetric = isMetric;
					hw_writeConfig(EEPROM_CONTROLLER_CONFIG_ADDRESS, isMetric);
				} else if (type == I_CHILDREN) {
					if (repeaterMode && msg.getString()[0] == 'C') {
						// Clears child relay data for this node
						debug(PSTR("clear\n"));
						uint8_t i = 255;
						do {
							hw_writeConfig(EEPROM_ROUTES_ADDRESS+i, 0xff);
						} while (i--);
						// Clear parent node id & distance to gw
						hw_writeConfig(EEPROM_PARENT_NODE_ID_ADDRESS, 0xFF);
						hw_writeConfig(EEPROM_DISTANCE_ADDRESS, 0xFF);
						// Find parent node
						findParentNode();
						sendRoute(build(msg, nc.nodeId, GATEWAY_ADDRESS, NODE_SENSOR_ID, C_INTERNAL, I_CHILDREN,false).set(""));
					}
				} else if (type == I_TIME) {
					if (timeCallback != NULL) {
						// Deliver time to callback
						timeCallback(msg.getULong());
					}
				}
				return false;
			}
		}
#ifdef MY_OTA_FIRMWARE_FEATURE
		else if (command == C_STREAM) {
			if (type == ST_FIRMWARE_CONFIG_RESPONSE) {
				NodeFirmwareConfig *firmwareConfigResponse = (NodeFirmwareConfig *)msg.data;
				// compare with current node configuration, if they differ, start fw fetch process
				if (memcmp(&fc,firmwareConfigResponse,sizeof(NodeFirmwareConfig))) {
					debug(PSTR("fw update\n"));
					// copy new FW config
					memcpy(&fc,firmwareConfigResponse,sizeof(NodeFirmwareConfig));
					// Init flash
					if (!flash.initialize()) {
						debug(PSTR("flash init fail\n"));
						fwUpdateOngoing = false;
					} else {
						// erase lower 32K -> max flash size for ATMEGA328
						flash.blockErase32K(0);
						// wait until flash erased
						while ( flash.busy() );
						fwBlock = fc.blocks;
						fwUpdateOngoing = true;
						// reset flags
						fwRetry = MY_OTA_RETRY+1;
						fwLastRequestTime = 0;
					}
					return false;
				} else debug(PSTR("fw update skipped\n"));
			} else if (type == ST_FIRMWARE_RESPONSE) {
				if (fwUpdateOngoing) {
					// Save block to flash
					debug(PSTR("fw block %d\n"), fwBlock);
					// extract FW block
					ReplyFWBlock *firmwareResponse = (ReplyFWBlock *)msg.data;
					// write to flash
					flash.writeBytes( ((fwBlock - 1) * FIRMWARE_BLOCK_SIZE) + FIRMWARE_START_OFFSET, firmwareResponse->data, FIRMWARE_BLOCK_SIZE);
					// wait until flash written
					while ( flash.busy() );
					fwBlock--;
					if (!fwBlock) {
						// We're finished! Do a checksum and reboot.
						fwUpdateOngoing = false;
						if (isValidFirmware()) {
							debug(PSTR("fw checksum ok\n"));
							// All seems ok, write size and signature to flash (DualOptiboot will pick this up and flash it)	
							uint16_t fwsize = FIRMWARE_BLOCK_SIZE * fc.blocks;
							uint8_t OTAbuffer[10] = {'F','L','X','I','M','G',':',(fwsize >> 8),fwsize,':'};
							flash.writeBytes(0, OTAbuffer, 10);
							// Write the new firmware config to eeprom
							hw_writeConfigBlock((void*)&fc, (void*)EEPROM_FIRMWARE_TYPE_ADDRESS, sizeof(NodeFirmwareConfig));
							hw_reboot();
						} else {
							debug(PSTR("fw checksum fail\n"));
						}
					}		
					// reset flags
					fwRetry = MY_OTA_RETRY+1;
					fwLastRequestTime = 0;
				} else {
					debug(PSTR("No fw update ongoing\n"));
				}
				return false;
			}

		}
#endif
		// Call incoming message callback if available
		if (msgCallback != NULL) {
			msgCallback(msg);
		}
		// Return true if message was addressed for this node...
		return true;
	} else if (repeaterMode && nc.nodeId != AUTO) {
		// If this node have an id, relay the message

		if (command == C_INTERNAL && type == I_FIND_PARENT) {
			if (sender != nc.parentNodeId) {
				if (nc.distance == DISTANCE_INVALID)
					findParentNode();

				if (nc.distance != DISTANCE_INVALID) {
					// Relaying nodes should always answer ping messages
					// Wait a random delay of 0-2 seconds to minimize collision
					// between ping ack messages from other relaying nodes
					wait(hw_millis() & 0x3ff);
					sendWrite(sender, build(msg, nc.nodeId, sender, NODE_SENSOR_ID, C_INTERNAL, I_FIND_PARENT_RESPONSE, false).set(nc.distance));
				}
			}
		} else if (to == nc.nodeId) {
			// We should try to relay this message to another node
			sendRoute(msg);
		}
	}
	return false;
}

MyMessage& MySensor::getLastMessage() {
	return msg;
}

void MySensor::saveState(uint8_t pos, uint8_t value) {
	hw_writeConfig(EEPROM_LOCAL_CONFIG_ADDRESS+pos, value);
}
uint8_t MySensor::loadState(uint8_t pos) {
	return hw_readConfig(EEPROM_LOCAL_CONFIG_ADDRESS+pos);
}

void MySensor::wait(unsigned long ms) {
	unsigned long enter = hw_millis();
	while (hw_millis() - enter < ms) {
		process();
	}
}

void MySensor::sleep(unsigned long ms) {
#ifdef MY_OTA_FIRMWARE_FEATURE
	if (fwUpdateOngoing) {
		// Do not sleep node while fw update is ongoing
		process();
	} else {
#endif
		radio.powerDown();
		hw.sleep(ms);
#ifdef MY_OTA_FIRMWARE_FEATURE
	}
#endif
}

bool MySensor::sleep(uint8_t interrupt, uint8_t mode, unsigned long ms) {
#ifdef MY_OTA_FIRMWARE_FEATURE
	if (fwUpdateOngoing) {
		// Do not sleep node while fw update is ongoing
		process();
		return false;
	} else {
#endif
		radio.powerDown();
		return hw.sleep(interrupt, mode, ms) ;
#ifdef MY_OTA_FIRMWARE_FEATURE
	}
#endif
}

int8_t MySensor::sleep(uint8_t interrupt1, uint8_t mode1, uint8_t interrupt2, uint8_t mode2, unsigned long ms) {
#ifdef MY_OTA_FIRMWARE_FEATURE
	if (fwUpdateOngoing) {
		// Do not sleep node while fw update is ongoing
		process();
		return -1;
	} else {
#endif
		radio.powerDown();
		return hw.sleep(interrupt1, mode1, interrupt2, mode2, ms) ;
#ifdef MY_OTA_FIRMWARE_FEATURE
	}
#endif
}

