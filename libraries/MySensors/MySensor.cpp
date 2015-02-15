 /*
 The MySensors library adds a new layer on top of the RF24 library.
 It handles radio network routing, relaying and ids.

 Created by Henrik Ekblad <henrik.ekblad@gmail.com>

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.
 */

#include "MySensor.h"
#include "utility/LowPower.h"
#include "utility/RF24.h"
#include "utility/RF24_config.h"


// Inline function and macros
inline MyMessage& build (MyMessage &msg, uint8_t sender, uint8_t destination, uint8_t sensor, uint8_t command, uint8_t type, bool enableAck) {
	msg.sender = sender;
	msg.destination = destination;
	msg.sensor = sensor;
	msg.type = type;
	mSetCommand(msg,command);
	mSetRequestAck(msg,enableAck);
	mSetAck(msg,false);
	return msg;
}

MySensor::MySensor(uint8_t _cepin, uint8_t _cspin) : RF24(_cepin, _cspin) {
}

void MySensor::begin(void (*_msgCallback)(const MyMessage &), uint8_t _nodeId, boolean _repeaterMode, uint8_t _parentNodeId, rf24_pa_dbm_e paLevel, uint8_t channel, rf24_datarate_e dataRate) {
	Serial.begin(BAUD_RATE);
	isGateway = false;
	repeaterMode = _repeaterMode;
	msgCallback = _msgCallback;

	if (repeaterMode) {
		setupRepeaterMode();
	}
	setupRadio(paLevel, channel, dataRate);

	// Read settings from eeprom
	eeprom_read_block((void*)&nc, (void*)EEPROM_NODE_ID_ADDRESS, sizeof(NodeConfig));
	// Read latest received controller configuration from EEPROM
	eeprom_read_block((void*)&cc, (void*)EEPROM_CONTROLLER_CONFIG_ADDRESS, sizeof(ControllerConfig));
	if (cc.isMetric == 0xff) {
		// Eeprom empty, set default to metric
		cc.isMetric = 0x01;
	}

	if (_parentNodeId != AUTO) {
		if (_parentNodeId != nc.parentNodeId) {
			nc.parentNodeId = _parentNodeId;
			// Save static parent id in eeprom
			eeprom_write_byte((uint8_t*)EEPROM_PARENT_NODE_ID_ADDRESS, _parentNodeId);
		}
		autoFindParent = false;
	} else {
		autoFindParent = true;
	}

	if ( (_nodeId != AUTO) && (nc.nodeId != _nodeId) ) {
	    // Set static id
	    nc.nodeId = _nodeId;
	    // Save static id in eeprom
	    eeprom_write_byte((uint8_t*)EEPROM_NODE_ID_ADDRESS, _nodeId);
	}

	// If no parent was found in eeprom. Try to find one.
	if (autoFindParent && nc.parentNodeId == 0xff) {
		findParentNode();
	}

	// Try to fetch node-id from gateway
	if (nc.nodeId == AUTO) {
		requestNodeId();
	}

	debug(PSTR("%s started, id %d\n"), repeaterMode?"repeater":"sensor", nc.nodeId);

	// If we got an id, set this node to use it
	if (nc.nodeId != AUTO) { 
		setupNode();
		// Wait configuration reply.
		wait(2000);
	}
}

void MySensor::setupRadio(rf24_pa_dbm_e paLevel, uint8_t channel, rf24_datarate_e dataRate) {
	failedTransmissions = 0;

	// Start up the radio library
	RF24::begin();

	if (!RF24::isPVariant()) {
		debug(PSTR("check wires\n"));
		while(1);
	}
	RF24::setAutoAck(1);
	RF24::setAutoAck(BROADCAST_PIPE,false); // Turn off auto ack for broadcast
	RF24::enableAckPayload();
	RF24::setChannel(channel);
	RF24::setPALevel(paLevel);
	RF24::setDataRate(dataRate);
	RF24::setRetries(5,15);
	RF24::setCRCLength(RF24_CRC_16);
	RF24::enableDynamicPayloads();

	// All nodes listen to broadcast pipe (for FIND_PARENT_RESPONSE messages)
	RF24::openReadingPipe(BROADCAST_PIPE, TO_ADDR(BROADCAST_ADDRESS));
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
	RF24::openReadingPipe(CURRENT_NODE_PIPE, TO_ADDR(nc.nodeId));
	sendRoute(build(msg, nc.nodeId, GATEWAY_ADDRESS, NODE_SENSOR_ID, C_INTERNAL, I_ID_REQUEST, false).set(""));
	wait(2000);
}

