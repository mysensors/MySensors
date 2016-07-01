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

// SM: transitions and update states
static State stInit = { stInitTransition, NULL };
static State stParent = { stParentTransition, stParentUpdate };
static State stID = { stIDTransition, stIDUpdate };
static State stUplink = { stUplinkTransition, NULL };
static State stOK = { stOKTransition, stOKUpdate };
static State stFailure = { stFailureTransition, stFailureUpdate };

static transportSM _transportSM;

// stInit: initialize transport HW
void stInitTransition() {
	debug(PSTR("TSM:INIT\n"));
	// initialize status variables
	_transportSM.failedUplinkTransmissions = 0;
	_transportSM.pingActive = false;
	_transportSM.transportActive = false;

	// Read node settings (ID, parentId, GW distance) from EEPROM
	hwReadConfigBlock((void*)&_nc, (void*)EEPROM_NODE_ID_ADDRESS, sizeof(NodeConfig));

	// initialize radio
	if (!transportInit()) {
		debug(PSTR("!RADIO:fail\n"));
		setIndication(INDICATION_ERR_INIT_TRANSPORT);
		transportSwitchSM(stFailure);
	}
	else {
		debug(PSTR("RADIO:OK\n"));
		_transportSM.transportActive = true;
		#if defined(MY_GATEWAY_FEATURE)
			// Set configuration for gateway
			debug(PSTR("TSM:GW MODE\n"));
			_nc.parentNodeId = GATEWAY_ADDRESS;
			_nc.distance = 0;
			_nc.nodeId = GATEWAY_ADDRESS;
			transportSetAddress(GATEWAY_ADDRESS);
			transportSwitchSM(stOK);
		#else
			if (MY_NODE_ID != AUTO) {
				// Set static id
				_nc.nodeId = MY_NODE_ID;
				// Save static id in eeprom
				hwWriteConfig(EEPROM_NODE_ID_ADDRESS, MY_NODE_ID);
			}
			// set ID if static or set in EEPROM
			if(_nc.nodeId!=AUTO) transportAssignNodeID(_nc.nodeId);
			transportSwitchSM(stParent);
		#endif	
	}
}

// stParent: find parent
void stParentTransition()  {
	debug(PSTR("TSM:PARENT\n"));
	_transportSM.preferredParentFound = false;
	_transportSM.findingParentNode = true;
	_transportSM.failedUplinkTransmissions = 0;
	_transportSM.uplinkOk = false;
	// Set distance to max and invalidate parent node id
	_nc.distance = DISTANCE_INVALID;
	_nc.parentNodeId = AUTO;
	// Broadcast find parent request
	setIndication(INDICATION_FIND_PARENT);
	transportRouteMessage(build(_msgTmp, _nc.nodeId, BROADCAST_ADDRESS, NODE_SENSOR_ID, C_INTERNAL, I_FIND_PARENT, false).set(""));
}

// stParentUpdate
void stParentUpdate() {
	if (transportTimeInState() > STATE_TIMEOUT || _transportSM.preferredParentFound) {
		// timeout or preferred parent found
		if (_nc.parentNodeId != AUTO) {
			_transportSM.findingParentNode = false;
			setIndication(INDICATION_GOT_PARENT);
			transportSwitchSM(stID);
		}
		else if (transportTimeInState() > STATE_TIMEOUT) {
			if (_transportSM.retries < STATE_RETRIES) {
				// reenter if timeout and retries left
				transportSwitchSM(stParent);
			}
			else {
				setIndication(INDICATION_ERR_FIND_PARENT);
				transportSwitchSM(stFailure);
			}
		}
	}
}

// stID: verify and request ID if necessary
void stIDTransition() {
	debug(PSTR("TSM:ID\n"));
	if (_nc.nodeId == AUTO) {
		// send ID request
		setIndication(INDICATION_REQ_NODEID);
		transportRouteMessage(build(_msgTmp, _nc.nodeId, GATEWAY_ADDRESS, NODE_SENSOR_ID, C_INTERNAL, I_ID_REQUEST, false).set(""));
	}
}

void stIDUpdate() {
	if (_nc.nodeId != AUTO) {
		// current node ID is valid, proceed to uplink check
		debug(PSTR("node ID=%d\n"), _nc.nodeId);
		setIndication(INDICATION_GOT_NODEID);
		// check uplink
		transportSwitchSM(stUplink);	
	}
	else if (transportTimeInState() > STATE_TIMEOUT) {
		if (_transportSM.retries < STATE_RETRIES) {
			// re-enter if retries left
			transportSwitchSM(stID);
		}
		else {
			// fail
			setIndication(INDICATION_ERR_GET_NODEID);
			transportSwitchSM(stFailure);
		}
	}
}

