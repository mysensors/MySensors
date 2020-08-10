/*
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2019 Sensnology AB
 * Full contributor list: https://github.com/mysensors/MySensors/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

#include "MyHwLGT8F328P.h"
#include <PMU.h>

bool hwInit(void)
{
#if !defined(MY_DISABLED_SERIAL)
	MY_SERIALDEVICE.begin(MY_BAUD_RATE);
	// MY_SERIALDEVICE.print("MY_SERIALDEVICE OK");
#if defined(MY_GATEWAY_SERIAL)
	while (!MY_SERIALDEVICE) {}
#endif
#endif
	return true;
}

#define WDTO_SLEEP_FOREVER		(0xFFu)

volatile uint8_t _wokeUpByInterrupt =
    INVALID_INTERRUPT_NUM;    // Interrupt number that woke the mcu.
volatile uint8_t _wakeUp1Interrupt  =
    INVALID_INTERRUPT_NUM;    // Interrupt number for wakeUp1-callback.
volatile uint8_t _wakeUp2Interrupt  =
    INVALID_INTERRUPT_NUM;    // Interrupt number for wakeUp2-callback.

static uint32_t sleepRemainingMs = 0ul;

void wakeUp1(void)
{
	// Disable sleep. When an interrupt occurs after attachInterrupt,
	// but before sleeping the CPU would not wake up.
	// Ref: http://playground.arduino.cc/Learning/ArduinoSleepCode
	sleep_disable();
	detachInterrupt(_wakeUp1Interrupt);
	// First interrupt occurred will be reported only
	if (INVALID_INTERRUPT_NUM == _wokeUpByInterrupt) {
		_wokeUpByInterrupt = _wakeUp1Interrupt;
	}
}
void wakeUp2(void)
{
	sleep_disable();
	detachInterrupt(_wakeUp2Interrupt);
	// First interrupt occurred will be reported only
	if (INVALID_INTERRUPT_NUM == _wokeUpByInterrupt) {
		_wokeUpByInterrupt = _wakeUp2Interrupt;
	}
}

inline bool interruptWakeUp(void)
{
	return _wokeUpByInterrupt != INVALID_INTERRUPT_NUM;
}

void clearPendingInterrupt(const uint8_t interrupt)
{
	EIFR = _BV(interrupt);
}

// Watchdog Timer interrupt service routine. This routine is required
// to allow automatic WDIF and WDIE bit clearance in hardware.
// ISR (WDT_vect)
// {
// }

void hwPowerDown(const uint8_t wdto)
{
	// Let serial prints finish (debug, log etc)
#ifndef MY_DISABLED_SERIAL
	MY_SERIALDEVICE.flush();
#endif
    // uncomment following to save more power if you just use them in pull_up
    // mode or just don't use them
#if defined(LGT8F_POWERSAVING)
	uint8_t ddrb_reg = DDRB;
	uint8_t portb_reg = PORTB;
	uint8_t ddrc_reg = DDRC;
	uint8_t portc_reg = PORTC;
	uint8_t ddrd_reg = DDRD;
	uint8_t portd_reg = PORTD;
	DDRB &= 0x3f;
	PORTB |= 0xc0;
	DDRC &= 0x00;
	PORTC |= 0xFF;
	DDRD &= 0xc0;
	PORTD |= 0x3F;
#endif// defined(LGT8F_POWERSAVING)
	
    ADCSRA &= ~(1 << ADEN);
    PMU.sleep(PM_POFFS1, wdto);

    // uncomment following to save more power if you just use them in pull_up
    // mode or just don't use them
    // restore I/O
#if defined(LGT8F_POWERSAVING)
    PORTD = portd_reg;
    DDRD = ddrd_reg;
    PORTC = portc_reg;
    DDRC = ddrc_reg;
    PORTB = portb_reg;
    DDRB = ddrb_reg;
#endif// defined(LGT8F_POWERSAVING)
}

uint32_t hwInternalSleep(uint32_t ms)
{
	// Sleeping with watchdog only supports multiples of 64ms(@32KHz clock).
	// Round up to next multiple of 64ms, to assure we sleep at least the
	// requested amount of time. Sleep of 0ms will not sleep at all!
	ms += 63u;

	while (!interruptWakeUp() && ms >= 64) {
		for (uint8_t period = 9u; ; --period) {
			const uint16_t comparatorMS = 1 << (period + 6);
			if ( ms >= comparatorMS) {
				// debug(PSTR(LOG_SLEEP "hwInternalSleep,ms=%lu,period=%u,comparatorMS=%u\n"),ms,period,comparatorMS);
				hwPowerDown(period); // 32s => 9, 64ms => 0
				ms -= comparatorMS;
				break;
			}
		}
	}
	if (interruptWakeUp()) {
		return ms;
	}
	return 0ul;
}

int8_t hwSleep(uint32_t ms)
{
	// Return what woke the mcu.
	// Default: no interrupt triggered, timer wake up
	int8_t ret = MY_WAKE_UP_BY_TIMER;
	sleepRemainingMs = 0ul;
	if (ms > 0u) {
		// sleep for defined time
		sleepRemainingMs = hwInternalSleep(ms);
	} else {
		// sleep until ext interrupt triggered
		hwPowerDown(WDTO_SLEEP_FOREVER);
	}
	if (interruptWakeUp()) {
		ret = static_cast<int8_t>(_wokeUpByInterrupt);
	}
	// Clear woke-up-by-interrupt flag, so next sleeps won't return immediately.
	_wokeUpByInterrupt = INVALID_INTERRUPT_NUM;

	return ret;

//   // TODO: Not supported!
//   (void)ms;
//   return MY_SLEEP_NOT_POSSIBLE;
}

int8_t hwSleep(const uint8_t interrupt, const uint8_t mode, uint32_t ms)
{
	return hwSleep(interrupt, mode, INVALID_INTERRUPT_NUM, 0u, ms);

//   // TODO: Not supported!
//   (void)interrupt;
//   (void)mode;
//   (void)ms;
//   return MY_SLEEP_NOT_POSSIBLE;
}

int8_t hwSleep(const uint8_t interrupt1, const uint8_t mode1, const uint8_t interrupt2,
               const uint8_t mode2,
               uint32_t ms)
{
	// LGT8F328P supports following modes to wake from sleep: LOW, CHANGE, RISING, FALLING
	// Datasheet states only LOW can be used with INT0/1 to wake from sleep, which is incorrect.
	// Ref: http://gammon.com.au/interrupts

	// Disable interrupts until going to sleep, otherwise interrupts occurring between attachInterrupt()
	// and sleep might cause the ATMega to not wakeup from sleep as interrupt has already be handled!
	cli();
	// attach interrupts
	_wakeUp1Interrupt  = interrupt1;
	_wakeUp2Interrupt  = interrupt2;

	// Attach external interrupt handlers, and clear any pending interrupt flag
	// to prevent waking immediately again.
	// Ref: https://forum.arduino.cc/index.php?topic=59217.0
	if (interrupt1 != INVALID_INTERRUPT_NUM) {
		clearPendingInterrupt(interrupt1);
		attachInterrupt(interrupt1, wakeUp1, mode1);
	}
	if (interrupt2 != INVALID_INTERRUPT_NUM) {
		clearPendingInterrupt(interrupt2);
		attachInterrupt(interrupt2, wakeUp2, mode2);
	}

	sleepRemainingMs = 0ul;
	if (ms > 0u) {
		// sleep for defined time
		sleepRemainingMs = hwInternalSleep(ms);
	} else {
		// sleep until ext interrupt triggered
		hwPowerDown(WDTO_SLEEP_FOREVER);
	}

	// Assure any interrupts attached, will get detached when they did not occur.
	if (interrupt1 != INVALID_INTERRUPT_NUM) {
		detachInterrupt(interrupt1);
	}
	if (interrupt2 != INVALID_INTERRUPT_NUM) {
		detachInterrupt(interrupt2);
	}

	// Return what woke the mcu.
	// Default: no interrupt triggered, timer wake up
	int8_t ret = MY_WAKE_UP_BY_TIMER;
	if (interruptWakeUp()) {
		ret = static_cast<int8_t>(_wokeUpByInterrupt);
	}
	// Clear woke-up-by-interrupt flag, so next sleeps won't return immediately.
	_wokeUpByInterrupt = INVALID_INTERRUPT_NUM;

	return ret;

//   // TODO: Not supported!
//   (void)interrupt1;
//   (void)mode1;
//   (void)interrupt2;
//   (void)mode2;
//   (void)ms;
//   return MY_SLEEP_NOT_POSSIBLE;
}

uint32_t hwGetSleepRemaining(void)
{
	return sleepRemainingMs;
}

inline void hwRandomNumberInit(void)
{
	// This function initializes the random number generator with a seed
	// of 32 bits.  This method is good enough to earn FIPS 140-2 conform
	// random data. This should reach to generate 32 Bit for randomSeed().
	uint32_t seed = 0;
	uint32_t timeout = millis() + 20;

	// Trigger floating effect of an unconnected pin
	pinMode(MY_SIGNING_SOFT_RANDOMSEED_PIN, INPUT_PULLUP);
	pinMode(MY_SIGNING_SOFT_RANDOMSEED_PIN, INPUT);
	delay(10);

	// Generate 32 bits of datas
	for (uint8_t i=0; i<32; i++) {
		const int pinValue = analogRead(MY_SIGNING_SOFT_RANDOMSEED_PIN);
		// Wait until the analog value has changed
		while ((pinValue == analogRead(MY_SIGNING_SOFT_RANDOMSEED_PIN)) && (timeout>=millis())) {
			seed ^= (millis() << i);
			// Check of data generation is slow
			if (timeout<=millis()) {
				// Trigger pin again
				pinMode(MY_SIGNING_SOFT_RANDOMSEED_PIN, INPUT_PULLUP);
				pinMode(MY_SIGNING_SOFT_RANDOMSEED_PIN, INPUT);
				// Pause a short while
				delay(seed % 10);
				timeout = millis() + 20;
			}
		}
	}
	randomSeed(seed);
}

bool hwUniqueID(unique_id_t *uniqueID)
{
#if defined(__LGT8FX8E__) || defined(__LGT8FX8P__)
	// padding
	(void)memset(uniqueID, MY_HWID_PADDING_BYTE, sizeof(unique_id_t));
	// use GUID0-3
	uint32_t guid = *((uint32_t *)&GUID0);
	(void)memcpy((void *)uniqueID, (void *)&GUID0, 4);
	return true; // unique ID returned
#else
	return false;	// no unique ID returned
#endif
}

uint16_t hwCPUVoltage(void)
{
#if defined(__LGT8FX8E__) || defined(__LGT8FX8P__)
	// Measure Vcc against 2.56V Vref 2.048/(4/5)
	analogReference(INTERNAL2V56);
	delay(1);
	uint16_t value = analogRead(VCCM);
	// return Vcc in mV
	return value*2.56*1000/1024;
#else
#error Not supported
#endif
}

uint16_t hwCPUFrequency(void)	//TODO：lgt8f
{
	// return frequency in 1/10MHz (accuracy +- 10%)
	return F_CPU / 100000UL;;
}

int8_t hwCPUTemperature(void)	//TODO：lgt8f
{
	return -127; // not available
}

uint16_t hwFreeMem(void)
{
	extern int __heap_start, *__brkval;
	int v;
	return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}
