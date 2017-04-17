/**
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of
 * the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2015 Sensnology AB
 * Copyright (C) 2017 Frank Holtz
 * Full contributor list:
 * https://github.com/mysensors/Arduino/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

#include "MyHwNRF5.h"

#define INVALID_INTERRUPT_NUM (0xFFu)

volatile uint8_t _wokeUpByInterrupt =
    INVALID_INTERRUPT_NUM; // Interrupt number that woke the mcu.
volatile uint8_t _wakeUp1Interrupt =
    INVALID_INTERRUPT_NUM; // Interrupt number for wakeUp1-callback.
volatile uint8_t _wakeUp2Interrupt =
    INVALID_INTERRUPT_NUM; // Interrupt number for wakeUp2-callback.

void wakeUp1() // place to send the interrupts
{
	_wokeUpByInterrupt = _wakeUp1Interrupt;
}
void wakeUp2() // place to send the second interrupts
{
	_wokeUpByInterrupt = _wakeUp2Interrupt;
}

void hwReadConfigBlock(void *buf, void *adr, size_t length)
{
	uint8_t *dst = static_cast<uint8_t *>(buf);
	const int offs = reinterpret_cast<int>(adr);
	(void)NVRAM.read_block(dst, offs, length);
}

void hwWriteConfigBlock(void *buf, void *adr, size_t length)
{
	uint8_t *src = static_cast<uint8_t *>(buf);
	const int offs = reinterpret_cast<int>(adr);
	// use update() instead of write() to reduce e2p wear off
	(void)NVRAM.write_block(src, offs, length);
}

uint8_t hwReadConfig(int adr)
{
	return NVRAM.read(adr);
}

void hwWriteConfig(int adr, uint8_t value)
{
	(void)NVRAM.write(adr, value);
}

bool hwInit()
{

#ifdef MY_DISABLED_SERIAL
	// Disable UART, when not configured
	NRF_UART0->POWER = 1;
#else
	// Confiure UART
	MY_SERIALDEVICE.begin(MY_BAUD_RATE);
#if defined(MY_GATEWAY_SERIAL)
	while (!MY_SERIALDEVICE) {
	}
#endif
#endif

	return true;
}

void hwRandomNumberInit()
{
	// Start HWRNG
#ifdef NRF51
	NRF_RNG->POWER = 1;
#endif
	NRF_RNG->TASKS_START = 1;
	NRF_RNG->EVENTS_VALRDY = 0;
	NRF_RNG->CONFIG = RNG_CONFIG_DERCEN_Enabled << RNG_CONFIG_DERCEN_Pos;

	uint32_t seed = 0;
	for (uint8_t i = 0; i < 4; i++) {
		// Wait for an random number
		while (NRF_RNG->EVENTS_VALRDY == 0) {
			yield();
		}
		seed = (seed << 8) | (uint32_t)NRF_RNG->VALUE;
		NRF_RNG->EVENTS_VALRDY = 0;
	}
	randomSeed(seed);

	// Stop HWRNG
	NRF_RNG->TASKS_STOP = 1;
#ifdef NRF51
	NRF_RNG->POWER = 0;
#endif
}

void hwWatchdogReset()
{
	NRF_WDT->RR[0] = WDT_RR_RR_Reload;
}

void hwReboot()
{
	NVIC_SystemReset();
	while (true)
		;
}

static volatile bool nrf5_rtc_event_triggered;
static volatile bool nrf5_pwr_hfclk;

void hwSleepPrepare(unsigned long ms)
{
	// Enable low power sleep mode
	NRF_POWER->TASKS_LOWPWR = 1;

	if (ms > 0) {
		// Configure RTC
#ifdef NRF51
		MY_HW_RTC->POWER = 1;
#endif
		// Reset RTC
		MY_HW_RTC->TASKS_CLEAR = 1;

		// Calculate sleep time
		// 8 Hz -> max 582.542 hours sleep.
		MY_HW_RTC->PRESCALER = 4095;
		// Set compare register to 1/125ms + 2 to garantee event triggering
		MY_HW_RTC->CC[0] = (ms / 125) + 2;

		MY_HW_RTC->INTENSET = RTC_INTENSET_COMPARE0_Msk;
		MY_HW_RTC->EVTENSET = RTC_EVTENSET_COMPARE0_Msk;
		MY_HW_RTC->EVENTS_COMPARE[0] = 0;
		nrf5_rtc_event_triggered = false;
		MY_HW_RTC->TASKS_START = 1;
		NVIC_SetPriority(MY_HW_RTC_IRQN, 15);
		NVIC_ClearPendingIRQ(MY_HW_RTC_IRQN);
		NVIC_EnableIRQ(MY_HW_RTC_IRQN);
	} else {
		NRF_RTC1->TASKS_STOP = 1;
	}

	// Stop HFCLK
	nrf5_pwr_hfclk = NRF_CLOCK->EVENTS_HFCLKSTARTED;
	NRF_CLOCK->TASKS_HFCLKSTOP = 1;

	// Idle serial device
#ifndef MY_DISABLED_SERIAL
	NRF_UART0->TASKS_STOPRX = 1;
	NRF_UART0->TASKS_STOPTX = 1;
	NRF_UART0->TASKS_SUSPEND = 1;
#endif

	// Clear nvram if needed and a time frame of 4 seconds available
	if (ms > 40000) {
		// preserve some bytes for writing to NVRAM
		NVRAM.clean_up(16);
	}
}

void hwSleepEnd(unsigned long ms)
{
	// Start HFCLK
	if (nrf5_pwr_hfclk) {
		NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
		NRF_CLOCK->TASKS_HFCLKSTART = 1;
		while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0)
			;
		// Enable low latency sleep mode
		NRF_POWER->TASKS_CONSTLAT = 1;
	}

	if (ms > 0) {
		// Stop RTC
#ifdef NRF51
		MY_HW_RTC->POWER = 0;
#endif
		MY_HW_RTC->INTENCLR = RTC_INTENSET_COMPARE0_Msk;
		MY_HW_RTC->EVTENCLR = RTC_EVTENSET_COMPARE0_Msk;
		MY_HW_RTC->TASKS_STOP = 1;
		NVIC_DisableIRQ(MY_HW_RTC_IRQN);
	} else {
		// Start Arduino RTC for millis()
		NRF_RTC1->TASKS_START = 1;
	}

	// Start serial device
#ifndef MY_DISABLED_SERIAL
	NRF_UART0->TASKS_STARTRX = 1;
	NRF_UART0->TASKS_STARTTX = 1;
#endif
}

inline void hwWaitForInterrupt()
{
	__DSB();
	__WFI();
}

// Sleep in System ON mode
inline void hwSleep()
{
	__WFE();
	__SEV();
	__WFE();
}

int8_t hwSleep(unsigned long ms)
{
	hwSleepPrepare(ms);
	while (nrf5_rtc_event_triggered == false) {
		hwSleep();
	}
	hwSleepEnd(ms);
	return MY_WAKE_UP_BY_TIMER;
}

int8_t hwSleep(uint8_t interrupt, uint8_t mode, unsigned long ms)
{
	return hwSleep(interrupt, mode, INVALID_INTERRUPT_NUM, 0u, ms);
}

int8_t hwSleep(uint8_t interrupt1, uint8_t mode1, uint8_t interrupt2,
               uint8_t mode2, unsigned long ms)
{
	// Disable interrupts until going to sleep, otherwise interrupts occurring
	// between attachInterrupt()
	// and sleep might cause the MCU to not wakeup from sleep as interrupt has
	// already be handled!
	MY_CRITICAL_SECTION {
		// attach interrupts
		_wakeUp1Interrupt = interrupt1;
		_wakeUp2Interrupt = interrupt2;

		if (interrupt1 != INVALID_INTERRUPT_NUM)
		{
			attachInterrupt(interrupt1, wakeUp1, mode1);
		}
		if (interrupt2 != INVALID_INTERRUPT_NUM)
		{
			attachInterrupt(interrupt2, wakeUp2, mode2);
		}

		// Reset attrubute
		_wokeUpByInterrupt = INVALID_INTERRUPT_NUM;
	}

	// Prepare Timer and Hardware
	hwSleepPrepare(ms);

	// Sleep until timeout or interrupt
	while ((nrf5_rtc_event_triggered == false) and
	        (_wokeUpByInterrupt == INVALID_INTERRUPT_NUM)) {
		hwSleep();
	}

	// Assure any interrupts attached, will get detached when they did not occur.
	if (interrupt1 != INVALID_INTERRUPT_NUM) {
		detachInterrupt(interrupt1);
	}
	if (interrupt2 != INVALID_INTERRUPT_NUM) {
		detachInterrupt(interrupt2);
	}

	// Wake up Hardware
	hwSleepEnd(ms);

	// Return what woke the mcu.
	int8_t ret =
	    MY_WAKE_UP_BY_TIMER; // default: no interrupt triggered, timer wake up
	if (_wokeUpByInterrupt != INVALID_INTERRUPT_NUM) {
		ret = static_cast<int8_t>(_wokeUpByInterrupt);
	}
	// Clear woke-up-by-interrupt flag, so next sleeps won't return immediately.
	_wokeUpByInterrupt = INVALID_INTERRUPT_NUM;

	return ret;
}

extern "C" {
	void MY_HW_RTC_IRQ_HANDLER(void)
	{
		if (MY_HW_RTC->EVENTS_COMPARE[0] > 0) {
			nrf5_rtc_event_triggered = true;

			NRF_RESET_EVENT(MY_HW_RTC->EVENTS_COMPARE[0]);
		}
	}
}

#if defined(MY_DEBUG) || defined(MY_SPECIAL_DEBUG)
uint16_t hwCPUVoltage()
{
	return 0;
}

uint16_t hwCPUFrequency()
{
	return F_CPU / 100000UL;
}

uint16_t hwFreeMem()
{
	// TODO: Not supported!
	return 0;
}
#endif

#ifdef MY_DEBUG
void hwDebugPrint(const char *fmt, ...)
{
	if (MY_SERIALDEVICE) {
		char fmtBuffer[MY_SERIAL_OUTPUT_SIZE];
#ifdef MY_GATEWAY_FEATURE
		// prepend debug message to be handled correctly by controller (C_INTERNAL,
		// I_LOG_MESSAGE)
		snprintf(fmtBuffer, sizeof(fmtBuffer), PSTR("0;255;%d;0;%d;"), C_INTERNAL,
		         I_LOG_MESSAGE);
		MY_SERIALDEVICE.print(fmtBuffer);
#endif
		va_list args;
		va_start(args, fmt);
#ifdef MY_GATEWAY_FEATURE
		// Truncate message if this is gateway node
		vsnprintf(fmtBuffer, sizeof(fmtBuffer), fmt, args);
		fmtBuffer[sizeof(fmtBuffer) - 2] = '\n';
		fmtBuffer[sizeof(fmtBuffer) - 1] = '\0';
#else
		vsnprintf(fmtBuffer, sizeof(fmtBuffer), fmt, args);
#endif
		va_end(args);
		MY_SERIALDEVICE.print(fmtBuffer);
		//	MY_SERIALDEVICE.flush();

		// MY_SERIALDEVICE.write(freeRam());
	}
}
#endif
