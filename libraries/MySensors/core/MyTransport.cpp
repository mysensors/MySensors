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


#include "MyTransport.h"

bool _autoFindParent;
uint8_t _failedTransmissions;

static inline bool isValidDistance( const uint8_t distance ) {
	return distance != DISTANCE_INVALID;
}


inline void transportProcess() {
	uint8_t to = 0;
	if (!transportAvailable(&to))
	{
		#ifdef MY_OTA_FIRMWARE_FEATURE
			firmwareOTAUpdateRequest();
		#endif
		return;
	}

	(void)signerCheckTimer(); // Manage signing timeout

	uint8_t payloadLength = transportReceive((uint8_t *)&_msg);
	(void)payloadLength; //until somebody makes use of it
	setIndication(INDICATION_RX);

	
	uint8_t command = mGetCommand(_msg);
	uint8_t type = _msg.type;
	uint8_t sender = _msg.sender;
	uint8_t last = _msg.last;
	uint8_t destination = _msg.destination;
	

	// Reject massages that do not pass verification
	if (!signerVerifyMsg(_msg)) {
		setIndication(INDICATION_ERR_SIGN);
		debug(PSTR("verify fail\n"));
		return;	
	}

	if (destination == _nc.nodeId) {
		debug(PSTR("read: %d-%d-%d s=%d,c=%d,t=%d,pt=%d,l=%d,sg=%d:%s\n"),
					sender, _msg.last, destination, _msg.sensor, mGetCommand(_msg), type, mGetPayloadType(_msg), mGetLength(_msg), mGetSigned(_msg), _msg.getString(_convBuf));
	}
	else
	{
	#if defined(MY_REPEATER_FEATURE)
		debug(PSTR("read and forward: %d-%d-%d s=%d,c=%d,t=%d,pt=%d,l=%d,sg=%d\n"),
					sender, _msg.last, destination, _msg.sensor, mGetCommand(_msg), type, mGetPayloadType(_msg), mGetLength(_msg), mGetSigned(_msg), _msg.getString(_convBuf));
	#else
		debug(PSTR("read and drop: %d-%d-%d s=%d,c=%d,t=%d,pt=%d,l=%d,sg=%d:%s\n"),
					sender, _msg.last, destination, _msg.sensor, mGetCommand(_msg), type, mGetPayloadType(_msg), mGetLength(_msg),  mGetSigned(_msg), _msg.getString(_convBuf));
	#endif
	}

	if(!(mGetVersion(_msg) == PROTOCOL_VERSION)) {
        setIndication(INDICATION_ERR_VERSION);
		debug(PSTR("ver mismatch\n"));
		return;
	}

	if (destination == _nc.nodeId) {
		// This message is addressed to this node
		// prevent buffer overflow by limiting max. possible message length (5 bits=31 bytes max) to MAX_PAYLOAD (25 bytes)
		mSetLength(_msg, min(mGetLength(_msg),MAX_PAYLOAD));
		// null terminate data
		_msg.data[mGetLength(_msg)] = 0x00;
		
		#if defined(MY_REPEATER_FEATURE)
			if (_msg.last != _nc.parentNodeId) {
				// Message is from one of the child nodes. Add it to routing table.
				hwWriteConfig(EEPROM_ROUTES_ADDRESS+sender, _msg.last);
			}
		#endif

		// Check if sender requests an ack back.
		if (mGetRequestAck(_msg)) {
			// Copy message
			_msgTmp = _msg;
			mSetRequestAck(_msgTmp,false); // Reply without ack flag (otherwise we would end up in an eternal loop)
			mSetAck(_msgTmp,true);
			_msgTmp.sender = _nc.nodeId;
			_msgTmp.destination = sender;
			_sendRoute(_msgTmp);
		}

		if (command == C_INTERNAL) {
			// Process signing related internal messages
			if (signerProcessInternal(_msg)) {
				return; // Signer processing indicated no further action needed
			}
			if (type == I_FIND_PARENT_RESPONSE) {
				if (_autoFindParent) {
					// We've received a reply to a FIND_PARENT message. Check if the distance is
					// shorter than we already have.
					uint8_t distance = _msg.getByte();
					if (isValidDistance(distance))
					{
						// Distance to gateway is one more for us w.r.t. parent
						distance++;
						if (isValidDistance(distance) && (distance < _nc.distance)) {
							// Found a neighbor closer to GW than previously found
                            setIndication(INDICATION_GOT_PARENT);
							_nc.distance = distance;
							_nc.parentNodeId = sender;
							hwWriteConfig(EEPROM_PARENT_NODE_ID_ADDRESS, _nc.parentNodeId);
							hwWriteConfig(EEPROM_DISTANCE_ADDRESS, _nc.distance);
							debug(PSTR("parent=%d, d=%d\n"), _nc.parentNodeId, _nc.distance);
						}
					}
				}
				return;
			} else if (sender == GATEWAY_ADDRESS) {
				if (type == I_ID_RESPONSE) {
					_nc.nodeId = _msg.getByte();
					if (_nc.nodeId == AUTO) {
						// sensor net gateway will return max id if all sensor id are taken
                        setIndication(INDICATION_ERR_NET_FULL);
						debug(PSTR("full\n"));
						// Nothing else we can do...
						_infiniteLoop();
						
					}
                    setIndication(INDICATION_GOT_NODEID);
					transportPresentNode();
					if (presentation)
						presentation();
					// Write id to EEPROM
					hwWriteConfig(EEPROM_NODE_ID_ADDRESS, _nc.nodeId);
					debug(PSTR("id=%d\n"), _nc.nodeId);
				} else {
					_processInternalMessages();
				}
				return;
			}
		} else if (command == C_STREAM) {
			#if defined(MY_OTA_FIRMWARE_FEATURE)
				if(firmwareOTAUpdateProcess()){
					return; // OTA FW update processing indicated no further action needed
				}
			#endif
		}
		#if defined(MY_GATEWAY_FEATURE)
			// Hand over message to controller
			gatewayTransportSend(_msg);
		#endif
		// Call incoming message callback if available
		if (receive) {
			receive(_msg);
		}
		return;
	} else if (destination == BROADCAST_ADDRESS) {
		if (command == C_INTERNAL) {
			if (type==I_DISCOVER && last==_nc.parentNodeId) {
				// only process if received from parent
				debug(PSTR("discovery signal\n"));
				// random wait to minimize collisions
				wait(hwMillis() & 0x3ff);
				_sendRoute(build(_msgTmp, _nc.nodeId, sender, NODE_SENSOR_ID, C_INTERNAL, I_DISCOVER_RESPONSE, false).set(_nc.parentNodeId));
				// repeat bc signal
				#if defined(MY_REPEATER_FEATURE)
				// controlled repeating
				debug(PSTR("repeat discovery signal\n"));
				_sendRoute(_msg);
				#endif
				return;
			}
		}
	}
	#if defined(MY_REPEATER_FEATURE)
		if (_nc.nodeId != AUTO) {
			// If this node have an id, relay the message
			if (command == C_INTERNAL && type == I_FIND_PARENT) {
				if (sender != _nc.parentNodeId) {
					if (_nc.distance == DISTANCE_INVALID)
						transportFindParentNode();

					if (_nc.distance != DISTANCE_INVALID) {
						// Relaying nodes should always answer ping messages
						// Wait a random delay of 0-2 seconds to minimize collision
						// between ping ack messages from other relaying nodes
						wait(hwMillis() & 0x3ff);
						transportSendWrite(sender, build(_msg, _nc.nodeId, sender, NODE_SENSOR_ID, C_INTERNAL, I_FIND_PARENT_RESPONSE, false).set(_nc.distance));
					}
				}
			} else if (to == _nc.nodeId) {
				// We should try to relay this message to another node
				_sendRoute(_msg);
			}
		}
	#endif
}