void stUplinkTransition() {
	if (transportPingNode(GATEWAY_ADDRESS) == _nc.distance) {
		transportSwitchSM(stOK);
	}
	else transportSwitchSM(stParent);
}

void stOKTransition() {
	debug(PSTR("TSM:OK\n"));
	_transportSM.uplinkOk = true;
}

// stOK update: monitors uplink failures
void stOKUpdate() {
	#if !defined(MY_GATEWAY_FEATURE)		
		if (_transportSM.failedUplinkTransmissions > TRANSMISSION_FAILURES) {
			// too many uplink transmissions failed, find new parent
		#if !defined(MY_PARENT_NODE_IS_STATIC)
			debug(PSTR("!TSM:UPL fail, SNP\n"));
			transportSwitchSM(stParent);
		#else
			debug(PSTR("!TSM:UPL fail, STATP\n"));
			_transportSM.failedUplinkTransmissions = 0;
		#endif
		}
	#endif	
}

// stFailure: ented upon HW init failure or max retries exceeded
void stFailureTransition() {
	debug(PSTR("!TSM:FAILURE\n"));
	_transportSM.uplinkOk = false;
	_transportSM.transportActive = false;
	// power down transport, no need until re-init
	debug(PSTR("TSM:PDT\n"));
	transportPowerDown();
}

void stFailureUpdate() {
	if (transportTimeInState()> TIMEOUT_FAILURE_STATE) {
		transportSwitchSM(stInit);
	}
}

void transportSwitchSM(State& newState) {
	if (_transportSM.currentState != &newState) {
		// state change, reset retry counter
		_transportSM.retries = 0;
		// change state
		_transportSM.currentState = &newState;
	}
	else {
		_transportSM.retries++;	// increment retries
	}
	// Transition event
	if (_transportSM.currentState->Transition) _transportSM.currentState->Transition();
	// save time
	_transportSM.stateEnter = hwMillis();
}

uint32_t transportTimeInState() {
	return hwMillis() - _transportSM.stateEnter;
}

void transportUpdateSM(){
	if (_transportSM.currentState->Update) _transportSM.currentState->Update();
}

bool isTransportOK() {
	return (_transportSM.uplinkOk);
}


void transportInitialize() {
	// intial state
	_transportSM.currentState = &stFailure;
	transportSwitchSM(stInit);
}

// update TSM and process incoming messages
void transportProcess() {
	// update state machine
	transportUpdateSM();	
	// process transport FIFO
	if (_transportSM.transportActive) transportProcessFIFO();
}


void transportAssignNodeID(uint8_t newNodeId) {
	// verify if ID valid
	if (newNodeId != GATEWAY_ADDRESS && newNodeId != AUTO) {
		_nc.nodeId = newNodeId;
		transportSetAddress(newNodeId);
		// Write ID to EEPROM
		hwWriteConfig(EEPROM_NODE_ID_ADDRESS, newNodeId);
		debug(PSTR("assign node ID=%d\n"),newNodeId);
	}
	else {
		debug(PSTR("node ID invalid (%d)\n"),newNodeId);
		setIndication(INDICATION_ERR_NET_FULL);
		// Nothing else we can do...
		transportSwitchSM(stFailure);
	}
}

