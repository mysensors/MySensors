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

#define LED_ON_OFF_RATIO        (4)       // Power of 2 please
#define LED_PROCESS_INTERVAL_MS (MY_DEFAULT_LED_BLINK_PERIOD/LED_ON_OFF_RATIO)

// these variables don't need to be volatile, since we are not using interrupts
static uint8_t countRx;
static uint8_t countTx;
static uint8_t countErr;
static unsigned long prevTime = hwMillis() - LED_PROCESS_INTERVAL_MS;     // Substract some, to make sure leds gets updated on first run.


inline void ledsInit()
{
	// initialize counters
	countRx = 0;
	countTx = 0;
	countErr = 0;

	// Setup led pins
#if defined(MY_DEFAULT_RX_LED_PIN)
	pinMode(MY_DEFAULT_RX_LED_PIN,  OUTPUT);
#endif
#if defined(MY_DEFAULT_TX_LED_PIN)
	pinMode(MY_DEFAULT_TX_LED_PIN,  OUTPUT);
#endif
#if defined(MY_DEFAULT_ERR_LED_PIN)
	pinMode(MY_DEFAULT_ERR_LED_PIN, OUTPUT);
#endif
    ledsProcess();
}

void ledsProcess() {
	// Just return if it is not the time...
	if ((hwMillis() - prevTime) < LED_PROCESS_INTERVAL_MS)
		return;

	prevTime += LED_PROCESS_INTERVAL_MS;

    uint8_t state;

    // For an On/Off ratio of 4, the pattern repeated will be [on, on, on, off]
    // until the counter becomes 0.
#if defined(MY_DEFAULT_RX_LED_PIN)
    state = (countRx & (LED_ON_OFF_RATIO-1)) ? LED_ON : LED_OFF;
    hwDigitalWrite(MY_DEFAULT_RX_LED_PIN, state);
    if (countRx)  --countRx;
#endif

#if defined(MY_DEFAULT_TX_LED_PIN)
    state = (countTx & (LED_ON_OFF_RATIO-1)) ? LED_ON : LED_OFF;
    hwDigitalWrite(MY_DEFAULT_TX_LED_PIN, state);
    if (countTx)  --countTx;
#endif

#if defined(MY_DEFAULT_ERR_LED_PIN)
    state = (countErr & (LED_ON_OFF_RATIO-1)) ? LED_ON : LED_OFF;
    hwDigitalWrite(MY_DEFAULT_ERR_LED_PIN, state);
    if (countErr) --countErr;
#endif
}

void ledsBlinkRx(uint8_t cnt) {
  if (!countRx) { countRx = cnt*LED_ON_OFF_RATIO; }
}

void ledsBlinkTx(uint8_t cnt) {
  if(!countTx) { countTx = cnt*LED_ON_OFF_RATIO; }
}

void ledsBlinkErr(uint8_t cnt) {
  if(!countErr) { countErr = cnt*LED_ON_OFF_RATIO; }
}
