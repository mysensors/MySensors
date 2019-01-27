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

/**
 * @file MyOTALogging.h
 *
 * @brief API declaration for MyOTALogging
 * @defgroup MyOTALogginggrp MyOTALogging
 * @ingroup internals
 * @{
 *
 * @brief Enables ending and receiving debug messages over the air.
 */
#ifndef MyOTALogging_h
#define MyOTALogging_h

#include "MySensorsCore.h"
#include "MyTransport.h"

/**
 * @brief Send a log message to a node
 *
 * If MY_OTA_LOG_RECEIVER_FEATURE is enabled on the destination node, the given
 * message is printed to the serial port. You have to define MY_OTA_LOG_SENDER_FEATURE
 * or MY_DEBUG_OTA to enable the OTALog() function.
 *
 * Output format of each line:
 * Node ID;CHILD_NODE_ID;C_INTERNAL;I_LOG_MESSAGE;hwMillis() MESSAGE
 *
 * You will see the hwMillis() of the receiving node. After each \n character, a
 * new debug message line starts. Incomplete messages are ending with '...'
 *
 * @param logNode    Destination node ID
 * @param enableAck  Enable or disable ACK flag
 * @param fmt        printf format string
 * @param ...        arguments
 */
void OTALog(uint8_t logNode, bool enableAck, const char *fmt, ... );

/**
 * @brief Handles output of OTA log or debug messages
 *
 * This function is used by MyTransport.cpp
 *
 * Output format of each line:
 * Node ID;CHILD_NODE_ID;C_INTERNAL;I_LOG_MESSAGE;hwMillis() MESSAGE
 *
 * You will see the hwMillis() of the receiving node. After each \n character, a
 * new debug message line starts. Incomplete messages are ending with '...'
 *
 * @param message Message buffer to use.
 */
inline void OTALogPrint(const MyMessage &message);

#endif /* MyOTALogging_h */

/** @}*/
