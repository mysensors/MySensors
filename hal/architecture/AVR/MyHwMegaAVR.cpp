/*
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2022 Sensnology AB
 * Full contributor list: https://github.com/mysensors/MySensors/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

#include "MyHwMegaAVR.h"

void hwRtcInit(void)
{
	// Initialize RTC to 1kHz internal oscillator
	while (RTC.STATUS > 0) { } /* Wait for all register to be synchronized */

	RTC.CTRLA = RTC_PRESCALER_DIV32768_gc /* 32768 */
							| 0 << RTC_RTCEN_bp       /* Enable: disabled */
							| 0 << RTC_RUNSTDBY_bp;   /* Run In Standby: disabled */

	RTC.CLKSEL = RTC_CLKSEL_INT1K_gc; /* 32KHz divided by 32 */
}

bool hwInit(void)
{
	hwRtcInit();
#if !defined(MY_DISABLED_SERIAL)
   MY_SERIALDEVICE.begin(MY_BAUD_RATE);
#if defined(MY_GATEWAY_SERIAL)
   while (!MY_SERIALDEVICE) {}
#endif
#endif
   return true;
}

#define PIT_SLEEP_FOREVER      0

volatile uint8_t _wokeUpByInterrupt =
   INVALID_INTERRUPT_NUM;    // Interrupt number that woke the mcu.
volatile uint8_t _wakeUp1Interrupt  =
   INVALID_INTERRUPT_NUM;    // Interrupt number for wakeUp1-callback.
volatile uint8_t _wakeUp2Interrupt  =
   INVALID_INTERRUPT_NUM;    // Interrupt number for wakeUp2-callback.

static uint32_t sleepRemainingMs = 0ul;

void wakeUp1(void)
{
	HARDWARE_DEBUG(PSTR("wakeUp1(%" PRIu8 ")\n"), _wakeUp1Interrupt);
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
	HARDWARE_DEBUG(PSTR("wakeUp2(%" PRIu8 ")\n"), _wakeUp2Interrupt);
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
   // TODO: Check do I need that
   //EIFR = _BV(interrupt);
}

void pit_reset()
{
	RTC.PITCTRLA = 0;
	RTC.PITINTCTRL = 0;
}

void hwPitRtcInit(uint8_t cycles) {
	while (RTC.PITSTATUS > 0) { }/* Wait for all register to be synchronized */

	/*
		RTC_PERIOD_CYC16_gc = (0x03<<3),  /* RTC Clock Cycles 16
		RTC_PERIOD_CYC32_gc = (0x04<<3),  /* RTC Clock Cycles 32
		RTC_PERIOD_CYC64_gc = (0x05<<3),  /* RTC Clock Cycles 64
		RTC_PERIOD_CYC128_gc = (0x06<<3),  /* RTC Clock Cycles 128
		RTC_PERIOD_CYC256_gc = (0x07<<3),  /* RTC Clock Cycles 256
		RTC_PERIOD_CYC512_gc = (0x08<<3),  /* RTC Clock Cycles 512
		RTC_PERIOD_CYC1024_gc = (0x09<<3),  /* RTC Clock Cycles 1024
		RTC_PERIOD_CYC2048_gc = (0x0A<<3),  /* RTC Clock Cycles 2048
		RTC_PERIOD_CYC4096_gc = (0x0B<<3),  /* RTC Clock Cycles 4096
		RTC_PERIOD_CYC8192_gc = (0x0C<<3),  /* RTC Clock Cycles 8192
		RTC_PERIOD_CYC16384_gc = (0x0D<<3),  /* RTC Clock Cycles 16384
		RTC_PERIOD_CYC32768_gc = (0x0E<<3),  /* RTC Clock Cycles 32768
	*/
	RTC.PITCTRLA = cycles
							| 1 << RTC_PITEN_bp;   /* Enable: enabled */

	RTC.PITINTCTRL = 1 << RTC_PI_bp; /* Periodic Interrupt: enabled */
}

uint8_t getSleepingPeriod(uint32_t ms)
{
	for (uint8_t period = 14u; period > 2; --period) {
		const uint16_t comparatorMS = (32768 >> (15 - period));
		if (ms >= comparatorMS)
		{
			return period;
		}
	}

	return 3; // 16ms with 1kHz internal oscillator
}

