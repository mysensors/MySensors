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
static transportState stInit = { stInitTransition, NULL };
static transportState stParent = { stParentTransition, stParentUpdate };
static transportState stID = { stIDTransition, stIDUpdate };
static transportState stUplink = { stUplinkTransition, NULL };
static transportState stReady = { stReadyTransition, stReadyUpdate };
static transportState stFailure = { stFailureTransition, stFailureUpdate };

// transport SM variables
static transportSM _transportSM;

// stInit: initialize transport HW
void stInitTransition() {
	TRANSPORT_DEBUG(PSTR("TSM:INIT\n"));
	// initialize status variables
	_transportSM.pingActive = false;
	_transportSM.transportActive = false;
	#if defined(MY_TRANSPORT_SANITY_CHECK) || defined(MY_REPEATER_FEATURE)
		_transportSM.lastSanityCheck = hwMillis();
	#endif
	_transportSM.lastUplinkCheck = 0;
	// Read node settings (ID, parentId, GW distance) from EEPROM
	hwReadConfigBlock((void*)&_nc, (void*)EEPROM_NODE_ID_ADDRESS, sizeof(NodeConfig));

	// initialize radio
	if (!transportInit()) {
		TRANSPORT_DEBUG(PSTR("!TSM:INIT:TSP FAIL\n"));
		setIndication(INDICATION_ERR_INIT_TRANSPORT);
		transportSwitchSM(stFailure);
	}
	else {
		TRANSPORT_DEBUG(PSTR("TSM:INIT:TSP OK\n"));
		_transportSM.transportActive = true;
		#if defined(MY_GATEWAY_FEATURE)
			// Set configuration for gateway
			TRANSPORT_DEBUG(PSTR("TSM:INIT:GW MODE\n"));
			_nc.parentNodeId = GATEWAY_ADDRESS;
			_nc.distance = 0;
			_nc.nodeId = GATEWAY_ADDRESS;
			transportSetAddress(GATEWAY_ADDRESS);
			// GW mode: skip FPAR,ID,UPL states
			transportSwitchSM(stReady);
		#else
			if ((uint8_t)MY_NODE_ID != AUTO) {
				TRANSPORT_DEBUG(PSTR("TSM:INIT:STATID,ID=%d\n"),MY_NODE_ID);
				// Set static id
				_nc.nodeId = MY_NODE_ID;
				// Save static id in eeprom
				hwWriteConfig(EEPROM_NODE_ID_ADDRESS, MY_NODE_ID);
			}
			// set ID if static or set in EEPROM
			if (_nc.nodeId == AUTO || transportAssignNodeID(_nc.nodeId)) {
				// if node ID > 0, proceed to next state
				transportSwitchSM(stParent);
			}
			else {
				// ID invalid (=0), nothing we can do
				transportSwitchSM(stFailure);
			}
		#endif	
	}
}

// stParent: find parent
void stParentTransition()  {
	TRANSPORT_DEBUG(PSTR("TSM:FPAR\n"));	// find parent
	setIndication(INDICATION_FIND_PARENT);
	_transportSM.uplinkOk = false;
	_transportSM.preferredParentFound = false;
	#if defined(MY_PARENT_NODE_IS_STATIC)
		TRANSPORT_DEBUG(PSTR("TSM:FPAR:STATP=%d\n"),MY_PARENT_NODE_ID);	// static parent
		_transportSM.findingParentNode = false;
		_nc.distance = 1;	// assumption, CHKUPL:GWDC will update this variable
		_nc.parentNodeId = MY_PARENT_NODE_ID;
		// skipping find parent
		setIndication(INDICATION_GOT_PARENT);
		transportSwitchSM(stID);
	#else
		_transportSM.findingParentNode = true;
		_nc.distance = DISTANCE_INVALID;	// Set distance to max and invalidate parent node ID
		_nc.parentNodeId = AUTO;
		// Broadcast find parent request
		transportRouteMessage(build(_msgTmp, _nc.nodeId, BROADCAST_ADDRESS, NODE_SENSOR_ID, C_INTERNAL, I_FIND_PARENT_REQUEST, false).set(""));
	#endif
}

