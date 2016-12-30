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
 * Based on TMRh20 RF24 library, Copyright (c) 2015 Charles-Henri Hallard <tmrh20@gmail.com>
 */

#ifndef _SPI_H_
#define _SPI_H_

#include <stdio.h>
#include "bcm2835.h"

#define SPI_HAS_TRANSACTION

// SPI Clock divider
#define SPI_CLOCK_DIV2 2
#define SPI_CLOCK_DIV4 4
#define SPI_CLOCK_DIV8 8
#define SPI_CLOCK_DIV16 16
#define SPI_CLOCK_DIV32 32
#define SPI_CLOCK_DIV64 64
#define SPI_CLOCK_DIV128 128

// SPI Data mode
#define SPI_MODE0 BCM2835_SPI_MODE0
#define SPI_MODE1 BCM2835_SPI_MODE1
#define SPI_MODE2 BCM2835_SPI_MODE2
#define SPI_MODE3 BCM2835_SPI_MODE3

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
		init(BCM2835_SPI_CLOCK_DIVIDER_32, BCM2835_SPI_BIT_ORDER_MSBFIRST, BCM2835_SPI_MODE0);
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

		switch (clock) {
		case 500000:
			divider = BCM2835_SPI_CLOCK_DIVIDER_512;
			break;
		case 1000000:
			divider = BCM2835_SPI_CLOCK_DIVIDER_256;
			break;
		case 2000000:
			divider = BCM2835_SPI_CLOCK_DIVIDER_128;
			break;
		case 4000000:
			divider = BCM2835_SPI_CLOCK_DIVIDER_64;
			break;
		case 8000000:
			divider = BCM2835_SPI_CLOCK_DIVIDER_32;
			break;
		case 16000000:
			divider = BCM2835_SPI_CLOCK_DIVIDER_16;
			break;
		default:
			// 8Mhz
			divider = BCM2835_SPI_CLOCK_DIVIDER_32;
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

	friend class SPIClass;
};

/**
 * SPIClass class
 */
class SPIClass
{

private:
	static uint8_t initialized; //!< @brief SPI initialized flag.

public:
	/**
	 * @brief Checks if SPI was initialized.
	 *
	 * @return 0 if wasn't initialized, else 1 or more.
	 */
	static uint8_t is_initialized();
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
};

uint8_t SPIClass::transfer(uint8_t data)
{
	return bcm2835_spi_transfer(data);
}

void SPIClass::transfernb(char* tbuf, char* rbuf, uint32_t len)
{
	bcm2835_spi_transfernb( tbuf, rbuf, len);
}

void SPIClass::transfern(char* buf, uint32_t len)
{
	transfernb(buf, buf, len);
}

extern SPIClass SPI;

#endif
