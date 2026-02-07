/*
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2026 Sensnology AB
 * Full contributor list: https://github.com/mysensors/MySensors/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

/**
 * @file MyHwSTM32.cpp
 * @brief Hardware abstraction layer for STM32 microcontrollers using STM32duino core
 *
 * This implementation uses the official STM32duino Arduino core which provides
 * STM32Cube HAL underneath. It supports a wide range of STM32 families including
 * F0, F1, F4, L0, L4, G0, G4, H7, and more.
 *
 * Tested on:
 * - STM32F401CC/CE Black Pill
 * - STM32F411CE Black Pill
 *
 * Pin Mapping Example (STM32F4 Black Pill):
 *
 * nRF24L01+ Radio (SPI1):
 * - SCK:  PA5
 * - MISO: PA6
 * - MOSI: PA7
 * - CSN:  PA4
 * - CE:   PB0 (configurable via MY_RF24_CE_PIN)
 *
 * RFM69/RFM95 Radio (SPI1):
 * - SCK:  PA5
 * - MISO: PA6
 * - MOSI: PA7
 * - CS:   PA4
 * - IRQ:  PA3 (configurable)
 * - RST:  PA2 (configurable)
 */

#include "MyHwSTM32.h"

// Sleep mode state variables
static volatile uint8_t _wokeUpByInterrupt = INVALID_INTERRUPT_NUM;
static volatile uint8_t _wakeUp1Interrupt = INVALID_INTERRUPT_NUM;
static volatile uint8_t _wakeUp2Interrupt = INVALID_INTERRUPT_NUM;
static uint32_t sleepRemainingMs = 0ul;

// RTC handle for wake-up timer
static RTC_HandleTypeDef hrtc = {0};
static bool rtcInitialized = false;

// Forward declarations for sleep helper functions
static bool hwSleepInit(void);
static bool hwSleepConfigureTimer(uint32_t ms);
static void hwSleepRestoreSystemClock(void);
static void wakeUp1ISR(void);
static void wakeUp2ISR(void);

bool hwInit(void)
{
#if !defined(MY_DISABLED_SERIAL)
	MY_SERIALDEVICE.begin(MY_BAUD_RATE);
#if defined(MY_GATEWAY_SERIAL)
	// Wait for serial port to connect (needed for native USB)
	while (!MY_SERIALDEVICE) {
		; // Wait for serial port connection
	}
#endif
#endif

	// STM32duino EEPROM library auto-initializes on first use
	// No explicit initialization required
	return true;
}

void hwReadConfigBlock(void *buf, void *addr, size_t length)
{
	uint8_t *dst = static_cast<uint8_t *>(buf);
	int pos = reinterpret_cast<int>(addr);

	for (size_t i = 0; i < length; i++) {
		dst[i] = EEPROM.read(pos + i);
	}
}

void hwWriteConfigBlock(void *buf, void *addr, size_t length)
{
	uint8_t *src = static_cast<uint8_t *>(buf);
	int pos = reinterpret_cast<int>(addr);

	for (size_t i = 0; i < length; i++) {
		EEPROM.update(pos + i, src[i]);
	}
}

uint8_t hwReadConfig(const int addr)
{
	return EEPROM.read(addr);
}

void hwWriteConfig(const int addr, uint8_t value)
{
	EEPROM.update(addr, value);
}

void hwWatchdogReset(void)
{
#if defined(HAL_IWDG_MODULE_ENABLED) && defined(IWDG)
	// Reset independent watchdog if enabled
	// Use direct register write to reload watchdog counter
	// This works whether IWDG was initialized by HAL or LL drivers
	IWDG->KR = IWDG_KEY_RELOAD;
#endif
}

void hwReboot(void)
{
	NVIC_SystemReset();
}

void hwRandomNumberInit(void)
{
	// Use internal temperature sensor and ADC noise as entropy source
	// This provides reasonably good random seed values

#ifdef ADC1
	uint32_t seed = 0;

	// Read multiple samples from different sources for entropy
	for (uint8_t i = 0; i < 32; i++) {
		uint32_t value = 0;

#ifdef TEMP_SENSOR_AVAILABLE
		// Try to read internal temperature sensor if available
		value ^= analogRead(ATEMP);
#endif

#ifdef VREF_AVAILABLE
		// Mix in internal voltage reference reading
		value ^= analogRead(AVREF);
#endif

		// Mix in current time
		value ^= hwMillis();

		// Mix in system tick
		value ^= micros();

		// Accumulate into seed
		seed ^= (value & 0x7) << (i % 29);

		// Small delay to ensure values change
		delayMicroseconds(100);
	}

	randomSeed(seed);
#else
	// Fallback: use millis as weak entropy source
	randomSeed(hwMillis());
#endif // ADC1
}