bool transportRouteMessage(MyMessage &message) {
	uint8_t destination = message.destination;
	uint8_t route;
	
	if (_transportSM.findingParentNode && destination != BROADCAST_ADDRESS) {
		debug(PSTR("finding parent active, msg not send\n"));
		// request to send a non-BC message while finding parent active, abort
		return false;
	}

	if (destination == GATEWAY_ADDRESS) {
		route = _nc.parentNodeId;		// message to GW always routes via parent
	}
	else if (destination == BROADCAST_ADDRESS) {
		route = BROADCAST_ADDRESS;		// message to BC does not require routing
	}
	else {
		#if defined(MY_REPEATER_FEATURE)
			// destination not GW & not BC, get route
			route = hwReadConfig(EEPROM_ROUTES_ADDRESS + destination);
			if (route == AUTO) {
				// route unknown
				if (message.last != _nc.parentNodeId) {
					// message not from parent, i.e. child node - route it to parent
					debug(PSTR("Destination %d unknown, send to parent %d\n"), destination, _nc.parentNodeId);
					route = _nc.parentNodeId;
				} 
				else {
					// route unknown and msg received from parent, send it to destination assuming in rx radius
					route = destination;
				}
			}
		#else
			route = _nc.parentNodeId;	// not a repeater, all traffic routed via parent
		#endif
	}
	// send message
	bool ok = transportSendWrite(route, message);
	#if !defined(MY_GATEWAY_FEATURE)
		// update counter
		if (route == _nc.parentNodeId) {
			if (!ok) {
				setIndication(INDICATION_ERR_TX);
				_transportSM.failedUplinkTransmissions++;
			}
			else _transportSM.failedUplinkTransmissions = 0;
		}
	#else
		if(!ok) setIndication(INDICATION_ERR_TX);

	#endif

	return ok;
}

bool transportSendRoute(MyMessage &message) {
	if (isTransportOK()) {
		return transportRouteMessage(message);
	}
	else {
		// TNR: transport not ready
		debug(PSTR("!TNR\n"));
		return false;
	}
}

// only be used inside transport
bool transportWait(uint32_t ms, uint8_t cmd, uint8_t msgtype){
	uint32_t enter = hwMillis();
	// invalidate msg type
	_msg.type = !msgtype;
	bool expectedResponse = false;
	while ((hwMillis() - enter < ms) && !expectedResponse) {
		transportProcess();
		#if defined(ARDUINO_ARCH_ESP8266)
			yield();
		#endif
		expectedResponse = (mGetCommand(_msg) == cmd && _msg.type == msgtype);
	}
	return expectedResponse;
}

uint8_t transportPingNode(uint8_t targetId) {
	if(!_transportSM.pingActive){
		if(targetId == _nc.nodeId) {
			// ping to ourself
			return 0;
		}
		_transportSM.pingActive = true;
		_transportSM.pingResponse = INVALID_HOPS;
		debug(PSTR("pinging node %d\n"), targetId);
		transportRouteMessage(build(_msgTmp, _nc.nodeId, targetId, NODE_SENSOR_ID, C_INTERNAL, I_PING, false).set((uint8_t)0x01));
		// Wait for ping reply or timeout
		transportWait(2000, C_INTERNAL, I_PONG);
		return _transportSM.pingResponse;
	}
	else {
		return INVALID_HOPS;
	}
}

void transportClearRoutingTable() {
	debug(PSTR("CRT\n"));	// clear routing table
	for(uint8_t i = 0; i!=255; i++)
		hwWriteConfig(EEPROM_ROUTES_ADDRESS+i, BROADCAST_ADDRESS);
}

uint32_t transportGetHeartbeat() {
	return transportTimeInState();
}

