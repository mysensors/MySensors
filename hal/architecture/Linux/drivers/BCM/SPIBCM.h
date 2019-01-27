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
 *
 * Based on TMRh20 RF24 library, Copyright (c) 2015 Charles-Henri Hallard <tmrh20@gmail.com>
 */

#ifndef SPIBCM_h
#define SPIBCM_h

#include <stdio.h>
#include "bcm2835.h"
#include "BCM.h"

#define SPI_HAS_TRANSACTION

#define SPI_CLOCK_BASE 256000000

// SPI Clock divider
#define SPI_CLOCK_DIV1 BCM2835_SPI_CLOCK_DIVIDER_1
#define SPI_CLOCK_DIV2 BCM2835_SPI_CLOCK_DIVIDER_2
#define SPI_CLOCK_DIV4 BCM2835_SPI_CLOCK_DIVIDER_4
#define SPI_CLOCK_DIV8 BCM2835_SPI_CLOCK_DIVIDER_8
#define SPI_CLOCK_DIV16 BCM2835_SPI_CLOCK_DIVIDER_16
#define SPI_CLOCK_DIV32 BCM2835_SPI_CLOCK_DIVIDER_32
#define SPI_CLOCK_DIV64 BCM2835_SPI_CLOCK_DIVIDER_64
#define SPI_CLOCK_DIV128 BCM2835_SPI_CLOCK_DIVIDER_128
#define SPI_CLOCK_DIV256 BCM2835_SPI_CLOCK_DIVIDER_256
#define SPI_CLOCK_DIV512 BCM2835_SPI_CLOCK_DIVIDER_512
#define SPI_CLOCK_DIV1024 BCM2835_SPI_CLOCK_DIVIDER_1024
#define SPI_CLOCK_DIV2048 BCM2835_SPI_CLOCK_DIVIDER_2048
#define SPI_CLOCK_DIV4096 BCM2835_SPI_CLOCK_DIVIDER_4096
#define SPI_CLOCK_DIV8192 BCM2835_SPI_CLOCK_DIVIDER_8192
#define SPI_CLOCK_DIV16384 BCM2835_SPI_CLOCK_DIVIDER_16384
#define SPI_CLOCK_DIV32768 BCM2835_SPI_CLOCK_DIVIDER_32768
#define SPI_CLOCK_DIV65536 BCM2835_SPI_CLOCK_DIVIDER_65536

// SPI Data mode
#define SPI_MODE0 BCM2835_SPI_MODE0
#define SPI_MODE1 BCM2835_SPI_MODE1
#define SPI_MODE2 BCM2835_SPI_MODE2
#define SPI_MODE3 BCM2835_SPI_MODE3

#define LSBFIRST BCM2835_SPI_BIT_ORDER_LSBFIRST
#define MSBFIRST BCM2835_SPI_BIT_ORDER_MSBFIRST

const uint8_t SS   = 24;
const uint8_t MOSI = 19;
const uint8_t MISO = 21;
const uint8_t SCK  = 23;

/**
 * SPISettings class
 */
class SPISettings
{

public:
	/**
	 * @brief SPISettings constructor.
	 *
	 * Default clock speed is 8Mhz.
	 */
	SPISettings()
	{
		init(SPI_CLOCK_DIV32, MSBFIRST, SPI_MODE0);
	}
	/**
	 * @brief SPISettings constructor.
	 *
	 * @param clock SPI clock speed in Hz.
	 * @param bitOrder SPI bit order.
	 * @param dataMode SPI data mode.
	 */
	SPISettings(uint32_t clock, uint8_t bitOrder, uint8_t dataMode)
	{
		uint16_t divider;

		if (clock >= SPI_CLOCK_BASE) {
			divider = SPI_CLOCK_DIV1;
		} else if (clock >= SPI_CLOCK_BASE / 2) {
			divider = SPI_CLOCK_DIV2;
		} else if (clock >= SPI_CLOCK_BASE / 4) {
			divider = SPI_CLOCK_DIV4;
		} else if (clock >= SPI_CLOCK_BASE / 8) {
			divider = SPI_CLOCK_DIV8;
		} else if (clock >= SPI_CLOCK_BASE / 16) {
			divider = SPI_CLOCK_DIV16;
		} else if (clock >= SPI_CLOCK_BASE / 32) {
			divider = SPI_CLOCK_DIV32;
		} else if (clock >= SPI_CLOCK_BASE / 64) {
			divider = SPI_CLOCK_DIV64;
		} else if (clock >= SPI_CLOCK_BASE / 128) {
			divider = SPI_CLOCK_DIV128;
		} else if (clock >= SPI_CLOCK_BASE / 256) {
			divider = SPI_CLOCK_DIV256;
		} else if (clock >= SPI_CLOCK_BASE / 512) {
			divider = SPI_CLOCK_DIV512;
		} else if (clock >= SPI_CLOCK_BASE / 1024) {
			divider = SPI_CLOCK_DIV1024;
		} else if (clock >= SPI_CLOCK_BASE / 2048) {
			divider = SPI_CLOCK_DIV2048;
		} else if (clock >= SPI_CLOCK_BASE / 4096) {
			divider = SPI_CLOCK_DIV4096;
		} else if (clock >= SPI_CLOCK_BASE / 8192) {
			divider = SPI_CLOCK_DIV8192;
		} else if (clock >= SPI_CLOCK_BASE / 16384) {
			divider = SPI_CLOCK_DIV16384;
		} else if (clock >= SPI_CLOCK_BASE / 32768) {
			divider = SPI_CLOCK_DIV32768;
		} else if (clock >= SPI_CLOCK_BASE / 65536) {
			divider = SPI_CLOCK_DIV65536;
		} else {
			// Default to 8Mhz
			divider = SPI_CLOCK_DIV32;
		}

		init(divider, bitOrder, dataMode);
	}

