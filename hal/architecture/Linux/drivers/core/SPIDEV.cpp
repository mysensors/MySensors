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

#include "SPIDEV.h"
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
SPIDEVClass SPIDEV = SPIDEVClass();

uint8_t SPIDEVClass::initialized = 0;
int SPIDEVClass::fd = -1;
std::string SPIDEVClass::device = SPI_SPIDEV_DEVICE;
uint8_t SPIDEVClass::mode = SPI_MODE0;
uint32_t SPIDEVClass::speed = SPI_CLOCK_BASE;
uint8_t SPIDEVClass::bit_order = MSBFIRST;
struct spi_ioc_transfer SPIDEVClass::tr = {0,0,0,0,0,8,0,0,0,0};	// 8 bits_per_word, 0 cs_change

SPIDEVClass::SPIDEVClass()
{
	pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
	pthread_mutex_init(&spiMutex, &attr);
}

void SPIDEVClass::begin(int busNo)
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

void SPIDEVClass::end()
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

void SPIDEVClass::setBitOrder(uint8_t border)
{
	pthread_mutex_lock(&spiMutex);

	/*
	 * bit order
	 */
	bit_order = border;
	int ret = ioctl(fd, SPI_IOC_WR_LSB_FIRST, &bit_order);
	if (ret == -1) {
		logError("Can't set SPI bit order.\n");
		abort();
	}

	ret = ioctl(fd, SPI_IOC_RD_LSB_FIRST, &bit_order);
	if (ret == -1) {
		logError("Can't set SPI bit order.\n");
		abort();
	}

	pthread_mutex_unlock(&spiMutex);
}

void SPIDEVClass::setDataMode(uint8_t data_mode)
{
	pthread_mutex_lock(&spiMutex);

	/*
	 * spi mode
	 */
	mode = data_mode;
	int ret = ioctl(fd, SPI_IOC_WR_MODE, &mode);
	if (ret == -1) {
		logError("Can't set SPI mode.\n");
		abort();
	}

	ret = ioctl(fd, SPI_IOC_RD_MODE, &mode);
	if (ret == -1) {
		logError("Can't set SPI mode.\n");
		abort();
	}

	pthread_mutex_unlock(&spiMutex);
}

void SPIDEVClass::setClockDivider(uint16_t divider)
{
	if (divider == 0) {
		return;
	}

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

	ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
	if (ret == -1) {
		logError("Can't set SPI max speed hz.\n");
		abort();
	}

	pthread_mutex_unlock(&spiMutex);
}

void SPIDEVClass::chipSelect(int csn_chip)
{
	if (csn_chip >= 0 && csn_chip <= 9) {
		device[13] = '0' + (csn_chip % 10);

		init();
	}
}

uint8_t SPIDEVClass::transfer(uint8_t data)
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

void SPIDEVClass::transfernb(char* tbuf, char* rbuf, uint32_t len)
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

void SPIDEVClass::transfern(char* buf, uint32_t len)
{
	transfernb(buf, buf, len);
}

void SPIDEVClass::beginTransaction(SPISettings settings)
{
	int ret;

	pthread_mutex_lock(&spiMutex);

	/*
	 * spi mode
	 */
	if (settings.dmode != mode) {
		mode = settings.dmode;

		ret = ioctl(fd, SPI_IOC_WR_MODE, &mode);
		if (ret == -1) {
			logError("Can't set spi mode.\n");
			abort();
		}

		ret = ioctl(fd, SPI_IOC_RD_MODE, &mode);
		if (ret == -1) {
			logError("Can't set spi mode.\n");
			abort();
		}
	}

	/*
	 * speed
	 */
	if (settings.clock != speed) {
		speed = settings.clock;

		ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
		if (ret == -1) {
			logError("Can't set SPI max speed hz.\n");
			abort();
		}

		ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
		if (ret == -1) {
			logError("Can't set SPI max speed hz.\n");
			abort();
		}
	}

	/*
	 * bit order
	 */
	if (settings.border != bit_order) {
		bit_order = settings.border;

		ret = ioctl(fd, SPI_IOC_WR_LSB_FIRST, &bit_order);
		if (ret == -1) {
			logError("Can't set SPI bit order.\n");
			abort();
		}

		ret = ioctl(fd, SPI_IOC_RD_LSB_FIRST, &bit_order);
		if (ret == -1) {
			logError("Can't set SPI bit order.\n");
			abort();
		}
	}
}

void SPIDEVClass::endTransaction()
{
	pthread_mutex_unlock(&spiMutex);
}

void SPIDEVClass::usingInterrupt(uint8_t interruptNumber)
{
	(void)interruptNumber;
}

void SPIDEVClass::notUsingInterrupt(uint8_t interruptNumber)
{
	(void)interruptNumber;
}

void SPIDEVClass::init()
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
	int ret = ioctl(fd, SPI_IOC_WR_MODE, &mode);
	if (ret == -1) {
		logError("Can't set SPI mode.\n");
		abort();
	}

	ret = ioctl(fd, SPI_IOC_RD_MODE, &mode);
	if (ret == -1) {
		logError("Can't set SPI mode.\n");
		abort();
	}

	/*
	 * bits per word
	 */
	uint8_t bits = 8;	// 8 bits per word
	ret = ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits);
	if (ret == -1) {
		logError("Can't set SPI bits per word.\n");
		abort();
	}

	ret = ioctl(fd, SPI_IOC_RD_BITS_PER_WORD, &bits);
	if (ret == -1) {
		logError("Can't set SPI bits per word.\n");
		abort();
	}

	/*
	 * max speed hz
	 */
	ret = ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed);
	if (ret == -1) {
		logError("Can't set SPI max speed hz.\n");
		abort();
	}

	ret = ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &speed);
	if (ret == -1) {
		logError("Can't set SPI max speed hz.\n");
		abort();
	}

	/*
	 * bit order
	 */
	ret = ioctl(fd, SPI_IOC_WR_LSB_FIRST, &bit_order);
	if (ret == -1) {
		logError("Can't set SPI bit order.\n");
		abort();
	}

	ret = ioctl(fd, SPI_IOC_RD_LSB_FIRST, &bit_order);
	if (ret == -1) {
		logError("Can't set SPI bit order.\n");
		abort();
	}

	pthread_mutex_unlock(&spiMutex);
}