bool hwUniqueID(unique_id_t *uniqueID)
{
#ifdef UID_BASE
	// padding
	(void)memset(reinterpret_cast<uint8_t *>(uniqueID), MY_HWID_PADDING_BYTE, sizeof(unique_id_t));
	(void)memcpy(reinterpret_cast<uint8_t *>(uniqueID), (uint32_t *)UID_BASE, 12);
	return true;
#else
	// Unique ID not available on this variant
	return false;
#endif
}

uint16_t hwCPUVoltage(void)
{
#if defined(VREF_AVAILABLE) && defined(AVREF) && defined(__HAL_RCC_ADC1_CLK_ENABLE)
	// Read internal voltage reference to calculate VDD
	// VREFINT is typically 1.2V (varies by STM32 family)

	uint32_t vrefint = analogRead(AVREF);

	if (vrefint > 0) {
		// Calculate VDD in millivolts
		// Formula: VDD = 3.3V * 4096 / ADC_reading
		// Adjusted: VDD = 1200mV * 4096 / vrefint_reading
		return (uint16_t)((1200UL * 4096UL) / vrefint);
	}
#endif

	// Return typical 3.3V if measurement not available
	return 3300;
}

uint16_t hwCPUFrequency(void)
{
	// Return CPU frequency in 0.1 MHz units
	// F_CPU is defined by the build system (e.g., 84000000 for 84 MHz)
	return F_CPU / 100000UL;
}

int8_t hwCPUTemperature(void)
{
#if defined(TEMP_SENSOR_AVAILABLE) && defined(ATEMP) && defined(__HAL_RCC_ADC1_CLK_ENABLE)
	// Read internal temperature sensor
	// Note: Requires calibration values for accurate results

	int32_t temp_raw = analogRead(ATEMP);

#ifdef TEMP110_CAL_ADDR
	// Use factory calibration if available (STM32F4, L4, etc.)
	uint16_t *temp30_cal = (uint16_t *)TEMP30_CAL_ADDR;
	uint16_t *temp110_cal = (uint16_t *)TEMP110_CAL_ADDR;

	if (temp30_cal && temp110_cal && *temp110_cal != *temp30_cal) {
		// Calculate temperature using two-point calibration
		// Formula: T = ((110-30) / (CAL_110 - CAL_30)) * (raw - CAL_30) + 30
		int32_t temp = 30 + ((110 - 30) * (temp_raw - *temp30_cal)) /
		               (*temp110_cal - *temp30_cal);

		// Apply user calibration
		temp = (temp - MY_STM32_TEMPERATURE_OFFSET) / MY_STM32_TEMPERATURE_GAIN;

		return (int8_t)temp;
	}
#endif // TEMP110_CAL_ADDR

	// Fallback: use typical values (less accurate)
	// Typical slope: 2.5 mV/°C, V25 = 0.76V for STM32F4
	// This is a rough approximation
	float voltage = (temp_raw * 3.3f) / 4096.0f;
	int32_t temp = 25 + (int32_t)((voltage - 0.76f) / 0.0025f);

	return (int8_t)((temp - MY_STM32_TEMPERATURE_OFFSET) / MY_STM32_TEMPERATURE_GAIN);
#else
	// Temperature sensor not available
	return FUNCTION_NOT_SUPPORTED;
#endif
}

uint16_t hwFreeMem(void)
{
	// Calculate free heap memory
	// This uses newlib's mallinfo if available

#ifdef STACK_TOP
	extern char *__brkval;
	extern char __heap_start;

	char *heap_end = __brkval ? __brkval : &__heap_start;
	char stack_var;

	// Calculate space between heap and stack
	return (uint16_t)(&stack_var - heap_end);
#else
	// Alternative method: try to allocate and measure
	// Not implemented to avoid fragmentation
	return FUNCTION_NOT_SUPPORTED;
#endif
}

// ======================== Sleep Mode Helper Functions ========================

/**
 * @brief Initialize RTC for sleep wake-up timer
 * @return true if successful, false on error
 */
