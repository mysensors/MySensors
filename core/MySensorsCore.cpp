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

#include "MySensorsCore.h"

ControllerConfig _cc; // Configuration coming from controller
NodeConfig _nc; // Essential settings for node to work
MyMessage _msg;  // Buffer for incoming messages.
MyMessage _msgTmp; // Buffer for temporary messages (acks and nonces among others).

bool _nodeRegistered = false;

#if defined(MY_DEBUG)
	char _convBuf[MAX_PAYLOAD*2+1];
#endif

void (*_timeCallback)(unsigned long); // Callback for requested time messages

void _process() {
	hwWatchdogReset();

	#if defined (MY_DEFAULT_TX_LED_PIN) || defined(MY_DEFAULT_RX_LED_PIN) || defined(MY_DEFAULT_ERR_LED_PIN)
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

void _infiniteLoop() {
	while(1) {
		#if defined(ARDUINO_ARCH_ESP8266)
			yield();
		#endif
		#if defined (MY_DEFAULT_TX_LED_PIN) || defined(MY_DEFAULT_RX_LED_PIN) || defined(MY_DEFAULT_ERR_LED_PIN)
			ledsProcess();
		#endif
	}
}

void _begin() {

	if (preHwInit)
		preHwInit();

	#if !defined(MY_DISABLED_SERIAL)
		hwInit();
	#endif

	debug(PSTR("MCO:BGN:INIT " MY_NODE_TYPE ",CP=" MY_CAPABILITIES ",VER=" MYSENSORS_LIBRARY_VERSION "\n"));

	// Call before() in sketch (if it exists)
	if (before) {
		debug(PSTR("MCO:BGN:BFR\n"));	// before callback
		before();
	}

	#if defined(MY_DEFAULT_TX_LED_PIN) || defined(MY_DEFAULT_RX_LED_PIN) || defined(MY_DEFAULT_ERR_LED_PIN)
		ledsInit();
	#endif

	signerInit();

	// Read latest received controller configuration from EEPROM
	// Note: _cc.isMetric is bool, hence empty EEPROM (=0xFF) evaluates to true (default)
	hwReadConfigBlock((void*)&_cc, (void*)EEPROM_CONTROLLER_CONFIG_ADDRESS, sizeof(ControllerConfig));

	#if defined(MY_OTA_FIRMWARE_FEATURE)
		// Read firmware config from EEPROM, i.e. type, version, CRC, blocks
		readFirmwareSettings();
	#endif

	#if defined(MY_RADIO_FEATURE)
		// Save static parent id in eeprom (used by bootloader)
		hwWriteConfig(EEPROM_PARENT_NODE_ID_ADDRESS, MY_PARENT_NODE_ID);
		transportInitialize();
		while (!isTransportReady()) {
			hwWatchdogReset();
			transportProcess();
			#if defined(ARDUINO_ARCH_ESP8266)
				yield();
			#endif
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
				setIndication(INDICATION_ERR_LOCKED);
				debug(PSTR("MCO:BGN:NODE UNLOCKED\n"));
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

	#if defined(MY_GATEWAY_FEATURE)
		#if defined(MY_INCLUSION_BUTTON_FEATURE)
	    	inclusionInit();
		#endif

	    // initialize the transport driver
		if (!gatewayTransportInit()) {
			setIndication(INDICATION_ERR_INIT_GWTRANSPORT);
			debug(PSTR("!MCO:BGN:TSP FAIL\n"));
			// Nothing more we can do
			_infiniteLoop();
		}
	#endif

	#if !defined(MY_GATEWAY_FEATURE)
		presentNode();
	#endif

	// register node
	_registerNode();

	// Call sketch setup
	if (setup) {
		debug(PSTR("MCO:BGN:STP\n"));	// setup callback
		setup();
	}

	debug(PSTR("MCO:BGN:INIT OK,ID=%d,PAR=%d,DIS=%d,REG=%d\n"), _nc.nodeId, _nc.parentNodeId, _nc.distance, _nodeRegistered);
}


void _registerNode() {
	#if defined (MY_REGISTRATION_FEATURE) && !defined(MY_GATEWAY_FEATURE)
		debug(PSTR("MCO:REG:REQ\n"));	// registration request
		setIndication(INDICATION_REQ_REGISTRATION);
		_nodeRegistered = MY_REGISTRATION_DEFAULT;
		uint8_t counter = MY_REGISTRATION_RETRIES;
		// only proceed if register response received or retries exceeded
		do {
			_sendRoute(build(_msgTmp, _nc.nodeId, GATEWAY_ADDRESS, NODE_SENSOR_ID, C_INTERNAL, I_REGISTRATION_REQUEST, false).set(MY_CORE_VERSION));
		} while (!wait(2000, C_INTERNAL, I_REGISTRATION_RESPONSE) && counter--);

	#else
		_nodeRegistered = true;
		debug(PSTR("MCO:REG:NOT NEEDED\n"));
	#endif
}

void presentNode() {
	setIndication(INDICATION_PRESENT);
	// Present node and request config
	#if defined(MY_GATEWAY_FEATURE)
		// Send presentation for this gateway device
		#if defined(MY_REPEATER_FEATURE)
			present(NODE_SENSOR_ID, S_ARDUINO_REPEATER_NODE);
		#else
			present(NODE_SENSOR_ID, S_ARDUINO_NODE);
		#endif
	#else

		#if defined(MY_OTA_FIRMWARE_FEATURE)
				presentBootloaderInformation();
		#endif

		// Send signing preferences for this node to the GW
		signerPresentation(_msgTmp, GATEWAY_ADDRESS);

			// Send presentation for this radio node
		#if defined(MY_REPEATER_FEATURE)
				present(NODE_SENSOR_ID, S_ARDUINO_REPEATER_NODE);
		#else
				present(NODE_SENSOR_ID, S_ARDUINO_NODE);
		#endif

		// Send a configuration exchange request to controller
		// Node sends parent node. Controller answers with latest node configuration
		_sendRoute(build(_msgTmp, _nc.nodeId, GATEWAY_ADDRESS, NODE_SENSOR_ID, C_INTERNAL, I_CONFIG, false).set(_nc.parentNodeId));

		// Wait configuration reply.
		wait(2000, C_INTERNAL, I_CONFIG);

	#endif

	if (presentation)
		presentation();

}


uint8_t getNodeId() {
	return _nc.nodeId;
}

uint8_t getParentNodeId() {
	return _nc.parentNodeId;
}

ControllerConfig getConfig() {
	return _cc;
}


bool _sendRoute(MyMessage &message) {
	#if defined(MY_CORE_ONLY)
		(void)message;
	#endif
	#if defined(MY_GATEWAY_FEATURE)
		if (message.destination == _nc.nodeId) {
			// This is a message sent from a sensor attached on the gateway node.
			// Pass it directly to the gateway transport layer.
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
	mSetCommand(message, C_SET);
	mSetRequestAck(message, enableAck);

	#if defined(MY_REGISTRATION_FEATURE) && !defined(MY_GATEWAY_FEATURE)
		if (_nodeRegistered) {
			return _sendRoute(message);
		}
		else {
			debug(PSTR("!MCO:SND:NODE NOT REG\n"));	// node not registered
			return false;
		}
	#else
		return _sendRoute(message);
	#endif
	}

void sendBatteryLevel(uint8_t value, bool enableAck) {
	_sendRoute(build(_msgTmp, _nc.nodeId, GATEWAY_ADDRESS, NODE_SENSOR_ID, C_INTERNAL, I_BATTERY_LEVEL, enableAck).set(value));
}

void sendHeartbeat(void) {
	#if defined(MY_RADIO_NRF24) || defined(MY_RADIO_RFM69) || defined(MY_RS485)
		uint32_t heartbeat = transportGetHeartbeat();
	#else
		uint32_t heartbeat = hwMillis();
	#endif
	_sendRoute(build(_msgTmp, _nc.nodeId, GATEWAY_ADDRESS, NODE_SENSOR_ID, C_INTERNAL, I_HEARTBEAT_RESPONSE, false).set(heartbeat));
}

void present(uint8_t childSensorId, uint8_t sensorType, const char *description, bool enableAck) {
	_sendRoute(build(_msgTmp, _nc.nodeId, GATEWAY_ADDRESS, childSensorId, C_PRESENTATION, sensorType, enableAck).set(childSensorId==NODE_SENSOR_ID?MYSENSORS_LIBRARY_VERSION:description));
}

void sendSketchInfo(const char *name, const char *version, bool enableAck) {
	if (name) _sendRoute(build(_msgTmp, _nc.nodeId, GATEWAY_ADDRESS, NODE_SENSOR_ID, C_INTERNAL, I_SKETCH_NAME, enableAck).set(name));
    if (version) _sendRoute(build(_msgTmp, _nc.nodeId, GATEWAY_ADDRESS, NODE_SENSOR_ID, C_INTERNAL, I_SKETCH_VERSION, enableAck).set(version));
}

void request(uint8_t childSensorId, uint8_t variableType, uint8_t destination) {
	_sendRoute(build(_msgTmp, _nc.nodeId, destination, childSensorId, C_REQ, variableType, false).set(""));
}

void requestTime() {
	_sendRoute(build(_msgTmp, _nc.nodeId, GATEWAY_ADDRESS, NODE_SENSOR_ID, C_INTERNAL, I_TIME, false).set(""));
}

// Message delivered through _msg
bool _processInternalMessages() {
	uint8_t type = _msg.type;

	if (_msg.sender == GATEWAY_ADDRESS) {
		if (type == I_REBOOT) {
			#if !defined(MY_DISABLE_REMOTE_RESET)
				// Requires MySensors or other bootloader with watchdogs enabled
				setIndication(INDICATION_REBOOT);
				hwReboot();
			#endif
		}
		else if (type == I_REGISTRATION_RESPONSE) {
			#if defined (MY_REGISTRATION_FEATURE) && !defined(MY_GATEWAY_FEATURE)
				_nodeRegistered = _msg.getBool();
				setIndication(INDICATION_GOT_REGISTRATION);
				debug(PSTR("MCO:PIM:NODE REG=%d\n"), _nodeRegistered);	// node registration
			#endif
		}
		else if (type == I_CONFIG) {
			// Pick up configuration from controller (currently only metric/imperial) and store it in eeprom if changed
			_cc.isMetric = _msg.data[0] == 0x00 || _msg.data[0] == 'M'; // metric if null terminated or M
			hwWriteConfig(EEPROM_CONTROLLER_CONFIG_ADDRESS, _cc.isMetric);
		}
		else if (type == I_PRESENTATION) {
			// Re-send node presentation to controller
			presentNode();
		}
		else if (type == I_HEARTBEAT_REQUEST) {
			sendHeartbeat();
		}
		else if (type == I_TIME) {
			// Deliver time to callback
			if (receiveTime)
				receiveTime(_msg.getULong());
		}
		else if (type == I_CHILDREN) {
			#if defined(MY_REPEATER_FEATURE)
				if (_msg.data[0] == 'C') {
					// Clears child relay data for this node
					setIndication(INDICATION_CLEAR_ROUTING);
					transportClearRoutingTable();
					_sendRoute(build(_msgTmp, _nc.nodeId, GATEWAY_ADDRESS, NODE_SENSOR_ID, C_INTERNAL, I_CHILDREN, false).set("ok"));
				}
			#endif
		}
		else if (type == I_DEBUG) {
			#if defined(MY_DEBUG) || defined(MY_SPECIAL_DEBUG)
				char debug_msg = _msg.data[0];
				if (debug_msg == 'R') {		// routing table
				#if defined(MY_REPEATER_FEATURE)
					for (uint8_t cnt = 0; cnt != 255; cnt++) {
						uint8_t route = hwReadConfig(EEPROM_ROUTES_ADDRESS + cnt);
						if (route != BROADCAST_ADDRESS) {
							debug(PSTR("MCO:PIM:ROUTE N=%d,R=%d\n"), cnt, route);
							uint8_t OutBuf[2] = { cnt,route };
							_sendRoute(build(_msgTmp, _nc.nodeId, GATEWAY_ADDRESS, NODE_SENSOR_ID, C_INTERNAL, I_DEBUG, false).set(OutBuf, 2));
							wait(200);
						}
					}
				#endif
				}
				else if (debug_msg == 'V') {	// CPU voltage
					_sendRoute(build(_msgTmp, _nc.nodeId, GATEWAY_ADDRESS, NODE_SENSOR_ID, C_INTERNAL, I_DEBUG, false).set(hwCPUVoltage()));
				}
				else if (debug_msg == 'F') {	// CPU frequency in 1/10Mhz
					_sendRoute(build(_msgTmp, _nc.nodeId, GATEWAY_ADDRESS, NODE_SENSOR_ID, C_INTERNAL, I_DEBUG, false).set(hwCPUFrequency()));
				}
				else if (debug_msg == 'M') {	// free memory
					_sendRoute(build(_msgTmp, _nc.nodeId, GATEWAY_ADDRESS, NODE_SENSOR_ID, C_INTERNAL, I_DEBUG, false).set(hwFreeMem()));
				}
				else if (debug_msg == 'E') {	// clear MySensors eeprom area and reboot
					_sendRoute(build(_msgTmp, _nc.nodeId, GATEWAY_ADDRESS, NODE_SENSOR_ID, C_INTERNAL, I_DEBUG, false).set("ok"));
					for (int i = EEPROM_START; i<EEPROM_LOCAL_CONFIG_ADDRESS; i++) hwWriteConfig(i, 0xFF);
					setIndication(INDICATION_REBOOT);
					hwReboot();
				}
			#endif
		}
		else return false;
	}
	else {
		// sender is a node
		if (type == I_REGISTRATION_REQUEST) {
			#if defined(MY_GATEWAY_FEATURE)
				// register request are exclusively handled by GW/Controller
				// !!! eventually define if AUTO ACK or register request forwarded to controller
				#if !defined(MY_REGISTRATION_CONTROLLER)
					// auto register if version compatible
					bool approveRegistration = true;

					#if defined(MY_CORE_COMPATIBILITY_CHECK)
							approveRegistration = (_msg.getByte() >= MY_CORE_MIN_VERSION);
					#endif
					_sendRoute(build(_msgTmp, _nc.nodeId, _msg.sender, NODE_SENSOR_ID, C_INTERNAL, I_REGISTRATION_RESPONSE, false).set(approveRegistration));
				#else
					return false;	// processing of this request via controller
				#endif
			#endif
		}
		else return false;
	}
	return true;
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
		#if defined(ARDUINO_ARCH_ESP8266)
			yield();
		#endif
	}
}

bool wait(unsigned long ms, uint8_t cmd, uint8_t msgtype) {
	unsigned long enter = hwMillis();
	// invalidate msg type
	_msg.type = !msgtype;
	bool expectedResponse = false;
	while ( (hwMillis() - enter < ms) && !expectedResponse ) {
		_process();
		#if defined(ARDUINO_ARCH_ESP8266)
			yield();
		#endif
		expectedResponse = (mGetCommand(_msg) == cmd && _msg.type == msgtype);
	}
	return expectedResponse;
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
		setIndication(INDICATION_SLEEP);
		const int8_t res = hwSleep(ms);
		setIndication(INDICATION_WAKEUP);
		return res;
	#endif
}

int8_t smartSleep(unsigned long ms) {
	// notify controller about going to sleep
	sendHeartbeat();
	// listen for incoming messages
	wait(MY_SMART_SLEEP_WAIT_DURATION);
	return sleep(ms);
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
		setIndication(INDICATION_SLEEP);
		const int8_t res = hwSleep(interrupt, mode, ms);
		setIndication(INDICATION_WAKEUP);
		return res;
	#endif
}

int8_t smartSleep(uint8_t interrupt, uint8_t mode, unsigned long ms) {
	// notify controller about going to sleep
	sendHeartbeat();
	// listen for incoming messages
	wait(MY_SMART_SLEEP_WAIT_DURATION);
	return sleep(interrupt, mode, ms);
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
		setIndication(INDICATION_SLEEP);
		const int8_t res = hwSleep(interrupt1, mode1, interrupt2, mode2, ms);
		setIndication(INDICATION_WAKEUP);
		return res;
	#endif
}

int8_t smartSleep(uint8_t interrupt1, uint8_t mode1, uint8_t interrupt2, uint8_t mode2, unsigned long ms) {
	// notify controller about going to sleep
	sendHeartbeat();
	// listen for incoming messages
	wait(MY_SMART_SLEEP_WAIT_DURATION);
	return sleep(interrupt1, mode1, interrupt2, mode2, ms);
}

#ifdef MY_NODE_LOCK_FEATURE
void nodeLock(const char* str) {
	// Make sure EEPROM is updated to locked status
	hwWriteConfig(EEPROM_NODE_LOCK_COUNTER, 0);
	while (1) {
		setIndication(INDICATION_ERR_LOCKED);
		debug(PSTR("MCO:NLK:NODE LOCKED. TO UNLOCK, GND PIN %d AND RESET\n"), MY_NODE_UNLOCK_PIN);
		#if defined(ARDUINO_ARCH_ESP8266)
			yield();
		#endif
		_sendRoute(build(_msgTmp, _nc.nodeId, GATEWAY_ADDRESS, NODE_SENSOR_ID,C_INTERNAL, I_LOCKED, false).set(str));
		#if defined(MY_RADIO_FEATURE)
			transportPowerDown();
		#endif
		setIndication(INDICATION_SLEEP);
		(void)hwSleep((unsigned long)1000*60*30); // Sleep for 30 min before resending LOCKED message
		setIndication(INDICATION_WAKEUP);
	}
}
#endif
