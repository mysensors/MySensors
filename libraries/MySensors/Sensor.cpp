 /*
 The Sensor Net Arduino library adds a new layer on top of the RF24 library.
 It handles radio network routing, relaying and ids.

 Created by Henrik Ekblad <henrik.ekblad@gmail.com>

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.
 */

#include "Sensor.h"


Sensor::Sensor(uint8_t _cepin, uint8_t _cspin) : RF24(_cepin, _cspin) {
	isRelay = false;
}


void Sensor::setupRadio(rf24_pa_dbm_e paLevel, uint8_t channel, rf24_datarate_e dataRate) {
	failedTransmissions = 0;

	// Start up the radio library
	RF24::begin();
	RF24::enableDynamicPayloads();
    RF24::setAutoAck(false);
    RF24::setRetries(15, 15);
	RF24::setPALevel(paLevel);
	RF24::setChannel(channel);
	RF24::setDataRate(dataRate);
	RF24::setCRCLength(RF24_CRC_16);

	// All repeater nodes and gateway listen to broadcast pipe (for PING messages)
	if (isRelay) {
		RF24::openReadingPipe(BROADCAST_PIPE, TO_ADDR(BROADCAST_ADDRESS));
	}
}

void Sensor::begin(uint8_t _radioId, rf24_pa_dbm_e paLevel, uint8_t channel, rf24_datarate_e dataRate) {
	Serial.begin(BAUD_RATE);

	radioId = _radioId;

	debug(PSTR("Started %s.\n"), isRelay?"relay":"sensor");

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

	// Open reading pipe for messages directed to this node
	RF24::openReadingPipe(CURRENT_NODE_PIPE, TO_ADDR(radioId));

	// Send presentation for this radio node
	sendSensorPresentation(NODE_CHILD_ID, isRelay? S_ARDUINO_RELAY : S_ARDUINO_NODE);

	// Send relay information back to sensor net gateway.
	sendInternal(I_RELAY_NODE, itoa(relayId, convBuffer, 10));

	// Initialize ack message header
	ack.header.version = PROTOCOL_VERSION;
	ack.header.binary = 0;
	ack.header.from = radioId;
	ack.header.to = GATEWAY_ADDRESS;
	ack.header.messageType = M_SET_VARIABLE;
}


uint8_t Sensor::getRadioId() {
	return radioId;
}

