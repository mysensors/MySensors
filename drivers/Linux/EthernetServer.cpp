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

#include <sys/socket.h>
#include <cstring>
#include <sys/socket.h>
#include <pthread.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "EthernetServer.h"

struct server_vars {
	uint16_t port;
	struct IPAddress address;
	std::list<int> *new_clients;
	std::vector<int> *clients;
};

static pthread_mutex_t new_clients_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void *incoming_connections(void *);

EthernetServer::EthernetServer(uint16_t port) : port(port)
{
	clients.reserve(MY_GATEWAY_MAX_CLIENTS);
}

void EthernetServer::begin()
{
	begin(IPAddress(0,0,0,0));
}

void EthernetServer::begin(IPAddress address)
{
	pthread_t thread_id;
	pthread_attr_t detached_attr;

	struct server_vars *vars = new struct server_vars;
	vars->port = port;
	vars->address = address;
	vars->new_clients = &new_clients;
	vars->clients = &clients;

	pthread_attr_init(&detached_attr);
	pthread_attr_setdetachstate(&detached_attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&thread_id, &detached_attr, &incoming_connections, vars);
}

void EthernetServer::beginPacket(IPAddress address, uint16_t port)
{
	//TODO: UDP
	(void)address;
	(void)port;
}

int EthernetServer::parsePacket()
{
	//TODO: UDP

	return 0;
}

bool EthernetServer::hasClient()
{
	bool empty;

	pthread_mutex_lock(&new_clients_mutex);
	empty = new_clients.empty();
	pthread_mutex_unlock(&new_clients_mutex);

	return !empty;
}

EthernetClient EthernetServer::available()
{
	int sock;

	pthread_mutex_lock(&new_clients_mutex);
	if (new_clients.empty()) {
		sock = -1;
	} else {
		sock = new_clients.front();
		new_clients.pop_front();
	}
	pthread_mutex_unlock(&new_clients_mutex);

	return EthernetClient(sock);
}

size_t EthernetServer::write(uint8_t b) 
{
	return write(&b, 1);
}

size_t EthernetServer::write(const uint8_t *buffer, size_t size) 
{
	size_t n = 0;
	size_t i = 0;

	pthread_mutex_lock(&clients_mutex);
	while (i < clients.size()) {
		EthernetClient client(clients[i]);
		if (client.connected()) {
			n += client.write(buffer, size);
			i++;
		} else {
			clients[i] = clients.back();
			clients.pop_back();
		}
	}
	pthread_mutex_unlock(&clients_mutex);

	return n;
}

size_t EthernetServer::write(const char *str) {
	if (str == NULL) return 0;
	return write((const uint8_t *)str, strlen(str));
}
size_t EthernetServer::write(const char *buffer, size_t size) {
	return write((const uint8_t *)buffer, size);
}

void *incoming_connections(void* thread_arg)
{
	struct server_vars *vars = (struct server_vars*) thread_arg;
	int sockfd, new_fd;
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_storage client_addr;
	socklen_t sin_size;
	int yes=1;
	int rv;
	char ipstr[INET_ADDRSTRLEN];
	char portstr[6];

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	sprintf(portstr, "%d", vars->port);
	if ((rv = getaddrinfo(vars->address.toString().c_str(), portstr, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return NULL;
	}

	// loop through all the results and bind to the first we can
	for (p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			perror("socket");
			continue;
		}

		if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
			perror("setsockopt");
			freeaddrinfo(servinfo);
			return NULL;
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("bind");
			continue;
		}

		break;
	}

	if (p == NULL)  {
		fprintf(stderr, "failed to bind\n");
		freeaddrinfo(servinfo);
		return NULL;
	}

	if (listen(sockfd, ETHERNETSERVER_BACKLOG) == -1) {
		perror("listen");
		freeaddrinfo(servinfo);
		return NULL;
	}

	freeaddrinfo(servinfo);

	struct sockaddr_in *ipv4 = (struct sockaddr_in *)p->ai_addr;
	void *addr = &(ipv4->sin_addr);
	inet_ntop(p->ai_family, addr, ipstr, sizeof ipstr);
	ETHERNETSERVER_DEBUG("Listening for connections on %s:%s\n", ipstr, portstr);

	while (1) {  // accept() loop
		sin_size = sizeof client_addr;
		new_fd = accept(sockfd, (struct sockaddr *)&client_addr, &sin_size);
		if (new_fd == -1) {
			perror("accept");
			continue;
		}

		void *addr = &(((struct sockaddr_in*)&client_addr)->sin_addr);
		inet_ntop(client_addr.ss_family, addr, ipstr, sizeof ipstr);
		ETHERNETSERVER_DEBUG("New connection from %s\n", ipstr);

		pthread_mutex_lock(&new_clients_mutex);
		pthread_mutex_lock(&clients_mutex);
		int connected_clients = vars->new_clients->size() + vars->clients->size();

		if (connected_clients == MY_GATEWAY_MAX_CLIENTS) {
			pthread_mutex_unlock(&clients_mutex);
			pthread_mutex_unlock(&new_clients_mutex);
			ETHERNETSERVER_DEBUG("Max number of ethernet clients reached.");
			close(new_fd);
			usleep(5000000);
		} else {
			vars->new_clients->push_back(new_fd);
			vars->clients->push_back(new_fd);
			pthread_mutex_unlock(&clients_mutex);
			pthread_mutex_unlock(&new_clients_mutex);
		}
	}

	delete vars;

	return NULL;
}