	uint16_t cdiv; //!< @brief SPI clock divider.
	uint8_t border; //!< @brief SPI bit order.
	uint8_t dmode; //!< @brief SPI data mode.

private:
	/**
	 * @brief Initialized class members.
	 *
	 * @param divider SPI clock divider.
	 * @param bitOrder SPI bit order.
	 * @param dataMode SPI data mode.
	 */
	void init(uint16_t divider, uint8_t bitOrder, uint8_t dataMode)
	{
		cdiv = divider;
		border = bitOrder;
		dmode = dataMode;
	}

	friend class SPIBCMClass;
};

/**
 * SPIBCM class
 */
class SPIBCMClass
{

public:
	/**
	 * @brief Send and receive a byte.
	 *
	 * @param data to send.
	 * @return byte received.
	 */
	inline static uint8_t transfer(uint8_t data);
	/**
	 * @brief Send and receive a number of bytes.
	 *
	 * @param tbuf Sending buffer.
	 * @param rbuf Receive buffer.
	 * @param len Buffer length.
	 */
	inline static void transfernb(char* tbuf, char* rbuf, uint32_t len);
	/**
	 * @brief Send and receive a number of bytes.
	 *
	 * @param buf Buffer to read from and write to.
	 * @param len Buffer length.
	 */
	inline static void transfern(char* buf, uint32_t len);
	/**
	 * @brief Start SPI operations.
	 */
	static void begin();
	/**
	 * @brief End SPI operations.
	 */
	static void end();
	/**
	 * @brief Sets the SPI bit order.
	 *
	 * @param bit_order The desired bit order.
	 */
	static void setBitOrder(uint8_t bit_order);
	/**
	 * @brief Sets the SPI data mode.
	 *
	 * @param data_mode The desired data mode.
	 */
	static void setDataMode(uint8_t data_mode);
	/**
	 * @brief Sets the SPI clock divider and therefore the SPI clock speed.
	 *
	 * @param divider The desired SPI clock divider.
	 */
	static void setClockDivider(uint16_t divider);
	/**
	 * @brief Sets the chip select pin.
	 *
	 * @param csn_pin Specifies the CS pin.
	 */
	static void chipSelect(int csn_pin);
	/**
	 * @brief Start SPI transaction.
	 *
	 * @param settings for SPI.
	 */
	static void beginTransaction(SPISettings settings);
	/**
	 * @brief End SPI transaction.
	 */
	static void endTransaction();
	/**
	 * @brief Not implemented.
	 *
	 * @param interruptNumber ignored parameter.
	 */
	static void usingInterrupt(uint8_t interruptNumber);
	/**
	 * @brief Not implemented.
	 *
	 * @param interruptNumber ignored parameter.
	 */
	static void notUsingInterrupt(uint8_t interruptNumber);

private:
	static uint8_t initialized; //!< @brief SPI initialized flag.
};

uint8_t SPIBCMClass::transfer(uint8_t data)
{
	return bcm2835_spi_transfer(data);
}

void SPIBCMClass::transfernb(char* tbuf, char* rbuf, uint32_t len)
{
	bcm2835_spi_transfernb( tbuf, rbuf, len);
}

void SPIBCMClass::transfern(char* buf, uint32_t len)
{
	transfernb(buf, buf, len);
}

extern SPIBCMClass SPIBCM;

#endif
