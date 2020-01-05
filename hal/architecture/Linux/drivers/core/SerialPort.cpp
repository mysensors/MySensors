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

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <limits.h>
#include <unistd.h>
#include <termios.h>
#include <grp.h>
#include <errno.h>
#include <sys/stat.h>
#include "log.h"
#include "SerialPort.h"

SerialPort::SerialPort(const char *port, bool isPty) : serialPort(std::string(port)), isPty(isPty)
{
	sd = -1;
}

void SerialPort::begin(int bauds)
{
	if (!open(bauds)) {
		logError("Failed to open serial port.\n");
		exit(1);
	}
	logDebug("Serial port %s (%d baud) created\n", serialPort.c_str(), bauds);
}

bool SerialPort::open(int bauds)
{
	speed_t speed;
	struct termios options;

	if (isPty) {
		sd = posix_openpt(O_RDWR | O_NOCTTY | O_NDELAY);
		if (sd < 0) {
			logError("Couldn't open a PTY: %s\n", strerror(errno));
			return false;
		}

		if (grantpt(sd) != 0) {
			logError("Couldn't grant permission to the PTY: %s\n", strerror(errno));
			return false;
		}

		if (unlockpt(sd) != 0) {
			logError("Couldn't unlock the PTY: %s\n", strerror(errno));
			return false;
		}

		/* create a symlink with predictable name to the PTY device */
		unlink(serialPort.c_str());	// remove the symlink if it already exists
		if (symlink(ptsname(sd), serialPort.c_str()) != 0) {
			logError("Couldn't create a symlink '%s' to PTY! (%d) %s\n", serialPort.c_str(), errno,
			         strerror(errno));
			return false;
		}
	} else {
		if ((sd = ::open(serialPort.c_str(), O_RDWR | O_NOCTTY | O_NDELAY)) == -1) {
			logError("Unable to open the serial port %s\n", serialPort.c_str());
			return false;
		}

		// nonblocking mode
		fcntl(sd, F_SETFL, FNDELAY);
	}

	switch (bauds) {
	case     50:
		speed =     B50 ;
		break ;
	case     75:
		speed =     B75 ;
		break ;
	case    110:
		speed =    B110 ;
		break ;
	case    134:
		speed =    B134 ;
		break ;
	case    150:
		speed =    B150 ;
		break ;
	case    200:
		speed =    B200 ;
		break ;
	case    300:
		speed =    B300 ;
		break ;
	case    600:
		speed =    B600 ;
		break ;
	case   1200:
		speed =   B1200 ;
		break ;
	case   1800:
		speed =   B1800 ;
		break ;
	case   2400:
		speed =   B2400 ;
		break ;
	case   9600:
		speed =   B9600 ;
		break ;
	case  19200:
		speed =  B19200 ;
		break ;
	case  38400:
		speed =  B38400 ;
		break ;
	case  57600:
		speed =  B57600 ;
		break ;
	case 115200:
		speed = B115200 ;
		break ;
	default:
		speed = B115200 ;
		break ;
	}

	// Get the current options of the port
	if (tcgetattr(sd, &options) < 0) {
		logError("Couldn't get term attributes: %s\n", strerror(errno));
		return false;
	}

	// Clear all the options
	bzero(&options, sizeof(options));

	// Set the baud rate
	cfsetispeed(&options, speed);
	cfsetospeed(&options, speed);

	// Configure the device : 8 bits, no parity, no control
	options.c_cflag |= ( CLOCAL | CREAD |  CS8);
	// Ignore framing errors, parity errors and BREAK condition on input.
	options.c_iflag |= ( IGNPAR | IGNBRK );

	// Timer unused
	options.c_cc[VTIME]=0;
	// At least on character before satisfy reading
	options.c_cc[VMIN]=0;

	// Set parameters
	if (tcsetattr(sd, TCSANOW, &options) < 0) {
		logError("Couldn't set term attributes: %s\n", strerror(errno));
		return false;
	}

	// flush
	if (tcflush(sd, TCIOFLUSH) < 0) {
		logError("Couldn't flush serial: %s\n", strerror(errno));
		return false;
	}

	usleep(10000);

	return true;
}

bool SerialPort::setGroupPerm(const char *groupName)
{
	const mode_t ttyPermissions = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP;

	if (sd != -1 && groupName != NULL) {
		struct group *devGrp = getgrnam(groupName);
		if (devGrp == NULL) {
			logError("getgrnam: %s failed. (%d) %s\n", groupName, errno, strerror(errno));
			return false;
		}

		const char *dev;
		if (isPty) {
			dev = ptsname(sd);
		} else {
			dev = serialPort.c_str();
		}

		int ret = chown(dev, -1, devGrp->gr_gid);
		if (ret == -1) {
			logError("Could not change PTY owner! (%d) %s\n", errno, strerror(errno));
			return false;
		}

		ret = chmod(dev, ttyPermissions);
		if (ret != 0) {
			logError("Could not change PTY permissions! (%d) %s\n", errno, strerror(errno));
			return false;
		}

		return true;
	}
	return false;
}

int SerialPort::available()
{
	int nbytes = 0;

	if (ioctl(sd, FIONREAD, &nbytes) < 0) {
		logError("Failed to get byte count on serial.\n");
		exit(-1);
	}
	return nbytes;
}

int SerialPort::read()
{
	unsigned char c;

	int ret = ::read(sd, &c, 1);
	if (ret < 0) {
		logError("Serial - read failed: %s\n", strerror(errno));
	} else if (ret == 1) {
		return c;
	}

	return -1;
}

size_t SerialPort::write(uint8_t b)
{
	int ret = ::write(sd, &b, 1);
	if (ret < 0) {
		logError("Serial - write failed: %s\n", strerror(errno));
	}
	return ret;
}

size_t SerialPort::write(const uint8_t *buffer, size_t size)
{
	int ret = ::write(sd, buffer, size);
	if (ret < 0) {
		logError("Serial - write failed: %s\n", strerror(errno));
	}
	return ret;
}

int SerialPort::peek()
{
	FILE * f = fdopen(sd, "r+");
	int c = getc(f);
	if (c == EOF) {
		return -1;
	}
	ungetc(c, f);
	return c;
}

void SerialPort::flush()
{
	// Waits until all output written to sd has been transmitted
	if (tcdrain(sd) < 0) {
		logError("Couldn't flush serial: %s\n", strerror(errno));
	}
}

void SerialPort::end()
{
	close(sd);

	if (isPty) {
		unlink(serialPort.c_str());	// remove the symlink
	}
}