static bool hwSleepInit(void)
{
	if (rtcInitialized) {
		return true;
	}

	// Enable PWR clock
	__HAL_RCC_PWR_CLK_ENABLE();

	// Enable backup domain access
	HAL_PWR_EnableBkUpAccess();

	// Only reset backup domain if RTC is not already configured
	// This prevents disrupting other peripherals when MySensors radio is initialized first
	if ((RCC->BDCR & RCC_BDCR_RTCEN) != 0) {
		// RTC already enabled - check if it's the right clock source
		// If already configured, skip reset to avoid disrupting existing setup
	} else {
		// RTC not enabled - safe to reset backup domain for clean slate
		__HAL_RCC_BACKUPRESET_FORCE();
		HAL_Delay(10);
		__HAL_RCC_BACKUPRESET_RELEASE();
		HAL_Delay(10);
	}

	// Try LSE first (32.768 kHz external crystal - more accurate)
	// Fall back to LSI if LSE is not available
	bool useLSE = false;

	// Check if LSE is already running
	if ((RCC->BDCR & RCC_BDCR_LSERDY) != 0) {
		// LSE already ready - use it
		useLSE = true;
	} else {
		// Attempt to start LSE
		RCC->BDCR |= RCC_BDCR_LSEON;
		uint32_t timeout = 2000000;  // LSE takes longer to start
		while (((RCC->BDCR & RCC_BDCR_LSERDY) == 0) && (timeout > 0)) {
			timeout--;
		}

		if (timeout > 0) {
			// LSE started successfully
			useLSE = true;
		} else {
			// LSE failed - fall back to LSI
			if ((RCC->CSR & RCC_CSR_LSIRDY) == 0) {
				// LSI not ready, try to start it
				RCC->BDCR &= ~RCC_BDCR_LSEON;  // Disable failed LSE

				// Enable LSI (internal ~32 kHz oscillator)
				RCC->CSR |= RCC_CSR_LSION;
				timeout = 1000000;
				while (((RCC->CSR & RCC_CSR_LSIRDY) == 0) && (timeout > 0)) {
					timeout--;
				}

				if (timeout == 0) {
					return false;  // Both LSE and LSI failed
				}
			}
			// LSI ready (either was already running or just started)
		}
	}

	// Configure RTC clock source (only if not already configured correctly)
	uint32_t currentRtcSel = (RCC->BDCR & RCC_BDCR_RTCSEL);
	uint32_t desiredRtcSel = useLSE ? RCC_BDCR_RTCSEL_0 : RCC_BDCR_RTCSEL_1;

	if (currentRtcSel != desiredRtcSel) {
		// Need to change clock source - clear and set
		RCC->BDCR &= ~RCC_BDCR_RTCSEL;  // Clear selection
		RCC->BDCR |= desiredRtcSel;      // Set new selection
	}
	RCC->BDCR |= RCC_BDCR_RTCEN;  // Ensure RTC clock is enabled

	// Initialize RTC peripheral
	hrtc.Instance = RTC;

#if defined(STM32F1xx)
	// ============================================================
	// STM32F1: Legacy RTC with counter-based architecture
	// ============================================================
	// F1 RTC uses simple 32-bit counter with prescaler
	// No calendar, no wake-up timer - use RTC Alarm instead

	if (useLSE) {
		// LSE: 32.768 kHz exact - set prescaler for 1 Hz tick
		hrtc.Init.AsynchPrediv = 32767;  // (32767+1) = 32768 = 1 Hz
	} else {
		// LSI: ~40 kHz (STM32F1 LSI is typically 40kHz, not 32kHz)
		hrtc.Init.AsynchPrediv = 39999;  // (39999+1) = 40000 = 1 Hz (approximate)
	}

	hrtc.Init.OutPut = RTC_OUTPUTSOURCE_NONE;

	// F1 RTC initialization is simpler
	if (HAL_RTC_Init(&hrtc) != HAL_OK) {
		return false;
	}

	// CRITICAL: Enable RTC Alarm interrupt in NVIC (F1 uses Alarm, not WKUP)
	// Without this, the MCU cannot wake from STOP mode via RTC
	HAL_NVIC_SetPriority(RTC_Alarm_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(RTC_Alarm_IRQn);

#else
	// ============================================================
	// STM32F2/F3/F4/F7/L1/L4/L5/G0/G4/H7: Modern RTC
	// ============================================================
	// Modern RTC with BCD calendar and dedicated wake-up timer

	hrtc.Init.HourFormat = RTC_HOURFORMAT_24;

	if (useLSE) {
		// LSE: 32.768 kHz exact - perfect 1 Hz with these prescalers
		hrtc.Init.AsynchPrediv = 127;   // (127+1) = 128
		hrtc.Init.SynchPrediv = 255;    // (255+1) = 256, total = 32768
	} else {
		// LSI: ~32 kHz (variable) - approximate 1 Hz
		hrtc.Init.AsynchPrediv = 127;
		hrtc.Init.SynchPrediv = 249;    // Adjusted for typical LSI
	}

	hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
	hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
	hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;

	// Check if RTC is already initialized (INITS bit in ISR register)
	// If already initialized, we can skip HAL_RTC_Init which may fail
	// when called after other peripherals (like SPI) are already running
	if ((RTC->ISR & RTC_ISR_INITS) == 0) {
		// RTC not yet initialized - call HAL_RTC_Init
		if (HAL_RTC_Init(&hrtc) != HAL_OK) {
			return false;
		}
	} else {
		// RTC already initialized - just update the handle
		// This allows us to use it for sleep even if something else initialized it
		hrtc.State = HAL_RTC_STATE_READY;
	}

	// CRITICAL: Enable RTC wakeup interrupt in NVIC
	// Without this, the MCU cannot wake from STOP mode via RTC
	HAL_NVIC_SetPriority(RTC_WKUP_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(RTC_WKUP_IRQn);

#endif // STM32F1xx

	rtcInitialized = true;
	return true;
}

/**
 * @brief Configure RTC wake-up timer for specified duration
 * @param ms Milliseconds to sleep (0 = disable timer)
 * @return true if successful, false on error
 */
static bool hwSleepConfigureTimer(uint32_t ms)
{
	if (!rtcInitialized) {
		if (!hwSleepInit()) {
			return false;
		}
	}

	if (ms == 0) {
#if defined(STM32F1xx)
		// F1: Disable RTC Alarm
		HAL_RTC_DeactivateAlarm(&hrtc, RTC_ALARM_A);
#else
		// Modern STM32: Disable wake-up timer
		HAL_RTCEx_DeactivateWakeUpTimer(&hrtc);
#endif
		return true;
	}

#if defined(STM32F1xx)
	// ============================================================
	// STM32F1: Use RTC Alarm for wake-up
	// ============================================================
	// F1 doesn't have wake-up timer, use alarm instead
	// RTC counter runs at 1 Hz (configured in hwSleepInit)

	// Read current counter value
	// Note: On F1, read CNT register directly (HAL doesn't provide a clean way)
	uint32_t currentCounter = RTC->CNTL | (RTC->CNTH << 16);

	// Calculate alarm value (counter + seconds)
	// Convert ms to seconds (RTC runs at 1 Hz)
	uint32_t seconds = ms / 1000;
	if (seconds == 0) {
		seconds = 1;  // Minimum 1 second
	}
	if (seconds > 0xFFFFFFFF - currentCounter) {
		// Overflow protection
		seconds = 0xFFFFFFFF - currentCounter;
	}

	uint32_t alarmValue = currentCounter + seconds;

	// Configure alarm
	RTC_AlarmTypeDef sAlarm = {0};
	sAlarm.Alarm = alarmValue;

	if (HAL_RTC_SetAlarm_IT(&hrtc, &sAlarm, RTC_FORMAT_BIN) != HAL_OK) {
		return false;
	}

#else
	// ============================================================
	// STM32F2/F3/F4/F7/L1/L4/L5/G0/G4/H7: Use wake-up timer
	// ============================================================

	uint32_t wakeUpCounter;
	uint32_t wakeUpClock;

	// Choose appropriate clock and counter value based on sleep duration
	if (ms <= 32000) {
		// Up to 32 seconds: use RTCCLK/16 (2048 Hz, 0.488 ms resolution)
		wakeUpClock = RTC_WAKEUPCLOCK_RTCCLK_DIV16;
		// Counter = ms * 2048 / 1000 = ms * 2.048
		// Use bit shift for efficiency: ms * 2048 = ms << 11
		wakeUpCounter = (ms << 11) / 1000;
		if (wakeUpCounter < 2) {
			wakeUpCounter = 2;  // Minimum 2 ticks
		}
		if (wakeUpCounter > 0xFFFF) {
			wakeUpCounter = 0xFFFF;
		}
	} else {
		// More than 32 seconds: use CK_SPRE (1 Hz, 1 second resolution)
		wakeUpClock = RTC_WAKEUPCLOCK_CK_SPRE_16BITS;
		wakeUpCounter = ms / 1000;  // Convert to seconds
		if (wakeUpCounter == 0) {
			wakeUpCounter = 1;  // Minimum 1 second
		}
		if (wakeUpCounter > 0xFFFF) {
			wakeUpCounter = 0xFFFF;  // Max ~18 hours
		}
	}

	// Configure wake-up timer with interrupt
	if (HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, wakeUpCounter, wakeUpClock) != HAL_OK) {
		return false;
	}

#endif // STM32F1xx

	return true;
}

/**
 * @brief Restore system clock after wake-up from STOP mode
 * @note After STOP mode, system clock defaults to HSI (16 MHz). We always call
 *       SystemClock_Config() to restore the full clock configuration as the
 *       Arduino core and peripherals expect it.
 */
static void hwSleepRestoreSystemClock(void)
{
	// After STOP mode, system runs on HSI (16 MHz)
	// Always restore the system clock configuration to what the Arduino core expects
	SystemClock_Config();
}

/**
 * @brief ISR for wake-up interrupt 1
 */
static void wakeUp1ISR(void)
{
	_wokeUpByInterrupt = _wakeUp1Interrupt;
}

/**
 * @brief ISR for wake-up interrupt 2
 */
static void wakeUp2ISR(void)
{
	_wokeUpByInterrupt = _wakeUp2Interrupt;
}

/**
 * @brief RTC Wake-up Timer interrupt handler
 */
#if defined(STM32F1xx)
// F1: Use RTC Alarm interrupt
extern "C" void RTC_Alarm_IRQHandler(void)
{
	HAL_RTC_AlarmIRQHandler(&hrtc);
}
#else
// Modern STM32: Use dedicated wake-up timer interrupt
extern "C" void RTC_WKUP_IRQHandler(void)
{
	HAL_RTCEx_WakeUpTimerIRQHandler(&hrtc);
}
#endif

// ======================== Public Sleep Functions ========================

uint32_t hwGetSleepRemaining(void)
{
	return sleepRemainingMs;
}

int8_t hwSleep(uint32_t ms)
{
	// Initialize RTC if needed
	if (!rtcInitialized) {
		if (!hwSleepInit()) {
			return MY_SLEEP_NOT_POSSIBLE;
		}
	}

	// Configure RTC wake-up timer
	if (ms > 0) {
		if (!hwSleepConfigureTimer(ms)) {
			return MY_SLEEP_NOT_POSSIBLE;
		}
	}

	// Reset sleep remaining
	sleepRemainingMs = 0ul;

	// CRITICAL: Clear wakeup flags before entering sleep
	// This prevents spurious wakeups from previous events
#if defined(STM32F1xx)
	__HAL_RTC_ALARM_CLEAR_FLAG(&hrtc, RTC_FLAG_ALRAF);
#else
	__HAL_RTC_WAKEUPTIMER_CLEAR_FLAG(&hrtc, RTC_FLAG_WUTF);
#endif
	__HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);

	// Suspend SysTick to prevent 1ms interrupts during sleep
	HAL_SuspendTick();

	// NOTE: USB CDC will disconnect during STOP mode (expected behavior)
	// USB peripheral requires system clock which is stopped in STOP mode
	// After wake-up, the host will detect USB disconnect/reconnect
	// This is normal and unavoidable when using STOP mode sleep

	// Enter STOP mode with low-power regulator
	// This achieves 10-50 µA sleep current on STM32F4
	HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);

	// ====================================================================
	// === MCU is in STOP mode here (10-50 µA), waiting for wake-up ===
	// ====================================================================

	// After wake-up: restore system clock (defaults to HSI)
	hwSleepRestoreSystemClock();

	// Resume SysTick
	HAL_ResumeTick();

	// CRITICAL: Clear wakeup flags after wake-up
	// This ensures clean state for next sleep cycle
