 /*
 The MySensors library adds a new layer on top of the RF24 library.
 It handles radio network routing, relaying and ids.

 Created by Henrik Ekblad <henrik.ekblad@gmail.com>

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.
 */

#include "MySensor.h"

// Inline function and macros
inline MyMessage build (MyMessage msg, uint8_t sender, uint8_t destination, uint8_t sensor, uint8_t command, uint8_t type) {
	msg.sender = sender;
	msg.destination = destination;
	msg.sensor = sensor;
	msg.type = type;
	mSetCommand(msg,command);
	return msg;
}

// Created macros for these, compiler seems to do a bad job when doing sendRoute(build(...))
#define buildSend(_msg, _sender, _destination, _sensor, _cmd, _type) \
		build(_msg, _sender, _destination, _sensor, _cmd, _type);\
		sendRoute(_msg);

#define buildSendPayload(_msg, _sender, _destination, _sensor, _cmd, _type, _value) \
		build(_msg, _sender, _destination, _sensor, _cmd, _type).set(_value);\
		_msg.set(_value);\
		sendRoute(_msg);


MySensor::MySensor(uint8_t _cepin, uint8_t _cspin) : RF24(_cepin, _cspin) {
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
	RF24::enableAckPayload();
	RF24::setChannel(channel);
	RF24::setPALevel(paLevel);
	RF24::setDataRate(dataRate);
	RF24::setRetries(5,15);
	RF24::setCRCLength(RF24_CRC_16);
	RF24::enableDynamicPayloads();

	// All repeater nodes and gateway listen to broadcast pipe (for PING messages)
	if (relayMode) {
		RF24::openReadingPipe(BROADCAST_PIPE, TO_ADDR(BROADCAST_ADDRESS));
	}
}

void MySensor::setupRelayMode(){
	childNodeTable = new uint8_t[256];
	eeprom_read_block((void*)childNodeTable, (void*)EEPROM_ROUTES_ADDRESS, 256);
}

void MySensor::begin(void (*_msgCallback)(MyMessage), boolean _relayMode, uint8_t _nodeId, rf24_pa_dbm_e paLevel, uint8_t channel, rf24_datarate_e dataRate) {
	Serial.begin(BAUD_RATE);
	relayMode = _relayMode;
	msgCallback = _msgCallback;

	if (relayMode) {
		setupRelayMode();
	}
	setupRadio(paLevel, channel, dataRate);

	// Read settings from EEPROM
	eeprom_read_block((void*)&nc, (void*)EEPROM_NODE_ID_ADDRESS, sizeof(NodeConfig));
	// Read latest received controller configuration from EEPROM
	eeprom_read_block((void*)&cc, (void*)EEPROM_LOCAL_CONFIG_ADDRESS, sizeof(ControllerConfig));


	if (_nodeId != AUTO) {
		// Set static id
		nc.nodeId = _nodeId;
	}

	// If no parent was found. Try to find one.
	if (nc.parentNodeId == 0xff) {
		findParentNode();
	}

	// Try to fetch node-id from gateway
	if (nc.nodeId == AUTO) {
		requestNodeId();
	}

	debug(PSTR("%s started\n"), relayMode?"relay":"sensor");

	// Open reading pipe for messages directed to this node (set write pipe to same)
	RF24::openReadingPipe(WRITE_PIPE, TO_ADDR(nc.nodeId));
	RF24::openReadingPipe(CURRENT_NODE_PIPE, TO_ADDR(nc.nodeId));

	// Send presentation for this radio node (attach
	present(NODE_SENSOR_ID, relayMode? S_ARDUINO_RELAY : S_ARDUINO_NODE);

	// Send a configuration exchange request to controller
	// Node sends parent node. Controller answers with latest node configuration
	// which is picked up in process()
	buildSendPayload(msg, nc.nodeId, GATEWAY_ADDRESS, NODE_SENSOR_ID, C_INTERNAL, I_CONFIG, nc.parentNodeId);
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
	buildSend(msg, nc.nodeId, GATEWAY_ADDRESS, NODE_SENSOR_ID, C_INTERNAL, I_REQUEST_ID);
}


