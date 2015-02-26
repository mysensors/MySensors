 /*
 The MySensors library adds a new layer on top of the RF24 library.
 It handles radio network routing, relaying and ids.

 Created by Henrik Ekblad <henrik.ekblad@gmail.com>

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.
 */

#include "MySensor.h"

#define DISTANCE_INVALID (0xFF)

// Macros for manipulating signing requirement table
#define DO_SIGN(node) (doSign[node>>4]&(node%16))
#define SET_SIGN(node) (doSign[node>>4]|=(node%16))
#define CLEAR_SIGN(node) (doSign[node>>4]&=~(node%16))

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

MySensor::MySensor(MyRFDriver &_radio, MySigningDriver &_signer)
	:
	radio(_radio),
	signer(_signer)
{
}


void MySensor::begin(void (*_msgCallback)(const MyMessage &), uint8_t _nodeId, boolean _repeaterMode, uint8_t _parentNodeId, bool requestSignatures) {
	Serial.begin(BAUD_RATE);
	repeaterMode = _repeaterMode;
	msgCallback = _msgCallback;
	failedTransmissions = 0;
	requireSigning = requestSignatures;

	// Only gateway should use node id 0
	isGateway = _nodeId == 0;

	if (repeaterMode) {
		setupRepeaterMode();
	}

	// Setup radio
	radio.init();

	// Read settings from eeprom
	eeprom_read_block((void*)&nc, (void*)EEPROM_NODE_ID_ADDRESS, sizeof(NodeConfig));
	// Read latest received controller configuration from EEPROM
	eeprom_read_block((void*)&cc, (void*)EEPROM_CONTROLLER_CONFIG_ADDRESS, sizeof(ControllerConfig));
	// Read out the signing requirements from EEPROM
	eeprom_read_block((void*)doSign, (void*)EEPROM_SIGNING_REQUIREMENT_TABLE_ADDRESS, sizeof(doSign));

	if (isGateway) {
		nc.distance = 0;
	}

	if (cc.isMetric == 0xff) {
		// Eeprom empty, set default to metric
		cc.isMetric = 0x01;
	}

	autoFindParent = _parentNodeId == AUTO;
	if (!autoFindParent) {
		nc.parentNodeId = _parentNodeId;
		// Save static parent id in eeprom (used by bootloader)
		eeprom_update_byte((uint8_t*)EEPROM_PARENT_NODE_ID_ADDRESS, _parentNodeId);
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
		eeprom_update_byte((uint8_t*)EEPROM_NODE_ID_ADDRESS, _nodeId);
	} else if (isValidParent(nc.parentNodeId)) {
		// Try to fetch node-id from gateway
		requestNodeId();
	}

	setupNode();

	debug(PSTR("%s started, id=%d, parent=%d, distance=%d\n"), isGateway?"gateway":(repeaterMode?"repeater":"sensor"), nc.nodeId, nc.parentNodeId, nc.distance);
}

void MySensor::setupRepeaterMode(){
	childNodeTable = new uint8_t[256];
	eeprom_read_block((void*)childNodeTable, (void*)EEPROM_ROUTES_ADDRESS, 256);
}

uint8_t MySensor::getNodeId() {
	return nc.nodeId;
}

ControllerConfig MySensor::getConfig() {
	return cc;
}

void MySensor::requestNodeId() {
	debug(PSTR("req node id\n"));
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
		// Send presentation for this radio node (attach
		present(NODE_SENSOR_ID, repeaterMode? S_ARDUINO_REPEATER_NODE : S_ARDUINO_NODE);

		// Notify gateway (and possibly controller) about the signing preferences of this node
		sendRoute(build(msg, nc.nodeId, GATEWAY_ADDRESS, NODE_SENSOR_ID, C_INTERNAL, I_REQUEST_SIGNING, false).set(requireSigning));

		// Send a configuration exchange request to controller
		// Node sends parent node. Controller answers with latest node configuration
		// which is picked up in process()
		sendRoute(build(msg, nc.nodeId, GATEWAY_ADDRESS, NODE_SENSOR_ID, C_INTERNAL, I_CONFIG, false).set(nc.parentNodeId));

		// Wait configuration reply.
		wait(2000);
	}
}