#if defined(STM32F1xx)
	__HAL_RTC_ALARM_CLEAR_FLAG(&hrtc, RTC_FLAG_ALRAF);
#else
	__HAL_RTC_WAKEUPTIMER_CLEAR_FLAG(&hrtc, RTC_FLAG_WUTF);
#endif
	__HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);

	// Disable wake-up timer
	if (ms > 0) {
#if defined(STM32F1xx)
		HAL_RTC_DeactivateAlarm(&hrtc, RTC_ALARM_A);
#else
		HAL_RTCEx_DeactivateWakeUpTimer(&hrtc);
#endif
	}

	// Always timer wake-up for this variant
	return MY_WAKE_UP_BY_TIMER;
}

int8_t hwSleep(const uint8_t interrupt, const uint8_t mode, uint32_t ms)
{
	// Delegate to dual-interrupt variant with INVALID second interrupt
	return hwSleep(interrupt, mode, INVALID_INTERRUPT_NUM, 0, ms);
}

int8_t hwSleep(const uint8_t interrupt1, const uint8_t mode1,
               const uint8_t interrupt2, const uint8_t mode2, uint32_t ms)
{
	// Initialize RTC if needed
	if (!rtcInitialized) {
		if (!hwSleepInit()) {
			return MY_SLEEP_NOT_POSSIBLE;
		}
	}

	// Configure RTC wake-up timer (if ms > 0)
	if (ms > 0) {
		if (!hwSleepConfigureTimer(ms)) {
			return MY_SLEEP_NOT_POSSIBLE;
		}
	}

	// Reset sleep remaining
	sleepRemainingMs = 0ul;

	// Configure interrupt wake-up sources
	_wakeUp1Interrupt = interrupt1;
	_wakeUp2Interrupt = interrupt2;
	_wokeUpByInterrupt = INVALID_INTERRUPT_NUM;

	// Attach interrupts in critical section (prevent premature wake-up)
	MY_CRITICAL_SECTION {
		if (interrupt1 != INVALID_INTERRUPT_NUM)
		{
			attachInterrupt(digitalPinToInterrupt(interrupt1), wakeUp1ISR, mode1);
		}
		if (interrupt2 != INVALID_INTERRUPT_NUM)
		{
			attachInterrupt(digitalPinToInterrupt(interrupt2), wakeUp2ISR, mode2);
		}
	}

	// CRITICAL: Clear wakeup flags before entering sleep
#if defined(STM32F1xx)
	__HAL_RTC_ALARM_CLEAR_FLAG(&hrtc, RTC_FLAG_ALRAF);
#else
	__HAL_RTC_WAKEUPTIMER_CLEAR_FLAG(&hrtc, RTC_FLAG_WUTF);
#endif
	__HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);

	// Suspend SysTick
	HAL_SuspendTick();

	// NOTE: USB CDC will disconnect during STOP mode (expected behavior)
	// See note in timer-only hwSleep() variant above

	// Enter STOP mode with low-power regulator
	HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);

	// ====================================================================
	// === MCU is in STOP mode here (10-50 µA), waiting for wake-up ===
	// ====================================================================

	// After wake-up: restore system clock
	hwSleepRestoreSystemClock();

	// Resume SysTick
	HAL_ResumeTick();

	// CRITICAL: Clear wakeup flags after wake-up
