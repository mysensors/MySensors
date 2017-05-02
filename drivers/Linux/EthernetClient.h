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

#ifndef EthernetClient_h
#define EthernetClient_h

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

/**
 * EthernetClient class
 */
class EthernetClient : public Client
{

public:
	/**
	 * @brief EthernetClient constructor.
	 */
	EthernetClient();
	/**
	 * @brief EthernetClient constructor.
	 *
	 * @param sock Network socket.
	 */
	explicit EthernetClient(int sock);
	/**
	 * @brief Initiate a connection with host:port.
	 *
	 * @param host host name to resolve or a stringified dotted IP address.
	 * @param port to connect to.
	 * @return 1 if SUCCESS or -1 if FAILURE.
	 */
	virtual int connect(const char *host, uint16_t port);
	/**
	 * @brief Initiate a connection with ip:port.
	 *
	 * @param ip to connect to.
	 * @param port to connect to.
	 * @return 1 if SUCCESS or -1 if FAILURE.
	 */
	virtual int connect(IPAddress ip, uint16_t port);
	/**
	 * @brief Write a byte.
	 *
	 * @param b byte to write.
	 * @return 0 if FAILURE or 1 if SUCCESS.
	 */
	virtual size_t write(uint8_t b);
	/**
	 * @brief Write at most 'size' bytes.
	 *
	 * @param buf Buffer to read from.
	 * @param size of the buffer.
	 * @return 0 if FAILURE or the number of bytes sent.
	 */
	virtual size_t write(const uint8_t *buf, size_t size);
	/**
	 * @brief Write a null-terminated string.
	 *
	 * @param str String to write.
	 * @return 0 if FAILURE or number of characters sent.
	 */
	size_t write(const char *str);
	/**
	 * @brief Write at most 'size' characters.
	 *
	 * @param buffer to read from.
	 * @param size of the buffer.
	 * @return 0 if FAILURE or the number of characters sent.
	 */
	size_t write(const char *buffer, size_t size);
	/**
	 * @brief Checks if new data is available.
	 *
	 * @return number of bytes available.
	 */
	virtual int available();
	/**
	 * @brief Read a byte.
	 *
	 * @return 0 if FAILURE or 1 if SUCCESS.
	 * @note This function will block (until data becomes available or timeout is reached).
	 */
	virtual int read();
	/**
	 * @brief Read a number of bytes and store in a buffer.
	 *
	 * @return -1 if FAILURE or number of read bytes.
	 * @note This function will block (until data becomes available or timeout is reached).
	 */
	virtual int read(uint8_t *buf, size_t size);
	/**
	 * @brief Check if new data are available.
	 *
	 * @return 0 if no data, else number of bytes available.
	 */
	virtual int peek();
	/**
	 * @brief Flushes the network buffers.
	 *
	 * Empty function due to the way TCP works.
	 */
	virtual void flush();
	/**
	 * @brief Close the connection gracefully.
	 *
	 * Send a FIN and wait 1s for a response. If no response close it forcefully.
	 */
	virtual void stop();
	/**
	 * @brief Connection status.
	 *
	 * @return state according to W5100 library codes.
	 */
	uint8_t status();
	/**
	 * @brief Checks whether the socket is alive.
	 *
	 * @return 0 if disconnected or 1 if connected.
	 */
	virtual uint8_t connected();
	/**
	 * @brief Get the internal socket file descriptor.
	 *
	 * @return an integer, that is the socket number.
	 */
	int getSocketNumber();
	/**
	 * @brief Overloaded cast operators.
	 *
	 * Allow EthernetClient objects to be used where a bool is expected.
	 */
	virtual operator bool();
	/**
	 * @brief Overloaded cast operators.
	 *
	 */
	virtual bool operator==(const bool value)
	{
		return bool() == value;
	}
	/**
	 * @brief Overloaded cast operators.
	 *
	 */
	virtual bool operator!=(const bool value)
	{
		return bool() != value;
	}
	/**
	 * @brief Overloaded cast operators.
	 *
	 */
	virtual bool operator==(const EthernetClient& rhs);
	/**
	 * @brief Overloaded cast operators.
	 *
	 */
	virtual bool operator!=(const EthernetClient& rhs)
	{
		return !this->operator==(rhs);
	};

	friend class EthernetServer;

private:
	int _sock; //!< @brief Network socket.
};

#endif
