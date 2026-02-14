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
 * STM32Cube HAL underneath. It supports a wide range of STM32 families.
 *
 * Tested on:
 * - STM32F103C8 Blue Pill
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

// Forward declarations for RTC counter helpers (portable across F1 and modern)
static uint32_t hwRtcGetCounter(void);

/**
 * @brief Prevent HAL_RTC_Init from re-running clock setup
 * All clock and backup domain configuration is handled in hwSleepInit()
 */
extern "C" void HAL_RTC_MspInit(RTC_HandleTypeDef *hrtc_param)
{
	(void)hrtc_param;
}

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
	const intptr_t pos = reinterpret_cast<intptr_t>(addr);

	for (size_t i = 0; i < length; i++) {
		dst[i] = EEPROM.read(pos + i);
	}
}

void hwWriteConfigBlock(void *buf, void *addr, size_t length)
{
	uint8_t *src = static_cast<uint8_t *>(buf);
	const intptr_t pos = reinterpret_cast<intptr_t>(addr);

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
	IWDG->KR = IWDG_KEY_RELOAD;
#endif
}

void hwReboot(void)
{
	NVIC_SystemReset();
}

int32_t hwReadInternalTemp(void)
{
#if defined(ATEMP)
	// STM32duino defines ATEMP — use the easy path
	analogReadResolution(12);
	return (int32_t)analogRead(ATEMP);
#elif defined(STM32F1xx) && defined(ADC1)
	// STM32F1: ATEMP not defined in STM32duino, read ADC channel 16 directly
	// Temperature sensor is on ADC1_IN16, requires TSVREFE bit enabled

	// Enable ADC1 clock
	__HAL_RCC_ADC1_CLK_ENABLE();

	// Enable temperature sensor and VREFINT (TSVREFE bit in CR2)
	ADC1->CR2 |= ADC_CR2_TSVREFE;

	// Wait for sensor startup (~10 µs per datasheet)
	delayMicroseconds(20);

	// Configure channel 16, sample time = 239.5 cycles (longest, for accuracy)
	// SMP16 is bits [23:21] in SMPR1, value 0b111 = 239.5 cycles
	ADC1->SMPR1 |= (0x7UL << 18);  // SMP16 = 239.5 cycles

	// Set sequence: 1 conversion, channel 16
	ADC1->SQR3 = 16;   // First conversion = channel 16
	ADC1->SQR1 = 0;    // 1 conversion in sequence

	// Ensure ADC is on
	ADC1->CR2 |= ADC_CR2_ADON;
	delayMicroseconds(2);

	// Start conversion
	ADC1->CR2 |= ADC_CR2_ADON;  // Second write starts conversion on F1

	// Wait for conversion complete (EOC flag)
	uint32_t timeout = 100000;
	while (((ADC1->SR & ADC_SR_EOC) == 0) && (timeout > 0)) {
		timeout--;
	}

	if (timeout == 0) {
		return -1;
	}

	int32_t raw = (int32_t)(ADC1->DR & 0x0FFF);

	// Disable temperature sensor to save power
	ADC1->CR2 &= ~ADC_CR2_TSVREFE;

	return raw;
#else
	return -1;
#endif
}


