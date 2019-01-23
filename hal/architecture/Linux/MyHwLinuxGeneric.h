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

#ifndef MyHwLinuxGeneric_h
#define MyHwLinuxGeneric_h

#include <cstdlib>
#include <pthread.h>
#include "SerialPort.h"
#include "StdInOutStream.h"
#include <SPI.h>

#define CRYPTO_LITTLE_ENDIAN

#ifdef MY_LINUX_SERIAL_PORT
#ifdef MY_LINUX_SERIAL_IS_PTY
SerialPort Serial = SerialPort(MY_LINUX_SERIAL_PORT, true);
#else
SerialPort Serial = SerialPort(MY_LINUX_SERIAL_PORT, false);
#endif
#else
StdInOutStream Serial = StdInOutStream();
#endif

#ifndef MY_SERIALDEVICE
#define MY_SERIALDEVICE Serial
#endif

// Define these as macros (do nothing)
#define hwWatchdogReset()
#define hwReboot()

inline void hwDigitalWrite(uint8_t, uint8_t);
inline int hwDigitalRead(uint8_t);
inline void hwPinMode(uint8_t, uint8_t);

bool hwInit(void);
inline void hwReadConfigBlock(void *buf, void *addr, size_t length);
inline void hwWriteConfigBlock(void *buf, void *addr, size_t length);
inline uint8_t hwReadConfig(const int addr);
inline void hwWriteConfig(const int addr, uint8_t value);
inline void hwRandomNumberInit(void);
ssize_t hwGetentropy(void *__buffer, size_t __length);
#define MY_HW_HAS_GETENTROPY
inline uint32_t hwMillis(void);

// SOFTSPI
#ifdef MY_SOFTSPI
#error Soft SPI is not available on this architecture!
#endif
#define hwSPI SPI //!< hwSPI

#ifdef MY_RF24_IRQ_PIN
static pthread_mutex_t hw_mutex = PTHREAD_MUTEX_INITIALIZER;

static __inline__ void __hwUnlock(const  uint8_t *__s)
{
	pthread_mutex_unlock(&hw_mutex);
	(void)__s;
}

static __inline__ void __hwLock()
{
	pthread_mutex_lock(&hw_mutex);
}
#endif

#if defined(DOXYGEN)
#define ATOMIC_BLOCK_CLEANUP
#elif defined(MY_RF24_IRQ_PIN)
#define ATOMIC_BLOCK_CLEANUP uint8_t __atomic_loop \
	__attribute__((__cleanup__( __hwUnlock ))) = 1
#else
#define ATOMIC_BLOCK_CLEANUP
#endif	/* DOXYGEN */

#if defined(DOXYGEN)
#define ATOMIC_BLOCK
#elif defined(MY_RF24_IRQ_PIN)
#define ATOMIC_BLOCK for ( ATOMIC_BLOCK_CLEANUP, __hwLock(); \
                           __atomic_loop ; __atomic_loop = 0 )
#else
#define ATOMIC_BLOCK
#endif	/* DOXYGEN */

#ifndef DOXYGEN
#define MY_CRITICAL_SECTION ATOMIC_BLOCK
#endif	/* DOXYGEN */

#endif
