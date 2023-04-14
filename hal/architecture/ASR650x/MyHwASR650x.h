/**
* The MySensors Arduino library handles the wireless radio link and protocol
* between your home built sensors/actuators and HA controller of choice.
* The sensors forms a self healing radio network with optional repeaters. Each
* repeater and gateway builds a routing tables in EEPROM which keeps track of the
* network topology allowing messages to be routed to nodes.
*
* Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
* Copyright (C) 2013-2020 Sensnology AB
* Full contributor list: https://github.com/mysensors/MySensors/graphs/contributors
*
* Documentation: http://www.mysensors.org
* Support Forum: http://forum.mysensors.org
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* version 2 as published by the Free Software Foundation.
*
*/

#ifndef MyHwASR650x_h
#define MyHwASR650x_h

#include <SPI.h>
#include "atomic"
#include "board-config.h"

#ifdef __cplusplus
#include <Arduino.h>
#endif

#define CRYPTO_BIG_ENDIAN

#ifndef MY_SERIALDEVICE
#define MY_SERIALDEVICE Serial
#endif

#ifndef MY_DEBUGDEVICE
#define MY_DEBUGDEVICE MY_SERIALDEVICE
#endif

// Define these as macros to save valuable space
#define hwDigitalWrite(__pin, __value) digitalWrite(__pin, __value)
#define hwDigitalRead(__pin) digitalRead(__pin)
#define hwPinMode(__pin, __value) pinMode(__pin, __value)
#define hwMillis() millis()
#define hwGetSleepRemaining() (0ul)

void hwRandomNumberInit(void);
bool hwInit(void);
void hwWatchdogReset(void);
void hwReboot(void);

uint8_t hwReadConfig(const int pos);
void hwWriteConfig(const int pos, uint8_t value);
void hwReadConfigBlock(void *buffer, const void *pos, size_t length);
void hwWriteConfigBlock(void *buffer, const void *pos, size_t length);


// SOFTSPI
#ifdef MY_SOFTSPI
#error Soft SPI is not available on this architecture!
#endif
#ifndef hwSPI
#define hwSPI SPI //!< hwSPI
#endif

#define MY_HW_HAS_GETENTROPY

#ifndef digitalPinToInterrupt
#define digitalPinToInterrupt(__pin) (__pin)
#endif

// if we got a CubeCell, we will do all the necessary defines here
#if defined(CubeCell_Board) || defined(CubeCell_BoardPlus) || defined(CubeCell_Capsule) || defined(CubeCell_GPS) || defined(CubeCell_HalfAA) || defined(CubeCell_Module) || defined(CubeCell_ModulePlus)
#define MY_SX126x_RESET_PIN RADIO_RESET
#define MY_SX126x_CS_PIN RADIO_NSS
#define MY_SX126x_IRQ_PIN RADIO_DIO_1
#define MY_SX126x_IRQ_NUM digitalPinToInterrupt(MY_SX126x_IRQ_PIN)
#define MY_SX126x_BUSY_PIN RADIO_BUSY
#define MY_SX126x_USE_TCXO
#define MY_SX126x_TCXO_VOLTAGE (SX126x_TCXO_1V8)
#define MY_SX126c_TCXO_STARTUP_DELAY 5
#define MY_SX126x_USE_DIO2_ANT_SWITCH
#endif

#define yield()

#endif