void hwRandomNumberInit(void)
{
#if defined(HAL_RNG_MODULE_ENABLED) && defined(RNG)
	// ============================================================
	// Hardware RNG available (F2, F4x5/F4x7+, F7, L4, G0, G4, H7)
	// ============================================================
	__HAL_RCC_RNG_CLK_ENABLE();

	RNG_HandleTypeDef hrng = {0};
	hrng.Instance = RNG;

	if (HAL_RNG_Init(&hrng) == HAL_OK) {
		uint32_t seed = 0;
		if (HAL_RNG_GenerateRandomNumber(&hrng, &seed) == HAL_OK) {
			randomSeed(seed);
			// Disable RNG clock to save power after seeding
			HAL_RNG_DeInit(&hrng);
			__HAL_RCC_RNG_CLK_DISABLE();
			return;
		}
		HAL_RNG_DeInit(&hrng);
	}
	__HAL_RCC_RNG_CLK_DISABLE();
	// Fall through to software entropy if hardware RNG failed

#endif // HAL_RNG_MODULE_ENABLED && RNG

	// ============================================================
	// Software entropy fallback (F0, F1, F3, and chips without RNG)
	// Uses ADC noise from temp sensor, VREFINT, and timing jitter
	// ============================================================

#if defined(ADC1)
	uint32_t seed = 0;

	for (uint8_t i = 0; i < 32; i++) {
		uint32_t value = hwReadInternalTemp();

#if defined(AVREF)
		value ^= analogRead(AVREF);
#endif

		value ^= micros();

		// Rotate and XOR for better bit distribution
		seed ^= (i == 0) ? value : ((value << i) | (value >> (32 - i)));

		delayMicroseconds(100);
	}

	randomSeed(seed);
#else
	// Last resort: weak entropy from timing only
	randomSeed(hwMillis() ^ (micros() << 16));
#endif // ADC1
}

bool hwUniqueID(unique_id_t *uniqueID)
{
#if defined(UID_BASE)
	(void)memset(reinterpret_cast<uint8_t *>(uniqueID), MY_HWID_PADDING_BYTE, sizeof(unique_id_t));
	(void)memcpy(reinterpret_cast<uint8_t *>(uniqueID), (const uint8_t *)UID_BASE, 12);
	return true;
#else
	return false;
#endif
}

uint16_t hwCPUVoltage(void)
{
#if defined(AVREF) && defined(__HAL_RCC_ADC1_CLK_ENABLE)
	// Force 12-bit resolution for predictable raw values
	analogReadResolution(12);
	uint32_t vrefint = analogRead(AVREF);

	if (vrefint > 0) {
#if defined(VREFINT_CAL_ADDR) && defined(VREFINT_CAL_VREF)
		// Use factory calibration for accurate measurement
		// VREFINT_CAL was measured at VREFINT_CAL_VREF mV with 12-bit ADC
		uint16_t vrefint_cal = *((uint16_t *)VREFINT_CAL_ADDR);
		return (uint16_t)((uint32_t)VREFINT_CAL_VREF * vrefint_cal / vrefint);
#else
		// No factory calibration (e.g., STM32F103)
		// Assume typical VREFINT = 1.2V, 12-bit ADC
		return (uint16_t)((1200UL * 4095UL) / vrefint);
#endif
	}
#endif

	return 3300;
}

uint16_t hwCPUFrequency(void)
{
	// HAL_RCC_GetSysClockFreq() reads the actual clock configuration registers
	// and returns the current SYSCLK frequency in Hz
	return (uint16_t)(HAL_RCC_GetSysClockFreq() / 100000UL);
}

int8_t hwCPUTemperature(void)
{
	// cppcheck-suppress knownConditionTrueFalse
	int32_t temp_raw = hwReadInternalTemp();

	if (temp_raw < 0) { // cppcheck-suppress knownConditionTrueFalse
		return FUNCTION_NOT_SUPPORTED;
	}

#if defined(TEMP110_CAL_ADDR) && defined(TEMP30_CAL_ADDR)
	// Use factory calibration if available (STM32F4, L4, G4, etc.)
	uint16_t cal30 = *((uint16_t *)TEMP30_CAL_ADDR);
	uint16_t cal110 = *((uint16_t *)TEMP110_CAL_ADDR);

	if (cal110 != cal30) {
		int32_t temp = 30 + ((110 - 30) * (temp_raw - (int32_t)cal30)) /
		               ((int32_t)cal110 - (int32_t)cal30);

		temp = ((temp - MY_STM32_TEMPERATURE_OFFSET) * 100) / MY_STM32_TEMPERATURE_GAIN;
		return (int8_t)temp;
	}
#endif

	// Fallback: use typical datasheet values (no factory calibration)
#if defined(STM32F1xx)
	// STM32F1: V25 = 1.43V, slope = 4.3 mV/°C, slope is NEGATIVE (higher T = lower V)
	float voltage = (temp_raw * 3.3f) / 4096.0f;
	int32_t temp = 25 + (int32_t)((1.43f - voltage) / 0.0043f);
#else
	// STM32F4 and others without calibration: V25 = 0.76V, slope = 2.5 mV/°C
	float voltage = (temp_raw * 3.3f) / 4096.0f;
	int32_t temp = 25 + (int32_t)((voltage - 0.76f) / 0.0025f);
#endif

	return (int8_t)(((temp - MY_STM32_TEMPERATURE_OFFSET) * 100) / MY_STM32_TEMPERATURE_GAIN);
}

