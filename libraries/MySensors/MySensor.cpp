 /*
 The MySensors library adds a new layer on top of the RF24 library.
 It handles radio network routing, relaying and ids.

 Created by Henrik Ekblad <henrik.ekblad@gmail.com>

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.
 */

#include "MySensor.h"

MySensor::MySensor(uint8_t _cepin, uint8_t _cspin) : RF24(_cepin, _cspin) {
}


void MySensor::setupRadio(rf24_pa_dbm_e paLevel, uint8_t channel, rf24_datarate_e dataRate) {
	failedTransmissions = 0;

	// Start up the radio library
	RF24::begin();

	if (!RF24::isPVariant()) {
		debug(PSTR("err=check wire\n"));
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
	s.childNodeTable = (uint8_t *) malloc(256);
	eeprom_read_block((void*)s.childNodeTable, (void*)EEPROM_ROUTES_ADDRESS, 256);
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
	eeprom_read_block((void*)&s, (void*)EEPROM_START, 3*sizeof(uint8_t));
	if (_nodeId != AUTO) {
		// Set static id
		s.nodeId = _nodeId;
	}

	// If no parent was found. Try to find one.
	if (s.parentNodeId == 0xff) {
		findParentNode();
	}

	// Try to fetch node-id from gateway
	if (s.nodeId == AUTO) {
		requestNodeId();
	}
	debug(PSTR("%s started\n"), relayMode?"relay":"sensor");

	// Open reading pipe for messages directed to this node (set write pipe to same)
	RF24::openReadingPipe(WRITE_PIPE, TO_ADDR(s.nodeId));
	RF24::openReadingPipe(CURRENT_NODE_PIPE, TO_ADDR(s.nodeId));

	// Send presentation for this radio node
	present(NODE_CHILD_ID, relayMode? S_ARDUINO_RELAY : S_ARDUINO_NODE);

	// Send parent information back to sensor net gateway.
	sendInternal(I_RELAY_NODE, itoa(s.parentNodeId, convBuffer, 10));
}


uint8_t MySensor::getNodeId() {
	return s.nodeId;
}

void MySensor::requestNodeId() {
	debug(PSTR("req node id\n"));

	RF24::openReadingPipe(CURRENT_NODE_PIPE, TO_ADDR(s.nodeId));
	sendRoute(msg.build(s.nodeId, GATEWAY_ADDRESS, NODE_CHILD_ID, C_INTERNAL, I_REQUEST_ID));
}


void MySensor::findParentNode() {
	if (s.nodeId == GATEWAY_ADDRESS)
		return; // Gateway has no business here!

	failedTransmissions = 0;

	// Open a reading pipe for current nodeId (if it differs from broadcast)
	if (s.nodeId != BROADCAST_PIPE) {
		RF24::openReadingPipe(CURRENT_NODE_PIPE, TO_ADDR(s.nodeId));
	}
	// Set distance to max
	s.distance = 255;

	// Send ping message to BROADCAST_ADDRESS (to which all relaying nodes and gateway listens and should reply to)
	sendWrite(BROADCAST_ADDRESS, msg.build(s.nodeId, BROADCAST_ADDRESS, NODE_CHILD_ID, C_INTERNAL, I_PING));
	// Wait for replies for max 10 seconds (or when buffer for all parent nodes have been filled up)

	unsigned long enter = millis();
	// Wait for ack responses 5 seconds
	while (millis() - enter < 5000) {
		process();
	}

}





boolean MySensor::sendAck(MyMessage message) {
	// Build ack message
	ack = message;
	ack.setCommand(C_SET_VARIABLE);
	ack.setSender(s.nodeId);
	ack.setDestination(message.getSender());
	return sendRoute(ack);
}


boolean MySensor::sendRoute(MyMessage message) {
	// Make sure to process any incoming messages before sending (could this end up in recursive loop?)
	process();

	// If we still don't have any node id, re-request and skip this message.
	if (s.nodeId == AUTO && !(message.getCommand() == C_INTERNAL && message.getType() == I_REQUEST_ID)) {
		requestNodeId();
		return false;
	}

	if (relayMode) {
		uint8_t dest = message.getDestination();
		uint8_t route = getChildRoute(dest);
		if (route>0 && route<255 && dest != GATEWAY_ADDRESS) {
			debug(PSTR("ro=%d.\n"), route);
			// Message destination is not gateway and is in routing table for this node.
			// Send it downstream
			return sendWrite(route, message);
		}
	}

	if (s.nodeId != GATEWAY_ADDRESS) {
		debug(PSTR("ro=parent\n"));
		// Should be routed back to gateway.
		bool ok = sendWrite(s.parentNodeId, message);

		if (!ok) {
			// Failure when sending to parent node. The parent node might be down and we
			// need to find another 	route to gateway. Max 20 retries before giving up.
			if (failedTransmissions > FIND_RELAY_AFTER_FAILED_TRANSMISSIONS) {
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





boolean MySensor::sendWrite(uint8_t next, MyMessage message) {
	uint8_t length = message.getLength();
	message.setLast(s.nodeId);
	message.setCRC(crc8Message(message));
	// Only print payload if string type
	debug(PSTR("tx: %d-%d-%d-%d s=%d,c=%d,t=%d:%s\n"),
			message.getSender(),message.getLast(), next, message.getDestination(), message.getSensor(), message.getCommand(), message.getType(), message.getString());

	bool broadcast =  message.getCommand() == C_INTERNAL &&  message.getType() == I_PING;

	// Make sure radio has powered up
	RF24::powerUp();
	RF24::stopListening();
	RF24::openWritingPipe(TO_ADDR(next));
	bool ok = RF24::write(&message, min(MAX_MESSAGE_LENGTH, HEADER_SIZE + length), broadcast);
	RF24::startListening();

	if (ok)
		debug(PSTR("s=ok\n"));
	else
		debug(PSTR("s=fail\n"));


	return ok;
}

void MySensor::sendInternal(uint8_t variableType, const char *value) {
	sendRoute(msg.build(s.nodeId, GATEWAY_ADDRESS, NODE_CHILD_ID, C_INTERNAL, variableType).set(value));
}


bool MySensor::send(MyMessage message, bool enableAck, uint8_t retry) {
	message.setSender(s.nodeId);
	if (enableAck) {
		message.setCommand(C_SET_WITH_ACK);
		return wait(message, C_SET_VARIABLE, retry).getCommand() != C_FAILED;
	} else {
		return sendRoute(message);;
	}
}


void MySensor::present(uint8_t childSensorId, uint8_t sensorType) {
	sendRoute(msg.build(s.nodeId, GATEWAY_ADDRESS, childSensorId, C_PRESENTATION, sensorType).set(LIBRARY_VERSION));
}

void MySensor::sketchInfo(const char *name, const char *version) {
	if (name != NULL)
		sendInternal(I_SKETCH_NAME, name);
    if (version != NULL)
    	sendInternal(I_SKETCH_VERSION, version);
}

MyMessage MySensor::get( uint8_t childSensorId, uint8_t variableType, uint8_t destination, uint8_t retry) {
	msg.build(s.nodeId, destination, childSensorId, C_REQ_VARIABLE, variableType);
	return wait(msg,  C_SET_VARIABLE, retry);
}

void MySensor::request(uint8_t childSensorId, uint8_t variableType, uint8_t destination) {
	process(); // Make sure to process any queued messages before sending new request
	sendRoute(msg.build(s.nodeId, destination, childSensorId, C_REQ_VARIABLE, variableType));
}



void MySensor::sendBatteryLevel(int value) {
	sendInternal(I_BATTERY_LEVEL, ltoa(value, convBuffer, 10));
}

MyMessage MySensor::wait(MyMessage message, uint8_t expectedResponseCommand, uint8_t retry) {
	while (retry>0) {

		uint8_t sensor = message.getSensor();
		uint8_t type = message.getType();
		sendRoute(message);
		uint8_t i = 0;
		while (i < 100) {  // 1 seconds timeout before re-sending status request
			while (process()) {
				// Check that it is right type of message and not a routing message
				if (msg.getCommand() == expectedResponseCommand &&
					 	msg.getType() == type &&
						msg.getSensor() == sensor)  {
					return msg;
				}
			}
			delay(10);
			i++;
		}
		retry--;
	}
	msg.setCommand(C_FAILED);
	return msg;
}




MyMessage MySensor::getInternal(uint8_t variableType, uint8_t retry) {
	msg.build(s.nodeId, GATEWAY_ADDRESS, NODE_CHILD_ID, C_INTERNAL, variableType);
	return wait(msg, C_INTERNAL, retry);
}

/*unsigned long MySensor::getTime(uint8_t retry) {
	MyMessage message = getInternal(I_TIME, retry);
	return message == NULL ? 0UL : message.getULong();
}*/

void MySensor::requestTime() {
	sendRoute(msg.build(s.nodeId, GATEWAY_ADDRESS, NODE_CHILD_ID, C_INTERNAL, I_TIME));
}


/*bool MySensor::isMetricSystem(uint8_t retry) {
	MyMessage message = getInternal(I_UNIT, retry);
	return (message == NULL)
		return true;
	else
		return message.getByte() == 'M';
}*/

void MySensor::requestIsMetricSystem() {
	sendRoute(msg.build(s.nodeId, GATEWAY_ADDRESS, NODE_CHILD_ID, C_INTERNAL, I_UNIT));
}


boolean MySensor::process() {
	uint8_t pipe;
	boolean available = RF24::available(&pipe);


	if (available && pipe<7) {
		uint8_t len = RF24::getDynamicPayloadSize();
		RF24::read(&msg, len);
		RF24::writeAckPayload(pipe,&pipe, 1 );

		uint8_t valid = validate(msg);
		boolean ok = valid == VALIDATE_OK;

		// Make sure string gets terminated ok for full string messages.
		msg.data[len - HEADER_SIZE ] = '\0';
		debug(PSTR("rx: %d-%d-%d s=%d,c=%d,t=%d,cr=%s: %s\n"),
					msg.getSender(), msg.getLast(), msg.getDestination(),  msg.getSensor(), msg.getCommand(), msg.getType(), valid==0?"ok":valid==1?"ec":"ev", msg.getString());

		if (ok) {
			uint8_t command = msg.getCommand();
			uint8_t type = msg.getType();
			uint8_t sender = msg.getSender();
			uint8_t last = msg.getLast();
			uint8_t destination = msg.getDestination();

			if (relayMode && command == C_INTERNAL && type == I_PING && s.nodeId != AUTO) {
				// Relaying nodes should always answer ping messages
				// Wait a random delay of 0-2 seconds to minimize collision
				// between ping ack messages from other relaying nodes
				randomSeed(millis());
				delay(random(2000));
				uint8_t replyTo = msg.getSender();
				msg.build(s.nodeId, replyTo, NODE_CHILD_ID, C_INTERNAL, I_PING_ACK).set(s.distance);
				sendWrite(replyTo, msg);
				return false;
			} else if (destination == s.nodeId) {
				// This message is addressed to this node

				if (relayMode && last != s.parentNodeId) {
					// Message is from one of the child nodes. Add it to routing table.
					addChildRoute(sender, last);
				}

				if (command == C_INTERNAL) {
					if (type == I_PING_ACK) {
						uint8_t distance = msg.getByte();
						if (distance<s.distance-1) {
							// Found a neighbor closer to GW than previously found
							s.distance = distance + 1;
							s.parentNodeId = msg.getSender();
							eeprom_write_byte((uint8_t*)EEPROM_PARENT_NODE_ID_ADDRESS, s.parentNodeId);
							eeprom_write_byte((uint8_t*)EEPROM_DISTANCE_ADDRESS, s.distance);
							debug(PSTR("p=%d, d=%d\n"), s.parentNodeId, s.distance);
						}
					} else if (type == I_REQUEST_ID && sender == GATEWAY_ADDRESS) {
						if (s.nodeId == AUTO) {
							s.nodeId = msg.getByte();
							// Write id to EEPROM
							if (s.nodeId == AUTO) {
								// sensor net gateway will return max id if all sensor id are taken
								debug(PSTR("full or no reply\n"));
								while (1); // Wait here. Nothing else we can do...
							} else {
								eeprom_write_byte((uint8_t*)EEPROM_NODE_ID_ADDRESS, s.nodeId);
							}
							debug(PSTR("new node=%d\n"), s.nodeId);
						}
						return false;
					} else if (relayMode && sender == GATEWAY_ADDRESS) {
						if (type == I_RELAY_NODE && destination != GATEWAY_ADDRESS) {
							// Someone at sensor net gateway side wants this node to refresh its relay node
							findParentNode();
							// Send relay information back to sensor net gateway node device.
							sendInternal(I_RELAY_NODE, ltoa(s.parentNodeId, convBuffer, 10));
							return false;
						} else if (type == I_CHILDREN) {
							if (msg.getByte() == 'F') {
								// Send in as many children we can fit in a message (binary)
								sendChildren();
							} else if (msg.getByte() == 'C') {
								// Clears child relay data for this node
								clearChildRoutes();
							}
							return false;
						}
					}
				}
				// If this is variable message from sensor net gateway. Send ack back.
				if (command == C_SET_WITH_ACK) {
					sendAck(msg);
					// The library user should not need to care about this ack request. Just treat it as a normal SET.
					msg.setCommand(C_SET_VARIABLE);
				}
				// Call incoming message callback if available
				if (msgCallback != NULL) {
					msgCallback(msg);
				}
				// Return true if message was addressed for this node...
				return true;
			} else if (relayMode) {
				// We should try to relay this message to another node

				uint8_t route = getChildRoute(msg.getDestination());
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
					sendWrite(s.parentNodeId, msg);
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

					/*if (s.nodeId == GATEWAY_ADDRESS) {
						// We should perhaps inform gateway about the no-route situation?!
						debug(PSTR("ro=?\n"));

					} else {
						//debug(PSTR("Remove child node from routing table.\n"));
						removeChildRoute(sender);
					}*/
				}

			}
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
	uint8_t len = message.getLength();
	uint8_t crc = 0x00;
	uint8_t loop_count;
	uint8_t bit_counter;
	uint8_t data;
	uint8_t feedback_bit;
	uint8_t number_of_bytes_to_read = (uint8_t)sizeof(message);

	// Must set crc to a constant value.
	message.setCRC(0);

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
	uint8_t oldCrc = message.getCRC();
	uint8_t newCrc = crc8Message(message);

	if(!(message.getVersion() == PROTOCOL_VERSION)) return VALIDATE_BAD_VERSION;

	if(!(oldCrc == newCrc)) return VALIDATE_BAD_CRC;
	return VALIDATE_OK;
}


void MySensor::addChildRoute(uint8_t childId, uint8_t route) {
	if (s.childNodeTable[childId] != route) {
		s.childNodeTable[childId] = route;
		eeprom_write_byte((uint8_t*)EEPROM_ROUTES_ADDRESS+childId, route);
	}
}

void MySensor::removeChildRoute(uint8_t childId) {
	if (s.childNodeTable[childId] != 0xff) {
		s.childNodeTable[childId] = 0xff;
		eeprom_write_byte((uint8_t*)EEPROM_ROUTES_ADDRESS+childId, 0xff);
	}
}

uint8_t MySensor::getChildRoute(uint8_t childId) {
	return s.childNodeTable[childId];
}

void MySensor::clearChildRoutes() {
	debug(PSTR("rd=clear\n"));
	for (unsigned int i=0;i< sizeof(s.childNodeTable); i++) {
		removeChildRoute(i);
	}
	sendInternal(I_CHILDREN, "");
}

void MySensor::sendChildren() {
	// Send in which children that is using this node as parent.
	// TODO: Fix this
	debug(PSTR("rd=dump\n"));

	for (int i=0;i< 50; i++) {
//		Serial.println(s.childNodeTable[i]);
		debug(PSTR("rd=%d, %d\n"), i, getChildRoute(i) );
	}

	sendInternal(I_CHILDREN, "nit impl");
}





#ifdef DEBUG
void MySensor::debugPrint(const char *fmt, ... ) {
	char fmtBuffer[300];
	if (s.nodeId == GATEWAY_ADDRESS) {
		// prepend debug message to be handled correctly by gw
		Serial.print("0;0;4;11;");
	}
	va_list args;
	va_start (args, fmt );
	vsnprintf_P(fmtBuffer, 300, fmt, args);
	va_end (args);
	if (s.nodeId == GATEWAY_ADDRESS) {
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

	//Serial.print("0;0;4;11;Mem free:");
	//Serial.println(freeRam());
}
#endif


#ifdef DEBUG
int MySensor::freeRam (void) {
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}
#endif