void MySensor::findParentNode() {
	if (nc.nodeId == GATEWAY_ADDRESS)
		return; // Gateway has no business here!

	failedTransmissions = 0;

	// Open a reading pipe for current nodeId (if it differs from broadcast)
	if (nc.nodeId != BROADCAST_PIPE) {
		RF24::openReadingPipe(CURRENT_NODE_PIPE, TO_ADDR(nc.nodeId));
	}
	// Set distance to max
	nc.distance = 255;

	// Send ping message to BROADCAST_ADDRESS (to which all relaying nodes and gateway listens and should reply to)
	build(msg, nc.nodeId, BROADCAST_ADDRESS, NODE_SENSOR_ID, C_INTERNAL, I_PING);
	sendWrite(BROADCAST_ADDRESS, msg, true);

	unsigned long enter = millis();
	// Wait for ack responses slightly more than 1 second
	while (millis() - enter < 1200) {
		process();
	}

}

boolean MySensor::sendRoute(MyMessage message) {
	// Make sure to process any incoming messages before sending (could this end up in recursive loop?)
	// process();

	// If we still don't have any node id, re-request and skip this message.
	if (nc.nodeId == AUTO && !(mGetCommand(message) == C_INTERNAL && message.type == I_REQUEST_ID)) {
		requestNodeId();
		return false;
	}

	if (relayMode) {
		uint8_t dest = message.destination;
		uint8_t route = getChildRoute(dest);
		if (route>0 && route<255 && dest != GATEWAY_ADDRESS) {
			debug(PSTR("route %d.\n"), route);
			// Message destination is not gateway and is in routing table for this node.
			// Send it downstream
			return sendWrite(route, message);
		}
	}

	if (nc.nodeId != GATEWAY_ADDRESS) {
		debug(PSTR("route parent\n"));
		// Should be routed back to gateway.
		bool ok = sendWrite(nc.parentNodeId, message);

		if (!ok) {
			// Failure when sending to parent node. The parent node might be down and we
			// need to find another route to gateway.
			if (failedTransmissions > SEARCH_FAILURES) {
				findParentNode();
			}
			failedTransmissions++;
		} else {
			failedTransmissions = 0;
		}
		return ok;
	}
	return false;
}

boolean MySensor::sendWrite(uint8_t next, MyMessage message, bool broadcast) {
	uint8_t length = mGetLength(message);
	message.last = nc.nodeId;
	mSetVersion(message, PROTOCOL_VERSION);
	message.crc = crc8Message(message);

	// Make sure radio has powered up
	RF24::powerUp();
	RF24::stopListening();
	RF24::openWritingPipe(TO_ADDR(next));
	bool ok = RF24::write(&message, min(MAX_MESSAGE_LENGTH, HEADER_SIZE + length), broadcast);
	RF24::startListening();

	// Only debug print payload if it is of string type
	debug(PSTR("tx: %d-%d-%d-%d s=%d,c=%d,t=%d, st=%s:%s\n"),
			message.sender,message.last, next, message.destination, message.sensor, mGetCommand(message), message.type, ok?"ok":"fail", message.getString());

	return ok;
}


bool MySensor::send(MyMessage message, bool enableAck) {
	message.sender = nc.nodeId;
	if (enableAck) {
		mSetCommand(message,C_SET_WITH_ACK);
	} else {
		mSetCommand(message,C_SET);
	}
	return sendRoute(message);
}

void MySensor::sendBatteryLevel(uint8_t value) {
	buildSendPayload(msg, nc.nodeId, GATEWAY_ADDRESS, NODE_SENSOR_ID, C_INTERNAL, I_BATTERY_LEVEL, value);
}

void MySensor::present(uint8_t childSensorId, uint8_t sensorType) {
	buildSendPayload(msg, nc.nodeId, GATEWAY_ADDRESS, childSensorId, C_PRESENTATION, sensorType, LIBRARY_VERSION);
}

void MySensor::sendSketchInfo(const char *name, const char *version) {
	if (name != NULL) {
		buildSendPayload(msg, nc.nodeId, GATEWAY_ADDRESS, NODE_SENSOR_ID, C_INTERNAL, I_SKETCH_NAME, name);
	}
    if (version != NULL) {
    	buildSendPayload(msg, nc.nodeId, GATEWAY_ADDRESS, NODE_SENSOR_ID, C_INTERNAL, I_SKETCH_VERSION, version);
    }
}

void MySensor::request(uint8_t childSensorId, uint8_t variableType, uint8_t destination) {
	buildSend(msg, nc.nodeId, destination, childSensorId, C_REQ, variableType);
}

void MySensor::requestTime(void (* _timeCallback)(unsigned long)) {
	timeCallback = _timeCallback;
	buildSend(msg, nc.nodeId, GATEWAY_ADDRESS, NODE_SENSOR_ID, C_INTERNAL, I_TIME);
}


