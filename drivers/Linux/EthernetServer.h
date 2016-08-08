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

#ifndef ethernetserver_h
#define ethernetserver_h

#include <list>
#include <vector>
#include "Server.h"
#include "EthernetClient.h"

#ifndef ETHERNETSERVER_BACKLOG
	#define ETHERNETSERVER_BACKLOG 10
#endif

// debug 
#if defined(ETHERNETSERVER_VERBOSE)
	#define ETHERNETSERVER_DEBUG(x,...) debug(x, ##__VA_ARGS__)
#else
	#define ETHERNETSERVER_DEBUG(x,...)
#endif

class EthernetClient;

class EthernetServer : public Server {

private:
	uint16_t port;
	std::list<int> new_clients;
	std::vector<int> clients;

public:
	EthernetServer(uint16_t);
	virtual void begin();
	void begin(IPAddress);
	void beginPacket(IPAddress, uint16_t);
	int parsePacket();
	bool hasClient();
	EthernetClient available();
	virtual size_t write(uint8_t);
	virtual size_t write(const uint8_t *, size_t);
	size_t write(const char *str);
	size_t write(const char *buffer, size_t size);
};

#endif