void transportProcessMessage() {
	(void)signerCheckTimer(); // Manage signing timeout

	uint8_t payloadLength = transportReceive((uint8_t *)&_msg);
	(void)payloadLength;	// currently not used, but good to test for CRC-ok but corrupt msgs
	setIndication(INDICATION_RX);

	// verify protocol version
	if(mGetVersion(_msg) != PROTOCOL_VERSION) {
		setIndication(INDICATION_ERR_VERSION);
		debug(PSTR("!PVER mismatch\n"));
		return;
	}
		
	// Reject massages that do not pass verification
	if (!signerVerifyMsg(_msg)) {
		setIndication(INDICATION_ERR_SIGN);
		debug(PSTR("!SIGN verify fail\n"));
		return;	
	}
		
	uint8_t command = mGetCommand(_msg);
	uint8_t type = _msg.type;
	uint8_t sender = _msg.sender;
	uint8_t last = _msg.last;
	uint8_t destination = _msg.destination;

	debug(PSTR("read: %d-%d-%d s=%d,c=%d,t=%d,pt=%d,l=%d,sg=%d:%s\n"),
		sender, last, destination, _msg.sensor, mGetCommand(_msg), type, mGetPayloadType(_msg), mGetLength(_msg), mGetSigned(_msg), _msg.getString(_convBuf));


	if (destination == _nc.nodeId) {
		// This message is addressed to this node
		// prevent buffer overflow by limiting max. possible message length (5 bits=31 bytes max) to MAX_PAYLOAD (25 bytes)
		mSetLength(_msg, min(mGetLength(_msg),MAX_PAYLOAD));
		// null terminate data
		_msg.data[mGetLength(_msg)] = 0x00;
			
		// update routing table if msg not from parent
		#if defined(MY_REPEATER_FEATURE)
			if (last != _nc.parentNodeId) {
				// Message is from one of the child nodes. Add it to routing table.
				hwWriteConfig(EEPROM_ROUTES_ADDRESS+sender, last);
			}
		#endif

		// Check if sender requests an ack back.
		if (mGetRequestAck(_msg)) {
			_msgTmp = _msg;	// Copy message	
			mSetRequestAck(_msgTmp, false); // Reply without ack flag (otherwise we would end up in an eternal loop)
			mSetAck(_msgTmp, true); // set ACK flag
			_msgTmp.sender = _nc.nodeId;
			_msgTmp.destination = sender;
			// send ACK
			debug(PSTR("ACK msg\n"));
			// use transportSendRoute since ACK reply is not internal, i.e. if !transportOK do not reply
			transportSendRoute(_msgTmp);
		} 
		if(!mGetAck(_msg)) {
			// only process if not ACK
			if (command == C_INTERNAL) {
				// Process signing related internal messages
				if (signerProcessInternal(_msg)) {
					return; // Signer processing indicated no further action needed
				}
				#if !defined(MY_GATEWAY_FEATURE)
					if (type == I_ID_RESPONSE) {
					#if (MY_NODE_ID == AUTO)
						// only active if node ID dynamic
						transportAssignNodeID(_msg.getByte());
					#endif
					return; // no further processing required
					}
					if (type == I_FIND_PARENT_RESPONSE) {
						#if !defined(MY_GATEWAY_FEATURE)
							// Reply to a I_FIND_PARENT message. Check if the distance is shorter than we already have.
							uint8_t distance = _msg.getByte();
							debug(PSTR("node %d replied, d=%d, _nc.distance=%d\n"), sender, distance,_nc.distance);
							if (isValidDistance(distance)) {
								// Distance to gateway is one more for us w.r.t. parent
								distance++;
								// update settings if distance shorter or preferred parent found
								if (((isValidDistance(distance) && distance < _nc.distance) || (!_autoFindParent && sender == MY_PARENT_NODE_ID)) && !_transportSM.preferredParentFound) {
									// Found a neighbor closer to GW than previously found
									if (!_autoFindParent && sender == MY_PARENT_NODE_ID) {
										_transportSM.preferredParentFound = true;
										debug(PSTR("preferred parent found\n"));
									}
									_nc.distance = distance;
									_nc.parentNodeId = sender;
									debug(PSTR("parent=%d, d=%d\n"), _nc.parentNodeId, _nc.distance);
								}
							}
							return;
						#endif
					}
				#endif
				// general
				if (type == I_PING) {
					debug(PSTR("node pinged by %d, hops=%d\n"), sender, _msg.getByte());
					transportRouteMessage(build(_msgTmp, _nc.nodeId, sender, NODE_SENSOR_ID, C_INTERNAL, I_PONG, false).set((uint8_t)0x01));
					return; // no further processing required
				}
				if (type == I_PONG) {
					if (_transportSM.pingActive) {
						_transportSM.pingActive = false;
						_transportSM.pingResponse = _msg.getByte();
						debug(PSTR("ping replied, hops=%d\n"), _transportSM.pingResponse);
					}
					return; // no further processing required
				}

				if (_processInternalMessages()) {
					return; // no further processing required
				}
			} else if (command == C_STREAM) {
				#if defined(MY_OTA_FIRMWARE_FEATURE)
					if(firmwareOTAUpdateProcess()){
						return; // OTA FW update processing indicated no further action needed
					}
				#endif
			}
		}
		#if defined(MY_GATEWAY_FEATURE)
			// Hand over message to controller
			gatewayTransportSend(_msg);
		#else
			// Call incoming message callback if available
			if (receive) {
				receive(_msg);
			}
		#endif
		return;
	} 
	else if (destination == BROADCAST_ADDRESS) {
		// broadcast
		debug(PSTR("MSG:BC\n"));
		if (command == C_INTERNAL) {
			if (isTransportOK()) {
				// only reply if node is fully operational
				if (type == I_FIND_PARENT) {
					#if defined(MY_REPEATER_FEATURE)
						if (sender != _nc.parentNodeId) {	// no circular reference
							debug(PSTR("FPREQ from %d\n"), sender);	// FPR: find parent request
							// node is in our range, update routing table - important if node has new repeater as parent
							hwWriteConfig(EEPROM_ROUTES_ADDRESS + sender, sender);
							// check if uplink functional - node can only be parent node if link to GW functional
							// this also prevents circular references in case GW ooo
							if (transportPingNode(GATEWAY_ADDRESS) != INVALID_HOPS) { // eventually use _nc.distance instead of INVALID_HOPS
								// random wait to minimize collisions
								debug(PSTR("GWL ok, FPRES to %d\n"), sender); // GW link ok, respond to request
								delay(hwMillis() & 0x3ff);
								transportRouteMessage(build(_msgTmp, _nc.nodeId, sender, NODE_SENSOR_ID, C_INTERNAL, I_FIND_PARENT_RESPONSE, false).set(_nc.distance));
							}
						}
							
					#endif
				return; // no further processing required	
				}
			}
			if (type == I_DISCOVER) {
				if (last == _nc.parentNodeId) {
					// random wait to minimize collisions
					delay(hwMillis() & 0x3ff);
					transportRouteMessage(build(_msgTmp, _nc.nodeId, sender, NODE_SENSOR_ID, C_INTERNAL, I_DISCOVER_RESPONSE, false).set(_nc.parentNodeId));
					// no return here (for fwd if repeater)
				}
			}
		}
		// controlled BC relay
		#if defined(MY_REPEATER_FEATURE)
			// controlled BC repeating: forward only if message received from parent and sender not self to prevent circular fwds
			if(last == _nc.parentNodeId && sender != _nc.nodeId && isTransportOK()){
				debug(PSTR("FWD BC msg\n")); // forward BC msg
				transportRouteMessage(_msg);
			}
		#endif
		
		// Call incoming message callback if available, but only if message received from parent
		if (command != C_INTERNAL && last == _nc.parentNodeId && receive) {
			receive(_msg);
		}
				
	} 
	else {
		// msg not no us and not BC, relay msg 
		#if defined(MY_REPEATER_FEATURE)
		if (isTransportOK()) {
			debug(PSTR("REL MSG\n"));	// relay msg
			// only update routing table to uplink
			if (last != _nc.parentNodeId) {
				hwWriteConfig(EEPROM_ROUTES_ADDRESS + sender, last);
			}
			if (command == C_INTERNAL) {
				if (type == I_PING || type == I_PONG) {
					uint8_t hopsCnt = _msg.getByte();
					if (hopsCnt != MAX_HOPS) {
						debug(PSTR("relay ping/pong, hops=%d\n"), hopsCnt);
						_msg.set((uint8_t)(hopsCnt + 1));
					}
				}
			}
			// Relay this message to another node
			transportRouteMessage(_msg);
		}
		#else
			debug(PSTR("REL MSG, but not a repeater\n"));
		#endif
	}
}