// stParentUpdate
void stParentUpdate() {
	#if !defined(MY_PARENT_NODE_IS_STATIC)
		if (transportTimeInState() > STATE_TIMEOUT || _transportSM.preferredParentFound) {
			// timeout or preferred parent found
			if (_nc.parentNodeId != AUTO) {
				// parent assigned
				TRANSPORT_DEBUG(PSTR("TSM:FPAR:OK\n"));	// find parent ok
				_transportSM.findingParentNode = false;
				setIndication(INDICATION_GOT_PARENT);
				// go to next state
				transportSwitchSM(stID);
			}
			else if (transportTimeInState() > STATE_TIMEOUT) {
				// timeout w/o reply or valid parent
				if (_transportSM.retries < STATE_RETRIES) {
					// retries left
					TRANSPORT_DEBUG(PSTR("!TSM:FPAR:NO REPLY\n"));		// find parent, no reply
					// reenter state
					transportSwitchSM(stParent);
				}
				else {
					// no retries left, finding parent failed
					TRANSPORT_DEBUG(PSTR("!TSM:FPAR:FAIL\n"));	// find parent fail
					setIndication(INDICATION_ERR_FIND_PARENT);
					transportSwitchSM(stFailure);
				}
			}
		}
	#endif
}

// stID: verify and request ID if necessary
void stIDTransition() {
	TRANSPORT_DEBUG(PSTR("TSM:ID\n"));	// verify/request node ID
	if (_nc.nodeId == AUTO) {
		// send ID request
		setIndication(INDICATION_REQ_NODEID);
		TRANSPORT_DEBUG(PSTR("TSM:ID:REQ\n"));	// request node ID
		transportRouteMessage(build(_msgTmp, _nc.nodeId, GATEWAY_ADDRESS, NODE_SENSOR_ID, C_INTERNAL, I_ID_REQUEST, false).set(""));
	}
}

void stIDUpdate() {
	if (_nc.nodeId != AUTO) {
		// current node ID is valid
		TRANSPORT_DEBUG(PSTR("TSM:ID:OK,ID=%d\n"), _nc.nodeId);
		setIndication(INDICATION_GOT_NODEID);
		// proceed to next state
		transportSwitchSM(stUplink);	
	}
	else if (transportTimeInState() > STATE_TIMEOUT) {
		// timeout
		if (_transportSM.retries < STATE_RETRIES) {
			// reenter
			transportSwitchSM(stID);
		}
		else {
			// no retries left
			TRANSPORT_DEBUG(PSTR("!TSM:ID:FAIL,ID=%d\n"), _nc.nodeId);
			setIndication(INDICATION_ERR_GET_NODEID);
			transportSwitchSM(stFailure);
		}
	}
}

void stUplinkTransition() {
	TRANSPORT_DEBUG(PSTR("TSM:UPL\n"));
	// check uplink
	if(transportCheckUplink(true)) {
		// uplink ok, i.e. GW replied
		TRANSPORT_DEBUG(PSTR("TSM:UPL:OK\n"));	// uplink ok
		// proceed to next state
		transportSwitchSM(stReady);
	}
	else {
		// uplink failed, at this point, no retries or timeout
		TRANSPORT_DEBUG(PSTR("!TSM:UPL:FAIL\n"));	// uplink failed
		// go back to stParent
		transportSwitchSM(stParent);
	}
}

void stReadyTransition() {
	// transport is ready and fully operational
	TRANSPORT_DEBUG(PSTR("TSM:READY\n"));		// transport is ready
	_transportSM.uplinkOk = true;
	_transportSM.failedUplinkTransmissions = 0;	// reset counter
}

