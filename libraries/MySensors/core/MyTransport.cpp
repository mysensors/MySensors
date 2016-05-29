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

#include "MyTransport.h"

static transportStatus _transportStatus;

void transportInitialize() {
	// initialize status variables
	_transportStatus.failedUplinkTransmissions = 0;
	_transportStatus.heartbeat = 0;
	_transportStatus.nodeRegistered = false;
	_transportStatus.pingActive = false;
	_transportStatus.transportState = tsTRANSPORT_INIT;
}

bool transportSendWrite(uint8_t to, MyMessage &message) {
	// set protocol version and update last
	mSetVersion(message, PROTOCOL_VERSION);
	message.last = _nc.nodeId;
	// sign message if required
	if (!signerSignMsg(message)) {
		debug(PSTR("sign fail\n"));
		ledBlinkErr(1);
	}
	// msg length changes if signed
	uint8_t length = mGetSigned(message) ? MAX_MESSAGE_LENGTH : mGetLength(message);
	// increase heartbeat counter
	_transportStatus.heartbeat++;
	// send
	bool ok = transportSend(to, &message, min(MAX_MESSAGE_LENGTH, HEADER_SIZE + length));
	ledBlinkTx(1);
	
	debug(PSTR("send: %d-%d-%d-%d s=%d,c=%d,t=%d,pt=%d,l=%d,sg=%d,st=%s:%s\n"),
			message.sender,message.last, to, message.destination, message.sensor, mGetCommand(message), message.type,
			mGetPayloadType(message), mGetLength(message), mGetSigned(message), to==BROADCAST_ADDRESS ? "bc" : (ok ? "ok":"fail"), message.getString(_convBuf));
	
	return (ok || to==BROADCAST_ADDRESS);
}
bool transportRouteMessage(MyMessage &message) {
	uint8_t destination = message.destination;
	uint8_t route;
	// GW and BC are fix
	if(destination==GATEWAY_ADDRESS) {
		route = _nc.parentNodeId;
	} else if (destination==BROADCAST_ADDRESS) {
		route = BROADCAST_ADDRESS;
	} else {
		route = hwReadConfig(EEPROM_ROUTES_ADDRESS+destination);
		if(route==AUTO) {
			debug(PSTR("Destination %d unknown, send to parent\n"), destination);
			route = _nc.parentNodeId;
		}
	}
	bool ok = transportSendWrite(route, message);
	
	if (!ok) {
		// Failure when sending to parent node. The parent node might be down and we
		// need to find another route to gateway.
		ledBlinkErr(1);
	
		if(route==_nc.parentNodeId){
			_transportStatus.failedUplinkTransmissions++;
		}
	} else {
		if(route==_nc.parentNodeId){
			_transportStatus.failedUplinkTransmissions = 0;
		} 
	}
		
	return ok;
}

bool transportSendRoute(MyMessage &message) {
	if ( (isTransportOK() && _transportStatus.nodeRegistered) || mGetCommand(message) == C_INTERNAL) {
		return transportRouteMessage(message);
	} else {
		debug(PSTR("node not ready to send\n"));
		return false;
	}
}


bool transportRequestNodeId() {
	debug(PSTR("req id\n"));
	build(_msgTmp, _nc.nodeId, GATEWAY_ADDRESS, NODE_SENSOR_ID, C_INTERNAL, I_ID_REQUEST, false).set("");
	transportSendWrite(_nc.parentNodeId, _msgTmp);
	transportWait(2000, C_INTERNAL, I_ID_RESPONSE);
	return _nc.nodeId!=AUTO;
}

bool transportFindParentNode() {
	bool ok = false;
	if (!_transportStatus.findingParentNode) {
		_transportStatus.findingParentNode = true;
		_transportStatus.preferredParentFound = false;
		// Set distance to max
		_nc.distance = DISTANCE_INVALID;
		_nc.parentNodeId = AUTO;

		// Send ping message to BROADCAST_ADDRESS (to which all relaying nodes and gateway listens and should reply to)
		debug(PSTR("find parent\n"));

		build(_msgTmp, _nc.nodeId, BROADCAST_ADDRESS, NODE_SENSOR_ID, C_INTERNAL, I_FIND_PARENT, false).set("");
		transportSendWrite(BROADCAST_ADDRESS,_msgTmp);

		// Wait for responses
		transportWait(2000);
		
		ok = _nc.parentNodeId != AUTO;
	
		_transportStatus.findingParentNode = false;
	}
	return ok;
}