inline void transportProcessFIFO() {
	// process all msgs in FIFO
	while (transportAvailable()) {
		transportProcessMessage();
	}
	#if defined(MY_OTA_FIRMWARE_FEATURE)
		if (isTransportOK()) {
			// only process if transport ok
			firmwareOTAUpdateRequest();
		}
	#endif
}

bool transportSendWrite(uint8_t to, MyMessage &message) {
	// set protocol version and update last
	mSetVersion(message, PROTOCOL_VERSION);
	message.last = _nc.nodeId;

	// sign message if required
	if (!signerSignMsg(message)) {
		debug(PSTR("!sign fail\n"));
		setIndication(INDICATION_ERR_SIGN);
	}
	
	// msg length changes if signed
	uint8_t length = mGetSigned(message) ? MAX_MESSAGE_LENGTH : mGetLength(message);
	
	// send
	setIndication(INDICATION_TX);
	bool ok = transportSend(to, &message, min(MAX_MESSAGE_LENGTH, HEADER_SIZE + length));
	
	debug(PSTR("send: %d-%d-%d-%d s=%d,c=%d,t=%d,pt=%d,l=%d,sg=%d,st=%s:%s\n"),
			message.sender,message.last, to, message.destination, message.sensor, mGetCommand(message), message.type,
			mGetPayloadType(message), mGetLength(message), mGetSigned(message), to==BROADCAST_ADDRESS ? "bc" : (ok ? "ok":"fail"), message.getString(_convBuf));
	
	return (ok || to==BROADCAST_ADDRESS);
}