uint32_t hwPowerDown(uint32_t ms)
{
	HARDWARE_DEBUG(PSTR("hwPowerDown(%" PRIu32 ")\n"), ms);
	// Let serial prints finish (debug, log etc)
#ifndef MY_DISABLED_SERIAL
	MY_SERIALDEVICE.flush();
#endif

	// disable ADC for power saving
	ADC0.CTRLA &= ~ADC_ENABLE_bm;             // ADC off

	if (ms != PIT_SLEEP_FOREVER) {

		// Sleeping with watchdog only supports multiples of 16ms.
		// Round up to next multiple of 16ms, to assure we sleep at least the
		// requested amount of time. Sleep of 0ms will not sleep at all!
		ms += 15u;

		while (!interruptWakeUp() && ms >= 16) {
			uint8_t period = getSleepingPeriod(ms);
			const uint16_t comparatorMS = 32768 >> (15 - period);

			cli();
			hwPitRtcInit(period << 3);
			sei();

			HARDWARE_DEBUG(PSTR("   hwPowerDown(%" PRIu32 ", %" PRIu16 ", %" PRIu8 ")\n"), ms, comparatorMS, period << 3);

			set_sleep_mode(SLEEP_MODE_PWR_DOWN);
			sleep_enable();
			sleep_cpu();
			sleep_disable();

			cli();
			RTC.PITCTRLA = 0; /* RTC Clock Cycles Off, Enable: disabled */
			sei();

			ms -= comparatorMS;
   	}
	} else {
		// Eternal sleep
		set_sleep_mode(SLEEP_MODE_PWR_DOWN);
		sleep_enable();
		sleep_cpu();
		sleep_disable();

		sei();
	}

	ADC0.CTRLA |= ADC_ENABLE_bm;             // ADC on

	HARDWARE_DEBUG(PSTR("hwPowerUp(%" PRIu32 ")\n"), ms);
	if (interruptWakeUp()) {
      return ms;
   }
   return 0ul;
}

ISR (RTC_PIT_vect) {
   RTC.PITINTFLAGS = 1; // Clear interrupt flag
}

int8_t hwSleep(uint32_t ms)
{
   // Return what woke the mcu.
   // Default: no interrupt triggered, timer wake up
   int8_t ret = MY_WAKE_UP_BY_TIMER;
   sleepRemainingMs = 0ul;
   if (ms > 0u) {
      // sleep for defined time
      sleepRemainingMs = hwPowerDown(ms);
   } else {
      // sleep until ext interrupt triggered
      hwPowerDown(PIT_SLEEP_FOREVER);
   }
   if (interruptWakeUp()) {
      ret = static_cast<int8_t>(_wokeUpByInterrupt);
   }
   // Clear woke-up-by-interrupt flag, so next sleeps won't return immediately.
   _wokeUpByInterrupt = INVALID_INTERRUPT_NUM;

   return ret;
}

int8_t hwSleep(const uint8_t interrupt, const uint8_t mode, uint32_t ms)
{
   return hwSleep(interrupt, mode, INVALID_INTERRUPT_NUM, 0u, ms);
}

int8_t hwSleep(const uint8_t interrupt1, const uint8_t mode1, const uint8_t interrupt2,
               const uint8_t mode2,
               uint32_t ms)
{
   // ATMega328P supports following modes to wake from sleep: LOW, CHANGE, RISING, FALLING
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
			HARDWARE_DEBUG(PSTR("attachInterrupt(%" PRIu8 ")\n"), interrupt1);

      clearPendingInterrupt(interrupt1);
      attachInterrupt(interrupt1, wakeUp1, mode1);
   }
   if (interrupt2 != INVALID_INTERRUPT_NUM) {
			HARDWARE_DEBUG(PSTR("attachInterrupt(%" PRIu8 ")\n"), interrupt2);
      clearPendingInterrupt(interrupt2);
      attachInterrupt(interrupt2, wakeUp2, mode2);
   }

   sleepRemainingMs = 0ul;
   if (ms > 0u) {
      // sleep for defined time
      sleepRemainingMs = hwPowerDown(ms);
   } else {
      // sleep until ext interrupt triggered
      hwPowerDown(PIT_SLEEP_FOREVER);
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
}

uint32_t hwGetSleepRemaining(void)
{
	HARDWARE_DEBUG(PSTR("hwGetSleepRemaining(%" PRIu32 ")\n"), sleepRemainingMs);
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
   // padding
   (void)memset(uniqueID, MY_HWID_PADDING_BYTE, sizeof(unique_id_t));
   // no unique ID for non-PB AVR, use HW specifics for diversification
   *((uint8_t *)uniqueID) = SIGNATURE_2;
   *((uint8_t *)uniqueID + 1) = SIGNATURE_1;
   *((uint8_t *)uniqueID + 2) = SIGNATURE_0;
   *((uint8_t *)uniqueID + 3) = SIGROW.DEVICEID0;
   *((uint8_t *)uniqueID + 4) = SIGROW.DEVICEID1;
   *((uint8_t *)uniqueID + 5) = SIGROW.DEVICEID2;

	// SIGROW.SERNUM[9:0].
   for(uint8_t idx = 0; idx < 10; idx++) {
      *((uint8_t *)uniqueID + 6 + idx) = *(&SIGROW.SERNUM0 + idx);
   }


   return true; // unique ID returned
}

