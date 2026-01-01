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

#ifndef MyHwSTM32_h
#define MyHwSTM32_h

#include <SPI.h>
#include <EEPROM.h>

#ifdef __cplusplus
#include <Arduino.h>
#endif

// Crypto endianness
#define CRYPTO_LITTLE_ENDIAN

/**
 * @brief Default serial device for MySensors communication
 * @note Can be overridden in sketch before including MySensors.h
 */
#ifndef MY_SERIALDEVICE
#define MY_SERIALDEVICE Serial
#endif

/**
 * @brief Default debug output device
 */
#ifndef MY_DEBUGDEVICE
#define MY_DEBUGDEVICE MY_SERIALDEVICE
#endif

/**
 * @brief Temperature sensor offset calibration
 * @note Adjust based on your specific STM32 chip calibration
 */
#ifndef MY_STM32_TEMPERATURE_OFFSET
#define MY_STM32_TEMPERATURE_OFFSET (0.0f)
#endif

/**
 * @brief Temperature sensor gain calibration
 */
#ifndef MY_STM32_TEMPERATURE_GAIN
#define MY_STM32_TEMPERATURE_GAIN (1.0f)
#endif

// Printf format string compatibility
// Note: STM32duino core already defines these in avr/pgmspace.h
#ifndef snprintf_P
#define snprintf_P(s, n, ...) snprintf((s), (n), __VA_ARGS__)
#endif
#ifndef vsnprintf_P
#define vsnprintf_P(s, n, ...) vsnprintf((s), (n), __VA_ARGS__)
#endif

// redefine 8 bit types of inttypes.h
#undef PRId8
#undef PRIi8
#undef PRIo8
#undef PRIu8
#undef PRIx8
#undef PRIX8
#define PRId8		"d"
#define PRIi8		"i"
#define PRIo8		"o"
#define PRIu8		"u"
#define PRIx8		"x"
#define PRIX8		"X"

// Digital I/O macros - wrap Arduino functions
#define hwDigitalWrite(__pin, __value) digitalWrite(__pin, __value)
#define hwDigitalRead(__pin) digitalRead(__pin)
#define hwPinMode(__pin, __value) pinMode(__pin, __value)

// Timing functions
#define hwMillis() millis()

/**
 * @brief Get remaining sleep time
 * @return Remaining sleep time in milliseconds
 */
uint32_t hwGetSleepRemaining(void);

/**
 * @brief Initialize hardware
 * @return true if initialization successful
 */
bool hwInit(void);

/**
 * @brief Reset the watchdog timer
 */
void hwWatchdogReset(void);

/**
 * @brief Reboot the system
 */
void hwReboot(void);

/**
 * @brief Initialize random number generator
 * @note Uses internal temperature sensor as entropy source
 */
void hwRandomNumberInit(void);

/**
 * @brief Read configuration block from EEPROM
 * @param buf Destination buffer
 * @param addr EEPROM address (as void pointer for compatibility)
 * @param length Number of bytes to read
 */
void hwReadConfigBlock(void *buf, void *addr, size_t length);

/**
 * @brief Write configuration block to EEPROM
 * @param buf Source buffer
 * @param addr EEPROM address (as void pointer for compatibility)
 * @param length Number of bytes to write
 */
void hwWriteConfigBlock(void *buf, void *addr, size_t length);

/**
 * @brief Write single byte to EEPROM
 * @param addr EEPROM address
 * @param value Byte value to write
 */
void hwWriteConfig(const int addr, uint8_t value);

/**
 * @brief Read single byte from EEPROM
 * @param addr EEPROM address
 * @return Byte value read
 */
uint8_t hwReadConfig(const int addr);

/**
 * @brief Get unique chip ID
 * @param uniqueID Pointer to unique_id_t structure
 * @return true if successful
 */
bool hwUniqueID(unique_id_t *uniqueID);

/**
 * @brief Get CPU supply voltage
 * @return Voltage in millivolts
 */
uint16_t hwCPUVoltage(void);

/**
 * @brief Get CPU frequency
 * @return Frequency in 0.1 MHz units (e.g., 840 = 84 MHz)
 */
uint16_t hwCPUFrequency(void);

/**
 * @brief Get CPU temperature
 * @return Temperature in degrees Celsius
 */
int8_t hwCPUTemperature(void);

/**
 * @brief Get free memory (heap)
 * @return Free memory in bytes
 */
uint16_t hwFreeMem(void);

/**
 * @brief Sleep for specified milliseconds
 * @param ms Milliseconds to sleep (0 = sleep until interrupt)
 * @return MY_WAKE_UP_BY_TIMER (-1) if woken by timer, MY_SLEEP_NOT_POSSIBLE (-2) on error
 * @note Uses STOP mode with low-power regulator (10-50 µA sleep current)
 * @note Maximum sleep time depends on RTC configuration (~18 hours)
 */
int8_t hwSleep(uint32_t ms);

/**
 * @brief Sleep with interrupt wake
 * @param interrupt Arduino pin number for interrupt wake-up
 * @param mode Interrupt mode (RISING, FALLING, CHANGE)
 * @param ms Maximum sleep time in milliseconds (0 = no timeout)
 * @return Interrupt number (0-255) if woken by interrupt, MY_WAKE_UP_BY_TIMER (-1) if timeout,
 *         MY_SLEEP_NOT_POSSIBLE (-2) on error
 * @note Supports wake-up on any GPIO pin via EXTI (critical for radio IRQ)
 */
int8_t hwSleep(const uint8_t interrupt, const uint8_t mode, uint32_t ms);

/**
 * @brief Sleep with dual interrupt wake
 * @param interrupt1 First Arduino pin number for interrupt wake-up
 * @param mode1 First interrupt mode (RISING, FALLING, CHANGE)
 * @param interrupt2 Second Arduino pin number for interrupt wake-up
 * @param mode2 Second interrupt mode (RISING, FALLING, CHANGE)
 * @param ms Maximum sleep time in milliseconds (0 = no timeout)
 * @return Interrupt number that caused wake-up, MY_WAKE_UP_BY_TIMER (-1) if timeout,
 *         MY_SLEEP_NOT_POSSIBLE (-2) on error
 * @note Useful for hybrid sensors (e.g., button press OR periodic wake-up)
 */
int8_t hwSleep(const uint8_t interrupt1, const uint8_t mode1,
               const uint8_t interrupt2, const uint8_t mode2, uint32_t ms);

// SPI configuration
#ifdef MY_SOFTSPI
#error Soft SPI is not available on this architecture!
#endif
#define hwSPI SPI //!< Hardware SPI

/**
 * @brief Critical section implementation for STM32
 * @note Uses PRIMASK register to disable/restore interrupts
 */
static __inline__ uint8_t __disableIntsRetVal(void)
{
	__disable_irq();
	return 1;
}

static __inline__ void __priMaskRestore(const uint32_t *priMask)
{
	__set_PRIMASK(*priMask);
}

#ifndef DOXYGEN
#define MY_CRITICAL_SECTION    for ( uint32_t __savePriMask __attribute__((__cleanup__(__priMaskRestore))) = __get_PRIMASK(), __ToDo = __disableIntsRetVal(); __ToDo ; __ToDo = 0 )
#endif  /* DOXYGEN */

#endif // MyHwSTM32_h
