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
#ifndef MyHwNRF5_h
#define MyHwNRF5_h

#ifdef __cplusplus
#include <Arduino.h>
#endif

#define CRYPTO_LITTLE_ENDIAN

#ifndef MY_SERIALDEVICE
#define MY_SERIALDEVICE Serial
#endif

#ifndef MY_DEBUGDEVICE
#define MY_DEBUGDEVICE MY_SERIALDEVICE
#endif

// Define NRF5_SOFTDEVICE when SoftDevice found
#if defined(S110) || defined(S130) || defined(S132) || defined(S140)
#ifndef SOFTDEVICE_PRESENT
#define SOFTDEVICE_PRESENT
#endif
#endif

// Define ARDUINO_ARCH_NRF5, if not defined
#ifndef ARDUINO_ARCH_NRF5
#define ARDUINO_ARCH_NRF5
#endif

#include "hal/architecture/NRF5/drivers/nrf5_wiring_digital.c"
#include "hal/architecture/NRF5/drivers/wdt.h"
#include "hal/architecture/NRF5/drivers/nrf_temp.h"
#include "drivers/NVM/NVRAM.cpp"
#include "drivers/NVM/VirtualPage.cpp"
#include <avr/dtostrf.h>
#include <nrf.h>
#include <SPI.h>

// mapping
#ifndef strncpy_P
#define strncpy_P strncpy
#endif
#ifndef snprintf_P
#define snprintf_P(s, f, ...) snprintf((s), (f), __VA_ARGS__)
#endif
#ifndef vsnprintf_P
#define vsnprintf_P vsnprintf
#endif
#ifndef printf_P
#define printf_P printf
#endif

// redefine 8 bit types of inttypes.h until fix of https://github.com/sandeepmistry/arduino-nRF5/issues/197
#undef PRId8
#undef PRIi8
#undef PRIo8
#undef PRIu8
#undef PRIx8
#undef PRIX8
#undef PRIdLEAST8
#undef PRIiLEAST8
#undef PRIoLEAST8
#undef PRIuLEAST8
#undef PRIxLEAST8
#undef PRIXLEAST8
#undef PRIdFAST8
#undef PRIiFAST8
#undef PRIoFAST8
#undef PRIuFAST8
#undef PRIxFAST8
#undef PRIXFAST8
#define PRId8		"hd"
#define PRIi8		"hi"
#define PRIo8		"ho"
#define PRIu8		"hu"
#define PRIx8		"hx"
#define PRIX8		"hX"
#define PRIdLEAST8	"hd"
#define PRIiLEAST8	"hi"
#define PRIoLEAST8	"ho"
#define PRIuLEAST8	"hu"
#define PRIxLEAST8	"hx"
#define PRIXLEAST8	"hX"
#define PRIdFAST8	"hd"
#define PRIiFAST8	"hi"
#define PRIoFAST8	"ho"
#define PRIuFAST8	"hu"
#define PRIxFAST8	"hx"
#define PRIXFAST8	"hX"

// Define these as macros to save valuable space
#define hwDigitalWrite(__pin, __value) digitalWrite(__pin, __value)
#define hwDigitalRead(__pin) digitalRead(__pin)
#define hwPinMode(__pin, __value) nrf5_pinMode(__pin, __value)
#define hwMillis() millis()

bool hwInit(void);
void hwWatchdogReset(void);
void hwReboot(void);
void hwReadConfigBlock(void *buf, void *addr, size_t length);
void hwWriteConfigBlock(void *buf, void *addr, size_t length);
void hwWriteConfig(const int addr, uint8_t value);
uint8_t hwReadConfig(const int addr);
void hwRandomNumberInit(void);
ssize_t hwGetentropy(void *__buffer, size_t __length);
#define MY_HW_HAS_GETENTROPY

// SOFTSPI
#ifdef MY_SOFTSPI
#error Soft SPI is not available on this architecture!
#endif
#define hwSPI SPI //!< hwSPI


/**
 * Disable all interrupts.
 * Helper function for MY_CRITICAL_SECTION.
 */
static __inline__ uint8_t __disableIntsRetVal(void)
{
	__disable_irq();
	return 1;
}

/**
 * Restore priority mask register.
 * Helper function for MY_CRITICAL_SECTION.
 */
static __inline__ void __priMaskRestore(const uint32_t *priMask)
{
	__set_PRIMASK(*priMask);
}

/**
 * Reset events and read back on nRF52
 * http://infocenter.nordicsemi.com/pdf/nRF52_Series_Migration_v1.0.pdf
 */
#if __CORTEX_M == 0x04
#define NRF_RESET_EVENT(event)                                                 \
	event = 0;                                                                   \
	(void)event
#else
#define NRF_RESET_EVENT(event) event = 0
#endif
/**
 * RTC to use for hwSleep
 * RTC1 is used by arduino-nRF5
 * RTC2 is used on nRF52
 * RTC0 is used on nRF51. This conflicts with SoftDevice!
 */
#ifdef NRF_RTC2
#define MY_HW_RTC NRF_RTC2
#define MY_HW_RTC_IRQ_HANDLER RTC2_IRQHandler
#define MY_HW_RTC_IRQN RTC2_IRQn
#else
#define MY_HW_RTC NRF_RTC0
#define MY_HW_RTC_IRQ_HANDLER RTC0_IRQHandler
#define MY_HW_RTC_IRQN RTC0_IRQn
#endif

/** Datastructure for AES ECB unit
 */
typedef struct {
	/** AES Key
	 */
	uint8_t key[16];
	/** Unencrypted data
	 */
	uint8_t cleartext[16];
	/** Encrypted data
	 */
	uint8_t ciphertext[16];
} nrf_ecb_t;

#if !defined(DOXYGEN) && !defined(CPPCHECK)
#define MY_CRITICAL_SECTION                                                    \
	for (uint32_t __savePriMask                                                  \
	        __attribute__((__cleanup__(__priMaskRestore))) = __get_PRIMASK(),       \
	        __ToDo = __disableIntsRetVal();                                         \
	        __ToDo; __ToDo = 0)
#else
#define MY_CRITICAL_SECTION
#endif /* DOXYGEN || CPPCHECK */

#endif // #ifndef MyHwNRF5_h