bool transportPresentNode() {
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
			transportRouteMessage(build(_msgTmp, _nc.nodeId, GATEWAY_ADDRESS, NODE_SENSOR_ID, C_INTERNAL, I_CONFIG, false).set(_nc.parentNodeId));

			// Wait configuration reply
			transportWait(2000, C_INTERNAL, I_CONFIG);
		}
	#endif
	return true;
}

uint8_t transportPingNode(uint8_t nodeId) {
	uint8_t hopsCnt = INVALID_HOPS;
	if(!_transportStatus.pingActive){	
		debug(PSTR("pinging node %d\n"),nodeId);
		_transportStatus.pingActive = true;
		_transportStatus.pongReceived = false;
		transportRouteMessage(build(_msgTmp, _nc.nodeId, nodeId, NODE_SENSOR_ID, C_INTERNAL, I_PING, false).set((uint8_t)0x01));
		// Wait ping reply
		transportWait(2000, C_INTERNAL, I_PONG);
		_transportStatus.pingActive = false;
		// hops counter in payload
		if(_transportStatus.pongReceived){
			hopsCnt = _msg.getByte();
		}
	} 
	return hopsCnt;

}

bool transportRegisterNode() {
	#if defined (MY_REGISTER_NODE)
		debug(PSTR("register node\n"));
		_transportStatus.nodeRegistered = false;
		transportRouteMessage(build(_msgTmp, _nc.nodeId, GATEWAY_ADDRESS, NODE_SENSOR_ID, C_INTERNAL, I_REGISTER_REQUEST, false).set(MY_TRANSPORT_VERSION));
		transportWait(2000, C_INTERNAL, I_REGISTER_RESPONSE);
	#else
		// skip registration request and set flag
		_transportStatus.nodeRegistered = true;
	#endif
	return _transportStatus.nodeRegistered;
}

uint32_t transportGetHeartbeat() {
	return _transportStatus.heartbeat;
}

void transportWait(unsigned long ms) {
	unsigned long enter = hwMillis();
	while (hwMillis() - enter < ms) {
		hwWatchdogReset();
		transportProcess();
		yield();
	}
}
void transportWait(unsigned long ms, uint8_t cmd, uint8_t msgtype){
	unsigned long enter = hwMillis();
	_msg.type = !msgtype;
	while ( (hwMillis() - enter < ms) && !(mGetCommand(_msg) == cmd && _msg.type == msgtype) ) {
		hwWatchdogReset();
		transportProcess();
		yield();
	}
}

void transportStateMachine(){
	if (_transportStatus.transportState == tsOK) {
		// process incoming messages
		transportProcess();
		#if !defined(MY_GATEWAY_FEATURE)		
			if (_transportStatus.failedUplinkTransmissions > TRANSMISSION_FAILURES) {
				#if !defined(MY_PARENT_NODE_IS_STATIC)
					debug(PSTR("uplink msgs fail, search new parent\n"));
					_transportStatus.transportState = tsPARENT; 
				#else
					debug(PSTR("uplink msgs fail, but parent assignment is static\n"));
					_transportStatus.failedUplinkTransmissions = 0;
				#endif
			}
		#endif
		return;
	}
	if (_transportStatus.transportState == tsFAILURE) {
		if(millis()-_transportStatus.heartbeat>TIMEOUT_FAILURE_STATE) {
			transportInitialize();
		}
		return;
	}
	#if !defined(MY_GATEWAY_FEATURE)
	
		// find parent
		if(_transportStatus.transportState == tsPARENT) {
			if(transportFindParentNode()){
				_transportStatus.transportState = tsID;
			} else {
				_transportStatus.heartbeat = millis();
				_transportStatus.transportState = tsFAILURE;	
				return;
			}
		}
		// request id if necessary
		if(_transportStatus.transportState == tsID) {
			if(_nc.nodeId == AUTO){
				debug(PSTR("req ID\n"));
				transportRequestNodeId();		
			}
			if(_nc.nodeId != AUTO) {
				debug(PSTR("ID ok\n"));
				_transportStatus.transportState=tsLINK;
			} else {
				debug(PSTR("req ID fail\n"));
				_transportStatus.transportState = tsPARENT;
				return;
			}

		}
		
		// test link to GW
		if(_transportStatus.transportState == tsLINK) {
			debug(PSTR("check GW link\n"));
			uint8_t hopsCnt = transportPingNode(GATEWAY_ADDRESS);
			if( _msg.sender == GATEWAY_ADDRESS && hopsCnt == _nc.distance) {				
				_transportStatus.transportState=tsREGISTER;	
			} else {
				debug(PSTR("I_PONG: hops from GW %d, but distance %d\n"), hopsCnt, _nc.distance);
				_transportStatus.transportState = tsPARENT;
				return;
			}
		}

		// register node
		if (_transportStatus.transportState == tsREGISTER) {
			debug(PSTR("reg req\n"));
			if(transportRegisterNode()) {
				debug(PSTR("reg ok\n"));
				_transportStatus.transportState=tsOK;	
			} else {
				debug(PSTR("reg fail\n"));
				_transportStatus.transportState = tsLINK;
			}
			return; // return in any case
		}
					
	#endif
	// GW does not require all of the above
	if(_transportStatus.transportState == tsTRANSPORT_INIT) {
		if(transportInit()) {
			debug(PSTR("Radio init successful\n"));
			if(_nc.nodeId!=AUTO){
				transportSetAddress(_nc.nodeId);
			}
			#if !defined(MY_GATEWAY_FEATURE)
				_transportStatus.transportState = tsPARENT;
			#else
				_transportStatus.transportState = tsOK;
			#endif
		} else {
			debug(PSTR("Radio init failed. Check wiring.\n"));
			ledBlinkErr(1);
			_infiniteLoop();
		}		
	}
}

