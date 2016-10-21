/*
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Marcelo Aquino <marceloaqno@gmail.org>
 * Copyright (C) 2016 Marcelo Aquino
 * Full contributor list: https://github.com/mysensors/MySensors/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * Based on wiringPi Copyright (c) 2012 Gordon Henderson.
 */

#ifndef pins_io_h
#define pins_io_h

#include <stdint.h>
#include "bcm2835.h"

namespace rpi_util {

typedef enum {
	LSBFIRST = BCM2835_SPI_BIT_ORDER_LSBFIRST,
	MSBFIRST = BCM2835_SPI_BIT_ORDER_MSBFIRST
} rpi_bitorder;

typedef enum {
	INPUT = BCM2835_GPIO_FSEL_INPT,
	OUTPUT = BCM2835_GPIO_FSEL_OUTP
} rpi_pinmode;

typedef enum {
	CHANGE = 1,
	FALLING = 2,
	RISING = 3,
	NONE = 4
} rpi_pinedge;

typedef enum {
	PIN_SPI_SS = 24,
	PIN_SPI_MOSI = 19,
	PIN_SPI_MISO = 21,
	PIN_SPI_SCK = 23
} rpi_spipins;

const uint8_t SS   = PIN_SPI_SS;
const uint8_t MOSI = PIN_SPI_MOSI;
const uint8_t MISO = PIN_SPI_MISO;
const uint8_t SCK  = PIN_SPI_SCK;

/* Some useful arduino functions */
void pinMode(uint8_t physPin, uint8_t mode);
void digitalWrite(uint8_t physPin, uint8_t value);
uint8_t digitalRead(uint8_t physPin);
void attachInterrupt(uint8_t physPin,void (*func)(), uint8_t mode);
void detachInterrupt(uint8_t physPin);
uint8_t digitalPinToInterrupt(uint8_t physPin);
void interrupts();
void noInterrupts();

}

#endif
