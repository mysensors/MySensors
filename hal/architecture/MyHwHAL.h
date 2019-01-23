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

/**
* @file MyHwHAL.h
*
* MySensors hardware abstraction layer
*/

#ifndef MyHwHAL_h
#define MyHwHAL_h

/**
* @def MY_HWID_PADDING_BYTE
* @brief HwID padding byte
*/
#define MY_HWID_PADDING_BYTE	(0xAAu)

// Implement these as functions or macros
/*
#define hwInit() MY_SERIALDEVICE.begin(BAUD_RATE)
#define hwWatchdogReset() wdt_reset()
#define hwReboot() wdt_enable(WDTO_15MS); while (1)
#define hwMillis() millis()

#define hwDigitalWrite(__pin, __value)
#define hwDigitalRead(__pin)
#define hwPinMode(__pin, __value)

void hwReadConfigBlock(void *buf, void *addr, size_t length);
void hwWriteConfigBlock(void *buf, void *addr, size_t length);
void hwWriteConfig(const int addr, uint8_t value);
uint8_t hwReadConfig(const int addr);
*/

/**
 * @def MY_HW_HAS_GETENTROPY
 * @brief Define this, if hwGetentropy is implemented
 *
 * ssize_t hwGetentropy(void *__buffer, size_t __length);
 */
//#define MY_HW_HAS_GETENTROPY

/// @brief unique ID
typedef uint8_t unique_id_t[16];

/**
 * Sleep for a defined time, using minimum power.
 * @param ms         Time to sleep, in [ms].
 * @return MY_WAKE_UP_BY_TIMER.
 */
int8_t hwSleep(uint32_t ms);

/**
 * Sleep for a defined time, using minimum power, or until woken by interrupt.
 * @param interrupt  Interrupt number, which can wake the mcu from sleep.
 * @param mode       Interrupt mode, as passed to attachInterrupt.
 * @param ms         Time to sleep, in [ms].
 * @return MY_WAKE_UP_BY_TIMER when woken by timer, or interrupt number when woken by interrupt.
 */
int8_t hwSleep(const uint8_t interrupt, const uint8_t mode, uint32_t ms);

/**
 * Sleep for a defined time, using minimum power, or until woken by one of the interrupts.
 * @param interrupt1  Interrupt1 number, which can wake the mcu from sleep.
 * @param mode1       Interrupt1 mode, as passed to attachInterrupt.
 * @param interrupt2  Interrupt2 number, which can wake the mcu from sleep.
 * @param mode2       Interrupt2 mode, as passed to attachInterrupt.
 * @param ms          Time to sleep, in [ms].
 * @return MY_WAKE_UP_BY_TIMER when woken by timer, or interrupt number when woken by interrupt.
 */
int8_t hwSleep(const uint8_t interrupt1, const uint8_t mode1, const uint8_t interrupt2,
               const  uint8_t mode2,
               uint32_t ms);

/**
* Retrieve unique hardware ID
* @param uniqueID unique ID
* @return True if unique ID successfully retrieved
*/
bool hwUniqueID(unique_id_t *uniqueID);

/**
 * CPU voltage
 * @return CPU voltage in mV
 */
uint16_t hwCPUVoltage(void);

/**
 * CPU frequency
 * @return CPU frequency in 1/10Mhz
 */
uint16_t hwCPUFrequency(void);

/**
 * CPU temperature (if available)
 * Adjust calibration parameters via MY_<ARCH>_TEMPERATURE_OFFSET and MY_<ARCH>_TEMPERATURE_GAIN
 * @return CPU temperature in °C, -127 if not available
 */
int8_t hwCPUTemperature(void);

/**
 * Report free memory (if function available)
 * @return free memory in bytes
 */
uint16_t hwFreeMem(void);

#if defined(DEBUG_OUTPUT_ENABLED)
void hwDebugPrint(const char *fmt, ...);
#endif

/**
 * @def MY_CRITICAL_SECTION
 * @brief Creates a block of code that is guaranteed to be executed atomically.
 * Upon entering the block all interrupts are disabled, and re-enabled upon
 * exiting the block from any exit path.
 * A typical example that requires atomic access is a 16 (or more) bit variable
 * that is shared between the main execution path and an ISR, on an 8-bit
 * platform (e.g AVR):
 * @code
 * volatile uint16_t val = 0;
 *
 * void interrupHandler()
 * {
 *   val = ~val;
 * }
 *
 * void loop()
 * {
 *   uint16_t copy_val;
 *   MY_CRITICAL_SECTION
 *   {
 *     copy_val = val;
 *   }
 * }
 * @endcode
 * All code within the MY_CRITICAL_SECTION block will be protected from being
 * interrupted during execution.
 */
#ifdef DOXYGEN
#define MY_CRITICAL_SECTION
#define MY_HW_HAS_GETENTROPY
#endif  /* DOXYGEN */

#endif // #ifdef MyHw_h