boolean MySensor::process() {
	uint8_t pipe;
	boolean available = RF24::available(&pipe);

	if (!available || pipe>6)
		return false;

	uint8_t len = RF24::getDynamicPayloadSize();
	RF24::read(&msg, len);
	RF24::writeAckPayload(pipe,&pipe, 1 );

	uint8_t valid = validate(msg);
	boolean ok = valid == VALIDATE_OK;

	// Make sure string gets terminated ok for full string messages.
	msg.data[len - HEADER_SIZE ] = '\0';
	debug(PSTR("rx: %d-%d-%d s=%d,c=%d,t=%d,cr=%s: %s\n"),
				msg.sender, msg.last, msg.destination,  msg.sensor, mGetCommand(msg), msg.type, valid==0?"ok":valid==1?"ec":"ev", msg.getString());

	if (!ok)
		return false;

	uint8_t command = mGetCommand(msg);
	uint8_t type = msg.type;
	uint8_t sender = msg.sender;
	uint8_t last = msg.last;
	uint8_t destination = msg.destination;

	if (relayMode && command == C_INTERNAL && type == I_PING) {
		// Relaying nodes should always answer ping messages
		// Wait a random delay of 0-2 seconds to minimize collision
		// between ping ack messages from other relaying nodes
		delay(millis() & 0x3ff);
		build(msg, nc.nodeId, sender, NODE_SENSOR_ID, C_INTERNAL, I_PING_ACK);
		msg.set(nc.distance);
		sendWrite(sender, msg);
		return false;
	} else if (destination == nc.nodeId) {
		// This message is addressed to this node
		if (relayMode && last != nc.parentNodeId) {
			// Message is from one of the child nodes. Add it to routing table.
			addChildRoute(sender, last);
		}

		if (command == C_INTERNAL) {
			if (type == I_PING_ACK) {
				// We've received a reply to our PING message looking for parent.
				uint8_t distance = msg.getByte();
				if (distance<nc.distance-1) {
					// Found a neighbor closer to GW than previously found
					nc.distance = distance + 1;
					nc.parentNodeId = msg.sender;
					eeprom_write_byte((uint8_t*)EEPROM_PARENT_NODE_ID_ADDRESS, nc.parentNodeId);
					eeprom_write_byte((uint8_t*)EEPROM_DISTANCE_ADDRESS, nc.distance);
					debug(PSTR("p=%d, d=%d\n"), nc.parentNodeId, nc.distance);
				}
				return false;
			} else if (sender == GATEWAY_ADDRESS) {
				bool isMetric;

				switch (type) {
				case I_REQUEST_ID:
					if (nc.nodeId == AUTO) {
						nc.nodeId = msg.getByte();
						// Write id to EEPROM
						if (nc.nodeId == AUTO) {
							// sensor net gateway will return max id if all sensor id are taken
							debug(PSTR("full\n"));
							while (1); // Wait here. Nothing else we can do...
						} else {
							RF24::openReadingPipe(CURRENT_NODE_PIPE, TO_ADDR(nc.nodeId));
							eeprom_write_byte((uint8_t*)EEPROM_NODE_ID_ADDRESS, nc.nodeId);
						}
						debug(PSTR("id=%d\n"), nc.nodeId);
					}
					break;
				case I_CONFIG:
					// Pick up configuration from controller (currently only metric/imperial)
					// and store it in eeprom if changed
					isMetric = msg.getByte() == 'M' ;
					if (cc.isMetric != isMetric) {
						cc.isMetric = isMetric;
						eeprom_write_byte((uint8_t*)EEPROM_CONTROLLER_CONFIG_ADDRESS, isMetric);
						//eeprom_write_block((const void*)&cc, (uint8_t*)EEPROM_CONTROLLER_CONFIG_ADDRESS, sizeof(ControllerConfig));
					}
					break;
				case I_CHILDREN:
					if (relayMode && msg.getByte() == 'C') {
						// Clears child relay data for this node
						debug(PSTR("rd=clear\n"));
						for (uint8_t i=0;i< sizeof(childNodeTable); i++) {
							removeChildRoute(i);
						}
						buildSendPayload(msg, nc.nodeId, GATEWAY_ADDRESS, NODE_SENSOR_ID, C_INTERNAL, I_CHILDREN,"");
					}
					break;
				case I_TIME:
					if (timeCallback != NULL) {
						// Deliver time to callback
						timeCallback(msg.getULong());
					}
				}
				return false;
			}
		}
		// Check if sender requests an ack back.
		if (command == C_SET_WITH_ACK) {
			ack = msg;
			mSetCommand(ack,C_SET);
			ack.sender = nc.nodeId;
			ack.destination = msg.sender;
			sendRoute(ack);
			// The library user should not need to care about this ack request. Just treat it as a normal SET.
			mSetCommand(msg,C_SET);
		}
		// Call incoming message callback if available
		if (msgCallback != NULL) {
			msgCallback(msg);
		}
		// Return true if message was addressed for this node...
		return true;
	} else if (relayMode) {
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
		} else if (pipe == CURRENT_NODE_PIPE) {
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
		} else {
			// We're snooped a message directed to gateway from another branch
			// Make sure to remove the sender node from our routing table.
			//
			//    +-----B    <-- Message comes from here
			//  -A
			//    +-----C    <-- We're here
			//
			// So the sender of message should never be in our
			// routing table.
			//
			// We could also end up in this else-statement if message
			// destination has no route.
			// This can happen if a node never has sent any message to
			// sensor net gateway (which will build up routing information in all nodes
			// that gets passed).

			// THIS SHOULD NOT HAPPEN ANY MORE

			/*if (nc.nodeId == GATEWAY_ADDRESS) {
				// We should perhaps inform gateway about the no-route situation?!
				debug(PSTR("ro=?\n"));

			} else {
				//debug(PSTR("Remove child node from routing table.\n"));
				removeChildRoute(sender);
			}*/
		}

	}


	return false;
}


