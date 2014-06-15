 /*
 The MySensors library adds a new layer on top of the RF24 library.
 It handles radio network routing, relaying and ids.

 Created by Henrik Ekblad <henrik.ekblad@gmail.com>

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 2 as published by the Free Software Foundation.
 */

#include "MySensor.h"
#include "LowPower.h"


// Inline function and macros
inline MyMessage& build (MyMessage &msg, uint8_t sender, uint8_t destination, uint8_t sensor, uint8_t command, uint8_t type, bool enableAck) {
	msg.sender = sender;
	msg.destination = destination;
	msg.sensor = sensor;
	msg.type = type;
	mSetCommand(msg,command);
	mSetAck(msg,enableAck);
	return msg;
}


MySensor::MySensor(uint8_t _cepin, uint8_t _cspin) : RF24(_cepin, _cspin) {
}


void MySensor::begin(void (*_msgCallback)(const MyMessage &), uint8_t _nodeId, boolean _repeaterMode, rf24_pa_dbm_e paLevel, uint8_t channel, rf24_datarate_e dataRate) {
	Serial.begin(BAUD_RATE);
	isGateway = false;
	repeaterMode = _repeaterMode;
	msgCallback = _msgCallback;

	if (repeaterMode) {
		setupRepeaterMode();
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

	debug(PSTR("%s started, id %d\n"), repeaterMode?"repeater":"sensor", nc.nodeId);

	// Open reading pipe for messages directed to this node (set write pipe to same)
	RF24::openReadingPipe(WRITE_PIPE, TO_ADDR(nc.nodeId));
	RF24::openReadingPipe(CURRENT_NODE_PIPE, TO_ADDR(nc.nodeId));

	// Send presentation for this radio node (attach
	present(NODE_SENSOR_ID, repeaterMode? S_ARDUINO_REPEATER_NODE : S_ARDUINO_NODE);

	// Send a configuration exchange request to controller
	// Node sends parent node. Controller answers with latest node configuration
	// which is picked up in process()
	sendRoute(build(msg, nc.nodeId, GATEWAY_ADDRESS, NODE_SENSOR_ID, C_INTERNAL, I_CONFIG, false).set(nc.parentNodeId));

	// Wait configuration reply.
	waitForReply();
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

	// All nodes listen to broadcast pipe (for PING-ACK messages)
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
	waitForReply();
}


void MySensor::findParentNode() {
	failedTransmissions = 0;

	// Set distance to max
	nc.distance = 255;

	// Send ping message to BROADCAST_ADDRESS (to which all relaying nodes and gateway listens and should reply to)
	build(msg, nc.nodeId, BROADCAST_ADDRESS, NODE_SENSOR_ID, C_INTERNAL, I_PING, false).set("");
	sendWrite(BROADCAST_ADDRESS, msg, true);

	// Wait for ping response.
	waitForReply();
}

void MySensor::waitForReply() {
	unsigned long enter = millis();
	// Wait a couple of seconds for response
	while (millis() - enter < 2000) {
		process();
	}
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

boolean MySensor::sendWrite(uint8_t next, MyMessage &message, bool broadcast) {
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

	debug(PSTR("send: %d-%d-%d-%d s=%d,c=%d,t=%d, st=%s:%s\n"),
			message.sender,message.last, next, message.destination, message.sensor, mGetCommand(message), message.type, ok?"ok":"fail", message.getString(convBuf));

	return ok;
}


bool MySensor::send(MyMessage &message, bool enableAck) {
	message.sender = nc.nodeId;
	mSetCommand(message,C_SET);
	mSetAck(message,enableAck);
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
	sendRoute(build(msg, nc.nodeId, destination, childSensorId, C_REQ, variableType, false));
}

void MySensor::requestTime(void (* _timeCallback)(unsigned long)) {
	timeCallback = _timeCallback;
	sendRoute(build(msg, nc.nodeId, GATEWAY_ADDRESS, NODE_SENSOR_ID, C_INTERNAL, I_TIME, false));
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
	debug(PSTR("read: %d-%d-%d s=%d,c=%d,t=%d,cr=%s: %s\n"),
				msg.sender, msg.last, msg.destination,  msg.sensor, mGetCommand(msg), msg.type, valid==0?"ok":valid==1?"ec":"ev", msg.getString(convBuf));

	if (!ok)
		return false;

	uint8_t command = mGetCommand(msg);
	uint8_t type = msg.type;
	uint8_t sender = msg.sender;
	uint8_t last = msg.last;
	uint8_t destination = msg.destination;

	if (repeaterMode && command == C_INTERNAL && type == I_PING) {
		// Relaying nodes should always answer ping messages
		// Wait a random delay of 0-2 seconds to minimize collision
		// between ping ack messages from other relaying nodes
		delay(millis() & 0x3ff);
		sendWrite(sender, build(msg, nc.nodeId, sender, NODE_SENSOR_ID, C_INTERNAL, I_PING_ACK, false).set(nc.distance), true);
		return false;
	} else if (destination == nc.nodeId) {
		// Check if sender requests an ack back.
		if (mGetAck(msg)) {
			// Copy message
			ack = msg;
			mSetAck(ack,false); // Reply without ack flag (otherwise we would end up in an eternal loop)
			ack.sender = nc.nodeId;
			ack.destination = msg.sender;
			sendRoute(ack);
		}

		// This message is addressed to this node
		if (repeaterMode && last != nc.parentNodeId) {
			// Message is from one of the child nodes. Add it to routing table.
			addChildRoute(sender, last);
		}

		if (command == C_INTERNAL) {
			if (type == I_PING_ACK && !isGateway) {
				// We've received a reply to a PING message. Check if the distance is
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
				return false;
			} else if (sender == GATEWAY_ADDRESS) {
				bool isMetric;

				switch (type) {
				case I_ID_RESPONSE:
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
					if (repeaterMode && msg.getByte() == 'C') {
						// Clears child relay data for this node
						debug(PSTR("rd=clear\n"));
						for (uint8_t i=0;i< sizeof(childNodeTable); i++) {
							removeChildRoute(i);
						}
						sendRoute(build(msg, nc.nodeId, GATEWAY_ADDRESS, NODE_SENSOR_ID, C_INTERNAL, I_CHILDREN,false).set(""));
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
		// Call incoming message callback if available
		if (msgCallback != NULL) {
			msgCallback(msg);
		}
		// Return true if message was addressed for this node...
		return true;
	} else if (repeaterMode && pipe == CURRENT_NODE_PIPE) {
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


	return false;
}


MyMessage& MySensor::getLastMessage() {
	return msg;
}


/*
 * calculate CRC8 on MyMessage data taking care of data structure and protocol version
 */
uint8_t MySensor::crc8Message(MyMessage &message) {
	uint8_t len = mGetLength(message);
	uint8_t crc = 0x00;
	uint8_t loop_count;
	uint8_t bit_counter;
	uint8_t data;
	uint8_t feedback_bit;
	uint8_t number_of_bytes_to_read = (uint8_t)sizeof(MyMessage);

	// Must set crc to a constant value.
	message.crc = 0;

	// fill unused space by zeroes for string data only
	if(len>=0 && len < sizeof(message.data)) {
		memset(&message.data[len], 0, sizeof(message.data) - len);
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


uint8_t MySensor::validate(MyMessage &message) {
	uint8_t oldCrc = message.crc;
	uint8_t newCrc = crc8Message(message);

	if(!(mGetVersion(message) == PROTOCOL_VERSION)) return VALIDATE_BAD_VERSION;

	if(!(oldCrc == newCrc)) return VALIDATE_BAD_CRC;
	return VALIDATE_OK;
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


float MySensor::getInternalTemp(void)
{
  long result;
  // Read internal temp sensor against 1.1V reference
  ADMUX = _BV(REFS1) | _BV(REFS0) | _BV(MUX3);
  delay(2); // Wait until Vref has settled
  ADCSRA |= _BV(ADSC);
  while (bit_is_set(ADCSRA,ADSC));
  result = ADCL;
  result |= ADCH<<8;
  result = (result - 125) * 1075;

  return static_cast<float>(static_cast<int>(result/1000.)) / 10.;
}

int continueTimer = true;
void wakeUp()	 //place to send the interrupts
{
	continueTimer = false;
}

void MySensor::internalSleep(int ms) {
	while (continueTimer && ms >= 8000) { LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF); ms -= 8000; }
	if (continueTimer && ms >= 4000)    { LowPower.powerDown(SLEEP_4S, ADC_OFF, BOD_OFF); ms -= 4000; }
	if (continueTimer && ms >= 2000)    { LowPower.powerDown(SLEEP_2S, ADC_OFF, BOD_OFF); ms -= 2000; }
	if (continueTimer && ms >= 1000)    { LowPower.powerDown(SLEEP_1S, ADC_OFF, BOD_OFF); ms -= 1000; }
	if (continueTimer && ms >= 500)     { LowPower.powerDown(SLEEP_500MS, ADC_OFF, BOD_OFF); ms -= 500; }
	if (continueTimer && ms >= 250)     { LowPower.powerDown(SLEEP_250MS, ADC_OFF, BOD_OFF); ms -= 250; }
	if (continueTimer && ms >= 125)     { LowPower.powerDown(SLEEP_120MS, ADC_OFF, BOD_OFF); ms -= 120; }
	if (continueTimer && ms >= 64)      { LowPower.powerDown(SLEEP_60MS, ADC_OFF, BOD_OFF); ms -= 60; }
	if (continueTimer && ms >= 32)      { LowPower.powerDown(SLEEP_30MS, ADC_OFF, BOD_OFF); ms -= 30; }
	if (continueTimer && ms >= 16)      { LowPower.powerDown(SLEEP_15Ms, ADC_OFF, BOD_OFF); ms -= 15; }
}

void MySensor::sleep(int ms) {
	RF24::powerDown();
	continueTimer = true;
	internalSleep(ms);
}

void MySensor::sleep(int interrupt, int mode, int ms) {
	RF24::powerDown();
	attachInterrupt(interrupt, wakeUp, mode); //Interrupt on pin 3 for any change in solar power
	if (ms>0) {
		continueTimer = true;
		sleep(ms);
	} else {
		LowPower.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);
	}
	detachInterrupt(interrupt);
}



#ifdef DEBUG
void MySensor::debugPrint(const char *fmt, ... ) {
	char fmtBuffer[300];
	if (isGateway) {
		// prepend debug message to be handled correctly by gw (C_INTERNAL, I_LOG_MESSAGE)
		Serial.write("0;0;4;8;");
	}
	va_list args;
	va_start (args, fmt );
	vsnprintf_P(fmtBuffer, 300, fmt, args);
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
