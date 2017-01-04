/*
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2017 Sensnology AB
 * Full contributor list: https://github.com/mysensors/Arduino/graphs/contributors
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

#include "SPI.h"
#include <fcntl.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include "log.h"

static pthread_mutex_t spiMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutexattr_t attr;

// Declare a single default instance
SPIClass SPI = SPIClass();

uint8_t SPIClass::initialized = 0;
int SPIClass::fd = -1;
std::string SPIClass::device = SPI_SPIDEV_DEVICE;
uint32_t SPIClass::speed = SPI_CLOCK_BASE;
uint32_t SPIClass::speed_temp = SPI_CLOCK_BASE;
struct spi_ioc_transfer SPIClass::tr = {0,0,0,0,0,8,1,0,0,0};	// 8 bits_per_word, 1 cs_change

SPIClass::SPIClass()
{
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&spiMutex, &attr);
}

void SPIClass::begin(int busNo)
{
	if (!initialized) {
		/* set spidev accordingly to busNo like:
		 * busNo = 23 -> /dev/spidev2.3
		 *
		 * a bit messy but simple
		 * */
		device[11] += (busNo / 10) % 10;
		device[13] += busNo % 10;

		init();
	}

	initialized++; // reference count
}

void SPIClass::end()
{
	if (initialized) {
		initialized--;
	}

	if (!initialized) {
		if (!(fd < 0)) {
			close(fd);
			fd = -1;
		}
	}
}

void SPIClass::setBitOrder(uint8_t bit_order)
{
	pthread_mutex_lock(&spiMutex);

	/*
	 * bit order
	 */
	int lsb_setting = bit_order;
	int ret = ioctl(fd, SPI_IOC_WR_LSB_FIRST, &lsb_setting);
	if (ret == -1) {
		logError("Can't set SPI bit order.\n");
		abort();
	}

	pthread_mutex_unlock(&spiMutex);
}

void SPIClass::setDataMode(uint8_t data_mode)
{
	pthread_mutex_lock(&spiMutex);

	/*
	 * spi mode
	 */
	int ret = ioctl(fd, SPI_IOC_WR_MODE, &data_mode);
	if (ret == -1) {
		logError("Can't set SPI mode.\n");
		abort();
	}

	pthread_mutex_unlock(&spiMutex);
}

void SPIClass::setClockDivider(uint16_t divider)
{
	pthread_mutex_lock(&spiMutex);

	/*
	 * max speed hz
	 */
	speed = SPI_CLOCK_BASE / divider;
	int ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (ret == -1) {
		logError("Can't set SPI max speed hz.\n");
		abort();
	}

	pthread_mutex_unlock(&spiMutex);
}

void SPIClass::chipSelect(int csn_chip)
{
	if (csn_chip >= 0 && csn_chip <= 9) {
		device[13] = '0' + (csn_chip % 10);

		init();
	}
}

uint8_t SPIClass::transfer(uint8_t data)
{
	int ret;
	uint8_t tx[1] = {data};
	uint8_t rx[1] = {0};

	pthread_mutex_lock(&spiMutex);

	tr.tx_buf = (unsigned long)&tx[0];
	tr.rx_buf = (unsigned long)&rx[0];
	tr.len = 1;
	tr.speed_hz = speed;

	ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
	if (ret < 1) {
		logError("Can't send spi message.\n");
		abort();
	}

	pthread_mutex_unlock(&spiMutex);

	return rx[0];
}

void SPIClass::transfernb(char* tbuf, char* rbuf, uint32_t len)
{
	int ret;

	pthread_mutex_lock(&spiMutex);

	tr.tx_buf = (unsigned long)tbuf;
	tr.rx_buf = (unsigned long)rbuf;
	tr.len = len;
	tr.speed_hz = speed;

	ret = ioctl(fd, SPI_IOC_MESSAGE(1), &tr);
	if (ret < 1) {
		logError("Can't send spi message.\n");
		abort();
	}

	pthread_mutex_unlock(&spiMutex);
}

void SPIClass::transfern(char* buf, uint32_t len)
{
	transfernb(buf, buf, len);
}

void SPIClass::beginTransaction(SPISettings settings)
{
	int ret;

	pthread_mutex_lock(&spiMutex);

	/*
	 * spi mode
	 */
	ret = ioctl(fd, SPI_IOC_WR_MODE, &settings.dmode);
	if (ret == -1) {
		logError("Can't set spi mode.\n");
		abort();
	}

	// Save the current speed
	speed_temp = speed;
	speed = settings.clock;

	/*
	 * bit order
	 */
	ret = ioctl(fd, SPI_IOC_WR_LSB_FIRST, &settings.border);
	if (ret == -1) {
		logError("Can't set bits per word.\n");
		abort();
	}
}

void SPIClass::endTransaction()
{
	speed = speed_temp;
	pthread_mutex_unlock(&spiMutex);
}

void SPIClass::usingInterrupt(uint8_t interruptNumber)
{
	(void)interruptNumber;
}

void SPIClass::notUsingInterrupt(uint8_t interruptNumber)
{
	(void)interruptNumber;
}

void SPIClass::init()
{
	pthread_mutex_lock(&spiMutex);

	if (fd >= 0) {
		close(fd);
	}

	fd = open(device.c_str(), O_RDWR);
	if (fd < 0) {
		logError("Can't open SPI device: %s\n", device.c_str());
		abort();
	}

	/*
	 * spi mode
	 */
	uint8_t mode = SPI_MODE0;
	int ret = ioctl(fd, SPI_IOC_WR_MODE, &mode);
	if (ret == -1) {
		logError("Can't set SPI mode.\n");
		abort();
	}

	/*
	 * bits per word
	 */
	uint8_t bits = 0;	// 0 corresponds to 8 bits per word
	ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (ret == -1) {
		logError("Can't set SPI bits per word.\n");
		abort();
	}

	/*
	 * max speed hz
	 */
	speed = SPI_CLOCK_BASE;
	ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (ret == -1) {
		logError("Can't set SPI max speed hz.\n");
		abort();
	}

	/*
	 * bit order
	 */
	int lsb_setting = MSBFIRST;
	ret = ioctl(fd, SPI_IOC_WR_LSB_FIRST, &lsb_setting);
	if (ret == -1) {
		logError("Can't set SPI bit order.\n");
		abort();
	}

	pthread_mutex_unlock(&spiMutex);
}
