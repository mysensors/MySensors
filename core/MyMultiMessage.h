/*
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2022 Sensnology AB
 * Full contributor list: https://github.com/mysensors/MySensors/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * Multi message feature added by Constantin Petra <constantin.petra@gmail.com>
 * Copyright (C) 2022 Constantin Petra
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

/**
 * @file MyMultiMessage.h
 *
 * @brief API and type declarations for MySensors block object
 * @defgroup MyMultiMessage Multi-message support
 * @ingroup publics
 * @{
 *
 * @brief Block object that enables sending multiple sensor data in one message,
 *        including battery information.
 * @section MyBlobExample MyMultiMessage usage example
 *
 * <b>USE_BLOB</b> is a local define, only for emphasizing the changes needed
 * for using the blob feature.
 *
 * @code
 *
 * #define USE_BLOB
 *
 * // Initialize general message
 * #ifdef USE_BLOB
 * MyMessage msgBlob(0, V_MULTI_MESSAGE);
 * #else
 * MyMessage msgTEMPERATURE(CHILD_ID_TEMPERATURE, V_TEMP);
 * MyMessage msgHUMIDITY(CHILD_ID_HUMIDITY, V_HUM);
 * #endif
 * @endcode
 *
 * The code below shows a simple humidity/temperature/battery sensor sending everything in one
 * message:
 * @code
 *
 * #ifdef USE_BLOB
 *   MyMultiMessage blob(&msgBlob);
 *   blob.set(V_TEMP, CHILD_ID_TEMPERATURE , temperature, 1);
 *   blob.set(V_HUM, CHILD_ID_HUMIDITY, humidity, 1);
 *   blob.setBattery(batt_level);
 *   send(msgBlob);
 * #else
 *   send(msgTEMPERATURE.set(temperature, 1));
 *   wait(10); // the wait() is needed at least for rfm69, otherwise it takes a long time.
 *   send(msgHUMIDITY.set(humidity, 1));
 *   wait(10);
 * #endif
 * @endcode
 *
 * Fitting more messages may become more complex, though:
 * @todo  Add logic in MyMultiMessage to handle sending itself.
 * @code
 *
 * MyMultiMessage blob(&msgBlob);
 * if (!blob.set(V_TEMP, 0 , temperature, 1)) {
 *     send(msgBlob);
 *     wait(10);
 *     msgBlob.reset();
 *     blob.set(V_TEMP, 0 , temperature, 1)
 * }
 * if (!blob.setBattery(batt_level)) {
 *     send(msgBlob);
 *     wait(10);
 *     msgBlob.reset();
 *     blob.setBattery(batt_level)
 * }
 *
 * @endcode
 */

#ifndef MYMULTIMESSAGE_H
#define MYMULTIMESSAGE_H
#include "MyMessage.h"

/**
 * @brief A collection of APIs that convert a MyMessage object to a message group.
 */
class MyMultiMessage
{
private:
	// A pointer to the "real" message
	MyMessage *_msg;

	// Current offset inside the message
	size_t _offset;

	// Common setup, returns a pointer to the next area that can be used, NULL if no more room
	void *common(uint8_t type, uint8_t sensor, uint8_t ptype, uint8_t size, uint8_t cmd);

public:
	/**
	 * Constructor
	 * @param msg A MyMessage pointer, the type should be V_MULTI_MESSAGE
	 */
	explicit MyMultiMessage(MyMessage *msg);

	/**
	 * @brief Set payload to unsigned 8-bit integer value
	 * @param messageType
	 * @param sensorId
	 * @param value (0 to 255)
	 * @return false if the message won't fit, true if the message was added successfuly
	 */
	bool set(uint8_t messageType, uint8_t sensorId, uint8_t value);

	/**
	 * @brief Set payload to unsigned 16-bit integer value
	 * @param messageType
	 * @param sensorId
	 * @param value (0 to 65535)
	 * @return false if the message won't fit, true if the message was added successfuly
	 */
	bool set(uint8_t messageType, uint8_t sensorId, uint16_t value);

	/**
	 * @brief Set payload to signed 16-bit integer value
	 * @param messageType
	 * @param sensorId
	 * @param value (–32768 to 32767)
	 * @return false if the message won't fit, true if the message was added successfuly
	 */
	bool set(uint8_t messageType, uint8_t sensorId, int16_t value);

	/**
	 * @brief Set payload to unsigned 32-bit integer value
	 * @param messageType
	 * @param sensorId
	 * @param value (0 to 4294967295)
	 * @return false if the message won't fit, true if the message was added successfuly
	 */

	bool set(uint8_t messageType, uint8_t sensorId, uint32_t value);

	/**
	 * @brief Set payload to signed 32-bit integer value
	 * @param messageType
	 * @param sensorId
	 * @param value (–2147483648 to 2147483647)
	 * @return false if the message won't fit, true if the message was added successfuly
	 */
	bool set(uint8_t messageType, uint8_t sensorId, int32_t value);

	/**
	 * @brief Set payload to decimal number
	 * @param messageType
	 * @param sensorId
	 * @param value float
	 * @param decimals number of decimals to include
	 * @return false if the message won't fit, true if the message was added successfuly
	 */
	bool set(uint8_t messageType, uint8_t sensorId, float value, uint8_t decimals);

	/**
	 * @brief Reset the message contents, so that in can be reused after send()
	 */
	void reset();

	/**
	 * @brief Set battery level
	 * @param value battery level
	 */
	bool setBattery(uint8_t value);

	/**
	 * @brief get next message from the blob
	 * @param m MyMessage reference
	 */
	bool getNext(MyMessage &m);

	~MyMultiMessage();

};
#endif
/** @}*/