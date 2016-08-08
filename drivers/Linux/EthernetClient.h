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
* Based on Arduino ethernet library, Copyright (c) 2010 Arduino LLC. All right reserved.
*/

#ifndef ethernetclient_h
#define ethernetclient_h

#include "Client.h"
#include "IPAddress.h"

// State codes from W5100 library
#define ETHERNETCLIENT_W5100_CLOSED 0x00
#define ETHERNETCLIENT_W5100_LISTEN 0x14
#define ETHERNETCLIENT_W5100_SYNSENT 0x15
#define ETHERNETCLIENT_W5100_SYNRECV 0x16
#define ETHERNETCLIENT_W5100_ESTABLISHED 0x17
#define ETHERNETCLIENT_W5100_FIN_WAIT 0x18
#define ETHERNETCLIENT_W5100_CLOSING 0x1A
#define ETHERNETCLIENT_W5100_TIME_WAIT 0x1B
#define ETHERNETCLIENT_W5100_CLOSE_WAIT 0x1C
#define ETHERNETCLIENT_W5100_LAST_ACK 0x1D

// debug 
#if defined(ETHERNETCLIENT_VERBOSE)
	#define ETHERNETCLIENT_DEBUG(x,...) debug(x, ##__VA_ARGS__)
#else
	#define ETHERNETCLIENT_DEBUG(x,...)
#endif

class EthernetClient : public Client {

public:
	EthernetClient();
	EthernetClient(int sock);

	uint8_t status();
	virtual int connect(IPAddress ip, uint16_t port);
	virtual int connect(const char *host, uint16_t port);
	virtual size_t write(uint8_t);
	virtual size_t write(const uint8_t *buf, size_t size);
	size_t write(const char *str);
	size_t write(const char *buffer, size_t size);
	virtual int available();
	virtual int read();
	virtual int read(uint8_t *buf, size_t size);
	virtual int peek();
	virtual void flush();
	virtual void stop();
	virtual uint8_t connected();
	virtual operator bool();
	virtual bool operator==(const bool value) { return bool() == value; }
	virtual bool operator!=(const bool value) { return bool() != value; }
	virtual bool operator==(const EthernetClient&);
	virtual bool operator!=(const EthernetClient& rhs) { return !this->operator==(rhs); };
	uint8_t getSocketNumber();

	friend class EthernetServer;

private:
	int _sock;
};

#endif
