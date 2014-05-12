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
	RF24::setAutoAck(1);
	RF24::enableAckPayload();
	if (!RF24::isPVariant()) {
		debug(PSTR("Radio wiring NOT ok or not using P version of NRF24L01.\n"));
		while(1);
	}

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
	childNodeTable = (uint8_t *) malloc(256);
	// Read routing table from EEPROM
	for (unsigned int i=0;i<sizeof(childNodeTable);i++) {
		childNodeTable[i] = EEPROM.read(EEPROM_ROUTES_ADDRESS+i);
	}
}

void MySensor::begin(void (*_msgCallback)(MyMessage), boolean _relayMode, uint8_t _radioId, rf24_pa_dbm_e paLevel, uint8_t channel, rf24_datarate_e dataRate) {
	Serial.begin(BAUD_RATE);

	relayMode = _relayMode;
	msgCallback = _msgCallback;
	radioId = _radioId;

	debug(PSTR("Started %s.\n"), relayMode?"relay":"sensor");

	if (relayMode) {
		setupRelayMode();
	}
	setupRadio(paLevel, channel, dataRate);

	// Fetch relay from EEPROM
	relayId = EEPROM.read(EEPROM_RELAY_ID_ADDRESS);
	// Fetch distance from eeprom
	distance = EEPROM.read(EEPROM_DISTANCE_ADDRESS);

	if (relayId == 0xff) {
		// No relay previously fetched and stored in eeprom. Try to find one.
		findRelay();
	}
	debug(PSTR("Relay=%d, distance=%d\n"), relayId, distance);

	initializeRadioId();

	// Open reading pipe for messages directed to this node (set write pipe to same)
	RF24::openReadingPipe(WRITE_PIPE, TO_ADDR(radioId));
	RF24::openReadingPipe(CURRENT_NODE_PIPE, TO_ADDR(radioId));

	// Send presentation for this radio node
	present(NODE_CHILD_ID, relayMode? S_ARDUINO_RELAY : S_ARDUINO_NODE);

	// Send relay information back to sensor net gateway.
	sendInternal(I_RELAY_NODE, itoa(relayId, convBuffer, 10));
}


uint8_t MySensor::getRadioId() {
	return radioId;
}

void MySensor::initializeRadioId() {
	if (radioId == AUTO) {
		radioId = EEPROM.read(EEPROM_RADIO_ID_ADDRESS);
		if (radioId == 0xFF || radioId == 0) {
			radioId = AUTO;
			debug(PSTR("No radio id found in EEPROM fetching one from sensor net gateway\n"));
			// No radio id has been fetched yet ant EEPROM is unwritten.
			// Request new id from sensor net gateway. Use radioId 4095 temporarily
			// to be able to receive my correct nodeId.
			RF24::openReadingPipe(CURRENT_NODE_PIPE, TO_ADDR(radioId));
			radioId = getInternal(I_REQUEST_ID).getByte();
			// Write id to EEPROM
			if (radioId == AUTO) { // sensor net gateway will return max id if all sensor id are taken
				debug(PSTR("Sensor network is full! You already have the maximum of sensors!\n"));
				while (1) {} // Wait here. Nothing else we can do...
			} else {
				debug(PSTR("Radio id received: %d\n"), radioId);
				EEPROM.write(EEPROM_RADIO_ID_ADDRESS, radioId);
			}
		}
		debug(PSTR("Radio id stored in EEPROM was: %d\n"), radioId);
	}
}


