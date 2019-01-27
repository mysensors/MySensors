/*
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of
 * the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2019 Sensnology AB
 * Copyright (C) 2017 Frank Holtz
 * Full contributor list:
 * https://github.com/mysensors/MySensors/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

#include "MyHwNRF5.h"
#include <nrf.h>

#define CRYPTO_LITTLE_ENDIAN

#define INVALID_INTERRUPT_NUM (0xFFu)

volatile uint8_t _wokeUpByInterrupt = INVALID_INTERRUPT_NUM; // Interrupt number that woke the mcu.
volatile uint8_t _wakeUp1Interrupt =
    INVALID_INTERRUPT_NUM; // Interrupt number for wakeUp1-callback.
volatile uint8_t _wakeUp2Interrupt =
    INVALID_INTERRUPT_NUM; // Interrupt number for wakeUp2-callback.

void wakeUp1(void) // place to send the interrupts
{
	_wokeUpByInterrupt = _wakeUp1Interrupt;
}
void wakeUp2(void) // place to send the second interrupts
{
	_wokeUpByInterrupt = _wakeUp2Interrupt;
}

void hwReadConfigBlock(void *buf, void *addr, size_t length)
{
	uint8_t *dst = static_cast<uint8_t *>(buf);
	const int offs = reinterpret_cast<int>(addr);
	(void)NVRAM.read_block(dst, offs, length);
}

void hwWriteConfigBlock(void *buf, void *addr, size_t length)
{
	uint8_t *src = static_cast<uint8_t *>(buf);
	const int offs = reinterpret_cast<int>(addr);
	(void)NVRAM.write_block(src, offs, length);
}

uint8_t hwReadConfig(const int addr)
{
	return NVRAM.read(addr);
}

void hwWriteConfig(const int addr, uint8_t value)
{
	(void)NVRAM.write(addr, value);
}

bool hwInit(void)
{
#ifdef MY_LOCK_MCU
#ifdef NRF51
	// Lock MCU
	if((uint32_t)((NRF_UICR->RBPCONF & UICR_RBPCONF_PALL_Msk) >> UICR_RBPCONF_PALL_Pos) !=
	        UICR_RBPCONF_PALL_Enabled) {
		Flash.write((uint32_t *)&NRF_UICR->RBPCONF, (NRF_UICR->RBPCONF & ~UICR_RBPCONF_PALL_Msk));
		hwReboot();
	}
#else
	// Lock MCU
	if((uint32_t)((NRF_UICR->APPROTECT & UICR_APPROTECT_PALL_Msk) >> UICR_APPROTECT_PALL_Pos) !=
	        UICR_APPROTECT_PALL_Enabled) {
		Flash.write((uint32_t *)&NRF_UICR->APPROTECT, (NRF_UICR->APPROTECT & ~UICR_APPROTECT_PALL_Msk));
		hwReboot();
	}
#endif
#endif

#if defined(NRF51) && defined(CONFIG_ENABLE_PINRESET)
	// Enabling reset for NRF51 isn't handled by arduino-nrf5. Enable it, if requested.
	NRF_POWER->RESET = POWER_RESET_RESET_Enabled;
	NRF_POWER->RAMON |= (POWER_RAMON_ONRAM0_RAM0On << POWER_RAMON_ONRAM0_Pos) |
	                    (POWER_RAMON_ONRAM1_RAM1On << POWER_RAMON_ONRAM1_Pos);
#endif

	// Clock is manged by sleep modes. Radio depends on HFCLK.
	// Force to start HFCLK
	NRF_CLOCK->EVENTS_HFCLKSTARTED = 0;
	NRF_CLOCK->TASKS_HFCLKSTART = 1;
	while (NRF_CLOCK->EVENTS_HFCLKSTARTED == 0)
		;

	// Enable low latency sleep mode
	NRF_POWER->TASKS_CONSTLAT = 1;

	// Enable cache on >= NRF52
#ifndef NRF51
	NRF_NVMC->ICACHECNF = NVMC_ICACHECNF_CACHEEN_Msk;
#endif

	// Suspend UART
	NRF_UART0->TASKS_STOPRX = 1;
	NRF_UART0->TASKS_STOPTX = 1;
	NRF_UART0->TASKS_SUSPEND = 1;

#ifdef MY_DISABLED_SERIAL
	// Disable UART, when not configured
#ifdef NRF51
	NRF_UART0->POWER = 0;
#endif
#else
	// Configure UART
	MY_SERIALDEVICE.begin(MY_BAUD_RATE);
#if defined(MY_GATEWAY_SERIAL)
	while (!MY_SERIALDEVICE) {
	}
#endif
#endif

	return true;
}

static nrf_ecb_t hwRngData;
static int8_t hwRndDataReadPos = -1;

void hwRandomNumberInit(void)
{
	// Start HWRNG
#ifdef NRF51
	NRF_RNG->POWER = 1;
#endif
	// Enable "more random" numbers
	NRF_RNG->CONFIG = RNG_CONFIG_DERCEN_Enabled << RNG_CONFIG_DERCEN_Pos;
	NRF_RNG->TASKS_START = 1;
	NRF_RNG->EVENTS_VALRDY = 0;

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

	// Fill ESB data structure for fast random data generation
	uint8_t *ecbstruct = (uint8_t *)&hwRngData;
	for (uint8_t i = 0; i<sizeof(hwRngData); i++) {
		while (NRF_RNG->EVENTS_VALRDY == 0) {
			yield();
		}
		ecbstruct[i] = NRF_RNG->VALUE;
		NRF_RNG->EVENTS_VALRDY = 0;
	}
	hwRndDataReadPos = 0;

	// Stop HWRNG
	NRF_RNG->TASKS_STOP = 1;
#ifdef NRF51
	NRF_RNG->POWER = 0;
#endif
}

ssize_t hwGetentropy(void *__buffer, size_t __length)
{
	if (hwRndDataReadPos<0) {
		// Not initialized
		hwRandomNumberInit();
	}

	// cut length if > 256
	if (__length > 256) {
		__length = 256;
	}
	uint8_t *dst = (uint8_t *)__buffer;

	// Start random number generator
	for (size_t i = 0; i < __length; i++) {
		dst[i] = hwRngData.ciphertext[hwRndDataReadPos & 0xfu];
		MY_CRITICAL_SECTION {
			if (hwRndDataReadPos >= ((int8_t)sizeof(hwRngData.ciphertext)-1))
			{
				// Retry until no error
				bool need_data = true;
				while (need_data) {
					// Stop if another task is running
					NRF_ECB->TASKS_STOPECB = 1;
					NRF_ECB->EVENTS_ERRORECB = 0;
					NRF_ECB->EVENTS_ENDECB = 0;
					uint32_t ptrbackup = NRF_ECB->ECBDATAPTR;
					NRF_ECB->ECBDATAPTR = (uint32_t)&hwRngData;
					NRF_ECB->TASKS_STARTECB = 1;
					while (!NRF_ECB->EVENTS_ENDECB);
					NRF_ECB->ECBDATAPTR = ptrbackup;
					if (NRF_ECB->EVENTS_ERRORECB == 0) {
						need_data = false;
					}
				}
				hwRndDataReadPos=0;
				for (uint8_t i=0; i<sizeof(hwRngData.ciphertext); i++) {
					hwRngData.cleartext[i] ^= hwRngData.ciphertext[i];
				}
			} else
			{
				hwRndDataReadPos++;
			}
		}
	}

	return __length;
}

void hwWatchdogReset(void)
{
	NRF_WDT->RR[0] = WDT_RR_RR_Reload;
}

void hwReboot(void)
{
	NVIC_SystemReset();
	while (true)
		;
}

static volatile bool nrf5_rtc_event_triggered;
static volatile bool nrf5_pwr_hfclk;

void hwSleepPrepare(uint32_t ms)
{
	// Enable low power sleep mode
	NRF_POWER->TASKS_LOWPWR = 1;

	// Reset RTC trigger flag
	nrf5_rtc_event_triggered = false;

	if (ms > 0) {
		// Configure RTC
#ifdef NRF51
		MY_HW_RTC->POWER = 1;
#endif
		// Reset RTC
		MY_HW_RTC->TASKS_CLEAR = 1;

		// Calculate sleep time and prescaler
		if (ms<512000) {
			// prescaler 0, 30.517 μs resolution -> max 512 s sleep
			MY_HW_RTC->PRESCALER =  0;
			// Set compare register to 1/30.517 µs to guarantee event triggering
			// A minimum of 2 ticks must be guaranteed
			// (1000/32768)<<12 == 125
			MY_HW_RTC->CC[0] = max(((ms << 12) / 125), 2);
		} else {
			// 8 Hz -> max 582.542 hours sleep.
			MY_HW_RTC->PRESCALER = 4095;
			// Set compare register to 1/125ms
			// A minimum of 2 ticks must be guaranteed
			MY_HW_RTC->CC[0] = max((ms / 125), 2);
		}

		MY_HW_RTC->INTENSET = RTC_INTENSET_COMPARE0_Msk;
		MY_HW_RTC->EVTENSET = RTC_EVTENSET_COMPARE0_Msk;
		MY_HW_RTC->EVENTS_COMPARE[0] = 0;
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

	// Clear NVRAM log, if needed and a time frame of 4 seconds available
	if (ms > 40000) {
		// preserve some bytes for writing to NVRAM
		NVRAM.clean_up(32);
	}
}

void hwSleepEnd(uint32_t ms)
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

// Halt CPU until next interrupt event
inline void hwWaitForInterrupt(void)
{
	__DSB();
	__WFI();
}

// Sleep in System ON mode
inline void hwSleep(void)
{
	__WFE();
	__SEV();
	__WFE();
}

int8_t hwSleep(uint32_t ms)
{
	hwSleepPrepare(ms);
	while (nrf5_rtc_event_triggered == false) {
		hwSleep();
	}
	hwSleepEnd(ms);
	return MY_WAKE_UP_BY_TIMER;
}

int8_t hwSleep(const uint8_t interrupt, const uint8_t mode, uint32_t ms)
{
	return hwSleep(interrupt, mode, INVALID_INTERRUPT_NUM, 0u, ms);
}

int8_t hwSleep(uint8_t interrupt1, uint8_t mode1, uint8_t interrupt2,
               uint8_t mode2, uint32_t ms)
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

		// Reset attribute
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
	// RTC interrupt handler
	void MY_HW_RTC_IRQ_HANDLER(void)
	{
		if (MY_HW_RTC->EVENTS_COMPARE[0] > 0) {
			nrf5_rtc_event_triggered = true;

			NRF_RESET_EVENT(MY_HW_RTC->EVENTS_COMPARE[0]);
		}
	}
}

bool hwUniqueID(unique_id_t *uniqueID)
{
	uint32_t *buffer = (uint32_t *)uniqueID;
	buffer[0] = NRF_FICR->DEVICEID[0];
	buffer[1] = NRF_FICR->DEVICEID[1];
	buffer[2] = NRF_FICR->DEVICEADDR[0];
	buffer[3] = NRF_FICR->DEVICEADDR[1];
	return true;
}

uint16_t hwCPUVoltage(void)
{
	// VDD is prescaled 1/3 and compared with the internal 1.2V reference
#if defined(NRF_ADC)
	// NRF51:
	// Sampling is done with lowest resolution to minimize the time
	// 20uS@260uA

	// Concurrent resource: disable
	uint32_t lpcomp_enabled = NRF_LPCOMP->ENABLE;
	NRF_LPCOMP->ENABLE = 0;

	// Enable and configure ADC
	NRF_ADC->ENABLE = 1;
	NRF_ADC->CONFIG = (ADC_CONFIG_EXTREFSEL_None << ADC_CONFIG_EXTREFSEL_Pos) |
	                  (ADC_CONFIG_PSEL_Disabled << ADC_CONFIG_PSEL_Pos) |
	                  (ADC_CONFIG_REFSEL_VBG << ADC_CONFIG_REFSEL_Pos) |
	                  (ADC_CONFIG_INPSEL_SupplyOneThirdPrescaling << ADC_CONFIG_INPSEL_Pos) |
	                  (ADC_CONFIG_RES_8bit << ADC_CONFIG_RES_Pos);
	NRF_ADC->EVENTS_END = 0;
	NRF_ADC->TASKS_START = 1;
	while(!NRF_ADC->EVENTS_END);
	NRF_ADC->EVENTS_END = 0;
	int32_t sample = (int32_t)NRF_ADC->RESULT;
	NRF_ADC->TASKS_STOP = 1;
	NRF_ADC->ENABLE = 0;

	// Restore LPCOMP state
	NRF_LPCOMP->ENABLE = lpcomp_enabled;

	return (sample*3600)/255;

#elif defined(NRF_SAADC)
	// NRF52:
	// Sampling time 3uS@700uA
	int32_t sample;
	NRF_SAADC->ENABLE = SAADC_ENABLE_ENABLE_Enabled << SAADC_ENABLE_ENABLE_Pos;
	NRF_SAADC->RESOLUTION = SAADC_RESOLUTION_VAL_8bit << SAADC_RESOLUTION_VAL_Pos;
	NRF_SAADC->CH[0].PSELP = SAADC_CH_PSELP_PSELP_VDD << SAADC_CH_PSELP_PSELP_Pos;
	NRF_SAADC->CH[0].CONFIG = (SAADC_CH_CONFIG_BURST_Disabled << SAADC_CH_CONFIG_BURST_Pos) |
	                          (SAADC_CH_CONFIG_MODE_SE << SAADC_CH_CONFIG_MODE_Pos) |
	                          (SAADC_CH_CONFIG_TACQ_3us << SAADC_CH_CONFIG_TACQ_Pos) |
	                          (SAADC_CH_CONFIG_REFSEL_Internal << SAADC_CH_CONFIG_REFSEL_Pos) |
	                          (SAADC_CH_CONFIG_GAIN_Gain1_6 << SAADC_CH_CONFIG_GAIN_Pos) |
	                          (SAADC_CH_CONFIG_RESN_Bypass << SAADC_CH_CONFIG_RESN_Pos) |
	                          (SAADC_CH_CONFIG_RESP_Bypass << SAADC_CH_CONFIG_RESP_Pos);
	NRF_SAADC->OVERSAMPLE = SAADC_OVERSAMPLE_OVERSAMPLE_Bypass << SAADC_OVERSAMPLE_OVERSAMPLE_Pos;
	NRF_SAADC->SAMPLERATE = SAADC_SAMPLERATE_MODE_Task << SAADC_SAMPLERATE_MODE_Pos;
	NRF_SAADC->RESULT.MAXCNT = 1;
	NRF_SAADC->RESULT.PTR = (uint32_t)&sample;

	NRF_SAADC->EVENTS_STARTED = 0;
	NRF_SAADC->TASKS_START = 1;
	while (!NRF_SAADC->EVENTS_STARTED);
	NRF_SAADC->EVENTS_STARTED = 0;

	NRF_SAADC->EVENTS_END = 0;
	NRF_SAADC->TASKS_SAMPLE = 1;
	while (!NRF_SAADC->EVENTS_END);
	NRF_SAADC->EVENTS_END = 0;

	NRF_SAADC->EVENTS_STOPPED = 0;
	NRF_SAADC->TASKS_STOP = 1;
	while (!NRF_SAADC->EVENTS_STOPPED);
	NRF_SAADC->EVENTS_STOPPED = 1;

	NRF_SAADC->ENABLE = (SAADC_ENABLE_ENABLE_Disabled << SAADC_ENABLE_ENABLE_Pos);

	return (sample*3600)/255;
#else
	// unknown MCU
	return 0;
#endif
}

uint16_t hwCPUFrequency(void)
{
#if defined(VARIANT_MCK)
	return (VARIANT_MCK) / 100000UL;
#elif defined(F_CPU)
	return (F_CPU) / 100000UL;
#else
	return 160;
#endif
}

int8_t hwCPUTemperature(void)
{
	return -127; // not implemented yet
}

uint16_t hwFreeMem(void)
{
	// TODO: Not supported!
	return FUNCTION_NOT_SUPPORTED;
}