void MySensor::setupNode() {
	// Open reading pipe for messages directed to this node (set write pipe to same)
	RF24::openReadingPipe(WRITE_PIPE, TO_ADDR(nc.nodeId));
	RF24::openReadingPipe(CURRENT_NODE_PIPE, TO_ADDR(nc.nodeId));

	// Send presentation for this radio node (attach
	present(NODE_SENSOR_ID, repeaterMode? S_ARDUINO_REPEATER_NODE : S_ARDUINO_NODE);

	// Send a configuration exchange request to controller
	// Node sends parent node. Controller answers with latest node configuration
	// which is picked up in process()
	sendRoute(build(msg, nc.nodeId, GATEWAY_ADDRESS, NODE_SENSOR_ID, C_INTERNAL, I_CONFIG, false).set(nc.parentNodeId));
}

void MySensor::findParentNode() {
	failedTransmissions = 0;

	// Set distance to max
	nc.distance = 255;

	// Send ping message to BROADCAST_ADDRESS (to which all relaying nodes and gateway listens and should reply to)
	build(msg, nc.nodeId, BROADCAST_ADDRESS, NODE_SENSOR_ID, C_INTERNAL, I_FIND_PARENT, false).set("");
	sendWrite(BROADCAST_ADDRESS, msg, true);

	// Wait for ping response.
	wait(2000);
}

boolean MySensor::sendRoute(MyMessage &message) {
	// Make sure to process any incoming messages before sending (could this end up in recursive loop?)
	// process();
	bool isInternal = mGetCommand(message) == C_INTERNAL;

	// If we still don't have any node id, re-request and skip this message.
	if (nc.nodeId == AUTO && !(isInternal && message.type == I_ID_REQUEST)) {
		requestNodeId();
		return false;
	}

	if (repeaterMode) {
		uint8_t dest = message.destination;
		uint8_t route = getChildRoute(dest);
		if (route>GATEWAY_ADDRESS && route<BROADCAST_ADDRESS && dest != GATEWAY_ADDRESS) {
			// --- debug(PSTR("route %d.\n"), route);
			// Message destination is not gateway and is in routing table for this node.
			// Send it downstream
			return sendWrite(route, message);
		} else if (isInternal && message.type == I_ID_RESPONSE && dest==BROADCAST_ADDRESS) {
			// Node has not yet received any id. We need to send it
			// by doing a broadcast sending,
			return sendWrite(BROADCAST_ADDRESS, message, true);
		}
	}

	if (!isGateway) {
		// --- debug(PSTR("route parent\n"));
		// Should be routed back to gateway.
		bool ok = sendWrite(nc.parentNodeId, message);

		if (!ok) {
			// Failure when sending to parent node. The parent node might be down and we
			// need to find another route to gateway.
			if (autoFindParent && failedTransmissions > SEARCH_FAILURES) {
				findParentNode();
			} else {
				failedTransmissions++;
			}
		} else {
			failedTransmissions = 0;
		}
		return ok;
	}
	return false;
}