void MySensor::findRelay() {
	// This can be improved by using status = read_register(OBSERVE_TX,&observe_tx,1) on the
	// sending side.. the problem is getting this information here without too much fuss.
	// Try to find relay 3 times before giving up
	if (radioId == GATEWAY_ADDRESS)
		return; // Gateway has no business here!

	failedTransmissions = 0;

	// Open a reading pipe for current radioId (if it differs from broadcast)
	if (radioId != BROADCAST_PIPE) {
		RF24::openReadingPipe(CURRENT_NODE_PIPE, TO_ADDR(radioId));
		debug(PSTR("Open ping reading pipe: %d\n"), radioId);
	}
	distance = 255;
	uint8_t oldRelayId = relayId;
	uint8_t retries=0;
	while (distance == 255 && retries<FIND_RELAY_RETRIES) {
		// Send ping message to BROADCAST_ADDRESS (to which all nodes listens and should reply to)
		msg.build(radioId, BROADCAST_ADDRESS, NODE_CHILD_ID, C_INTERNAL, I_PING);
		sendWrite(BROADCAST_ADDRESS, msg);
		// Wait for replies for max 10 seconds (or when buffer for all relay nodes have been filled up)
		unsigned long enter = millis();
		uint8_t neighborDistanceToGW;

		// Wait for ack responses 5 seconds
		while (millis() - enter < 5000) {
			if (process()) {
				if (msg.getCommand() == C_INTERNAL &&
						msg.getType() == I_PING_ACK &&
						msg.getDestination() == radioId)  {
					neighborDistanceToGW = msg.getByte();
					if (neighborDistanceToGW<distance-1) {
						// Found a neighbor closer to GW than previously found
						distance = neighborDistanceToGW + 1;
						relayId = msg.getSender();
						debug(PSTR("Using relay %d. Distance is %d\n"), msg.getSender(), neighborDistanceToGW);
						if (neighborDistanceToGW == 0) // We found gateway. Search no more.
							break;
					} else {
						debug(PSTR("Discarded relay %d. Distance is %d\n"), msg.getSender(), neighborDistanceToGW);
					}
				}
			}
		}
		if (distance == 255) {
			debug(PSTR("No relay nodes was found. Trying again in 10 seconds.\n"));
			delay(10000);
		}
		retries++;
	}

	// Store new relay address in EEPROM
	if (relayId != oldRelayId) {
		EEPROM.write(EEPROM_RELAY_ID_ADDRESS, relayId);
		EEPROM.write(EEPROM_DISTANCE_ADDRESS, distance);
	}
}





boolean MySensor::sendAck(MyMessage message) {
	// Build ack message
	ack = message;
//	memcpy((void *)ack, (void *)message, sizeof(MyMessage));
	ack.setSender(radioId);
	ack.setDestination(message.getSender());
	return sendRoute(ack);
}