inline bool isTransportOK() {
	return (_transportStatus.transportState==tsOK);
}

inline void transportAssignNodeID(){
	_nc.nodeId = _msg.getByte();
	// verify if ID valid
	if (_nc.nodeId>GATEWAY_ADDRESS && _nc.nodeId<AUTO){
		transportSetAddress(_nc.nodeId);
		transportPresentNode();
		if (presentation)
			presentation();
		// Write ID to EEPROM
		hwWriteConfig(EEPROM_NODE_ID_ADDRESS, _nc.nodeId);
		debug(PSTR("id=%d\n"), _nc.nodeId);
	} else {
		debug(PSTR("ID error\n"));
		ledBlinkErr(1);
		// Nothing else we can do...
		_infiniteLoop();		
	}	
}

void transportClearRoutingTable() {
	debug(PSTR("clear routing table\n"));
	uint8_t i = 255;
	do {
		hwWriteConfig(EEPROM_ROUTES_ADDRESS+i, BROADCAST_ADDRESS);
	} while (i--);
}

void transportProcess() {
	uint8_t to = 0;
	while(transportAvailable(&to)){
		// Manage signing timeout
		(void)signerCheckTimer(); 
		// retrieve msg from FIFO
		uint8_t payloadLength = transportReceive((uint8_t *)&_msg);
		(void)payloadLength; //until somebody makes use of it
		ledBlinkRx(1);
		
		// Reject messages with incompatible protocol version
		if(!(mGetVersion(_msg) == PROTOCOL_VERSION)) {
			debug(PSTR("ver mismatch\n"));
			ledBlinkErr(1);
			break;
		}		
		// message processing variables
		uint8_t command = mGetCommand(_msg);
		uint8_t type = _msg.type;
		uint8_t sender = _msg.sender;
		uint8_t last = _msg.last;
		uint8_t destination = _msg.destination;
		
		if (destination == _nc.nodeId) {
			// message addressed to us
			debug(PSTR("read: %d-%d-%d s=%d,c=%d,t=%d,pt=%d,l=%d,sg=%d:%s\n"),
				sender, last, destination, _msg.sensor, mGetCommand(_msg), type, mGetPayloadType(_msg), mGetLength(_msg), mGetSigned(_msg), _msg.getString(_convBuf));
			#if defined(MY_REPEATER_FEATURE) || defined(MY_GATEWAY_FEATURE)
				if (_msg.last != _nc.parentNodeId) {
					// Message is from one of the child nodes. Add it to routing table.
					hwWriteConfig(EEPROM_ROUTES_ADDRESS+sender, _msg.last);
				}
			#endif
			// Reject messages that do not pass verification (additional id test inside signerVerifyMsg - remove)
			if (!signerVerifyMsg(_msg)) {
				debug(PSTR("verify fail\n"));
				ledBlinkErr(1);
				break;	
			}
			// prevent buffer overflow by limiting max. possible message length (5 bits=31 bytes max) to MAX_PAYLOAD (25 bytes)
			mSetLength(_msg, min(mGetLength(_msg),MAX_PAYLOAD));
			// null terminate data
			_msg.data[mGetLength(_msg)] = 0x00;
			// ACK handling
			if (mGetRequestAck(_msg)) {
				// copy message
				_msgTmp = _msg;
				mSetRequestAck(_msgTmp,false); // Reply without ack flag (otherwise we would end up in an eternal loop)
				mSetAck(_msgTmp,true);
				_msgTmp.sender = _nc.nodeId;
				_msgTmp.destination = sender;
				transportRouteMessage(_msgTmp);
			}
			if (command == C_INTERNAL) {
				// Process signing related internal messages
				if (signerProcessInternal(_msg)) {
					break; // Signer processing indicated no further action needed
				}
				// network-specific commands, move to new type later
				#if defined(MY_GATEWAY_FEATURE)
					if (type == I_REGISTER_REQUEST) {
						debug(PSTR("Node=%d request registering\n"),sender);
						#if defined(MY_ENABLE_COMPATIBILITY_CHECK)
							if(_msg.getByte()>=MY_TRANSPORT_MIN_VERSION) 
							{
								transportRouteMessage(build(_msgTmp, _nc.nodeId, sender, NODE_SENSOR_ID, C_INTERNAL, I_REGISTER_RESPONSE, false).set(true));
							} else {
								debug(PSTR("Node has incompatible library version\n"),sender);
								transportRouteMessage(build(_msgTmp, _nc.nodeId, sender, NODE_SENSOR_ID, C_INTERNAL, I_REGISTER_RESPONSE, false).set(false));
							}
						#else
							transportRouteMessage(build(_msgTmp, _nc.nodeId, sender, NODE_SENSOR_ID, C_INTERNAL, I_REGISTER_RESPONSE, false).set(true));
						#endif
						break;	
					}
				#else
					if (type == I_ID_RESPONSE) {
						transportAssignNodeID();
						break;
					}	
					if (type == I_REGISTER_RESPONSE) {
						// node receives confirmation
						if (_msg.getBool()){
							debug(PSTR("Node registered\n"));
							_transportStatus.nodeRegistered = true;
						} else {
							debug(PSTR("Node not registered\n"));
							_transportStatus.nodeRegistered = false;
						}
						break;
					}
					#if !defined(MY_GATEWAY_FEATURE)
					if (type == I_FIND_PARENT_RESPONSE) {
						// We've received a reply to a I_FIND_PARENT message. Check if the distance is
						// shorter than we already have.
						uint8_t distance = _msg.getByte();
						debug(PSTR("node %d replied, d=%d\n"), sender, distance);
						if (isValidDistance(distance)) {
							// Distance to gateway is one more for us w.r.t. parent
							distance++;
							if (((isValidDistance(distance) && distance < _nc.distance) || (!_autoFindParent && sender == MY_PARENT_NODE_ID)) && !_transportStatus.preferredParentFound) {
								// Found a neighbor closer to GW than previously found
								if(!_autoFindParent && sender == MY_PARENT_NODE_ID) {
									_transportStatus.preferredParentFound = true;
									debug(PSTR("preferred parent found\n"));
								} 
								_nc.distance = distance;
								_nc.parentNodeId = sender;
								debug(PSTR("parent=%d, d=%d\n"), _nc.parentNodeId, _nc.distance);
							}
						}
						break;
					}
					#endif
				#endif
				// general
				if (type == I_PING) {
					debug(PSTR("node pinged by %d, hops=%d\n"), sender, _msg.getByte());
					transportWait(hwMillis() & 0x3ff);
					transportRouteMessage(build(_msgTmp, _nc.nodeId, sender, NODE_SENSOR_ID, C_INTERNAL, I_PONG, false).set((uint8_t)0x01));
					break; // no further processing required
				} 
				if (type == I_PONG) {
					debug(PSTR("I_PONG, hops=%d\n"), _msg.getByte());
					_transportStatus.pongReceived = true;
					break; // no further processing required
				} 
				if (sender == GATEWAY_ADDRESS) {
					_processInternalMessages();
					break; // no further processing required
				}
			} else if (command == C_STREAM) {
				#if defined(MY_OTA_FIRMWARE_FEATURE)	
					// Process OTA FW update related messages
					if(firmwareOTAUpdateProcess()){
						break; // OTA FW update processing indicated no further action needed
					}
				#endif
	
			}	
			#if defined(MY_GATEWAY_FEATURE)
				// Hand over message to controller
				gatewayTransportSend(_msg);
			#endif
			// call incoming message callback if available
			if (receive) {
				receive(_msg);
			}
		} else if (destination == BROADCAST_ADDRESS) {
			// broadcast
			debug(PSTR("read: %d-%d-%d s=%d,c=%d,t=%d,pt=%d,l=%d,sg=%d:%s\n"),
				sender, last, destination, _msg.sensor, mGetCommand(_msg), type, mGetPayloadType(_msg), mGetLength(_msg), mGetSigned(_msg), _msg.getString(_convBuf));
						
			if (command == C_INTERNAL) {
				#if defined(MY_REPEATER_FEATURE)
					if (type == I_FIND_PARENT) {
						if(_nc.nodeId != AUTO && sender != _nc.parentNodeId) {
							debug(PSTR("find parent from %d\n"), sender);
							// nodes in our range, update routing table - important if node has new repeater as parent
							// routing corrected via re-registering after findparent
							hwWriteConfig(EEPROM_ROUTES_ADDRESS+sender, sender);
							
							// check if parent responding
							#if !defined(MY_GATEWAY_FEATURE)
								if(transportPingNode(GATEWAY_ADDRESS)!=INVALID_HOPS) {
								// random wait to minimize collisions
								transportWait(hwMillis() & 0x3ff);
								transportRouteMessage(build(_msgTmp, _nc.nodeId, sender, NODE_SENSOR_ID, C_INTERNAL, I_FIND_PARENT_RESPONSE, false).set(_nc.distance));
								} else {
									// parent not responding, search new parent
									_transportStatus.transportState = tsPARENT;
								}
							#else
								transportWait(hwMillis() & 0x3ff);
								transportRouteMessage(build(_msgTmp, _nc.nodeId, sender, NODE_SENSOR_ID, C_INTERNAL, I_FIND_PARENT_RESPONSE, false).set(_nc.distance));
								
							#endif							
						}
						break; // no further processing required				
					}
				#endif			
				if (last == _nc.parentNodeId) {	
					if (type==I_DISCOVER){
						debug(PSTR("I_DISCOVER\n"));
						// random wait to minimize collisions
						transportWait(hwMillis() & 0x3ff);
						transportRouteMessage(build(_msgTmp, _nc.nodeId, sender, NODE_SENSOR_ID, C_INTERNAL, I_DISCOVER_RESPONSE, false).set(_nc.parentNodeId));
					}
				}
			}
			#if defined(MY_REPEATER_FEATURE)
				// controlled BC repeating: forward only if message received from parent and sender not self to prevent circular fwds
				if(last == _nc.parentNodeId && sender != _nc.nodeId){
					debug(PSTR("fwd BC: %d-%d-%d s=%d,c=%d,t=%d,pt=%d,l=%d,sg=%d:%s\n"),
						sender, last, destination, _msg.sensor, mGetCommand(_msg), type, mGetPayloadType(_msg), mGetLength(_msg), mGetSigned(_msg), _msg.getString(_convBuf));
					transportRouteMessage(_msg);
				}
			#endif
			// Call incoming message callback if available
			if (command != C_INTERNAL && receive) {
				receive(_msg);
			}	
		} else if (_transportStatus.transportState == tsOK) {
			// message not addressed to us and not BC
			#if defined(MY_REPEATER_FEATURE) || defined(MY_GATEWAY_FEATURE)
				// if no ID assigned, do not repeat messages
				if (_nc.nodeId == AUTO) {
					break;
				}
				// only update routing table to uplink
				if (last != _nc.parentNodeId) {
					hwWriteConfig(EEPROM_ROUTES_ADDRESS+sender, last);
				} 
				if(command == C_INTERNAL){
					if (type == I_PING || type == I_PONG) {
						uint8_t hopsCnt = _msg.getByte(); 
						if(hopsCnt!=MAX_HOPS) {
							debug(PSTR("incr. ping/pong hops=%d\n"),hopsCnt);
							_msg.set((uint8_t)(hopsCnt + 1));	
						}
					}
				}		
				// We should try to relay this message to another node
				debug(PSTR("fwd msg: %d-%d-%d s=%d,c=%d,t=%d,pt=%d,l=%d,sg=%d:%s\n"),
					sender, last, destination, _msg.sensor, mGetCommand(_msg), type, mGetPayloadType(_msg), mGetLength(_msg), mGetSigned(_msg), _msg.getString(_convBuf));
				transportRouteMessage(_msg);
			#else
				debug(PSTR("drop msg: %d-%d-%d s=%d,c=%d,t=%d,pt=%d,l=%d,sg=%d:%s\n"),
					sender, last, destination, _msg.sensor, mGetCommand(_msg), type, mGetPayloadType(_msg), mGetLength(_msg), mGetSigned(_msg), _msg.getString(_convBuf));
			#endif
		}
	} // while
	
	(void)to; // intermediate, to be removed from radio drivers
	
	#ifdef MY_OTA_FIRMWARE_FEATURE
		firmwareOTAUpdateRequest();
	#endif
}