extern "C" caddr_t _sbrk(int incr);

uint16_t hwFreeMem(void)
{
	// Use ARM semihosting / newlib approach
	// _sbrk(0) returns current program break (top of heap)

	char stack_var;
	char *heap_end = (char *)_sbrk(0);

	if (heap_end != (char *)-1) {
		return (uint16_t)(&stack_var - heap_end);
	}

	return FUNCTION_NOT_SUPPORTED;
}

// ======================== RTC Counter Helpers ========================

/**
 * @brief Read current RTC counter value (portable)
 * @return 32-bit counter on F1, subsecond-approximated tick on modern STM32
 */
static uint32_t hwRtcGetCounter(void)
{
#if defined(STM32F1xx)
	return (uint32_t)(RTC->CNTL | (RTC->CNTH << 16));
#else
	// Modern STM32: use HAL to get time and compute seconds since epoch
	// For sleep remaining calculation, we only need relative elapsed seconds
	RTC_TimeTypeDef sTime = {0};
	HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
	// Must read date after time to unlock shadow registers
	RTC_DateTypeDef sDate = {0};
	HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
	return (uint32_t)(sTime.Hours * 3600 + sTime.Minutes * 60 + sTime.Seconds);
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

	// ---- Enable PWR and backup domain access ----
#if defined(STM32F1xx)
	// STM32F1 requires BKP clock enabled BEFORE PWR clock
	__HAL_RCC_BKP_CLK_ENABLE();
	__DSB();
	__HAL_RCC_PWR_CLK_ENABLE();
	__DSB();

	// Dummy read to ensure clocks are active before accessing PWR registers
	volatile uint32_t dummy = RCC->APB1ENR;
	(void)dummy;

	// Direct register write for backup domain access
	PWR->CR |= PWR_CR_DBP;
	__DSB();
#else
	// Modern STM32 (F2/F3/F4/F7/L0/L1/L4/L5/G0/G4/H7)
	__HAL_RCC_PWR_CLK_ENABLE();
	HAL_PWR_EnableBkUpAccess();
#endif

	// ---- Reset backup domain if RTC not already configured ----
	if ((RCC->BDCR & RCC_BDCR_RTCEN) == 0) {
		__HAL_RCC_BACKUPRESET_FORCE();
		HAL_Delay(10);
		__HAL_RCC_BACKUPRESET_RELEASE();
		HAL_Delay(10);
	}

	// ---- Select RTC clock source: try LSE, fall back to LSI ----
	bool useLSE = false;

	if ((RCC->BDCR & RCC_BDCR_LSERDY) != 0) {
		useLSE = true;
	} else {
		RCC->BDCR |= RCC_BDCR_LSEON;
		uint32_t timeout = 2000000;
		while (((RCC->BDCR & RCC_BDCR_LSERDY) == 0) && (timeout > 0)) {
			timeout--;
		}

		if (timeout > 0) {
			useLSE = true;
		} else {
			RCC->BDCR &= ~RCC_BDCR_LSEON;  // Disable failed LSE

			if ((RCC->CSR & RCC_CSR_LSIRDY) == 0) {
				RCC->CSR |= RCC_CSR_LSION;
				timeout = 1000000;
				while (((RCC->CSR & RCC_CSR_LSIRDY) == 0) && (timeout > 0)) {
					timeout--;
				}
				if (timeout == 0) {
					return false;  // Both LSE and LSI failed
				}
			}
		}
	}

	// ---- Configure RTC clock source ----
	uint32_t currentRtcSel = (RCC->BDCR & RCC_BDCR_RTCSEL);
	uint32_t desiredRtcSel = useLSE ? RCC_BDCR_RTCSEL_0 : RCC_BDCR_RTCSEL_1;

	if (currentRtcSel != desiredRtcSel) {
		// Changing RTCSEL requires backup domain reset on most STM32 families
		__HAL_RCC_BACKUPRESET_FORCE();
		__HAL_RCC_BACKUPRESET_RELEASE();
		RCC->BDCR |= desiredRtcSel;
	}
	RCC->BDCR |= RCC_BDCR_RTCEN;

	// ---- Initialize RTC peripheral ----
	hrtc.Instance = RTC;

#if defined(STM32F1xx)
	// ============================================================
	// STM32F1: Legacy RTC with counter-based architecture
	// ============================================================

	if (useLSE) {
		hrtc.Init.AsynchPrediv = 32767;  // 32768 Hz / 32768 = 1 Hz
	} else {
		hrtc.Init.AsynchPrediv = 39999;  // ~40 kHz / 40000 = 1 Hz (F1 LSI is ~40 kHz)
	}

	hrtc.Init.OutPut = RTC_OUTPUTSOURCE_NONE;

	if (HAL_RTC_Init(&hrtc) != HAL_OK) {
		return false;
	}

	// Clear any stale alarm flags before enabling interrupts
	RTC->CRL &= ~RTC_CRL_ALRF;
	EXTI->PR = EXTI_PR_PR17;
	NVIC_ClearPendingIRQ(RTC_Alarm_IRQn);

	// Configure EXTI line 17 for RTC Alarm wake-up from STOP mode
	// Without EXTI, the alarm flag is set but the CPU does not wake
	EXTI->IMR |= EXTI_IMR_MR17;     // Unmask EXTI line 17
	EXTI->RTSR |= EXTI_RTSR_TR17;   // Rising edge trigger

	HAL_NVIC_SetPriority(RTC_Alarm_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(RTC_Alarm_IRQn);

#elif defined(STM32U0xx)
	// ============================================================
	// STM32U0: Modern RTC with ICSR register (instead of ISR)
	// ============================================================
	// Similar to other modern STM32 families but with renamed registers
	// and combined RTC/TAMP interrupt vector

	hrtc.Init.HourFormat = RTC_HOURFORMAT_24;

	if (useLSE) {
		hrtc.Init.AsynchPrediv = 127;  // 128
		hrtc.Init.SynchPrediv  = 255;  // 256, total = 32768
	} else {
		hrtc.Init.AsynchPrediv = 127;
		hrtc.Init.SynchPrediv  = 249;  // Adjusted for typical ~32 kHz LSI
	}

	hrtc.Init.OutPut         = RTC_OUTPUT_DISABLE;
	hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
	hrtc.Init.OutPutType     = RTC_OUTPUT_TYPE_OPENDRAIN;

	// Check if RTC is already initialized (avoid reinit conflicts with SPI etc.)
	if ((RTC->ICSR & RTC_ICSR_INITS) == 0) {
		if (HAL_RTC_Init(&hrtc) != HAL_OK) {
			return false;
		}
	} else {
		hrtc.State = HAL_RTC_STATE_READY;
	}

	// Configure interrupt for RTC wake_up timer
	HAL_NVIC_SetPriority(RTC_TAMP_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(RTC_TAMP_IRQn);

#else
	// ============================================================
	// STM32F2/F3/F4/F7/L1/L4/L5/G0/G4/H7: Modern RTC
	// ============================================================

	hrtc.Init.HourFormat = RTC_HOURFORMAT_24;

	if (useLSE) {
		hrtc.Init.AsynchPrediv = 127;   // 128
		hrtc.Init.SynchPrediv = 255;    // 256, total = 32768
	} else {
		hrtc.Init.AsynchPrediv = 127;
		hrtc.Init.SynchPrediv = 249;    // Adjusted for typical ~32 kHz LSI
	}

	hrtc.Init.OutPut = RTC_OUTPUT_DISABLE;
	hrtc.Init.OutPutPolarity = RTC_OUTPUT_POLARITY_HIGH;
	hrtc.Init.OutPutType = RTC_OUTPUT_TYPE_OPENDRAIN;

	// Check if RTC is already initialized (avoid reinit conflicts with SPI etc.)
	if ((RTC->ISR & RTC_ISR_INITS) == 0) {
		if (HAL_RTC_Init(&hrtc) != HAL_OK) {
			return false;
		}
	} else {
		hrtc.State = HAL_RTC_STATE_READY;
	}

	// Configure EXTI line 22 for RTC wake-up timer (required for STOP mode wake)
	// HAL_RTCEx_SetWakeUpTimer_IT handles this on most families, but be explicit
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
		return false;
	}

	if (ms == 0) {
#if defined(STM32F1xx)
		HAL_RTC_DeactivateAlarm(&hrtc, RTC_ALARM_A);
#else
		HAL_RTCEx_DeactivateWakeUpTimer(&hrtc);
#endif
		return true;
	}

#if defined(STM32F1xx)
	// ============================================================
	// STM32F1: Use RTC Alarm (counter-based)
	// ============================================================

	uint32_t currentCounter = hwRtcGetCounter();

	uint32_t seconds = ms / 1000;
	if (seconds == 0) {
		seconds = 1;  // Minimum 1 second
	}
	if (seconds > 0xFFFFFFFF - currentCounter) {
		seconds = 0xFFFFFFFF - currentCounter;
	}

	uint32_t alarmValue = currentCounter + seconds;

	// Write alarm registers directly via configuration mode
	while ((RTC->CRL & RTC_CRL_RTOFF) == 0) { }  // Wait for last write
	RTC->CRL |= RTC_CRL_CNF;                       // Enter config mode

	RTC->ALRH = (alarmValue >> 16) & 0xFFFF;
	RTC->ALRL = alarmValue & 0xFFFF;

	RTC->CRL &= ~RTC_CRL_CNF;                      // Exit config mode
	while ((RTC->CRL & RTC_CRL_RTOFF) == 0) { }    // Wait for write complete

	// Enable alarm interrupt in RTC
	RTC->CRH |= RTC_CRH_ALRIE;

#elif defined(STM32U0xx)
	// ============================================================
	// STM32U0: Use wake-up timer
	// ============================================================
	// STM32U0 HAL requires a 4th argument: WakeUpAutoClr (auto-clear of wakeup flag)

	uint32_t wakeUpCounter;
	uint32_t wakeUpClock;

	if (ms <= 32000) {
		// Up to 32 seconds: use RTCCLK/16 (2048 Hz, ~0.49 ms resolution)
		wakeUpClock   = RTC_WAKEUPCLOCK_RTCCLK_DIV16;
		wakeUpCounter = (ms << 11) / 1000;  // ms * 2048 / 1000
		if (wakeUpCounter < 2) {
			wakeUpCounter = 2;
		}
		if (wakeUpCounter > 0xFFFF) {
			wakeUpCounter = 0xFFFF;
		}
	} else {
		// Over 32 seconds: use CK_SPRE (1 Hz, 1 second resolution)
		wakeUpClock   = RTC_WAKEUPCLOCK_CK_SPRE_16BITS;
		wakeUpCounter = ms / 1000;
		if (wakeUpCounter == 0) {
			wakeUpCounter = 1;
		}
		if (wakeUpCounter > 0xFFFF) {
			wakeUpCounter = 0xFFFF;  // Max ~18 hours
		}
	}

	if (HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, wakeUpCounter, wakeUpClock, 0) != HAL_OK) {
		return false;
	}

#else
	// ============================================================
	// STM32F2/F3/F4/F7/L1/L4/L5/G0/G4/H7: Use wake-up timer
	// ============================================================

	uint32_t wakeUpCounter;
	uint32_t wakeUpClock;

	if (ms <= 32000) {
		// Up to 32 seconds: use RTCCLK/16 (2048 Hz, ~0.49 ms resolution)
		wakeUpClock = RTC_WAKEUPCLOCK_RTCCLK_DIV16;
		wakeUpCounter = (ms << 11) / 1000;  // ms * 2048 / 1000
		if (wakeUpCounter < 2) {
			wakeUpCounter = 2;
		}
		if (wakeUpCounter > 0xFFFF) {
			wakeUpCounter = 0xFFFF;
		}
	} else {
		// Over 32 seconds: use CK_SPRE (1 Hz, 1 second resolution)
		wakeUpClock = RTC_WAKEUPCLOCK_CK_SPRE_16BITS;
		wakeUpCounter = ms / 1000;
		if (wakeUpCounter == 0) {
			wakeUpCounter = 1;
		}
		if (wakeUpCounter > 0xFFFF) {
			wakeUpCounter = 0xFFFF;  // Max ~18 hours
		}
	}

	if (HAL_RTCEx_SetWakeUpTimer_IT(&hrtc, wakeUpCounter, wakeUpClock) != HAL_OK) {
		return false;
	}

#endif // STM32F1xx

	return true;
}

