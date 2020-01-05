/*
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Tomas Hozza <thozza@gmail.com>
 * Copyright (C) 2015  Tomas Hozza
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
* @file MyGatewayTransport.h
*
* @defgroup MyGatewayTransportgrp MyGatewayTransport
* @ingroup internals
* @{
*
* Gateway transport-related log messages, format: [!]SYSTEM:[SUB SYSTEM:]MESSAGE
* - [!] Exclamation mark is prepended in case of error
* - SYSTEM:
*  - <b>GWT</b>: messages emitted by MyGatewayTransport
* - SUB SYSTEMS:
*  - GWT:<b>TIN</b>		from @ref gatewayTransportInit()
*  - GWT:<b>TPS</b>		from @ref gatewayTransportSend()
*  - GWT:<b>IMQ</b>		from incomingMQTT()
*  - GWT:<b>RMQ</b>		from reconnectMQTT()
*  - GWT:<b>TPC</b>		from gatewayTransportConnect()
*  - GWT:<b>RFC</b>		from _readFromClient()
*  - GWT:<b>TSA</b>		from @ref gatewayTransportAvailable()
*  - GWT:<b>TRC</b>		from @ref gatewayTransportReceive()
*
* Gateway transport debug log messages :
*
* |E| SYS | SUB   | Message                   | Comment
* |-|-----|-------|---------------------------|---------------------------------------------------------------------
* | | GWT | TIN   | CONNECTING...             | Connecting to router
* | | GWT | TIN   | IP=%%s                    | IP address [%%s] obtained
* |!| GWT | TIN   | DHCP FAIL                 | DHCP request failed
* | | GWT | TIN   | ETH OK                    | Connected to network
* |!| GWT | TIN   | ETH FAIL                  | Connection failed
* | | GWT | TPS   | TOPIC=%%s,MSG SENT        | MQTT message sent on topic [%%s]
* | | GWT | TPS   | ETH OK                    | Connected to network
* |!| GWT | TPS   | ETH FAIL                  | Connection failed
* | | GWT | IMQ   | TOPIC=%%s,MSG RECEIVE     | MQTT message received on topic [%%s]
* | | GWT | RMQ   | CONNECTING...             | Connecting to MQTT broker
* | | GWT | RMQ   | OK                        | Connected to MQTT broker
* |!| GWT | RMQ   | FAIL                      | Connection to MQTT broker failed
* | | GWT | TPC   | CONNECTING...             | Obtaining IP address
* | | GWT | TPC   | IP=%%s                    | IP address [%%s] obtained
* |!| GWT | TPC   | DHCP FAIL                 | DHCP request failed
* | | GWT | RFC   | C=%%d,MSG=%%s             | Received message [%%s] from client [%%d]
* |!| GWT | RFC   | C=%%d,MSG TOO LONG        | Received message from client [%%d] too long
* | | GWT | TSA   | UDP MSG=%%s               | Received UDP message [%%s]
* | | GWT | TSA   | ETH OK                    | Connected to network
* |!| GWT | TSA   | ETH FAIL                  | Connection failed
* | | GWT | TSA   | C=%d,DISCONNECTED         | Client [%%d] disconnected
* | | GWT | TSA   | C=%d,CONNECTED            | Client [%%d] connected
* |!| GWT | TSA   | NO FREE SLOT              | No free slot for client
* |!| GWT | TRC   | IP RENEW FAIL             | IP renewal failed
*
* @brief API declaration for MyGatewayTransport
*
*/

#ifndef MyGatewayTransport_h
#define MyGatewayTransport_h

#include "MyProtocol.h"
#include "MySensorsCore.h"

#define MSG_GW_STARTUP_COMPLETE "Gateway startup complete."		//!< Gateway startup message

#if defined(MY_DEBUG_VERBOSE_GATEWAY)
#define GATEWAY_DEBUG(x,...)	DEBUG_OUTPUT(x, ##__VA_ARGS__)	//!< debug output
#else
#define GATEWAY_DEBUG(x,...)									//!< debug NULL
#endif

/**
 * @brief Process gateway-related messages
 */
void gatewayTransportProcess(void);

/**
 * @brief Initialize gateway transport driver
 * @return true if transport initialized
 */
bool gatewayTransportInit(void);

/**
 * @brief Send message to controller
 * @param message to send
 * @return true if message delivered
 */
bool gatewayTransportSend(MyMessage &message);

/**
 * @brief Check if a new message is available from controller
 * @return true if message available
 */
bool gatewayTransportAvailable(void);

/**
 * @brief Pick up last message received from controller
 * @return message
 */
MyMessage& gatewayTransportReceive(void);

#endif /* MyGatewayTransportEthernet_h */

/** @}*/