boolean transportSendWrite(uint8_t to, MyMessage &message) {

	mSetVersion(message, PROTOCOL_VERSION);
	uint8_t length = mGetSigned(message) ? MAX_MESSAGE_LENGTH : mGetLength(message);
	message.last = _nc.nodeId;
    setIndication(INDICATION_TX);

	bool ok = transportSend(to, &message, min(MAX_MESSAGE_LENGTH, HEADER_SIZE + length));
    if (!ok) setIndication(INDICATION_ERR_TX);

	debug(PSTR("send: %d-%d-%d-%d s=%d,c=%d,t=%d,pt=%d,l=%d,sg=%d,st=%s:%s\n"),
			message.sender,message.last, to, message.destination, message.sensor, mGetCommand(message), message.type,
			mGetPayloadType(message), mGetLength(message), mGetSigned(message), to==BROADCAST_ADDRESS ? "bc" : (ok ? "ok":"fail"), message.getString(_convBuf));

	return ok;
}


boolean transportSendRoute(MyMessage &message) {
	#if defined(MY_REPEATER_FEATURE)
	uint8_t last = message.last;
	#endif
	bool ok;

	// If we still don't have any parent id, re-request and skip this message.
	if (_nc.parentNodeId == AUTO) {
        setIndication(INDICATION_ERR_FIND_PARENT);
        transportFindParentNode();
		return false;
	}

	// If we still don't have any node id, re-request and skip this message.
	if (_nc.nodeId == AUTO) {
        setIndication(INDICATION_ERR_GET_NODEID);
		transportRequestNodeId();
		return false;
	}

	mSetVersion(message, PROTOCOL_VERSION);

	if (!signerSignMsg(message)) {
       setIndication(INDICATION_ERR_SIGN);
		debug(PSTR("sign fail\n"));
	}

	#if !defined(MY_REPEATER_FEATURE)

		// None repeating node... We can only send to our parent
		ok = transportSendWrite(_nc.parentNodeId, message);
	#else
		uint8_t sender = message.sender;
		uint8_t dest = message.destination;
		if (dest == GATEWAY_ADDRESS) {
			// Store this address in routing table (if repeater)
			hwWriteConfig(EEPROM_ROUTES_ADDRESS+sender, last);
			// If destination is the gateway or if we aren't a repeater, let
			// our parent take care of the message
			ok = transportSendWrite(_nc.parentNodeId, message);
		} else {
			// Relay the message
			uint8_t route;
			// INTERMEDIATE FIX: make sure corrupted routing table is not interfering with BC - observed several cases -tekka
			if (dest!=BROADCAST_ADDRESS) {
				route = hwReadConfig(EEPROM_ROUTES_ADDRESS+dest);
			} else route = BROADCAST_ADDRESS;
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
				return transportSendWrite(route, message);
			} else if (sender == GATEWAY_ADDRESS && dest == BROADCAST_ADDRESS) {
				// Node has not yet received any id. We need to send it
				// by doing a broadcast sending,
				return transportSendWrite(BROADCAST_ADDRESS, message);
			}
			#if defined (MY_GATEWAY_FEATURE)
				// Destination isn't in our routing table and isn't a broadcast address
				// Nothing to do here
				debug(PSTR("Destination %d unknown\n"), dest);
				return false;
			# else
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
				ok = transportSendWrite(_nc.parentNodeId, message);

				// Add this child to our "routing table" if it not already exist
				hwWriteConfig(EEPROM_ROUTES_ADDRESS+sender, last);

			#endif
		}
	#endif

	if (!ok) {
		// Failure when sending to parent node. The parent node might be down and we
		// need to find another route to gateway.
		_failedTransmissions++;
		if (_autoFindParent && _failedTransmissions > SEARCH_FAILURES) {
			transportFindParentNode();
		}
	} else {
		_failedTransmissions = 0;
	}

	return ok;
}


