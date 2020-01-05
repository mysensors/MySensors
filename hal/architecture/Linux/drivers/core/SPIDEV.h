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

#ifndef SPIDEV_h
#define SPIDEV_h

#include <stdint.h>
#include <string>
#include <linux/spi/spidev.h>

#define SPI_HAS_TRANSACTION

#define MSBFIRST 0
#define LSBFIRST SPI_LSB_FIRST

#define SPI_CLOCK_BASE 16000000		// 16Mhz

#define SPI_CLOCK_DIV1 1
#define SPI_CLOCK_DIV2 2
#define SPI_CLOCK_DIV4 4
#define SPI_CLOCK_DIV8 8
#define SPI_CLOCK_DIV16 16
#define SPI_CLOCK_DIV32 32
#define SPI_CLOCK_DIV64 64
#define SPI_CLOCK_DIV128 128
#define SPI_CLOCK_DIV256 256

// SPI Data mode
#define SPI_MODE0 SPI_MODE_0
#define SPI_MODE1 SPI_MODE_1
#define SPI_MODE2 SPI_MODE_2
#define SPI_MODE3 SPI_MODE_3

#ifndef SPI_SPIDEV_DEVICE
#define SPI_SPIDEV_DEVICE "/dev/spidev0.0"
#endif

// Default to Raspberry Pi
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
	 */
	SPISettings()
	{
		init(SPI_CLOCK_BASE, MSBFIRST, SPI_MODE0);
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
		init(clock, bitOrder, dataMode);
	}

	uint32_t clock; //!< @brief SPI clock.
	uint8_t border; //!< @brief SPI bit order.
	uint8_t dmode; //!< @brief SPI data mode.

private:
	/**
	 * @brief Initialized class members.
	 *
	 * @param clk SPI clock.
	 * @param bitOrder SPI bit order.
	 * @param dataMode SPI data mode.
	 */
	void init(uint32_t clk, uint8_t bitOrder, uint8_t dataMode)
	{
		clock = clk;
		border = bitOrder;
		dmode = dataMode;
	}

	friend class SPIDEVClass;
};

/**
 * SPIDEV class
 */
class SPIDEVClass
{

public:
	/**
	 * @brief SPIDEVClass constructor.
	 */
	SPIDEVClass();
	/**
	 * @brief Start SPI operations.
	 */
	static void begin(int busNo=0);
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
	 * @param csn_chip Specifies the CS chip.
	 */
	static void chipSelect(int csn_chip);
	/**
	* @brief Transfer a single byte
	*
	* @param data Byte to send
	* @return Data returned via spi
	*/
	static uint8_t transfer(uint8_t data);
	/**
	* @brief Transfer a buffer of data
	*
	* @param tbuf Transmit buffer
	* @param rbuf Receive buffer
	* @param len Length of the data
	*/
	static void transfernb(char* tbuf, char* rbuf, uint32_t len);
	/**
	* @brief Transfer a buffer of data without an rx buffer
	*
	* @param buf Pointer to a buffer of data
	* @param len Length of the data
	*/
	static void transfern(char* buf, uint32_t len);
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
	static int fd; //!< @brief SPI device file descriptor.
	static std::string device; //!< @brief Default SPI device.
	static uint8_t mode; //!< @brief SPI mode.
	static uint32_t speed; //!< @brief SPI speed.
	static uint8_t bit_order; //!< @brief SPI bit order.
	static struct spi_ioc_transfer tr; //!< @brief Auxiliar struct for data transfer.

	static void init();
};

extern SPIDEVClass SPIDEV;

#endif