void MySensor::findParentNode() {
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
}

boolean MySensor::sendRoute(MyMessage &message) {
	uint8_t sender = message.sender;
	uint8_t dest = message.destination;
	uint8_t last = message.last;
	bool ok;

	// If we still don't have any parent id, re-request and skip this message.
	if (nc.parentNodeId == AUTO) {
		findParentNode();
		return false;
	}

	// If we still don't have any node id, re-request and skip this message.
	if (nc.nodeId == AUTO) {
		requestNodeId();
		return false;
	}

	mSetVersion(message, PROTOCOL_VERSION);

	// If destination is known to require signed messages and we are the sender, sign this message unless it is an ACK or a signing handshake message
	if (DO_SIGN(message.destination) && message.sender == nc.nodeId && !mGetAck(message) && mGetLength(msg) &&
		(mGetCommand(message) != C_INTERNAL || (message.type != I_GET_NONCE && message.type != I_GET_NONCE_RESPONSE && message.type != I_REQUEST_SIGNING))) {
		if (!sign(message)) {
			debug(PSTR("Message signing failed\n"));
			return false;
		}
		// After this point, only the 'last' member of the message structure is allowed to be altered if the message has been signed,
		// or signature will become invalid and the message rejected by the receiver
	} else mSetSigned(message, 0); // Message is not supposed to be signed, make sure it is marked unsigned

	if (dest == GATEWAY_ADDRESS || !repeaterMode) {
		// If destination is the gateway or if we aren't a repeater, let
		// our parent take care of the message
		ok = sendWrite(nc.parentNodeId, message);
	} else {
		// Relay the message
		uint8_t route = getChildRoute(dest);
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
			addChildRoute(sender, last);
		}
	}

	if (!ok) {
		// Failure when sending to parent node. The parent node might be down and we
		// need to find another route to gateway.
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

void MySensor::present(uint8_t childSensorId, uint8_t sensorType, bool enableAck) {
	sendRoute(build(msg, nc.nodeId, GATEWAY_ADDRESS, childSensorId, C_PRESENTATION, sensorType, enableAck).set(LIBRARY_VERSION));
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
	uint8_t to = 0;
	if (!radio.available(&to))
		return false;

	(void)signer.checkTimer(); // Manage signing timeout
	
	uint8_t len = radio.receive((uint8_t *)&msg);

	// Before processing message, reject unsigned messages if signing is required and check signature (if it is signed and addressed to us)
	// Note that we do not care at all about any signature found if we do not require signing
	if (requireSigning && msg.destination == nc.nodeId && mGetLength(msg) &&
		(mGetCommand(msg) != C_INTERNAL || (msg.type != I_GET_NONCE_RESPONSE && msg.type != I_GET_NONCE && msg.type != I_REQUEST_SIGNING))) {
		if (!mGetSigned(msg)) return false; // Received an unsigned message but we do require signing. This message gets nowhere!
		else if (!signer.verifyMsg(msg)) {
			debug(PSTR("Message verification failed\n"));
			return false; // This signed message has been tampered with!
		}
	}

	// Add string termination, good if we later would want to print it.
	msg.data[mGetLength(msg)] = '\0';
	debug(PSTR("read: %d-%d-%d s=%d,c=%d,t=%d,pt=%d,l=%d,sg=%d:%s\n"),
				msg.sender, msg.last, msg.destination, msg.sensor, mGetCommand(msg), msg.type, mGetPayloadType(msg), mGetLength(msg), mGetSigned(msg), msg.getString(convBuf));
	mSetSigned(msg,0); // Clear the sign-flag now as verification (and debug printing) is completed

	if(!(mGetVersion(msg) == PROTOCOL_VERSION)) {
		debug(PSTR("version: %d\n"),mGetVersion(msg));
		debug(PSTR("version mismatch\n"));
		return false;
	}

	uint8_t command = mGetCommand(msg);
	uint8_t type = msg.type;
	uint8_t sender = msg.sender;
	uint8_t last = msg.last;
	uint8_t destination = msg.destination;

	if (destination == nc.nodeId) {
		// This message is addressed to this node

		if (repeaterMode && last != nc.parentNodeId) {
			// Message is from one of the child nodes. Add it to routing table.
			addChildRoute(sender, last);
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
							eeprom_update_byte((uint8_t*)EEPROM_PARENT_NODE_ID_ADDRESS, nc.parentNodeId);
							eeprom_update_byte((uint8_t*)EEPROM_DISTANCE_ADDRESS, nc.distance);
							debug(PSTR("new parent=%d, d=%d\n"), nc.parentNodeId, nc.distance);
						}
					}
				}
				return false;
			} else if (type == I_GET_NONCE) {
				if (signer.getNonce(msg)) {
					sendRoute(build(msg, nc.nodeId, GATEWAY_ADDRESS, NODE_SENSOR_ID, C_INTERNAL, I_GET_NONCE_RESPONSE, false));
				} else {
					return false;
				}
			} else if (type == I_REQUEST_SIGNING) {
				if (msg.getBool()) {
					// We received an indicator that the sender require us to sign all messages we send to it
					SET_SIGN(msg.sender);
				} else {
					// We received an indicator that the sender does not require us to sign all messages we send to it
					CLEAR_SIGN(msg.sender);
				}
				// Save updated table
				eeprom_update_block((void*)EEPROM_SIGNING_REQUIREMENT_TABLE_ADDRESS, (void*)doSign, sizeof(doSign));

				// Inform sender about our preference if we are a gateway, but only require signing if the sender required signing
				// We do not currently want a gateway to require signing from all nodes in a network just because it wants one node
				// to sign it's messages
				if (isGateway) {
					if (requireSigning && DO_SIGN(msg.sender))
						sendRoute(build(msg, nc.nodeId, msg.sender, NODE_SENSOR_ID, C_INTERNAL, I_REQUEST_SIGNING, false).set(true));
					else
						sendRoute(build(msg, nc.nodeId, msg.sender, NODE_SENSOR_ID, C_INTERNAL, I_REQUEST_SIGNING, false).set(false));
				}
			} else if (sender == GATEWAY_ADDRESS) {
				bool isMetric;

				if (type == I_REBOOT) {
					// Requires MySensors or other bootloader with watchdogs enabled
					wdt_enable(WDTO_15MS);
					for (;;);
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
						eeprom_update_byte((uint8_t*)EEPROM_NODE_ID_ADDRESS, nc.nodeId);
						debug(PSTR("id=%d\n"), nc.nodeId);
					}
				} else if (type == I_CONFIG) {
					// Pick up configuration from controller (currently only metric/imperial)
					// and store it in eeprom if changed
					isMetric = msg.getString()[0] == 'M' ;
					cc.isMetric = isMetric;
					eeprom_update_byte((uint8_t*)EEPROM_CONTROLLER_CONFIG_ADDRESS, isMetric);
				} else if (type == I_CHILDREN) {
					if (repeaterMode && msg.getString()[0] == 'C') {
						// Clears child relay data for this node
						debug(PSTR("rd=clear\n"));
						uint8_t i = 255;
						do {
							removeChildRoute(i);
						} while (i--);
						// Clear parent node id & distance to gw
						eeprom_update_byte((uint8_t*)EEPROM_PARENT_NODE_ID_ADDRESS, 0xFF);
						eeprom_update_byte((uint8_t*)EEPROM_DISTANCE_ADDRESS, 0xFF);
						// Find parent node
						findParentNode();
						sendRoute(build(msg, nc.nodeId, GATEWAY_ADDRESS, NODE_SENSOR_ID, C_INTERNAL, I_CHILDREN,false).set(""));
					}
				}
				return false;
			}
		}
		// Call incoming message callback if available
		if (msgCallback != NULL) {
			msgCallback(msg);
		}
		// Return true if message was addressed for this node...
		return true;
	} else if (repeaterMode && nc.nodeId != AUTO) {
		// If this node have an id, relay the message

		if (command == C_INTERNAL && type == I_FIND_PARENT) {
			if (nc.distance == DISTANCE_INVALID) {
				findParentNode();
			} else if (sender != nc.parentNodeId) {
				// Relaying nodes should always answer ping messages
				// Wait a random delay of 0-2 seconds to minimize collision
				// between ping ack messages from other relaying nodes
				delay(millis() & 0x3ff);
				sendWrite(sender, build(msg, nc.nodeId, sender, NODE_SENSOR_ID, C_INTERNAL, I_FIND_PARENT_RESPONSE, false).set(nc.distance));
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
	eeprom_update_byte((uint8_t*)(EEPROM_LOCAL_CONFIG_ADDRESS+pos), value);
}
uint8_t MySensor::loadState(uint8_t pos) {
	return eeprom_read_byte((uint8_t*)(EEPROM_LOCAL_CONFIG_ADDRESS+pos));
}

void MySensor::addChildRoute(uint8_t childId, uint8_t route) {
	childNodeTable[childId] = route;
	eeprom_update_byte((uint8_t*)EEPROM_ROUTES_ADDRESS+childId, route);
}

void MySensor::removeChildRoute(uint8_t childId) {
	childNodeTable[childId] = 0xff;
	eeprom_update_byte((uint8_t*)EEPROM_ROUTES_ADDRESS+childId, 0xff);
}

uint8_t MySensor::getChildRoute(uint8_t childId) {
	return childNodeTable[childId];
}

int8_t pinIntTrigger = 0;
void wakeUp()	 //place to send the interrupts
{
	pinIntTrigger = 1;
}
void wakeUp2()	 //place to send the second interrupts
{
	pinIntTrigger = 2;
}

void MySensor::internalSleep(unsigned long ms) {
	while (!pinIntTrigger && ms >= 8000) { LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF); ms -= 8000; }
	if (!pinIntTrigger && ms >= 4000)    { LowPower.powerDown(SLEEP_4S, ADC_OFF, BOD_OFF); ms -= 4000; }
	if (!pinIntTrigger && ms >= 2000)    { LowPower.powerDown(SLEEP_2S, ADC_OFF, BOD_OFF); ms -= 2000; }
	if (!pinIntTrigger && ms >= 1000)    { LowPower.powerDown(SLEEP_1S, ADC_OFF, BOD_OFF); ms -= 1000; }
	if (!pinIntTrigger && ms >= 500)     { LowPower.powerDown(SLEEP_500MS, ADC_OFF, BOD_OFF); ms -= 500; }
	if (!pinIntTrigger && ms >= 250)     { LowPower.powerDown(SLEEP_250MS, ADC_OFF, BOD_OFF); ms -= 250; }
	if (!pinIntTrigger && ms >= 125)     { LowPower.powerDown(SLEEP_120MS, ADC_OFF, BOD_OFF); ms -= 120; }
	if (!pinIntTrigger && ms >= 64)      { LowPower.powerDown(SLEEP_60MS, ADC_OFF, BOD_OFF); ms -= 60; }
	if (!pinIntTrigger && ms >= 32)      { LowPower.powerDown(SLEEP_30MS, ADC_OFF, BOD_OFF); ms -= 30; }
	if (!pinIntTrigger && ms >= 16)      { LowPower.powerDown(SLEEP_15Ms, ADC_OFF, BOD_OFF); ms -= 15; }
}

void MySensor::sleep(unsigned long ms) {
	// Let serial prints finish (debug, log etc)
	Serial.flush();
	radio.powerDown();
	pinIntTrigger = 0;
	internalSleep(ms);
}

void MySensor::wait(unsigned long ms) {
	// Let serial prints finish (debug, log etc)
	Serial.flush();
	unsigned long enter = millis();
	while (millis() - enter < ms) {
		// reset watchdog
		wdt_reset();
		process();
	}
}

bool MySensor::sleep(uint8_t interrupt, uint8_t mode, unsigned long ms) {
	// Let serial prints finish (debug, log etc)
	bool pinTriggeredWakeup = true;
	Serial.flush();
	radio.powerDown();
	attachInterrupt(interrupt, wakeUp, mode);
	if (ms>0) {
		pinIntTrigger = 0;
		sleep(ms);
		if (0 == pinIntTrigger) {
			pinTriggeredWakeup = false;
		}
	} else {
		Serial.flush();
		LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
	}
	detachInterrupt(interrupt);
	return pinTriggeredWakeup;
}

int8_t MySensor::sleep(uint8_t interrupt1, uint8_t mode1, uint8_t interrupt2, uint8_t mode2, unsigned long ms) {
	int8_t retVal = 1;
	Serial.flush(); // Let serial prints finish (debug, log etc)
	radio.powerDown();
	attachInterrupt(interrupt1, wakeUp, mode1);
	attachInterrupt(interrupt2, wakeUp2, mode2);
	if (ms>0) {
		pinIntTrigger = 0;
		sleep(ms);
		if (0 == pinIntTrigger) {
			retVal = -1;
		}
	} else {
		Serial.flush();
		LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
	}
	detachInterrupt(interrupt1);
	detachInterrupt(interrupt2);

	if (1 == pinIntTrigger) {
		retVal = (int8_t)interrupt1;
	} else if (2 == pinIntTrigger) {
		retVal = (int8_t)interrupt2;
	}
	return retVal;
}

bool MySensor::sign(MyMessage &message) {
	if (!sendRoute(build(tmpMsg, nc.nodeId, message.destination, message.sensor, C_INTERNAL, I_GET_NONCE, false).set(""))) {
		return false;
	} else {
		// We have to wait for the nonce to arrive before we can sign our original message
		// Other messages could come in-between. We trust process() takes care of them
		unsigned long enter = millis();
		msgSign = message; // Copy the message to sign since message buffer might be touched in process()
		while (millis() - enter < 5000) {
			if (process()) {
				if (mGetCommand(getLastMessage()) == C_INTERNAL && getLastMessage().type == I_GET_NONCE_RESPONSE) {
					// Proceed with signing if nonce has been received
					if (signer.putNonce(getLastMessage()) && signer.signMsg(msgSign)) {
						message = msgSign; // Write the signed message back
						return true;
					}
					break;
				}
			}
		}
	}
	return false;
}

#ifdef DEBUG
void MySensor::debugPrint(const char *fmt, ... ) {
	char fmtBuffer[300];
	if (isGateway) {
		// prepend debug message to be handled correctly by gw (C_INTERNAL, I_LOG_MESSAGE)
		snprintf_P(fmtBuffer, 299, PSTR("0;0;%d;0;%d;"), C_INTERNAL, I_LOG_MESSAGE);
		Serial.print(fmtBuffer);
	}
	va_list args;
	va_start (args, fmt );
	va_end (args);
	if (isGateway) {
		// Truncate message if this is gateway node
		vsnprintf_P(fmtBuffer, 60, fmt, args);
		fmtBuffer[59] = '\n';
		fmtBuffer[60] = '\0';
	} else {
		vsnprintf_P(fmtBuffer, 299, fmt, args);
	}
	va_end (args);
	Serial.print(fmtBuffer);
	Serial.flush();

	//Serial.write(freeRam());
}
#endif

#ifdef DEBUG
int MySensor::freeRam (void) {
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}
#endif
