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

#include "MyLeds.h"

// these variables don't need to be volatile, since we are not using interrupts
uint8_t _countRx;
uint8_t _countTx;
uint8_t _countErr;
unsigned long _blink_next_time;

inline void ledsInit() {
	// Setup led pins
	pinMode(MY_DEFAULT_RX_LED_PIN, OUTPUT);
	pinMode(MY_DEFAULT_TX_LED_PIN, OUTPUT);
	pinMode(MY_DEFAULT_ERR_LED_PIN, OUTPUT);

	// Set initial state of leds
	hwDigitalWrite(MY_DEFAULT_RX_LED_PIN, LED_OFF);
	hwDigitalWrite(MY_DEFAULT_TX_LED_PIN, LED_OFF);
	hwDigitalWrite(MY_DEFAULT_ERR_LED_PIN, LED_OFF);

	// initialize counters
	_countRx = 0;
	_countTx = 0;
	_countErr = 0;
}

inline void ledsProcess() {
	// Just return if it is not the time...
	// http://playground.arduino.cc/Code/TimingRollover
	if ((long)(hwMillis() - _blink_next_time) < 0)
		return;
	else
		_blink_next_time = hwMillis() + MY_DEFAULT_LED_BLINK_PERIOD;

	// do the actual blinking
	if(_countRx && _countRx != 255) {
		// switch led on
		hwDigitalWrite(MY_DEFAULT_RX_LED_PIN, LED_ON);
	}
	else if(!_countRx) {
		// switching off
		hwDigitalWrite(MY_DEFAULT_RX_LED_PIN, LED_OFF);
	}
	if(_countRx != 255)
		--_countRx;

	if(_countTx && _countTx != 255) {
		// switch led on
		hwDigitalWrite(MY_DEFAULT_TX_LED_PIN, LED_ON);
	}
	else if(!_countTx) {
		// switching off
		hwDigitalWrite(MY_DEFAULT_TX_LED_PIN, LED_OFF);
	}
	if(_countTx != 255)
		--_countTx;

	if(_countErr && _countErr != 255) {
		// switch led on
		hwDigitalWrite(MY_DEFAULT_ERR_LED_PIN, LED_ON);
	}
	else if(!_countErr) {
		// switching off
		hwDigitalWrite(MY_DEFAULT_ERR_LED_PIN, LED_OFF);
	}
	if(_countErr != 255)
		--_countErr;
}

void ledsBlinkRx(uint8_t cnt) {
  if(_countRx == 255) { _countRx = cnt; }
}

void ledsBlinkTx(uint8_t cnt) {
  if(_countTx == 255) { _countTx = cnt; }
}

void ledsBlinkErr(uint8_t cnt) {
  if(_countErr == 255) { _countErr = cnt; }
}

