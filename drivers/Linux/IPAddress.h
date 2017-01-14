/*
 * IPAddress.h - Base class that provides IPAddress
 * Copyright (c) 2011 Adrian McEwen.  All right reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 *
 * Modified by Marcelo Aquino <marceloaqno@gmail.org> for MySensors use
 */

#ifndef IPAddress_h
#define IPAddress_h

#include <stdint.h>
#include <string>

/**
 * @brief A class to make it easier to handle and pass around IP addresses
 */
class IPAddress
{
private:
	union {
		uint8_t bytes[4]; //!< IPv4 address as an array
		uint32_t dword; //!< IPv4 address in 32 bits format
	} _address;

	/**
	* @brief Access the raw byte array containing the address.
	*
	* Because this returns a pointer to the internal structure rather than a copy of the address
	* this function should only be used when you know that the usage of the returned uint8_t* will
	* be transient and not stored.
	*
	* @return pointer to the internal structure.
	*/
	uint8_t* raw_address()
	{
		return _address.bytes;
	}

public:
	/**
	 * @brief IPAddress constructor.
	 */
	IPAddress();
	/**
	 * @brief IPAddress constructor.
	 *
	 * @param first_octet first octet of the IPv4 address.
	 * @param second_octet second octet of the IPv4 address.
	 * @param third_octet third octet of the IPv4 address.
	 * @param fourth_octet fourth octet of the IPv4 address.
	 */
	IPAddress(uint8_t first_octet, uint8_t second_octet, uint8_t third_octet, uint8_t fourth_octet);
	/**
	 * @brief IPAddress constructor.
	 *
	 * @param address to be set from a 32 bits integer.
	 */
	explicit IPAddress(uint32_t address);
	/**
	 * @brief IPAddress constructor.
	 *
	 * @param address to be set from a byte array.
	 */
	explicit IPAddress(const uint8_t *address);
	/**
	 * @brief Set the IP from a array of characters.
	 *
	 * @param address to be set.
	 */
	bool fromString(const char *address);
	/**
	 * @brief Set the IP from a string class type.
	 *
	 * @param address to be set.
	 */
	bool fromString(const std::string &address)
	{
		return fromString(address.c_str());
	}
	/**
	 * @brief Overloaded cast operator
	 *
	 * Allow IPAddress objects to be used where a pointer to a four-byte uint8_t array is expected
	 */
	operator uint32_t() const
	{
		return _address.dword;
	}
	/**
	 * @brief Overloaded cast operator
	 *
	 */
	bool operator==(const IPAddress& addr) const
	{
		return _address.dword == addr._address.dword;
	}
	/**
	 * @brief Overloaded cast operator
	 *
	 */
	bool operator==(uint32_t addr) const
	{
		return _address.dword == addr;
	}
	/**
	 * @brief Overloaded cast operator
	 *
	 */
	bool operator==(const uint8_t* addr) const;
	/**
	 * @brief Overloaded index operator.
	 *
	 * Allow getting and setting individual octets of the address.
	 *
	 */
	uint8_t operator[](int index) const
	{
		return _address.bytes[index];
	}
	/**
	 * @brief Overloaded index operator
	 *
	 */
	uint8_t& operator[](int index)
	{
		return _address.bytes[index];
	}
	/**
	 * @brief Overloaded copy operators.
	 *
	 * Allow initialisation of IPAddress objects from byte array.
	 */
	IPAddress& operator=(const uint8_t *address);
	/**
	 * @brief Overloaded copy operator.
	 *
	 * Allow initialisation of IPAddress objects from a 32 bits integer.
	 */
	IPAddress& operator=(uint32_t address);
	/**
	 * @brief Convert the IP address to a string.
	 *
	 * @return A stringified IP address
	 */
	std::string toString();

	friend class Client;
};

#endif