#if defined(STM32F1xx)
	__HAL_RTC_ALARM_CLEAR_FLAG(&hrtc, RTC_FLAG_ALRAF);
#else
	__HAL_RTC_WAKEUPTIMER_CLEAR_FLAG(&hrtc, RTC_FLAG_WUTF);
#endif
	__HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);

	// Detach interrupts
	if (interrupt1 != INVALID_INTERRUPT_NUM) {
		detachInterrupt(digitalPinToInterrupt(interrupt1));
	}
	if (interrupt2 != INVALID_INTERRUPT_NUM) {
		detachInterrupt(digitalPinToInterrupt(interrupt2));
	}

	// Disable wake-up timer
	if (ms > 0) {
#if defined(STM32F1xx)
		HAL_RTC_DeactivateAlarm(&hrtc, RTC_ALARM_A);
#else
		HAL_RTCEx_DeactivateWakeUpTimer(&hrtc);
#endif
	}

	// Determine wake-up source
	int8_t ret = MY_WAKE_UP_BY_TIMER;  // Default: timer wake-up
	if (_wokeUpByInterrupt != INVALID_INTERRUPT_NUM) {
		ret = (int8_t)_wokeUpByInterrupt;  // Interrupt wake-up
	}

	// Reset interrupt tracking
	_wokeUpByInterrupt = INVALID_INTERRUPT_NUM;
	_wakeUp1Interrupt = INVALID_INTERRUPT_NUM;
	_wakeUp2Interrupt = INVALID_INTERRUPT_NUM;

	return ret;
}