// stReadyUpdate: monitors uplink failures
void stReadyUpdate() {
	#if !defined(MY_GATEWAY_FEATURE)		
		if (_transportSM.failedUplinkTransmissions > TRANSMISSION_FAILURES) {
			// too many uplink transmissions failed, find new parent (if non-static)
			#if !defined(MY_PARENT_NODE_IS_STATIC)
				TRANSPORT_DEBUG(PSTR("!TSM:READY:UPL FAIL,SNP\n"));		// uplink failed, search new parent
				transportSwitchSM(stParent);
			#else
				TRANSPORT_DEBUG(PSTR("!TSM:READY:UPL FAIL,STATP\n"));	// uplink failed, static parent
				// reset counter
				_transportSM.failedUplinkTransmissions = 0;
			#endif
		}
	#endif
}

// stFailure: entered upon HW init failure or max retries exceeded
void stFailureTransition() {
	TRANSPORT_DEBUG(PSTR("TSM:FAILURE\n"));
	_transportSM.uplinkOk = false;	// uplink nok
	_transportSM.transportActive = false;	// transport inactive
	setIndication(INDICATION_ERR_INIT_TRANSPORT);
	// power down transport, no need until re-init
	TRANSPORT_DEBUG(PSTR("TSM:FAILURE:PDT\n"));	// power down transport
	transportPowerDown();
}

void stFailureUpdate() {
	if (transportTimeInState()> TIMEOUT_FAILURE_STATE) {
		TRANSPORT_DEBUG(PSTR("TSM:FAILURE:RE-INIT\n"));	// attempt to re-initialize transport
		transportSwitchSM(stInit);
	}
}