boolean MySensor::sendWrite(uint8_t next, MyMessage &message, bool broadcast) {
	uint8_t length = mGetLength(message);
	message.last = nc.nodeId;
	mSetVersion(message, PROTOCOL_VERSION);
	// Make sure radio has powered up
	RF24::powerUp();
	RF24::stopListening();
	RF24::openWritingPipe(TO_ADDR(next));
	bool ok = RF24::write(&message, min(MAX_MESSAGE_LENGTH, HEADER_SIZE + length), broadcast);
	RF24::startListening();

	debug(PSTR("send: %d-%d-%d-%d s=%d,c=%d,t=%d,pt=%d,l=%d,st=%s:%s\n"),
			message.sender,message.last, next, message.destination, message.sensor, mGetCommand(message), message.type, mGetPayloadType(message), mGetLength(message), ok?"ok":"fail", message.getString(convBuf));

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
	uint8_t pipe;
	boolean available = RF24::available(&pipe);

	if (!available || pipe>6)
		return false;

	uint8_t len = RF24::getDynamicPayloadSize();
	RF24::read(&msg, len);

	// Add string termination, good if we later would want to print it.
	msg.data[mGetLength(msg)] = '\0';
	debug(PSTR("read: %d-%d-%d s=%d,c=%d,t=%d,pt=%d,l=%d:%s\n"),
				msg.sender, msg.last, msg.destination,  msg.sensor, mGetCommand(msg), msg.type, mGetPayloadType(msg), mGetLength(msg), msg.getString(convBuf));

	if(!(mGetVersion(msg) == PROTOCOL_VERSION)) {
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
			ack = msg;
			mSetRequestAck(ack,false); // Reply without ack flag (otherwise we would end up in an eternal loop)
			mSetAck(ack,true);
			ack.sender = nc.nodeId;
			ack.destination = msg.sender;
			sendRoute(ack);
		}

		if (command == C_INTERNAL) {
			if (type == I_FIND_PARENT_RESPONSE) {
				if (autoFindParent) {
					// We've received a reply to a FIND_PARENT message. Check if the distance is
					// shorter than we already have.
					uint8_t distance = msg.getByte();
					if (distance<nc.distance-1) {
						// Found a neighbor closer to GW than previously found
						nc.distance = distance + 1;
						nc.parentNodeId = msg.sender;
						eeprom_write_byte((uint8_t*)EEPROM_PARENT_NODE_ID_ADDRESS, nc.parentNodeId);
						eeprom_write_byte((uint8_t*)EEPROM_DISTANCE_ADDRESS, nc.distance);
						debug(PSTR("new parent=%d, d=%d\n"), nc.parentNodeId, nc.distance);
					}
				}
				return false;
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
						eeprom_write_byte((uint8_t*)EEPROM_NODE_ID_ADDRESS, nc.nodeId);
						debug(PSTR("id=%d\n"), nc.nodeId);
					}
				} else if (type == I_CONFIG) {
					// Pick up configuration from controller (currently only metric/imperial)
					// and store it in eeprom if changed
					isMetric = msg.getString()[0] == 'M' ;
					if (cc.isMetric != isMetric) {
						cc.isMetric = isMetric;
						eeprom_write_byte((uint8_t*)EEPROM_CONTROLLER_CONFIG_ADDRESS, isMetric);
					}
				} else if (type == I_CHILDREN) {
					if (repeaterMode && msg.getString()[0] == 'C') {
						// Clears child relay data for this node
						debug(PSTR("rd=clear\n"));
						uint8_t i = 255;
						do {
							removeChildRoute(i);
						} while (i--);
						// Clear parent node id & distance to gw
						eeprom_write_byte((uint8_t*)EEPROM_PARENT_NODE_ID_ADDRESS, 0xFF);
						eeprom_write_byte((uint8_t*)EEPROM_DISTANCE_ADDRESS, 0xFF);
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
		// Call incoming message callback if available
		if (msgCallback != NULL) {
			msgCallback(msg);
		}
		// Return true if message was addressed for this node...
		return true;
	} else if (repeaterMode && nc.nodeId != AUTO) {
		// Relaying nodes should answer only after set an id

		if (command == C_INTERNAL && type == I_FIND_PARENT) {
			if (nc.distance == 255) {
				findParentNode();
			} else if (sender != nc.parentNodeId) {
				// Relaying nodes should always answer ping messages
				// Wait a random delay of 0-2 seconds to minimize collision
				// between ping ack messages from other relaying nodes
				delay(millis() & 0x3ff);
				sendWrite(sender, build(msg, nc.nodeId, sender, NODE_SENSOR_ID, C_INTERNAL, I_FIND_PARENT_RESPONSE, false).set(nc.distance), true);
			}
		} else if (pipe == CURRENT_NODE_PIPE) {
			// We should try to relay this message to another node

			uint8_t route = getChildRoute(msg.destination);
			if (route>0 && route<255) {
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
				// lookup route in table and send message there
				sendWrite(route, msg);
			} else if (sender == GATEWAY_ADDRESS && destination == BROADCAST_ADDRESS) {
				// A net gateway reply to a message previously sent by us from a 255 node
				// We should broadcast this back to the node
				sendWrite(destination, msg, true);
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
				sendWrite(nc.parentNodeId, msg);
				// Add this child to our "routing table" if it not already exist
				addChildRoute(sender, last);
			}
		}
	}
	return false;
}

MyMessage& MySensor::getLastMessage() {
	return msg;
}

void MySensor::saveState(uint8_t pos, uint8_t value) {
	if (loadState(pos) != value) {
		eeprom_write_byte((uint8_t*)(EEPROM_LOCAL_CONFIG_ADDRESS+pos), value);
	}
}
uint8_t MySensor::loadState(uint8_t pos) {
	return eeprom_read_byte((uint8_t*)(EEPROM_LOCAL_CONFIG_ADDRESS+pos));
}

void MySensor::addChildRoute(uint8_t childId, uint8_t route) {
	if (childNodeTable[childId] != route) {
		childNodeTable[childId] = route;
		eeprom_write_byte((uint8_t*)EEPROM_ROUTES_ADDRESS+childId, route);
	}
}

void MySensor::removeChildRoute(uint8_t childId) {
	if (childNodeTable[childId] != 0xff) {
		childNodeTable[childId] = 0xff;
		eeprom_write_byte((uint8_t*)EEPROM_ROUTES_ADDRESS+childId, 0xff);
	}
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
	RF24::powerDown();
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
	RF24::powerDown();
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
	RF24::powerDown();
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
