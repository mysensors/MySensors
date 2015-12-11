/**
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

#include "MySensorCore.h"

ControllerConfig _cc; // Configuration coming from controller
NodeConfig _nc; // Essential settings for node to work
MyMessage _msg;  // Buffer for incoming messages.
MyMessage _msgTmp; // Buffer for temporary messages (acks and nonces among others).

#ifdef MY_DEBUG
	char _convBuf[MAX_PAYLOAD*2+1];
#endif

uint16_t _heartbeat;
void (*_timeCallback)(unsigned long); // Callback for requested time messages


void _process() {
	hwWatchdogReset();


	#if defined (MY_LEDS_BLINKING_FEATURE)
		ledsProcess();
	#endif

	#if defined(MY_INCLUSION_MODE_FEATURE)
		inclusionProcess();
	#endif

	#if defined(MY_GATEWAY_FEATURE)
		gatewayTransportProcess();
	#endif

	#if defined(MY_RADIO_FEATURE)
		transportProcess();
	#endif
}

#if defined(MY_RADIO_FEATURE)
static inline bool isValidParent( const uint8_t parent ) {
	return parent != AUTO;
}
#endif

void _begin() {
	#if !defined(MY_DISABLED_SERIAL)
	    hwInit();
    #endif
	
	#if defined(MY_SIGNING_SOFT)
		// initialize pseudo-RNG
		randomSeed(analogRead(MY_SIGNING_SOFT_RANDOMSEED_PIN));
	#endif

	debug(PSTR("Starting " MY_NODE_TYPE " (" MY_CAPABILIIES ", " LIBRARY_VERSION ")\n"));

	#if defined(MY_RADIO_FEATURE)
		_failedTransmissions = 0;

		// Setup radio
		if (!transportInit()) {
			debug(PSTR("Radio init failed. Check wiring.\n"));
			// Nothing more we can do
			while(1) {
				#if defined(MY_GATEWAY_ESP8266)
					yield();
				#endif
			};
		} else {
			debug(PSTR("Radio init successful.\n"));
		}
	#endif

	#if defined(MY_GATEWAY_FEATURE)
		#if defined(MY_INCLUSION_BUTTON_FEATURE)
	    	inclusionInit();
		#endif

	    // initialize the transport driver
		if (!gatewayTransportInit()) {
			debug(PSTR("Transport driver init fail\n"));
			// Nothing more we can do
			while(1) {
				#if defined(MY_GATEWAY_ESP8266)
					yield();
				#endif
			}
		}

	#endif


	#if defined(MY_SIGNING_FEATURE)
		// Read out the signing requirements from EEPROM
		hwReadConfigBlock((void*)_doSign, (void*)EEPROM_SIGNING_REQUIREMENT_TABLE_ADDRESS, sizeof(_doSign));
	#endif

	#if defined(MY_LEDS_BLINKING_FEATURE)
		ledsInit();
	#endif

	// Read latest received controller configuration from EEPROM
	hwReadConfigBlock((void*)&_cc, (void*)EEPROM_CONTROLLER_CONFIG_ADDRESS, sizeof(ControllerConfig));
	if (_cc.isMetric == 0xff) {
		// Eeprom empty, set default to metric
		_cc.isMetric = 0x01;
	}

	#if defined(MY_GATEWAY_FEATURE)
		// Set configuration for gateway
		_nc.parentNodeId = GATEWAY_ADDRESS;
		_nc.distance = 0;
		_nc.nodeId = GATEWAY_ADDRESS;
	#elif defined(MY_RADIO_FEATURE)
		// Read settings from eeprom
		hwReadConfigBlock((void*)&_nc, (void*)EEPROM_NODE_ID_ADDRESS, sizeof(NodeConfig));
		// Read latest received controller configuration from EEPROM
		hwReadConfigBlock((void*)&_cc, (void*)EEPROM_CONTROLLER_CONFIG_ADDRESS, sizeof(ControllerConfig));
		#ifdef MY_OTA_FIRMWARE_FEATURE
			// Read firmware config from EEPROM, i.e. type, version, CRC, blocks
			hwReadConfigBlock((void*)&_fc, (void*)EEPROM_FIRMWARE_TYPE_ADDRESS, sizeof(NodeFirmwareConfig));
		#endif

		_autoFindParent = MY_PARENT_NODE_ID == AUTO;
		if (!_autoFindParent) {
			_nc.parentNodeId = MY_PARENT_NODE_ID;
			// Save static parent id in eeprom (used by bootloader)
			hwWriteConfig(EEPROM_PARENT_NODE_ID_ADDRESS, MY_PARENT_NODE_ID);
			// We don't actually know the distance to gw here. Let's pretend it is 1.
			// If the current node is also repeater, be aware of this.
			_nc.distance = 1;
		} else if (!isValidParent(_nc.parentNodeId)) {
			// Auto find parent, but parent in eeprom is invalid. Try find one.
			transportFindParentNode();
		}

		if (MY_NODE_ID != AUTO) {
			// Set static id
			_nc.nodeId = MY_NODE_ID;
			// Save static id in eeprom
			hwWriteConfig(EEPROM_NODE_ID_ADDRESS, MY_NODE_ID);
		} else if (_nc.nodeId == AUTO && isValidParent(_nc.parentNodeId)) {
			// Try to fetch node-id from gateway
			transportRequestNodeId();
		}
	#endif

	// Call sketch setup
	if (setup)
		setup();


	#if defined(MY_RADIO_FEATURE)
		transportPresentNode();
	#endif
	if (presentation)
		presentation();
	debug(PSTR("Init complete, id=%d, parent=%d, distance=%d\n"), _nc.nodeId, _nc.parentNodeId, _nc.distance);
}




uint8_t getNodeId() {
	return _nc.nodeId;
}

ControllerConfig getConfig() {
	return _cc;
}


boolean _sendRoute(MyMessage &message) {
	#if defined(MY_GATEWAY_FEATURE)
		if (message.destination == _nc.nodeId) {
			// This is a message sent from a sensor attached on the gateway node.
			// Pass it directly to the gateway transport layer.
			ledBlinkTx(1);
			return gatewayTransportSend(message);
		}
	#endif
	#if defined(MY_RADIO_FEATURE)
		return transportSendRoute(message);
	#else
		return false;
	#endif
}

bool send(MyMessage &message, bool enableAck) {
	message.sender = _nc.nodeId;
	mSetCommand(message,C_SET);
    mSetRequestAck(message,enableAck);
	return _sendRoute(message);
}

void sendBatteryLevel(uint8_t value, bool enableAck) {
	_sendRoute(build(_msg, _nc.nodeId, GATEWAY_ADDRESS, NODE_SENSOR_ID, C_INTERNAL, I_BATTERY_LEVEL, enableAck).set(value));
}

void sendHeartbeat(void) {
	_sendRoute(build(_msg, _nc.nodeId, GATEWAY_ADDRESS, NODE_SENSOR_ID, C_INTERNAL, I_HEARTBEAT, false).set(_heartbeat++));

}

void present(uint8_t childSensorId, uint8_t sensorType, const char *description, bool enableAck) {
	_sendRoute(build(_msg, _nc.nodeId, GATEWAY_ADDRESS, childSensorId, C_PRESENTATION, sensorType, enableAck).set(childSensorId==NODE_SENSOR_ID?LIBRARY_VERSION:description));
}

void sendSketchInfo(const char *name, const char *version, bool enableAck) {
	if (name != NULL) {
		_sendRoute(build(_msg, _nc.nodeId, GATEWAY_ADDRESS, NODE_SENSOR_ID, C_INTERNAL, I_SKETCH_NAME, enableAck).set(name));
	}
    if (version != NULL) {
    	_sendRoute(build(_msg, _nc.nodeId, GATEWAY_ADDRESS, NODE_SENSOR_ID, C_INTERNAL, I_SKETCH_VERSION, enableAck).set(version));
    }
}

void request(uint8_t childSensorId, uint8_t variableType, uint8_t destination) {
	_sendRoute(build(_msg, _nc.nodeId, destination, childSensorId, C_REQ, variableType, false).set(""));
}

void requestTime() {
	_sendRoute(build(_msg, _nc.nodeId, GATEWAY_ADDRESS, NODE_SENSOR_ID, C_INTERNAL, I_TIME, false).set(""));
}

// Message delivered through _msg
void _processInternalMessages() {
	bool isMetric;
	uint8_t type = _msg.type;

	#if !defined(MY_DISABLE_REMOTE_RESET)
		if (type == I_REBOOT) {
			// Requires MySensors or other bootloader with watchdogs enabled
			hwReboot();
		} else
	#endif
		if (type == I_CONFIG) {
		// Pick up configuration from controller (currently only metric/imperial)
		// and store it in eeprom if changed
	 	if (_msg.getString() == NULL) {
			isMetric = true;
		} else {
			isMetric = _msg.getString()[0] == 'M';
		}
		_cc.isMetric = isMetric;
		hwWriteConfig(EEPROM_CONTROLLER_CONFIG_ADDRESS, isMetric);
	} else if (type == I_PRESENTATION) {
		if (!mGetAck(_msg)) {
			// Re-send node presentation to controller
			#if defined(MY_RADIO_FEATURE)
				transportPresentNode();
			#endif
			if (presentation)
				presentation();
		}
	} else if (type == I_HEARTBEAT) {
		if (!mGetAck(_msg)) {
			// Send heartbeat ack message back to sender (with the same payload)
			_msgTmp = _msg;
			mSetRequestAck(_msgTmp,false); // Reply without ack flag (otherwise we would end up in an eternal loop)
			mSetAck(_msgTmp,true);
			_msgTmp.sender = _nc.nodeId;
			_msgTmp.destination = _msg.sender;
			_sendRoute(_msgTmp);
		}
	} else if (type == I_TIME && receiveTime) {
		// Deliver time to callback
		if (receiveTime)
			receiveTime(_msg.getULong());
	}
	#if defined(MY_REPEATER_FEATURE)
		if (type == I_CHILDREN && _msg.getString()[0] == 'C') {
			// Clears child relay data for this node
			debug(PSTR("clear\n"));
			uint8_t i = 255;
			do {
				hwWriteConfig(EEPROM_ROUTES_ADDRESS+i, 0xff);
			} while (i--);
			// Clear parent node id & distance to gw
			hwWriteConfig(EEPROM_PARENT_NODE_ID_ADDRESS, 0xFF);
			hwWriteConfig(EEPROM_DISTANCE_ADDRESS, 0xFF);
			// Find parent node
			transportFindParentNode();
			_sendRoute(build(_msg, _nc.nodeId, GATEWAY_ADDRESS, NODE_SENSOR_ID, C_INTERNAL, I_CHILDREN,false).set(""));
		}
	#endif
}


void saveState(uint8_t pos, uint8_t value) {
	hwWriteConfig(EEPROM_LOCAL_CONFIG_ADDRESS+pos, value);
}
uint8_t loadState(uint8_t pos) {
	return hwReadConfig(EEPROM_LOCAL_CONFIG_ADDRESS+pos);
}


void wait(unsigned long ms) {
	unsigned long enter = hwMillis();
	while (hwMillis() - enter < ms) {
		_process();
		#if defined(MY_GATEWAY_ESP8266)
			yield();
		#endif
	}
}

void sleep(unsigned long ms) {
	#if defined(MY_OTA_FIRMWARE_FEATURE)
		if (_fwUpdateOngoing) {
			// Do not sleep node while fw update is ongoing
			_process();
			return;
		} else {
	#endif
		#if defined(MY_RADIO_FEATURE)
			transportPowerDown();
		#endif
			hwSleep(ms);
	#if defined(MY_OTA_FIRMWARE_FEATURE)
		}
	#endif
}

void smartSleep(unsigned long ms) {
	wait(MY_SMART_SLEEP_WAIT_DURATION);
	sleep(ms);
	sendHeartbeat();
}

bool sleep(uint8_t interrupt, uint8_t mode, unsigned long ms) {
	#if defined(MY_OTA_FIRMWARE_FEATURE)
	if (_fwUpdateOngoing) {
		// Do not sleep node while fw update is ongoing
		return false;
	} else {
	#endif
		#if defined(MY_RADIO_FEATURE)
			transportPowerDown();
		#endif
		return hwSleep(interrupt, mode, ms) ;
	#if defined(MY_OTA_FIRMWARE_FEATURE)
	}
	#endif
}

bool smartSleep(uint8_t interrupt, uint8_t mode, unsigned long ms) {
	wait(MY_SMART_SLEEP_WAIT_DURATION);
	bool ret = sleep(interrupt, mode, ms);
	sendHeartbeat();
	return ret;
}

int8_t sleep(uint8_t interrupt1, uint8_t mode1, uint8_t interrupt2, uint8_t mode2, unsigned long ms) {
	#if defined(MY_OTA_FIRMWARE_FEATURE)
	if (_fwUpdateOngoing) {
		// Do not sleep node while fw update is ongoing
		return -1;
	} else {
	#endif
		#if defined(MY_RADIO_FEATURE)
			transportPowerDown();
		#endif
		return hwSleep(interrupt1, mode1, interrupt2, mode2, ms) ;
	#if defined(MY_OTA_FIRMWARE_FEATURE)
	}
	#endif
}

int8_t smartSleep(uint8_t interrupt1, uint8_t mode1, uint8_t interrupt2, uint8_t mode2, unsigned long ms) {
	wait(MY_SMART_SLEEP_WAIT_DURATION);
	int8_t ret = sleep(interrupt1, mode1, interrupt2, mode2, ms);
	sendHeartbeat();
	return ret;
}