void transportRequestNodeId() {
    setIndication(INDICATION_REQ_NODEID);
	debug(PSTR("req id\n"));
	transportSetAddress(_nc.nodeId);
	build(_msg, _nc.nodeId, GATEWAY_ADDRESS, NODE_SENSOR_ID, C_INTERNAL, I_ID_REQUEST, false).set("");
	transportSendWrite(_nc.parentNodeId, _msg);
	wait(2000, C_INTERNAL, I_ID_RESPONSE);
}

void transportPresentNode() {
    setIndication(INDICATION_PRESENT);
	// Open reading pipe for messages directed to this node (set write pipe to same)
	transportSetAddress(_nc.nodeId);
	// Present node and request config
	#ifdef MY_GATEWAY_FEATURE
		// Send presentation for this gateway device
		#ifdef MY_REPEATER_FEATURE
			present(NODE_SENSOR_ID, S_ARDUINO_REPEATER_NODE);
		#else
			present(NODE_SENSOR_ID, S_ARDUINO_NODE);
		#endif
	#else
		if (_nc.nodeId != AUTO) {
			#ifdef MY_OTA_FIRMWARE_FEATURE
				presentBootloaderInformation();
			#endif
			// Send signing preferences for this node to the GW
			signerPresentation(_msg, GATEWAY_ADDRESS);

			// Send presentation for this radio node
			#ifdef MY_REPEATER_FEATURE
				present(NODE_SENSOR_ID, S_ARDUINO_REPEATER_NODE);
			#else
				present(NODE_SENSOR_ID, S_ARDUINO_NODE);
			#endif
			// Send a configuration exchange request to controller
			// Node sends parent node. Controller answers with latest node configuration
			// which is picked up in process()
			_sendRoute(build(_msg, _nc.nodeId, GATEWAY_ADDRESS, NODE_SENSOR_ID, C_INTERNAL, I_CONFIG, false).set(_nc.parentNodeId));

			// Wait configuration reply.
			wait(2000, C_INTERNAL, I_CONFIG);
		}
	#endif
}

void transportFindParentNode() {
	static boolean findingParentNode = false;

	if (findingParentNode)
		return;
	findingParentNode = true;

    setIndication(INDICATION_FIND_PARENT);

	_failedTransmissions = 0;

	// Set distance to max
	_nc.distance = 255;

	// Send ping message to BROADCAST_ADDRESS (to which all relaying nodes and gateway listens and should reply to)
	debug(PSTR("find parent\n"));

	build(_msg, _nc.nodeId, BROADCAST_ADDRESS, NODE_SENSOR_ID, C_INTERNAL, I_FIND_PARENT, false).set("");
	// Write msg, but suppress recursive parent search
	transportSendWrite(BROADCAST_ADDRESS, _msg);

	// Wait for ping response.
	wait(2000);
	findingParentNode = false;
}
