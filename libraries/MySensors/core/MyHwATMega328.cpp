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

#ifdef ARDUINO_ARCH_AVR

#include "MyHwATMega328.h"

int8_t pinIntTrigger = 0;
void wakeUp()	 //place to send the interrupts
{
	pinIntTrigger = 1;
}
void wakeUp2()	 //place to send the second interrupts
{
	pinIntTrigger = 2;
}

// Watchdog Timer interrupt service routine. This routine is required
// to allow automatic WDIF and WDIE bit clearance in hardware.
ISR (WDT_vect)
{
	// WDIE & WDIF is cleared in hardware upon entering this ISR
	wdt_disable();
}




void hwPowerDown(period_t period) {

	ADCSRA &= ~(1 << ADEN);

	if (period != SLEEP_FOREVER)
	{
		wdt_enable(period);
		WDTCSR |= (1 << WDIE);
	}
	#if defined __AVR_ATmega328P__
		do {
			set_sleep_mode(SLEEP_MODE_PWR_DOWN);
			cli();
			sleep_enable();
			sleep_bod_disable();
			sei();
			sleep_cpu();
			sleep_disable();
			sei();
		} while (0);
	#else
		do {
			set_sleep_mode(SLEEP_MODE_PWR_DOWN);
			cli();
			sleep_enable();
			sei();
			sleep_cpu();
			sleep_disable();
			sei();
		} while (0);
	#endif

	ADCSRA |= (1 << ADEN);
}

void hwInternalSleep(unsigned long ms) {
	// Let serial prints finish (debug, log etc)
  #ifndef MY_DISABLED_SERIAL
	  Serial.flush();
  #endif
	pinIntTrigger = 0;
	while (!pinIntTrigger && ms >= 8000) { hwPowerDown(SLEEP_8S); ms -= 8000; }
	if (!pinIntTrigger && ms >= 4000)    { hwPowerDown(SLEEP_4S); ms -= 4000; }
	if (!pinIntTrigger && ms >= 2000)    { hwPowerDown(SLEEP_2S); ms -= 2000; }
	if (!pinIntTrigger && ms >= 1000)    { hwPowerDown(SLEEP_1S); ms -= 1000; }
	if (!pinIntTrigger && ms >= 500)     { hwPowerDown(SLEEP_500MS); ms -= 500; }
	if (!pinIntTrigger && ms >= 250)     { hwPowerDown(SLEEP_250MS); ms -= 250; }
	if (!pinIntTrigger && ms >= 125)     { hwPowerDown(SLEEP_120MS); ms -= 120; }
	if (!pinIntTrigger && ms >= 64)      { hwPowerDown(SLEEP_60MS); ms -= 60; }
	if (!pinIntTrigger && ms >= 32)      { hwPowerDown(SLEEP_30MS); ms -= 30; }
	if (!pinIntTrigger && ms >= 16)      { hwPowerDown(SLEEP_15Ms); ms -= 15; }
}

void hwSleep(unsigned long ms) {
	hwInternalSleep(ms);
}

bool hwSleep(uint8_t interrupt, uint8_t mode, unsigned long ms) {
	// Let serial prints finish (debug, log etc)
	bool pinTriggeredWakeup = true;
	attachInterrupt(interrupt, wakeUp, mode);
	if (ms>0) {
		pinIntTrigger = 0;
		hwInternalSleep(ms);
		if (0 == pinIntTrigger) {
			pinTriggeredWakeup = false;
		}
	} else {
    #ifndef MY_DISABLED_SERIAL
		  Serial.flush();
    #endif
		hwPowerDown(SLEEP_FOREVER);
	}
	detachInterrupt(interrupt);
	return pinTriggeredWakeup;
}

uint8_t hwSleep(uint8_t interrupt1, uint8_t mode1, uint8_t interrupt2, uint8_t mode2, unsigned long ms) {
	int8_t retVal = 1;
	attachInterrupt(interrupt1, wakeUp, mode1);
	attachInterrupt(interrupt2, wakeUp2, mode2);
	if (ms>0) {
		pinIntTrigger = 0;
		hwInternalSleep(ms);
		if (0 == pinIntTrigger) {
			retVal = -1;
		}
	} else {
    #ifndef MY_DISABLED_SERIAL
		  Serial.flush();
    #endif
		hwPowerDown(SLEEP_FOREVER);
	}
	detachInterrupt(interrupt1);
	detachInterrupt(interrupt2);

	if (1 == pinIntTrigger) {
		retVal = (int8_t)interrupt1;
	} else if (2 == pinIntTrigger) {
		retVal = (int8_t)interrupt2;
	}
	return retVal;
}



#ifdef MY_DEBUG
void hwDebugPrint(const char *fmt, ... ) {
	char fmtBuffer[300];
	#ifdef MY_GATEWAY_FEATURE
		// prepend debug message to be handled correctly by controller (C_INTERNAL, I_LOG_MESSAGE)
		snprintf_P(fmtBuffer, 299, PSTR("0;0;%d;0;%d;"), C_INTERNAL, I_LOG_MESSAGE);
		Serial.print(fmtBuffer);
	#endif
	va_list args;
	va_start (args, fmt );
	va_end (args);
	#ifdef MY_GATEWAY_FEATURE
		// Truncate message if this is gateway node
		vsnprintf_P(fmtBuffer, MY_GATEWAY_MAX_SEND_LENGTH, fmt, args);
		fmtBuffer[MY_GATEWAY_MAX_SEND_LENGTH-1] = '\n';
		fmtBuffer[MY_GATEWAY_MAX_SEND_LENGTH] = '\0';
	#else
		vsnprintf_P(fmtBuffer, 299, fmt, args);
	#endif
	va_end (args);
	Serial.print(fmtBuffer);
	Serial.flush();

	//Serial.write(freeRam());
}
#endif

#endif // #ifdef ARDUINO_ARCH_AVR