void transportSwitchSM(transportState& newState) {
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

bool isTransportReady() {
	return _transportSM.uplinkOk;
}

bool isTransportSearchingParent() {
	return _transportSM.findingParentNode;
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
	transportProcessFIFO();
}


bool transportCheckUplink(bool force) {
	if (!force && (hwMillis() - _transportSM.lastUplinkCheck) < CHKUPL_INTERVAL) {
		TRANSPORT_DEBUG(PSTR("TSF:CHKUPL:OK,FCTRL\n"));	// flood control
		return true;
	}
	// ping GW
	uint8_t hopsCount = transportPingNode(GATEWAY_ADDRESS);
	// verify hops
	if (hopsCount != INVALID_HOPS) {
		// update
		_transportSM.lastUplinkCheck = hwMillis();
		TRANSPORT_DEBUG(PSTR("TSF:CHKUPL:OK\n"));
		// did distance to GW change upstream, eg. re-routing of uplink nodes
		if (hopsCount != _nc.distance) {
			TRANSPORT_DEBUG(PSTR("TSF:CHKUPL:DGWC,O=%d,N=%d\n"), _nc.distance, hopsCount);	// distance to GW changed
			_nc.distance = hopsCount;
		}
		return true;
	}
	else {
		TRANSPORT_DEBUG(PSTR("TSF:CHKUPL:FAIL\n"));
		return false;
	}
}

bool transportAssignNodeID(uint8_t newNodeId) {
	// verify if ID valid
	if (newNodeId != GATEWAY_ADDRESS && newNodeId != AUTO) {
		_nc.nodeId = newNodeId;
		transportSetAddress(newNodeId);
		// Write ID to EEPROM
		hwWriteConfig(EEPROM_NODE_ID_ADDRESS, newNodeId);
		TRANSPORT_DEBUG(PSTR("TSF:ASID:OK,ID=%d\n"),newNodeId);
		return true;
	}
	else {
		TRANSPORT_DEBUG(PSTR("!TSF:ASID:FAIL,ID=%d\n"),newNodeId);
		setIndication(INDICATION_ERR_NET_FULL);
		return false;
	}
}

bool transportRouteMessage(MyMessage &message) {
	uint8_t destination = message.destination;
	uint8_t route;
	
	if (_transportSM.findingParentNode && destination != BROADCAST_ADDRESS) {
		TRANSPORT_DEBUG(PSTR("!TSF:ROUTE:FPAR ACTIVE\n")); // find parent active, message not sent
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
					TRANSPORT_DEBUG(PSTR("!TSF:ROUTE:%d UNKNOWN\n"), destination);
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
	if (isTransportReady()) {
		return transportRouteMessage(message);
	}
	else {
		// TNR: transport not ready
		TRANSPORT_DEBUG(PSTR("!TSF:SEND:TNR\n"));
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
		// process incoming messages
		transportProcessFIFO();
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
			// ping to ourself, pingActive remains false
			return 0;
		}
		_transportSM.pingActive = true;
		_transportSM.pingResponse = INVALID_HOPS;
		TRANSPORT_DEBUG(PSTR("TSF:PING:SEND,TO=%d\n"), targetId);
		transportRouteMessage(build(_msgTmp, _nc.nodeId, targetId, NODE_SENSOR_ID, C_INTERNAL, I_PING, false).set((uint8_t)0x01));
		// Wait for ping reply or timeout
		transportWait(2000, C_INTERNAL, I_PONG);
		// make sure missing I_PONG msg does not block pinging function by leaving pignActive=true
		_transportSM.pingActive = false;
		return _transportSM.pingResponse;
	}
	else {
		return INVALID_HOPS;
	}
}

void transportClearRoutingTable() {
	for (uint8_t i = 0; i != 255; i++) {
		hwWriteConfig(EEPROM_ROUTES_ADDRESS + i, BROADCAST_ADDRESS);
	}
	TRANSPORT_DEBUG(PSTR("TSF:CRT:OK\n"));	// clear routing table
}

uint32_t transportGetHeartbeat() {
	return transportTimeInState();
}

void transportProcessMessage() {
	(void)signerCheckTimer(); // Manage signing timeout

	uint8_t payloadLength = transportReceive((uint8_t *)&_msg);
	(void)payloadLength;	// currently not used
	
	setIndication(INDICATION_RX);

	uint8_t command = mGetCommand(_msg);
	uint8_t type = _msg.type;
	uint8_t sender = _msg.sender;
	uint8_t last = _msg.last;
	uint8_t destination = _msg.destination;

	TRANSPORT_DEBUG(PSTR("TSF:MSG:READ,%d-%d-%d,s=%d,c=%d,t=%d,pt=%d,l=%d,sg=%d:%s\n"),
		sender, last, destination, _msg.sensor, mGetCommand(_msg), type, mGetPayloadType(_msg), mGetLength(_msg), mGetSigned(_msg), _msg.getString(_convBuf));

	// verify protocol version
	if(mGetVersion(_msg) != PROTOCOL_VERSION) {
		setIndication(INDICATION_ERR_VERSION);
		TRANSPORT_DEBUG(PSTR("!TSF:MSG:PVER,%d!=%d\n"), mGetVersion(_msg),PROTOCOL_VERSION);	// protocol version mismatch
		return;
	}
		
	// Reject messages that do not pass verification
	if (!signerVerifyMsg(_msg)) {
		setIndication(INDICATION_ERR_SIGN);
		TRANSPORT_DEBUG(PSTR("!TSF:MSG:SIGN VERIFY FAIL\n"));
		return;	
	}
		
	// Is message addressed to this node?
	if (destination == _nc.nodeId) {
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
			TRANSPORT_DEBUG(PSTR("TSF:MSG:ACK REQ\n"));	// ACK requested
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
						#if !defined(MY_GATEWAY_FEATURE) && !defined(MY_PARENT_NODE_IS_STATIC)
							if (_transportSM.findingParentNode) {	// only process if find parent active
								// Reply to a I_FIND_PARENT_REQUEST message. Check if the distance is shorter than we already have.
								uint8_t distance = _msg.getByte();
								TRANSPORT_DEBUG(PSTR("TSF:MSG:FPAR RES,ID=%d,D=%d\n"), sender, distance);	// find parent response
								if (isValidDistance(distance)) {
									// Distance to gateway is one more for us w.r.t. parent
									distance++;
									// update settings if distance shorter or preferred parent found
									if (((isValidDistance(distance) && distance < _nc.distance) || (!_autoFindParent && sender == MY_PARENT_NODE_ID)) && !_transportSM.preferredParentFound) {
										// Found a neighbor closer to GW than previously found
										if (!_autoFindParent && sender == MY_PARENT_NODE_ID) {
											_transportSM.preferredParentFound = true;
											TRANSPORT_DEBUG(PSTR("TSF:MSG:FPAR PREF FOUND\n"));	// find parent, preferred parent found
										}
										_nc.distance = distance;
										_nc.parentNodeId = sender;
										TRANSPORT_DEBUG(PSTR("TSF:MSG:FPAR OK,ID=%d,D=%d\n"), _nc.parentNodeId, _nc.distance);
									}
								}
							}
							else {
								TRANSPORT_DEBUG(PSTR("TSF:MSG:FPAR INACTIVE\n"));	// find parent response received, but inactive
							}
							return;
						#endif
					}
				#endif
				// general
				if (type == I_PING) {
					TRANSPORT_DEBUG(PSTR("TSF:MSG:PINGED,ID=%d,HP=%d\n"), sender, _msg.getByte()); // node pinged
					transportRouteMessage(build(_msgTmp, _nc.nodeId, sender, NODE_SENSOR_ID, C_INTERNAL, I_PONG, false).set((uint8_t)0x01));
					return; // no further processing required
				}
				if (type == I_PONG) {
					if (_transportSM.pingActive) {
						_transportSM.pingActive = false;
						_transportSM.pingResponse = _msg.getByte();
						TRANSPORT_DEBUG(PSTR("TSF:MSG:PONG RECV,HP=%d\n"), _transportSM.pingResponse); // pong received
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
		else {
			TRANSPORT_DEBUG(PSTR("TSF:MSG:ACK\n")); // received message is ACK, no internal processing, handover to msg callback
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
	} 
	else if (destination == BROADCAST_ADDRESS) {
		TRANSPORT_DEBUG(PSTR("TSF:MSG:BC\n"));	// broadcast msg
		if (command == C_INTERNAL) {
			if (isTransportReady()) {
				// only reply if node is fully operational
				if (type == I_FIND_PARENT_REQUEST) {
					#if defined(MY_REPEATER_FEATURE)
						if (sender != _nc.parentNodeId) {	// no circular reference
							TRANSPORT_DEBUG(PSTR("TSF:MSG:FPAR REQ,ID=%d\n"), sender);	// FPR: find parent request
							// node is in our range, update routing table - important if node has new repeater as parent
							hwWriteConfig(EEPROM_ROUTES_ADDRESS + sender, sender);
							// check if uplink functional - node can only be parent node if link to GW functional
							// this also prevents circular references in case GW ooo
							if(transportCheckUplink(false)){ 
								_transportSM.lastUplinkCheck = hwMillis();
								TRANSPORT_DEBUG(PSTR("TSF:MSG:GWL OK\n")); // GW uplink ok
								// delay minimizes collisions
								delay(hwMillis() & 0x3ff);
								transportRouteMessage(build(_msgTmp, _nc.nodeId, sender, NODE_SENSOR_ID, C_INTERNAL, I_FIND_PARENT_RESPONSE, false).set(_nc.distance));
							}
							else {
								TRANSPORT_DEBUG(PSTR("!TSF:MSG:GWL FAIL\n")); // GW uplink fail, do not respond to parent request
							}
						}
							
					#endif
				return; // no further processing required	
				}
			}
			if (type == I_DISCOVER_REQUEST) {
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
			if(last == _nc.parentNodeId && sender != _nc.nodeId && isTransportReady()){
				TRANSPORT_DEBUG(PSTR("TSF:MSG:FWD BC MSG\n")); // controlled broadcast msg forwarding
				transportRouteMessage(_msg);
			}
		#endif
		
		// Call incoming message callback if available, but only if message received from parent
		if (command != C_INTERNAL && last == _nc.parentNodeId && receive) {
			receive(_msg);
		}
				
	} 
	else {
		// msg not to us and not BC, relay msg 
		#if defined(MY_REPEATER_FEATURE)
		if (isTransportReady()) {
			TRANSPORT_DEBUG(PSTR("TSF:MSG:REL MSG\n"));	// relay msg
			// update routing table if message not received from parent
			if (last != _nc.parentNodeId) {
				hwWriteConfig(EEPROM_ROUTES_ADDRESS + sender, last);
			}
			if (command == C_INTERNAL) {
				if (type == I_PING || type == I_PONG) {
					uint8_t hopsCnt = _msg.getByte();
					if (hopsCnt != MAX_HOPS) {
						TRANSPORT_DEBUG(PSTR("TSF:MSG:REL PxNG,HP=%d\n"), hopsCnt);
						_msg.set((uint8_t)(hopsCnt + 1));
					}
				}
			}
			// Relay this message to another node
			transportRouteMessage(_msg);
		}
		#else
			TRANSPORT_DEBUG(PSTR("!TSF:MSG:REL MSG,NORP\n"));	// message relaying request, but not a repeater
		#endif
	}
}

void transportInvokeSanityCheck() {
	if (!transportSanityCheck()) {
		TRANSPORT_DEBUG(PSTR("!TSF:SANCHK:FAIL\n"));	// sanity check fail
		transportSwitchSM(stFailure);
		return;
	}
	else {
		TRANSPORT_DEBUG(PSTR("TSF:SANCHK:OK\n"));		// sanity check ok
	}
}

inline void transportProcessFIFO() {
	if (_transportSM.transportActive) {
		#if defined(MY_TRANSPORT_SANITY_CHECK) || defined(MY_REPEATER_FEATURE)
			if (hwMillis() - _transportSM.lastSanityCheck > MY_TRANSPORT_SANITY_CHECK_INTERVAL) {
				_transportSM.lastSanityCheck = hwMillis();
				transportInvokeSanityCheck();
			}
		#endif
	}
	else {
		// transport not active, nothing to be done
		return;
	}
	uint8_t _processedMessages = MAX_SUBSEQ_MSGS;
	// process all msgs in FIFO or counter exit
	while (transportAvailable() && _processedMessages--) {
		transportProcessMessage();
	}
	#if defined(MY_OTA_FIRMWARE_FEATURE)
		if (isTransportReady()) {
			// only process if transport ok
			firmwareOTAUpdateRequest();
		}
	#endif
}

bool transportSendWrite(uint8_t to, MyMessage &message) {
	// Update last
	message.last = _nc.nodeId;

	// sign message if required
	if (!signerSignMsg(message)) {
		TRANSPORT_DEBUG(PSTR("!TSF:MSG:SIGN FAIL\n"));
		setIndication(INDICATION_ERR_SIGN);
		return false;
	}
	
	// msg length changes if signed
	uint8_t length = mGetSigned(message) ? MAX_MESSAGE_LENGTH : mGetLength(message);
	
	// send
	setIndication(INDICATION_TX);
	bool ok = transportSend(to, &message, min(MAX_MESSAGE_LENGTH, HEADER_SIZE + length));
	
	TRANSPORT_DEBUG(PSTR("%sTSF:MSG:SEND,%d-%d-%d-%d,s=%d,c=%d,t=%d,pt=%d,l=%d,sg=%d,ft=%d,st=%s:%s\n"),
			(ok || to == BROADCAST_ADDRESS ? "" : "!"),message.sender,message.last, to, message.destination, message.sensor, mGetCommand(message), message.type,
			mGetPayloadType(message), mGetLength(message), mGetSigned(message), _transportSM.failedUplinkTransmissions, to==BROADCAST_ADDRESS ? "bc" : (ok ? "OK":"NACK"), message.getString(_convBuf));
	
	return (ok || to==BROADCAST_ADDRESS);
}

// EOF MyTransport.cpp