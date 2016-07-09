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

volatile int8_t pinIntTrigger = 0;

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
}

void hwPowerDown(period_t period) {

	// disable ADC for power saving
	ADCSRA &= ~(1 << ADEN);
	// save WDT settings
	uint8_t WDTsave = WDTCSR;
	if (period != SLEEP_FOREVER)
	{
		wdt_enable(period);
		// enable WDT interrupt before system reset
		WDTCSR |= (1 << WDCE) | (1 << WDIE);
	} else {
		// if sleeping forever, disable WDT
		wdt_disable();
	}
	set_sleep_mode(SLEEP_MODE_PWR_DOWN);
	cli();
	sleep_enable();
#if defined __AVR_ATmega328P__
	sleep_bod_disable();
#endif
	sei();
	// sleep until WDT or ext. interrupt
	sleep_cpu();
    sleep_disable();	
	// restore previous WDT settings
	cli();
	wdt_reset();
	// enable WDT changes
	WDTCSR |= (1 << WDCE) | (1 << WDE);
	// restore saved WDT settings
	WDTCSR = WDTsave;
	sei();
	// enable ADC
	ADCSRA |= (1 << ADEN);
}

void hwInternalSleep(unsigned long ms) {
	// Let serial prints finish (debug, log etc)
  #ifndef MY_DISABLED_SERIAL
	  MY_SERIALDEVICE.flush();
  #endif
	// reset interrupt trigger var
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
	if (!pinIntTrigger && ms >= 16)      { hwPowerDown(SLEEP_15MS); ms -= 15; }
}

int8_t hwSleep(unsigned long ms) {
	hwInternalSleep(ms);
	return -1;
}

int8_t hwSleep(uint8_t interrupt, uint8_t mode, unsigned long ms) {
	return hwSleep(interrupt,mode,0xFF,0x00,ms);
}

int8_t hwSleep(uint8_t interrupt1, uint8_t mode1, uint8_t interrupt2, uint8_t mode2, unsigned long ms) {
	// reset interrupt trigger var
	pinIntTrigger = 0;
	// attach interrupts 
	attachInterrupt(interrupt1, wakeUp, mode1);
	if (interrupt2!=0xFF) attachInterrupt(interrupt2, wakeUp2, mode2);
	
	if (ms>0) {
		// sleep for defined time
		hwInternalSleep(ms);
	} else {
		// sleep until ext interrupt triggered
    	hwPowerDown(SLEEP_FOREVER);
	}
	
	detachInterrupt(interrupt1);
	if (interrupt2!=0xFF) detachInterrupt(interrupt2);
	
	// default: no interrupt triggered, timer wake up	
	int8_t retVal = -1;

	if (pinIntTrigger == 1) {
		retVal = (int8_t)interrupt1;
	} else if (pinIntTrigger == 2) {
		retVal = (int8_t)interrupt2;
	}
	return retVal;
}

uint16_t hwCPUVoltage() {
	// Measure Vcc against 1.1V Vref 
	#if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
		ADMUX = (_BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1));
	#elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
		ADMUX = (_BV(MUX5) | _BV(MUX0));
	#elif defined (__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
		ADMUX = (_BV(MUX3) | _BV(MUX2));
	#else
		ADMUX = (_BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1));
	#endif 
	// Vref settle
	delay(70); 
	// Do conversion
	ADCSRA |= _BV(ADSC);
	while (bit_is_set(ADCSRA,ADSC));
	// return Vcc in mV
	return (1125300UL) / ADC;
}

uint16_t hwCPUFrequency() {
	noInterrupts();
	// setup timer1
	TIFR1 = 0xFF;   
	TCNT1 = 0; 
	TCCR1A = 0;
	TCCR1C = 0;
	// save WDT settings
	uint8_t WDTsave = WDTCSR;
	wdt_enable(WDTO_500MS);
	// enable WDT interrupt mode => first timeout WDIF, 2nd timeout reset
	WDTCSR |= (1 << WDIE);
	wdt_reset();
	// start timer1 with 1024 prescaling
	TCCR1B = _BV(CS12) | _BV(CS10);   
	// wait until wdt interrupt
	while (bit_is_clear(WDTCSR,WDIF));
	// stop timer
	TCCR1B = 0;
	// restore WDT settings
	wdt_reset();
	WDTCSR |= (1 << WDCE) | (1 << WDE);
	WDTCSR = WDTsave;
	interrupts();	
	// return frequency in 1/10MHz (accuracy +- 10%)
	return TCNT1 * 2048UL / 100000UL;
}

uint16_t hwFreeMem() {
	extern int __heap_start, *__brkval; 
	int v; 
	return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}



#ifdef MY_DEBUG
void hwDebugPrint(const char *fmt, ... ) {
	char fmtBuffer[300];
	#ifdef MY_GATEWAY_FEATURE
		// prepend debug message to be handled correctly by controller (C_INTERNAL, I_LOG_MESSAGE)
		snprintf_P(fmtBuffer, 299, PSTR("0;255;%d;0;%d;"), C_INTERNAL, I_LOG_MESSAGE);
		MY_SERIALDEVICE.print(fmtBuffer);
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
	MY_SERIALDEVICE.print(fmtBuffer);
	MY_SERIALDEVICE.flush();

	//MY_SERIALDEVICE.write(freeRam());
}
#endif

#endif // #ifdef ARDUINO_ARCH_AVR