/**
 * @brief Restore system clock after wake-up from STOP mode
 * @note After STOP mode, system clock defaults to HSI (8 MHz on F1, 16 MHz on F4).
 *       SystemClock_Config() restores the full PLL configuration.
 */
static void hwSleepRestoreSystemClock(void)
{
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
 * @brief RTC interrupt handlers
 */
#if defined(STM32F1xx)
extern "C" void RTC_Alarm_IRQHandler(void)
{
	// Lean handler: clear flags directly instead of going through HAL
	EXTI->PR = EXTI_PR_PR17;         // Clear EXTI line 17 pending
	RTC->CRL &= ~RTC_CRL_ALRF;      // Clear RTC alarm flag
}
#elif defined(STM32U0xx)
// STM32U0: Combined RTC/TAMP interrupt (no separate RTC_WKUP vector)
extern "C" void RTC_TAMP_IRQHandler(void)
{
	HAL_RTCEx_WakeUpTimerIRQHandler(&hrtc);
}
#else
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

/**
 * @brief Common sleep implementation
 * @param interrupt1 First interrupt pin (INVALID_INTERRUPT_NUM = none)
 * @param mode1 First interrupt mode
 * @param interrupt2 Second interrupt pin (INVALID_INTERRUPT_NUM = none)
 * @param mode2 Second interrupt mode
 * @param ms Sleep duration in ms (0 = interrupt-only, no timer)
 * @return Wake-up source or error code
 */
static int8_t hwSleepInternal(const uint8_t interrupt1, const uint8_t mode1,
                              const uint8_t interrupt2, const uint8_t mode2,
                              uint32_t ms)
{
	bool hasInterrupt = (interrupt1 != INVALID_INTERRUPT_NUM) ||
	                    (interrupt2 != INVALID_INTERRUPT_NUM);

	// Reject timer-only sleep with ms=0 (would sleep forever with no wake source)
	if (ms == 0 && !hasInterrupt) {
		return MY_SLEEP_NOT_POSSIBLE;
	}

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

	// Reset sleep state
	sleepRemainingMs = 0ul;
	_wokeUpByInterrupt = INVALID_INTERRUPT_NUM;

	// Attach interrupt wake-up sources
	if (hasInterrupt) {
		_wakeUp1Interrupt = interrupt1;
		_wakeUp2Interrupt = interrupt2;

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

		// Check if an interrupt already fired between attach and here
		if (_wokeUpByInterrupt != INVALID_INTERRUPT_NUM) {
			// Already woken — skip sleep, go to cleanup
			goto cleanup;
		}
	}

	// Record RTC counter before sleep (for sleepRemaining calculation)
	{
		const uint32_t sleepStartCounter = hwRtcGetCounter();

		// Clear all wake-up flags before entering STOP mode
#if defined(STM32F1xx)
		RTC->CRL &= ~RTC_CRL_ALRF;
		EXTI->PR = EXTI_PR_PR17;
		NVIC_ClearPendingIRQ(RTC_Alarm_IRQn);
#else
		__HAL_RTC_WAKEUPTIMER_CLEAR_FLAG(&hrtc, RTC_FLAG_WUTF);
#endif
		__HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);

		// Suspend SysTick to prevent 1ms interrupts during sleep
		HAL_SuspendTick();

		// Enter STOP mode with low-power regulator
		// NOTE: USB CDC will disconnect (expected; USB needs system clock)
		HAL_PWR_EnterSTOPMode(PWR_LOWPOWERREGULATOR_ON, PWR_STOPENTRY_WFI);

		// ================================================================
		// === MCU is in STOP mode here, waiting for wake-up event ===
		// ================================================================

		// Restore system clock (HSI -> PLL)
		hwSleepRestoreSystemClock();

		// Resume SysTick
		HAL_ResumeTick();

		// Clear wake-up flags after wake
#if defined(STM32F1xx)
		RTC->CRL &= ~RTC_CRL_ALRF;
		EXTI->PR = EXTI_PR_PR17;
#else
		__HAL_RTC_WAKEUPTIMER_CLEAR_FLAG(&hrtc, RTC_FLAG_WUTF);
#endif
		__HAL_PWR_CLEAR_FLAG(PWR_FLAG_WU);

		// Calculate sleep remaining
		if (ms > 0) {
			const uint32_t sleepEndCounter = hwRtcGetCounter();
			uint32_t elapsedSeconds;

#if defined(STM32F1xx)
			elapsedSeconds = sleepEndCounter - sleepStartCounter;
#else
			// Handle midnight rollover (86400 seconds in a day)
			if (sleepEndCounter >= sleepStartCounter) {
				elapsedSeconds = sleepEndCounter - sleepStartCounter;
			} else {
				elapsedSeconds = (86400 - sleepStartCounter) + sleepEndCounter;
			}
#endif

			const uint32_t elapsedMs = elapsedSeconds * 1000;
			sleepRemainingMs = (elapsedMs < ms) ? (ms - elapsedMs) : 0;
		}
	}

cleanup:
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
	int8_t ret = MY_WAKE_UP_BY_TIMER;
	if (_wokeUpByInterrupt != INVALID_INTERRUPT_NUM) {
		ret = (int8_t)_wokeUpByInterrupt;
	}

	// Reset interrupt tracking
	_wokeUpByInterrupt = INVALID_INTERRUPT_NUM;
	_wakeUp1Interrupt = INVALID_INTERRUPT_NUM;
	_wakeUp2Interrupt = INVALID_INTERRUPT_NUM;

	return ret;
}

int8_t hwSleep(uint32_t ms)
{
	return hwSleepInternal(INVALID_INTERRUPT_NUM, 0, INVALID_INTERRUPT_NUM, 0, ms);
}

int8_t hwSleep(const uint8_t interrupt, const uint8_t mode, uint32_t ms)
{
	return hwSleepInternal(interrupt, mode, INVALID_INTERRUPT_NUM, 0, ms);
}

int8_t hwSleep(const uint8_t interrupt1, const uint8_t mode1,
               const uint8_t interrupt2, const uint8_t mode2, uint32_t ms)
{
	return hwSleepInternal(interrupt1, mode1, interrupt2, mode2, ms);
}