boolean MySensor::sendRoute(MyMessage message) {
	if (relayMode) {
		uint8_t dest = message.getDestination();
		uint8_t route = getChildRoute(dest);
		if (route>0 && route<255 && dest != GATEWAY_ADDRESS) {
			debug(PSTR("Routing message to %d.\n"), route);
			// Message destination is not gateway and is in routing table for this node.
			// Send it downstream
			return sendWrite(route, message);
		}
	}

	if (radioId != GATEWAY_ADDRESS) {
		debug(PSTR("Relaying message back to gateway.\n"));
		// Should be routed back to gateway.
		bool ok = sendWrite(relayId, message);

		if (!ok) {
			// Failure when sending to relay node. The relay node might be down and we
			// need to find another route to gateway. Max 20 retries before giving up.
			if (failedTransmissions > FIND_RELAY_AFTER_FAILED_TRANSMISSIONS) {
				findRelay();
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
	message.setLast(radioId);
	message.setCRC(crc8Message(message));
	// Only print payload if string type
	debug(PSTR("Tx: fr=%d,to=%d,la=%d,ne=%d,ci=%d,mt=%d,ty=%d,cr=%d:%s\n"),
			message.getSender(),message.getDestination(), message.getLast(), next, message.getSensor(), message.getCommand(), message.getType(),  message.getCRC(), message.getString());

	bool broadcast =  message.getCommand() == C_INTERNAL &&  message.getType() == I_PING;

	// Make sure radio has powered up
	RF24::powerUp();
	RF24::stopListening();
	RF24::openWritingPipe(TO_ADDR(next));
	bool ok = RF24::write(&message, min(MAX_MESSAGE_LENGTH, HEADER_SIZE + length), broadcast);
	RF24::startListening();

	if (ok)
		debug(PSTR("Sent successfully\n"));
	else
		debug(PSTR("Send failed\n"));


	return ok;
}

void MySensor::sendInternal(uint8_t variableType, const char *value) {
	sendRoute(msg.build(radioId, GATEWAY_ADDRESS, NODE_CHILD_ID, C_INTERNAL, variableType).set(value));
}


bool MySensor::send(MyMessage message, bool enableAck, uint8_t retry) {
	message.setSender(radioId);
	if (enableAck) {
		message.setCommand(C_SET_WITH_ACK);
		return wait(message, C_SET_VARIABLE, retry).getCommand() != C_FAILED;
	} else {
		return sendRoute(message);;
	}
}


void MySensor::present(uint8_t childSensorId, uint8_t sensorType) {
	sendRoute(msg.build(radioId, GATEWAY_ADDRESS, childSensorId, C_PRESENTATION, sensorType).set(LIBRARY_VERSION));
}

void MySensor::sketchInfo(const char *name, const char *version) {
	if (name != NULL)
		sendInternal(I_SKETCH_NAME, name);
    if (version != NULL)
    	sendInternal(I_SKETCH_VERSION, version);
}

MyMessage MySensor::get( uint8_t childSensorId, uint8_t variableType, uint8_t destination, uint8_t retry) {
	msg.build(radioId, destination, childSensorId, C_REQ_VARIABLE, variableType);
	return wait(msg,  C_SET_VARIABLE, retry);
}

void MySensor::request(uint8_t childSensorId, uint8_t variableType, uint8_t destination) {
	process(); // Make sure to process any queued messages before sending new request
	sendRoute(msg.build(radioId, destination, childSensorId, C_REQ_VARIABLE, variableType));
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
	msg.build(radioId, GATEWAY_ADDRESS, NODE_CHILD_ID, C_INTERNAL, variableType);
	return wait(msg, C_INTERNAL, retry);
}

/*unsigned long MySensor::getTime(uint8_t retry) {
	MyMessage message = getInternal(I_TIME, retry);
	return message == NULL ? 0UL : message.getULong();
}*/

void MySensor::requestTime() {
	sendRoute(msg.build(radioId, GATEWAY_ADDRESS, NODE_CHILD_ID, C_INTERNAL, I_TIME));
}


/*bool MySensor::isMetricSystem(uint8_t retry) {
	MyMessage message = getInternal(I_UNIT, retry);
	return (message == NULL)
		return true;
	else
		return message.getByte() == 'M';
}*/

void MySensor::requestIsMetricSystem() {
	sendRoute(msg.build(radioId, GATEWAY_ADDRESS, NODE_CHILD_ID, C_INTERNAL, I_UNIT));
}


boolean MySensor::process() {
	uint8_t pipe;
	boolean available = RF24::available(&pipe);

	if (available) {
		debug(PSTR("Message available on pipe %d\n"),pipe);
	}

	if (available && pipe<7) {
		uint8_t len = RF24::getDynamicPayloadSize();
		RF24::read(&msg, len);
		RF24::writeAckPayload(pipe,&pipe, 1 );

		uint8_t valid = validate(msg);
		boolean ok = valid == VALIDATE_OK;

		// Make sure string gets terminated ok for full string messages.
		msg.data[len - HEADER_SIZE ] = '\0';
		debug(PSTR("Rx: fr=%d,to=%d,la=%d,ci=%d,mt=%d,t=%d,cr=%d(%s): %s\n"),
					msg.getSender(),msg.getDestination(), msg.getLast(), msg.getSensor(), msg.getCommand(), msg.getType(), msg.getCRC(), valid==0?"ok":valid==1?"ec":"ev", msg.getString());

		if (ok) {
			uint8_t command = msg.getCommand();
			uint8_t type = msg.getType();
			uint8_t sender = msg.getSender();
			uint8_t last = msg.getLast();
			uint8_t destination = msg.getDestination();

			if (relayMode && command == C_INTERNAL && type == I_PING) {
				// Relays should always answer ping messages
				// Wait a random delay of 0-2 seconds to minimize collision
				// between ping ack messages from other relaying nodes
				randomSeed(millis());
				delay(random(2000));
				uint8_t replyTo = msg.getSender();
				msg.build(radioId, replyTo, NODE_CHILD_ID, C_INTERNAL, I_PING_ACK).set(distance);
				sendWrite(replyTo, msg);
			} else if (sender == radioId) {
				// This message is addressed to this node
				if (relayMode && command == C_INTERNAL) {
					// Handle all internal messages to this node
					if (type == I_RELAY_NODE && destination != GATEWAY_ADDRESS) {
						// Someone at sensor net gateway side wants this node to refresh its relay node
						findRelay();
						// Send relay information back to sensor net gateway node device.
						sendInternal(I_RELAY_NODE, ltoa(relayId, convBuffer, 10));
						return false;
					} else if (type == I_CHILDREN && destination != GATEWAY_ADDRESS) {
						debug(PSTR("Route command received"));
						if (msg.getByte() == 'F') {
							// Send in as many children we can fit in a message (binary)
							sendChildren();
						} else if (msg.getByte() == 'C') {
							// Clears child relay data for this node
							clearChildRoutes();
						}
						return false;
					}
					// Return rest of internal messages back to sketch...
					if (last != GATEWAY_ADDRESS)
						addChildRoute(sender, last);

					return true;
				} else {
					// If this is variable message from sensor net gateway. Send ack back.
					debug(PSTR("Message addressed for this node.\n"));

					// Send set-message back to sender if sender wants this
					if (command == C_SET_WITH_ACK) {
						sendAck(msg);
						// The library user should not need to care about this ack request. Just treat it as a normal SET.
						msg.setCommand(C_SET_VARIABLE);
					}
					// Call incoming message callback if available
					if (msgCallback != NULL) {
						msgCallback(msg);
					}

					if (last != GATEWAY_ADDRESS)
						addChildRoute(sender, last);

					// Return true if message was addressed for this node...
					return true;
				}
			} else if (relayMode) {
				// We should try to relay this message to another node

				uint8_t route = getChildRoute(msg.getDestination());
				if (route>0 && route<255) {
					debug(PSTR("Routing message to child node.\n"));
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

					debug(PSTR("Routing message to relay.\n"));
					// This message should be routed back towards sensor net gateway
					sendWrite(relayId, msg);
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
					if (radioId == GATEWAY_ADDRESS) {
						// We should perhaps inform gateway about the no-route situation?!
						debug(PSTR("Unknown route from GW\n"));

					} else {
						debug(PSTR("Remove child node from routing table.\n"));
						removeChildRoute(sender);
					}
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
	if (childNodeTable[childId] != route) {
		childNodeTable[childId] = route;
		EEPROM.write(EEPROM_ROUTES_ADDRESS+childId, route);
	}
}

void MySensor::removeChildRoute(uint8_t childId) {
	if (childNodeTable[childId] != 0xff) {
		childNodeTable[childId] = 0xff;
		EEPROM.write(EEPROM_ROUTES_ADDRESS+childId, 0xff);
	}
}

uint8_t MySensor::getChildRoute(uint8_t childId) {
	return childNodeTable[childId];
}

void MySensor::clearChildRoutes() {
	debug(PSTR("Clear child routing data\n"));
	for (unsigned int i=0;i< sizeof(childNodeTable); i++) {
		removeChildRoute(i);
	}
	sendInternal(I_CHILDREN, "");
}

void MySensor::sendChildren() {
	// Send in which children that is using this node as an relay.
	// TODO: Fix this
	debug(PSTR("Dump beginning of routing table\n"));

	for (int i=0;i< 50; i++) {
//		Serial.println(childNodeTable[i]);
		debug(PSTR("rt:%d, %d\n"), i, getChildRoute(i) );
	}

	sendInternal(I_CHILDREN, "not implemented");
}





#ifdef DEBUG
void MySensor::debugPrint(const char *fmt, ... ) {
	char fmtBuffer[300];
	if (radioId == GATEWAY_ADDRESS) {
		// prepend debug message to be handled correctly by gw
		Serial.print("0;0;4;11;");
	}
	va_list args;
	va_start (args, fmt );
	vsnprintf_P(fmtBuffer, 300, fmt, args);
	va_end (args);
	if (radioId == GATEWAY_ADDRESS) {
		// Truncate message if this is gateway node
		vsnprintf_P(fmtBuffer, 60, fmt, args);
		fmtBuffer[59] = '\n';
		fmtBuffer[60] = '\0';
	} else {
		vsnprintf_P(fmtBuffer, 299, fmt, args);
	}
	va_end (args);
	Serial.print(fmtBuffer);


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
