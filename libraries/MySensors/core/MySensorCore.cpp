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

uint32_t _heartbeat = 0;
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

	// Call before() in sketch (if it exists)
	if (before) before();

	debug(PSTR("Starting " MY_NODE_TYPE " (" MY_CAPABILIIES ", " LIBRARY_VERSION ")\n"));

	signerInit();

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

#ifdef MY_NODE_LOCK_FEATURE
	// Check if node has been locked down
	if (hwReadConfig(EEPROM_NODE_LOCK_COUNTER) == 0) {
		// Node is locked, check if unlock pin is asserted, else hang the node
		pinMode(MY_NODE_UNLOCK_PIN, INPUT_PULLUP);
		// Make a short delay so we are sure any large external nets are fully pulled
		unsigned long enter = hwMillis();
		while (hwMillis() - enter < 2);
		if (digitalRead(MY_NODE_UNLOCK_PIN) == 0) {
			// Pin is grounded, reset lock counter
			hwWriteConfig(EEPROM_NODE_LOCK_COUNTER, MY_NODE_LOCK_COUNTER_MAX);
			// Disable pullup
			pinMode(MY_NODE_UNLOCK_PIN, INPUT);
			debug(PSTR("Node is unlocked.\n"));
		} else {
			// Disable pullup
			pinMode(MY_NODE_UNLOCK_PIN, INPUT);
			nodeLock("LDB"); //Locked during boot
		}
	} else if (hwReadConfig(EEPROM_NODE_LOCK_COUNTER) == 0xFF) {
		// Reset walue
		hwWriteConfig(EEPROM_NODE_LOCK_COUNTER, MY_NODE_LOCK_COUNTER_MAX);
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
	// increment heartbeat counter
	_heartbeat++;
	#if defined(MY_CORE_ONLY)
		(void)message;
	#endif
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
	_sendRoute(build(_msg, _nc.nodeId, GATEWAY_ADDRESS, NODE_SENSOR_ID, C_INTERNAL, I_HEARTBEAT_RESPONSE, false).set(_heartbeat));

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
		sendHeartbeat();
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

int8_t sleep(unsigned long ms) {
	#if defined(MY_OTA_FIRMWARE_FEATURE)
	if (_fwUpdateOngoing) {
		// Do not sleep node while fw update is ongoing
		wait(ms);
		return -1;
	}
	#endif
	// if repeater, do not sleep
	#if defined(MY_REPEATER_FEATURE)
		wait(ms);
		return -1;
	#else
		#if defined(MY_RADIO_FEATURE)
			transportPowerDown();
		#endif
		return hwSleep(ms);
	#endif
}

int8_t smartSleep(unsigned long ms) {
	int8_t ret = sleep(ms);
	// notifiy controller about wake up
	sendHeartbeat();
	// listen for incoming messages
	wait(MY_SMART_SLEEP_WAIT_DURATION);
	return ret;
}

int8_t sleep(uint8_t interrupt, uint8_t mode, unsigned long ms) {
	#if defined(MY_OTA_FIRMWARE_FEATURE)
	if (_fwUpdateOngoing) {
		// not supported
		return -2;
	}
	#endif
	#if defined(MY_REPEATER_FEATURE)
		// not supported
		(void)interrupt;
		(void)mode;
		(void)ms;
		return -2;
	#else
		#if defined(MY_RADIO_FEATURE)
			transportPowerDown();
		#endif
		return hwSleep(interrupt, mode, ms);
	#endif
}

int8_t smartSleep(uint8_t interrupt, uint8_t mode, unsigned long ms) {
	int8_t ret = sleep(interrupt, mode, ms);
	// notifiy controller about wake up
	sendHeartbeat();
	// listen for incoming messages
	wait(MY_SMART_SLEEP_WAIT_DURATION);
	return ret;
}

int8_t sleep(uint8_t interrupt1, uint8_t mode1, uint8_t interrupt2, uint8_t mode2, unsigned long ms) {
	#if defined(MY_OTA_FIRMWARE_FEATURE)
	if (_fwUpdateOngoing) {
		// not supported
		return -2;
	}
	#endif
	#if defined(MY_REPEATER_FEATURE)
		// not supported
		(void)interrupt1;
		(void)mode1;
		(void)interrupt2;
		(void)mode2;
		(void)ms;
		return -2;
	#else
		#if defined(MY_RADIO_FEATURE)
			transportPowerDown();
		#endif
		return hwSleep(interrupt1, mode1, interrupt2, mode2, ms);
	#endif
}

int8_t smartSleep(uint8_t interrupt1, uint8_t mode1, uint8_t interrupt2, uint8_t mode2, unsigned long ms) {
	int8_t ret = sleep(interrupt1, mode1, interrupt2, mode2, ms);
	// notifiy controller about wake up
	sendHeartbeat();
	// listen for incoming messages
	wait(MY_SMART_SLEEP_WAIT_DURATION);
	return ret;
}

#ifdef MY_NODE_LOCK_FEATURE
void nodeLock(const char* str) {
	// Make sure EEPROM is updated to locked status
	hwWriteConfig(EEPROM_NODE_LOCK_COUNTER, 0);
	while (1) {
		debug(PSTR("Node is locked. Ground pin %d and reset to unlock.\n"), MY_NODE_UNLOCK_PIN);
		#if defined(MY_GATEWAY_ESP8266)
			yield();
		#endif
		_sendRoute(build(_msg, _nc.nodeId, GATEWAY_ADDRESS, NODE_SENSOR_ID,
			C_INTERNAL, I_LOCKED, false).set(str));
		#if defined(MY_RADIO_FEATURE)
			transportPowerDown();
		#endif
		(void)hwSleep((unsigned long)1000*60*30); // Sleep for 30 min before resending LOCKED message
	}
}
#endif

