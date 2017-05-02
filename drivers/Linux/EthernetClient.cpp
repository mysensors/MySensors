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
 * Based on Arduino ethernet library, Copyright (c) 2010 Arduino LLC. All right reserved.
 */

#include <cstdio>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <cstring>
#include <unistd.h>
#include <sys/time.h>
#include <netinet/tcp.h>
#include <errno.h>
#include "log.h"
#include "EthernetClient.h"

EthernetClient::EthernetClient() : _sock(-1)
{
}

EthernetClient::EthernetClient(int sock) : _sock(sock)
{
}

int EthernetClient::connect(const char* host, uint16_t port)
{
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	char s[INET6_ADDRSTRLEN];
	char port_str[6];

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	sprintf(port_str, "%hu", port);
	if ((rv = getaddrinfo(host, port_str, &hints, &servinfo)) != 0) {
		logError("getaddrinfo: %s\n", gai_strerror(rv));
		return -1;
	}

	// loop through all the results and connect to the first we can
	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
		                     p->ai_protocol)) == -1) {
			logError("socket: %s\n", strerror(errno));
			continue;
		}

		if (::connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			logError("connect: %s\n", strerror(errno));
			continue;
		}

		break;
	}

	if (p == NULL) {
		logError("failed to connect\n");
		return -1;
	}

	_sock = sockfd;

	void *addr = &(((struct sockaddr_in*)p->ai_addr)->sin_addr);
	inet_ntop(p->ai_family, addr, s, sizeof s);
	logDebug("connected to %s\n", s);

	freeaddrinfo(servinfo); // all done with this structure

	return 1;
}

int EthernetClient::connect(IPAddress ip, uint16_t port)
{
	return connect(ip.toString().c_str(), port);
}

size_t EthernetClient::write(uint8_t b)
{
	return write(&b, 1);
}

size_t EthernetClient::write(const uint8_t *buf, size_t size)
{
	int bytes = 0;

	if (_sock == -1) {
		return 0;
	}

	while (size > 0) {
		int rc = send(_sock, buf + bytes, size, MSG_NOSIGNAL | MSG_DONTWAIT);
		if (rc == -1) {
			logError("send: %s\n", strerror(errno));
			close(_sock);
			_sock = -1;
			break;
		}
		bytes += rc;
		size -= rc;
	}

	return bytes;
}

size_t EthernetClient::write(const char *str)
{
	if (str == NULL) {
		return 0;
	}
	return write((const uint8_t *)str, strlen(str));
}
size_t EthernetClient::write(const char *buffer, size_t size)
{
	return write((const uint8_t *)buffer, size);
}

int EthernetClient::available()
{
	int count = 0;

	if (_sock != -1) {
		ioctl(_sock, FIONREAD, &count);
		return count;
	}
	return 0;
}

int EthernetClient::read()
{
	uint8_t b;
	if ( recv(_sock, &b, 1, 0) > 0 ) {
		// recv worked
		return b;
	} else {
		// No data available
		return -1;
	}
}

int EthernetClient::read(uint8_t *buf, size_t size)
{
	return recv(_sock, buf, size, 0);
}

int EthernetClient::peek()
{
	uint8_t b;

	return recv(_sock, &b, 1, MSG_PEEK | MSG_DONTWAIT);
}

void EthernetClient::flush()
{
	// There isn't much we can do here
	return;
}

void EthernetClient::stop()
{
	if (_sock == -1) {
		return;
	}

	// attempt to close the connection gracefully (send a FIN to other side)
	shutdown(_sock, SHUT_RDWR);

	timeval startTime, curTime;
	gettimeofday(&startTime, NULL);

	// wait up to a second for the connection to close
	uint8_t s;
	do {
		s = status();
		if (s == ETHERNETCLIENT_W5100_CLOSED) {
			break; // exit the loop
		}
		usleep(1000);
		gettimeofday(&curTime, NULL);
	} while (((curTime.tv_sec - startTime.tv_sec) * 1000000) + (curTime.tv_usec - startTime.tv_usec) <
	         1000000);

	// if it hasn't closed, close it forcefully
	if (s != ETHERNETCLIENT_W5100_CLOSED) {
		close(_sock);
	}
	_sock = -1;
}

uint8_t EthernetClient::status()
{
	if (_sock == -1) {
		return ETHERNETCLIENT_W5100_CLOSED;
	}

	struct tcp_info tcp_info;
	int tcp_info_length = sizeof(tcp_info);

	if ( getsockopt( _sock, SOL_TCP, TCP_INFO, (void *)&tcp_info,
	                 (socklen_t *)&tcp_info_length ) == 0 ) {
		switch (tcp_info.tcpi_state) {
		case TCP_ESTABLISHED:
			return ETHERNETCLIENT_W5100_ESTABLISHED;
		case TCP_SYN_SENT:
			return ETHERNETCLIENT_W5100_SYNSENT;
		case TCP_SYN_RECV:
			return ETHERNETCLIENT_W5100_SYNRECV;
		case TCP_FIN_WAIT1:
		case TCP_FIN_WAIT2:
			return ETHERNETCLIENT_W5100_FIN_WAIT;
		case TCP_TIME_WAIT:
			return TCP_TIME_WAIT;
		case TCP_CLOSE:
			return ETHERNETCLIENT_W5100_CLOSED;
		case TCP_CLOSE_WAIT:
			return ETHERNETCLIENT_W5100_CLOSING;
		case TCP_LAST_ACK:
			return ETHERNETCLIENT_W5100_LAST_ACK;
		case TCP_LISTEN:
			return ETHERNETCLIENT_W5100_LISTEN;
		case TCP_CLOSING:
			return ETHERNETCLIENT_W5100_CLOSING;
		}
	}

	return ETHERNETCLIENT_W5100_CLOSED;
}

uint8_t EthernetClient::connected()
{
	if (_sock == -1) {
		return 0;
	}

	if (peek() < 0) {
		if (errno == EAGAIN) {
			return 1;
		}
		return 0;
	}
	return 1;
}

int EthernetClient::getSocketNumber()
{
	return _sock;
}

// the next function allows us to use the client returned by
// EthernetServer::available() as the condition in an if-statement.

EthernetClient::operator bool()
{
	return _sock != -1;
}

bool EthernetClient::operator==(const EthernetClient& rhs)
{
	return _sock == rhs._sock && _sock != -1 && rhs._sock != -1;
}
