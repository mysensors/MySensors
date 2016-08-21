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
 */

#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <limits.h>
#include <unistd.h>
#include <termios.h>
#include "SerialPort.h"

#define SERIAL_PORT "/dev/ttyAMA0"

SerialPort::SerialPort()
{
	SerialPort(SERIAL_PORT);
}

SerialPort::SerialPort(const char *port) : serialPort(std::string(port))
{}

void SerialPort::begin(int bauds)
{
	speed_t speed;
	struct termios options;

	if ((sd = open(serialPort.c_str(), O_RDWR | O_NOCTTY | O_NDELAY)) == -1) {
		fprintf(stderr, "Unable to open the serial port %s - \n", serialPort.c_str());
		exit(-1);
	}

	// nonblocking mode
	fcntl(sd, F_SETFL, FNDELAY);

	switch (bauds) {
		case     50:	speed =     B50 ; break ;
		case     75:	speed =     B75 ; break ;
		case    110:	speed =    B110 ; break ;
		case    134:	speed =    B134 ; break ;
		case    150:	speed =    B150 ; break ;
		case    200:	speed =    B200 ; break ;
		case    300:	speed =    B300 ; break ;
		case    600:	speed =    B600 ; break ;
		case   1200:	speed =   B1200 ; break ;
		case   1800:	speed =   B1800 ; break ;
		case   2400:	speed =   B2400 ; break ;
		case   9600:	speed =   B9600 ; break ;
		case  19200:	speed =  B19200 ; break ;
		case  38400:	speed =  B38400 ; break ;
		case  57600:	speed =  B57600 ; break ;
		case 115200:	speed = B115200 ; break ;
		default:		speed = B115200 ; break ;
	}

	// Get the current options of the port
	if (tcgetattr(sd, &options) < 0) {
		perror("Couldn't get term attributes");
        exit(1);
	}

	// make raw
	cfmakeraw(&options);

	// Set the baud rate
	cfsetispeed(&options, speed);
	cfsetospeed(&options, speed);

	// turn on READ & ignore ctrl lines
	options.c_cflag |= (CLOCAL | CREAD);
	// 8N1
	options.c_cflag &= ~CSTOPB;

	options.c_lflag &= ~ECHOE;

	// Timer unused
	options.c_cc[VTIME]=0;
	// At least on character before satisfy reading
	options.c_cc[VMIN]=0;

	// Set parameters
	if (tcsetattr(sd, TCSANOW, &options) < 0) {
		perror("Couldn't set term attributes");
		exit(1);
	}

	// flush
	if (tcflush(sd, TCIOFLUSH) < 0) {
		perror("Couldn't flush serial");
		exit(1);
	}

	usleep(10000);
}

int SerialPort::available()
{
	int nbytes = 0;

	if (ioctl(sd, FIONREAD, &nbytes) < 0) {
		fprintf(stderr, "Failed to get byte count on serial.\n");
		exit(-1);
	}
	return nbytes;
}

int SerialPort::read()
{
	unsigned char c;

	::read(sd, &c, 1);
	return c;
}

size_t SerialPort::write(uint8_t b)
{
	return ::write(sd, &b, 1);
}

size_t SerialPort::write(const uint8_t *buffer, size_t size)
{
	return ::write(sd, buffer, size);
}

int SerialPort::peek()
{
	FILE * f = fdopen(sd, "r+");
	int c = getc(f);
	if (c == EOF) return -1;
	ungetc(c, f);
	return c;
}

void SerialPort::flush()
{
	tcflush(sd, TCIFLUSH);
}

void SerialPort::end()
{
	close(sd);
}
