/*
 The Sensor Net Arduino library adds a new layer on top of the RF24 library.
 It handles radio network routing, relaying and ids.

 Created by Henrik Ekblad <henrik.ekblad@gmail.com>

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.
*/

#include "Relay.h"

Relay::Relay(uint8_t _cepin, uint8_t _cspin) : Sensor(_cepin, _cspin) {
	isRelay = true;
}


void Relay::begin(uint8_t radioId, rf24_pa_dbm_e paLevel, uint8_t channel, rf24_datarate_e dataRate) {
	Sensor::begin(radioId, paLevel, channel, dataRate);
	// Read routing table from EEPROM
	for (unsigned int i=0;i<sizeof(childNodeTable);i++) {
		childNodeTable[i] = EEPROM.read(EEPROM_ROUTES_ADDRESS+i);
	}
}


boolean Relay::sendData(uint8_t from, uint8_t to, uint8_t childId, uint8_t messageType, uint8_t type, const char *data, uint8_t length, boolean binary) {
	bool ok = false;
	if (length < sizeof(msg.data)) {
		uint8_t route = getChildRoute(to);
		if (route>0) {
			debug(PSTR("Found child in routing table. Sending to %d\n"),route);
			buildMsg(from, to, childId, messageType, type, data, length, binary);
			// Found node in route table
			ok = sendWrite(route, msg, length);
		} else if (radioId == GATEWAY_ADDRESS) {
			// If we're GW (no parent...). As a last resort try sending message directly to node.
			debug(PSTR("No route... try sending direct.\n"));
			buildMsg(from, to, childId, messageType, type, data, length, binary);
			// Found node in route table
			ok = sendWrite(to, msg, length);
		} else {
			// We are probably a repeater node which should send message back to relay
			ok = Sensor::sendData(from, to, childId, messageType, type, data, length, binary);
		}
	} else {
		debug(PSTR("Message too large\n"));
	}
	return ok;
}



boolean Relay::send(message_s message, int length) {
	bool ok = true;
	uint8_t route = getChildRoute(msg.header.to);

	if (route>0 && route<255 && message.header.to != GATEWAY_ADDRESS) {
		debug(PSTR("Routing message to %d.\n"), route);
		// Message destination is not gateway and is in routing table for this node.
		// Send it downstream
		ok = sendWrite(route, message, length);


	} else if (radioId != GATEWAY_ADDRESS) {
		debug(PSTR("Sending message back towards gw.\n"));
		// Should be routed back to gateway. Sensor code knows how to do this.
		ok = Sensor::send(message, length);
	}

	if (!ok) {
		debug(PSTR("No ack received.\n"));
	}
	return ok;
}



boolean Relay::messageAvailable() {
	uint8_t pipe;
	boolean available = RF24::available(&pipe);

	if (available) {
		debug(PSTR("Message available on pipe %d\n"),pipe);
	}

	if (available && pipe<7) {
		uint8_t len = RF24::getDynamicPayloadSize()-sizeof(msg.header);
		boolean ok = readMessage();
		if (ok) {
			if (msg.header.messageType == M_INTERNAL &&
				msg.header.type == I_PING) {
					// We always answer ping messages
					// Wait a random delay of 0-2 seconds to minimize collision
					// between ping ack messages from other relaying nodes
					randomSeed(millis());
					delay(random(2000));
					ltoa(distance, convBuffer, 10);
					uint8_t to = msg.header.from;
					debug(PSTR("Answer ping message. %d\n"), strlen(convBuffer));
					buildMsg(radioId, to, NODE_CHILD_ID, M_INTERNAL, I_PING_ACK, convBuffer,strlen(convBuffer), false);
					sendWrite(to, msg, strlen(convBuffer));
			} else if (msg.header.to == radioId) {
				// This message is addressed to this node
				if (msg.header.messageType == M_INTERNAL) {
					// Handle all internal messages to this node
					if (msg.header.type == I_RELAY_NODE && msg.header.to != GATEWAY_ADDRESS) {
						// Someone at sensor net gateway side wants this node to refresh its relay node
						findRelay();
						// Send relay information back to sensor net gateway node device.
						sendInternal(I_RELAY_NODE, ltoa(relayId, convBuffer, 10));
						return false;
					} else if (msg.header.type == I_CHILDREN && msg.header.to != GATEWAY_ADDRESS) {
						debug(PSTR("Route command received"));
						if (strncmp(msg.data,"F", 1) == 0) {
							// Send in as many children we can fit in a message (binary)
							sendChildren();
						} else if (strncmp(msg.data,"C", 1) == 0) {
							// Clears child relay data for this node
							clearChildRoutes();
						}
						return false;
					}
					// Return rest of internal messages back to sketch...
					if (msg.header.last != GATEWAY_ADDRESS)
						addChildRoute(msg.header.from, msg.header.last);

					return true;
				} else {
					// If this is variable message from sensor net gateway. Send ack back.
					debug(PSTR("Message addressed for this node.\n"));
					if (msg.header.from == GATEWAY_ADDRESS &&
						msg.header.messageType == M_SET_VARIABLE) {
						// Send back ack message to sensor net gateway
						sendVariableAck();
					}
					// Return message to waiting sketch...
					if (msg.header.last != GATEWAY_ADDRESS)
						addChildRoute(msg.header.from, msg.header.last);

					return true;
				}
			} else {
				// We should probably try to relay this message
				relayMessage(len, pipe);
			}
		}
	}
	return false;
}




void Relay::relayMessage(uint8_t length, uint8_t pipe) {
	uint8_t route = getChildRoute(msg.header.to);
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
		sendWrite(route, msg, length);
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
		sendWrite(relayId, msg, length);
		// Add this child to our "routing table" if it not already exist
		addChildRoute(msg.header.from, msg.header.last);
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
			removeChildRoute(msg.header.from);
		}
	}
}


void Relay::addChildRoute(uint8_t childId, uint8_t route) {
	if (childNodeTable[childId] != route) {
		childNodeTable[childId] = route;
		EEPROM.write(EEPROM_ROUTES_ADDRESS+childId, route);
	}
}

void Relay::removeChildRoute(uint8_t childId) {
	if (childNodeTable[childId] != 0xff) {
		childNodeTable[childId] = 0xff;
		EEPROM.write(EEPROM_ROUTES_ADDRESS+childId, 0xff);
	}
}

uint8_t Relay::getChildRoute(uint8_t childId) {
	return childNodeTable[childId];
}

void Relay::clearChildRoutes() {
	debug(PSTR("Clear child routing data\n"));
	for (unsigned int i=0;i< sizeof(childNodeTable); i++) {
		removeChildRoute(i);
	}
	sendInternal(I_CHILDREN, "");
}

void Relay::sendChildren() {
	// Send in which children that is using this node as an relay.
	// TODO: Fix this
	debug(PSTR("Send child info to sensor gateway.\n"));

	for (int i=0;i< 10; i++) {
//		Serial.println(childNodeTable[i]);
		debug(PSTR("rt:%d, %d\n"), i, getChildRoute(i) );
	}

	//sendInternal(I_CHILDREN, "not implemented");
}