uint16_t hwCPUVoltage(void)
{
	analogReference(INTERNAL1V024);
	int voltage = analogRead(ADC_VDDDIV10);

	HARDWARE_DEBUG(PSTR("hwCPUVoltage( %" PRIi16 ")\n"), (voltage * 10));

	return voltage  * (uint16_t) 10.24; // value in mV
}

uint16_t hwCPUFrequency(void)
{
	HARDWARE_DEBUG(PSTR("hwCPUVoltage( %" PRIu16 ")\n"), ( F_CPU / 100000UL));
#ifdef F_CPU
	return F_CPU / 100000UL;
#else
	cli();
	const uint8_t rtcPitCtrlA = RTC.PITCTRLA;
	const uint8_t rtcPitIntCtrl = RTC.PITINTCTRL;
	const uint8_t rtcPitPitIntFlags = RTC.PITINTFLAGS;

	const uint8_t tcaEvCtrl = TCA0.SINGLE.EVCTRL;
	const uint8_t tcaCtrlA = TCA0.SINGLE.CTRLA;


	// setup timer1
	TCA0.SINGLE.CNT = 0x0; /* Count: 0x0 */

	TCA0.SINGLE.EVCTRL = 0 << TCA_SINGLE_CNTAEI_bp           /* Count on Event Input A: disabled */
										| 0 << TCA_SINGLE_CNTBEI_bp            /* Count on Event Input B: disabled */
										| TCA_SINGLE_EVACTA_CNT_POSEDGE_gc     /* Count on positive edge event */
										| TCA_SINGLE_EVACTB_UPDOWN_gc;         /* Count on prescaled clock. Event controls count direction.
																															Up-count when event line is 0, down-count when event line
																															is 1. */
	TCA0.SINGLE.CTRLA = TCA_SINGLE_CLKSEL_DIV1024_gc         /* System Clock */
										| 1 << TCA_SINGLE_ENABLE_bp            /* Module Enable: enabled */
										| 0 << TCA_SINGLE_RUNSTDBY_bp;         /* Run Standby: disabled */

	// set pit
	while (RTC.PITSTATUS > 0) {}                             /* Wait for all register to be synchronized */
	RTC.PITCTRLA = RTC_PERIOD_CYC16384_gc                    /* RTC Clock Cycles 16384 - 500ms */
									| 1 << RTC_PITEN_bp;                      /* Enable: enabled */
	RTC.PITINTCTRL = 1 << RTC_PI_bp;                         /* Periodic Interrupt: enabled */

	// wait for pit to interrupt
	while (bit_is_clear(RTC.PITINTFLAGS, RTC_PI_bp)) {};

	TCA0.SINGLE.CTRLA = 0;                                   /* Enable: disabled */

	RTC.PITCTRLA &= ~(1 << RTC_PITEN_bp);                    /* Enable: disabled */

	sei();

	uint16_t count = TCA0.SINGLE.CNT;
	const uint16_t result = (TCA0.SINGLE.CNT * 2048UL) / 100000UL;

	// Reset timers
	RTC.PITCTRLA = rtcPitCtrlA;
	RTC.PITINTCTRL = rtcPitIntCtrl;
	RTC.PITINTFLAGS = rtcPitPitIntFlags;

	TCA0.SINGLE.EVCTRL = tcaEvCtrl;
	TCA0.SINGLE.CTRLA = tcaCtrlA;

	// return frequency in 1/10MHz (accuracy +- 10%)
   return result;
#endif
}

int8_t hwCPUTemperature(void)
{
	analogReference(INTERNAL1V024);
	uint16_t adc_reading = analogReadEnh(ADC_TEMPERATURE, 12);

	int8_t sigrow_offset = SIGROW.TEMPSENSE1; // Read signed offset from signature row
	uint8_t sigrow_gain = SIGROW.TEMPSENSE0; 	// Read unsigned gain/slope from signature row

	uint32_t temp = adc_reading - sigrow_offset;
	temp *= sigrow_gain; 											// Result might overflow 16-bit variable (10-bit + 8-bit)
	temp += 0x80; 														// Add 256/2 to get correct integer rounding on division below
	temp >>= 8; 															// Divide result by 256 to get processed temperature in Kelvin
	uint16_t temperature_in_K = temp;

	return temperature_in_K - 273.15; 				// value in Celsius
}

uint16_t hwFreeMem(void)
{
   extern int __heap_start, *__brkval;
   int v;
   return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}