MyMessage MySensor::getLastMessage() {
	return msg;
}


/*
 * calculate CRC8 on MyMessage data taking care of data structure and protocol version
 */
uint8_t MySensor::crc8Message(MyMessage message) {
	uint8_t len = mGetLength(message);
	uint8_t crc = 0x00;
	uint8_t loop_count;
	uint8_t bit_counter;
	uint8_t data;
	uint8_t feedback_bit;
	uint8_t number_of_bytes_to_read = (uint8_t)sizeof(message);

	// Must set crc to a constant value.
	message.crc = 0;

	// fill unused space by zeroes for string data only
	if(len>=0 && len < sizeof(message.data)-1) {
		memset(&message.data[len], 0, sizeof(message.data) - 1 - len);
	}


	for (loop_count = 0; loop_count != number_of_bytes_to_read; loop_count++)
	{
		data = ((uint8_t*)&message)[loop_count];

		bit_counter = 8;
		do {
		  feedback_bit = (crc ^ data) & 0x01;

		  if ( feedback_bit == 0x01 ) {
			crc = crc ^ 0x18;              //0X18 = X^8+X^5+X^4+X^0
		  }
		  crc = (crc >> 1) & 0x7F;
		  if ( feedback_bit == 0x01 ) {
			crc = crc | 0x80;
		  }

		  data = data >> 1;
		  bit_counter--;

		} while (bit_counter > 0);
	}
	return crc;
}


uint8_t MySensor::validate(MyMessage message) {
	uint8_t oldCrc = message.crc;
	uint8_t newCrc = crc8Message(message);

	if(!(mGetVersion(message) == PROTOCOL_VERSION)) return VALIDATE_BAD_VERSION;

	if(!(oldCrc == newCrc)) return VALIDATE_BAD_CRC;
	return VALIDATE_OK;
}

void MySensor::saveState(uint8_t pos, uint8_t value) {
	if (loadState(pos) != value) {
		eeprom_write_byte((uint8_t*)EEPROM_LOCAL_CONFIG_ADDRESS+pos, value);
	}
}
uint8_t MySensor::loadState(uint8_t pos) {
	return eeprom_read_byte((uint8_t*)EEPROM_LOCAL_CONFIG_ADDRESS+pos);
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


#ifdef DEBUG
void MySensor::debugPrint(const char *fmt, ... ) {
	char fmtBuffer[300];
	if (nc.nodeId == GATEWAY_ADDRESS) {
		// prepend debug message to be handled correctly by gw
		Serial.write("0;0;4;11;");
	}
	va_list args;
	va_start (args, fmt );
	vsnprintf_P(fmtBuffer, 300, fmt, args);
	va_end (args);
	if (nc.nodeId == GATEWAY_ADDRESS) {
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

	//Serial.write("0;0;4;11;Mem free:");
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