void Sensor::initializeRadioId() {
	if (radioId == AUTO) {
		radioId = EEPROM.read(EEPROM_RADIO_ID_ADDRESS);
		if (radioId == 0xFF || radioId == 0) {
			radioId = AUTO;
			debug(PSTR("No radio id found in EEPROM fetching one from sensor net gateway\n"));
			// No radio id has been fetched yet ant EEPROM is unwritten.
			// Request new id from sensor net gateway. Use radioId 4095 temporarily
			// to be able to receive my correct nodeId.
			RF24::openReadingPipe(CURRENT_NODE_PIPE, TO_ADDR(radioId));
			radioId = atoi(getInternal(I_REQUEST_ID));
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


void Sensor::findRelay() {
	// This can be improved by using status = read_register(OBSERVE_TX,&observe_tx,1) on the
	// sending side.. the problem is getting this information here without too much fuss.
	// Stay in this method until one or more relay nodes answers ping message.
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
	while (distance == 255) {
		// Send ping message to BROADCAST_ADDRESS (to which all nodes listens and should reply to)
		buildMsg(radioId, BROADCAST_ADDRESS, NODE_CHILD_ID, M_INTERNAL, I_PING, "", 0, false);
		sendWrite(BROADCAST_ADDRESS, msg, 0);
		// Wait for replies for max 10 seconds (or when buffer for all relay nodes have been filled up)
		unsigned long enter = millis();
		uint8_t neighborDistanceToGW;

		// Wait for ack responses 5 seconds
		while (millis() - enter < 5000) {
			if (messageAvailable()) {
				if (msg.header.messageType == M_INTERNAL &&
						msg.header.type == I_PING_ACK &&
						msg.header.to == radioId)  {
					neighborDistanceToGW = atoi(msg.data);
					if (neighborDistanceToGW<distance-1) {
						// Found a neighbor closer to GW than previously found
						distance = neighborDistanceToGW + 1;
						relayId = msg.header.from;
						debug(PSTR("Using relay %d. Distance is %d\n"), msg.header.from, neighborDistanceToGW);
						if (neighborDistanceToGW == 0) // We found gateway. Search no more.
							break;
					} else {
						debug(PSTR("Discarded relay %d. Distance is %d\n"), msg.header.from, neighborDistanceToGW);
					}
				}
			}
		}
		if (distance == 255) {
			debug(PSTR("No relay nodes was found. Trying again in 10 seconds.\n"));
			delay(10000);
		}
	}

	// Store new relay address in EEPROM
	if (relayId != oldRelayId) {
		EEPROM.write(EEPROM_RELAY_ID_ADDRESS, relayId);
		EEPROM.write(EEPROM_DISTANCE_ADDRESS, distance);
	}
}


void Sensor::buildMsg(uint8_t from, uint8_t to, uint8_t childId, uint8_t messageType, uint8_t type, const char *data, uint8_t length, boolean binary) {
	msg.header.version = PROTOCOL_VERSION;
	msg.header.binary = binary;
	msg.header.from = from;
	msg.header.to = to;
	msg.header.childId = childId;
	msg.header.messageType = messageType;
	msg.header.type = type;
	strncpy(msg.data, data, length);
	if(length < sizeof(msg.data)-1) {
		memset(&msg.data[length], 0, sizeof(msg.data) - 1 - length);
	}
}


boolean Sensor::sendData(uint8_t from, uint8_t to, uint8_t childId, uint8_t messageType, uint8_t type, const char *data, uint8_t length, boolean binary) {
	if (length < sizeof(msg.data)) {
		buildMsg(from, to, childId, messageType, type, data, length, binary);
		return send(msg,length);
	} else {
		debug(PSTR("Message too large\n"));
	}
	return false;
}

boolean Sensor::sendVariableAck() {
	ack.header.childId = msg.header.childId;
	ack.header.type = msg.header.type;
	strncpy(ack.data, msg.data, sizeof(ack.data)-1);  // will terminate properly not exceeding message.data
	return send(ack,strlen(ack.data));
}

boolean Sensor::send(message_s message, int length) {
	debug(PSTR("Relaying message back to gateway.\n"));

	// We're a sensor node. Always send messages back to relay node
	bool ok = sendWrite(relayId, message, length);

	if (!ok && radioId != GATEWAY_ADDRESS) {
		// Failure when sending to relay node. The relay node might be down and we
		// need to find another route to gateway. Max 20 retries before giving up.
		if (failedTransmissions > FIND_RELAY_RETRIES) {
			findRelay();
		}
		failedTransmissions++;
	} else {
		failedTransmissions = 0;
	}
	return ok;
}


boolean Sensor::sendWrite(uint8_t dest, message_s message, int length) {

	message.header.last = radioId;
	message.header.crc = crc8Message(message, length);
	debug(PSTR("Tx: fr=%d,to=%d,la=%d,ne=%d,ci=%d,mt=%d,ty=%d,cr=%d: %s\n"),
			message.header.from,message.header.to, message.header.last, dest, message.header.childId, message.header.messageType, message.header.type,  message.header.crc, message.data);

	bool ok = true;
	bool broadcast =  message.header.messageType == M_INTERNAL &&  message.header.type == I_PING;
//	int retry = WRITE_RETRY;
	RF24::stopListening();
	RF24::openWritingPipe(TO_ADDR(dest));
	RF24::write(&message, min(MAX_MESSAGE_LENGTH, sizeof(message.header) + length), broadcast);
	RF24::closeReadingPipe(WRITE_PIPE); // Stop listening to write-pipe after transmit
	RF24::startListening();

	if (!broadcast) {
		// ---------------- WAIT FOR ACK ------------------
		 unsigned long startedWaiting = millis();
		 bool timeout = false;
		 // Wait for ack message maximum 50 ms
		 while ( !RF24::available() && !timeout ) {
			if (millis() - startedWaiting > ACK_MAX_WAIT ) {
				timeout = true;
				debug(PSTR("Ack: receive timeout\n"));
				ok = false;
			}
		 }
		 // Check payload size and content
		 if (!timeout) {
		   // Check payload size and content
		   if (RF24::getDynamicPayloadSize()==sizeof(uint8_t)) {
			 uint8_t idest;
			 RF24::read( &idest, sizeof(uint8_t));
			 if (dest != idest) {
				 debug(PSTR("Ack: received ack from the wrong sensor\n"));
				 ok = false;
			 } else {
				 debug(PSTR("Ack: received OK\n"));
			 }
		   } else {
			   ok = false;
			   debug(PSTR("Ack: received none ack msg.\n"));
		   }
		}
	}


	return ok;
}

void Sensor::sendInternal(uint8_t variableType, const char *value) {
	sendData(radioId, GATEWAY_ADDRESS, NODE_CHILD_ID, M_INTERNAL, variableType, value, strlen(value), false);
}

// This will be added in 1.4
/*void Sensor::sendVariable(uint8_t childId, uint8_t variableType,
		const char *value, length) {
	sendData(radioId, GATEWAY_ADDRESS, childId, M_SET_VARIABLE, variableType, value, length, true);
}*/


void Sensor::sendVariable(uint8_t childId, uint8_t variableType,
		const char *value) {
	sendData(radioId, GATEWAY_ADDRESS, childId, M_SET_VARIABLE, variableType, value, strlen(value), false);
}

void Sensor::sendVariable(uint8_t childId, uint8_t variableType, double value, int decimals) {
    sendVariable(childId, variableType, dtostrf(value,2,decimals,convBuffer));
}

void Sensor::sendVariable(uint8_t childId, uint8_t variableType, int value) {
    sendVariable(childId, variableType, itoa(value, convBuffer, 10));
}

void Sensor::sendVariable(uint8_t childId, uint8_t variableType, long value) {
    sendVariable(childId, variableType, ltoa(value, convBuffer, 10));
}

void Sensor::sendVariable(uint8_t childId, uint8_t variableType, unsigned long value) {
    sendVariable(childId, variableType, ltoa(value, convBuffer, 10));
}

void Sensor::sendVariable(uint8_t childId, uint8_t variableType, unsigned int value) {
    sendVariable(childId, variableType, ltoa(value, convBuffer, 10));
}

void Sensor::sendVariable(uint8_t nodeId, uint8_t childId, uint8_t variableType,
		const char *value) {
	sendData(radioId, nodeId, childId, M_SET_VARIABLE, variableType, value, strlen(value), false);
}

void Sensor::sendVariable(uint8_t nodeId, uint8_t childId, uint8_t variableType, double value, int decimals) {
    sendVariable(nodeId, childId, variableType, dtostrf(value,2,decimals,convBuffer));
}

void Sensor::sendVariable(uint8_t nodeId, uint8_t childId, uint8_t variableType, int value) {
    sendVariable(nodeId, childId, variableType, itoa(value, convBuffer, 10));
}

void Sensor::sendVariable(uint8_t nodeId, uint8_t childId, uint8_t variableType, long value) {
    sendVariable(nodeId, childId, variableType, ltoa(value, convBuffer, 10));
}

void Sensor::sendVariable(uint8_t nodeId, uint8_t childId, uint8_t variableType, unsigned long value) {
    sendVariable(nodeId, childId, variableType, ltoa(value, convBuffer, 10));
}

void Sensor::sendVariable(uint8_t nodeId, uint8_t childId, uint8_t variableType, unsigned int value) {
    sendVariable(nodeId, childId, variableType, ltoa(value, convBuffer, 10));
}

void Sensor::sendSensorPresentation(uint8_t childId, uint8_t sensorType) {
	sendData(radioId, GATEWAY_ADDRESS, childId, M_PRESENTATION, sensorType, LIBRARY_VERSION, strlen(LIBRARY_VERSION), false);
}

void Sensor::requestStatus(uint8_t childId, uint8_t variableType) {
	sendData(radioId, GATEWAY_ADDRESS, childId, M_REQ_VARIABLE, variableType, "", 0, false);
}

void Sensor::requestStatus(uint8_t nodeId, int8_t childId, uint8_t variableType) {
	sendData(radioId, nodeId, childId, M_REQ_VARIABLE, variableType, "", 0, false);
}

void Sensor::sendSketchInfo(const char *name, const char *version) {
        sendSketchName(name);
        sendSketchVersion(version);
}

void Sensor::sendSketchName(const char *name) {
        if (name != NULL)
                sendInternal(I_SKETCH_NAME, name);
}

void Sensor::sendSketchVersion(const char *version) {
        if (version != NULL)
                sendInternal(I_SKETCH_VERSION, version);
}

void Sensor::sendBatteryLevel(int value) {
	sendInternal(I_BATTERY_LEVEL, ltoa(value, convBuffer, 10));
}

char* Sensor::get(uint8_t nodeId, uint8_t childId, uint8_t sendType, uint8_t receiveType, uint8_t variableType) {
	while (1) {
		sendData(radioId, nodeId, childId, sendType, variableType, "", 0, false);
		uint8_t i = 0;
		while (i < 100) {  // 5 seconds timeout before re-sending status request
			while (messageAvailable()) {
				// Check that it is right type of message and not a routing message
				if (msg.header.messageType == receiveType &&
					 	msg.header.type == variableType &&
						msg.header.childId == childId)  {
					return msg.data;
				}
			}
			delay(50);
			i++;
		}
	}
	return NULL;
}

char* Sensor::getStatus(uint8_t childId, uint8_t variableType) {
	return get(GATEWAY_ADDRESS, childId, M_REQ_VARIABLE, M_ACK_VARIABLE, variableType);
}

char * Sensor::getStatus(uint8_t nodeId, int8_t childId, uint8_t variableType) {
	return get(nodeId, childId, M_REQ_VARIABLE, M_ACK_VARIABLE, variableType);
}



char* Sensor::getInternal(uint8_t variableType) {
	return get(GATEWAY_ADDRESS, NODE_CHILD_ID,  M_INTERNAL, M_INTERNAL, variableType);
}

unsigned long Sensor::getTime() {
	return atol(getInternal(I_TIME));
}

void Sensor::requestTime() {
	sendData(radioId, GATEWAY_ADDRESS, NODE_CHILD_ID, M_INTERNAL, I_TIME, "", 0, false);
}


bool Sensor::isMetricSystem() {
	return getInternal(I_UNIT)[0] == 'M';
}

void Sensor::requestIsMetricSystem() {
	sendData(radioId, GATEWAY_ADDRESS, NODE_CHILD_ID, M_INTERNAL, I_UNIT, "", 0, false);
}


boolean Sensor::messageAvailable() {
	uint8_t pipe;
	boolean available = RF24::available(&pipe);

	if (available) {
		debug(PSTR("Message available on pipe %d\n"),pipe);
	}


	if (available && pipe<7) {
		boolean ok = readMessage();
		if (ok && msg.header.to == radioId) {
			// This message is addressed to this node
			debug(PSTR("Message addressed for this node.\n"));
			if (msg.header.from == GATEWAY_ADDRESS &&
				// If this is variable message from sensor net gateway. Send ack back.
				msg.header.messageType == M_SET_VARIABLE) {
				// Send back ack message to sensor net gateway
				sendVariableAck();
			}
			// Return message to waiting sketch...
			return true;
		}
	}
	return false;
}


message_s Sensor::waitForMessage() {
	while (1) {
		if (messageAvailable()) {
			return msg;
		}
	}
}

message_s Sensor::getMessage() {
	return msg;
}


boolean Sensor::readMessage() {
	uint8_t len = RF24::getDynamicPayloadSize();
	RF24::read(&msg, len);

	if (!(msg.header.messageType==M_INTERNAL && msg.header.type == I_PING_ACK)) {
		delay(ACK_SEND_DELAY); // Small delay here to let other side switch to reading mode
		RF24::stopListening();
		RF24::openWritingPipe(TO_ADDR(msg.header.last));
		RF24::write(&radioId, sizeof(uint8_t));
		RF24::closeReadingPipe(WRITE_PIPE); // Stop listening to write-pipe after transmit
		RF24::startListening();
		debug(PSTR("Sent ack msg to %d\n"), msg.header.last);
	}
	uint8_t valid = validate(len-sizeof(header_s));
	boolean ok = valid == VALIDATE_OK;

	// Make sure string gets terminated ok for full sized messages.
	msg.data[len - sizeof(header_s) ] = '\0';
	debug(PSTR("Rx: fr=%d,to=%d,la=%d,ci=%d,mt=%d,t=%d,cr=%d(%s): %s\n"),
			msg.header.from,msg.header.to, msg.header.last, msg.header.childId, msg.header.messageType, msg.header.type, msg.header.crc, valid==0?"ok":valid==1?"ec":"ev", msg.data);
	return ok;
}

/*
 * calculate CRC8 on message_s data taking care of data structure and protocol version
 */
uint8_t Sensor::crc8Message(message_s var_msg, uint8_t len) {
	uint8_t crc = 0x00;
	uint8_t loop_count;
	uint8_t bit_counter;
	uint8_t data;
	uint8_t feedback_bit;
	uint8_t number_of_bytes_to_read = (uint8_t)sizeof(var_msg);

	// Must set crc to a constant value.
	var_msg.header.crc = 0;

	// fill unused space by zeroes for string data only
	if(len>=0 && len < sizeof(var_msg.data)-1) {
		memset(&var_msg.data[len], 0, sizeof(var_msg.data) - 1 - len);
	}


	for (loop_count = 0; loop_count != number_of_bytes_to_read; loop_count++)
	{
		data = ((uint8_t*)&var_msg)[loop_count];

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


uint8_t Sensor::validate(uint8_t length) {
	uint8_t oldCrc = msg.header.crc;
	uint8_t newCrc = crc8Message(msg, length);

	if(!(msg.header.version == PROTOCOL_VERSION)) return VALIDATE_BAD_VERSION;

	if(!(oldCrc == newCrc)) return VALIDATE_BAD_CRC;
	return VALIDATE_OK;
}

#ifdef DEBUG
void Sensor::debugPrint(const char *fmt, ... ) {
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
int Sensor::freeRam (void) {
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}
#endif
