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
 * CAN bus transport added by Adam Słowik <slowik.adam@gmail.com>
 * Copyright (C) 2022 Adam Słowik
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

/**
* @file MyTransportCAN.h
*
* @defgroup CANgrp CAN
* @ingroup internals
* @{
*
* CAN driver-related log messages, format: [!]SYSTEM:[SUB SYSTEM:]MESSAGE
* - [!] Exclamation mark is prepended in case of error
*
* |E| SYS | SUB  | Message                         | Comment
* |-|-----|------|---------------------------------|---------------------------------------------------------------------
* | | CAN | INIT | CS=%%d,INT=%%d,SPE=%%d",CLK=%%d | Initialise CAN MCP2515 module radio, chip select (CS),
  | |     |      |                                 | interrupt pin (INT), CAN speed (SPE), CAN clock (CLK)
* |!| CAN | RCV  | SLOT=%%d message dropped.       | clean buffer (SLOT) to make space for new message.
* | | CAN | RCV  | LCK=%%d,ADDR=%%d,PACK_ID=%%d,   | Print current slot info. Is locked (LCK), send from (ADDR),
* | |     |      | RCV_PARTS=%%d                   | packet id (PACK_ID), number of received parts (RCV_PARTS)
* | | CAN | SND  | CANH=%%d,ID=%%d,TOTAL=%%d,      | prints raw frame header (CANH), and decoded data from header:
* | |     |      | CURR=%%d,TO=%%d,FROM=%%d        | message id (ID), total number of frames to be send (TOTAL),
* | |     |      |                                 | current frame number (CURR), destination address (TO),
* | |     |      |                                 | source address (FROM)
* | | CAN | SND  | LN=%%d,NOF=%%d                  | prints total data length to be send (LN)
* | |     |      |                                 | and total number of frames (NOF)
* | | CAN | SND  | LN=%%d,DTA0=%%d,DTA1=%%d,       | prints body length of message (LN) and data itself (DTA0...DTA7)
* | |     |      | DTA2=%%d,DTA3=%%d,DTA4=%%d,     |
* | |     |      | DTA5=%%d,DTA6=%%d,DTA7=%%d      |
* | | CAN | SND  | OK                              | frame send
* |!| CAN | SND  | FAIL                            | frame not send
* | | CAN | RCV  | CANH=%%d,ID=%%d,TOTAL=%%d,      | received header (CANH), and decoded data from header:
* | |     |      | CURR=%%d,TO=%%d,FROM=%%d        | message id (ID), total number of frames to be send (TOTAL),
* | |     |      |                                 | current frame number (CURR), destination address (TO),
* | | CAN | RCV  | LN=%%d,DTA0=%%d,DTA1=%%d,       | prints body length of received message (LN)
* | |     |      | DTA2=%%d,DTA3=%%d,DTA4=%%d,     | and received data itself (DTA0...DTA7)
* | |     |      | DTA5=%%d,DTA6=%%d,DTA7=%%d      |
* | | CAN | RCV  | SLOT=%%d,PART=%%d               | frame stored in buffer under (SLOT). Number of received part (PART)
* | | CAN | RCV  | SLOT=%%d complete               | message in buffer under (SLOT) is complete.
*
*/
bool _initFilters();
bool transportInit(void);

void _cleanSlot(uint8_t slot);

uint8_t _findCanPacketSlot();

uint8_t _findCanPacketSlot(long unsigned int from, long unsigned int currentPart,
                           long unsigned int messageId);

bool transportSend(const uint8_t to, const void* data, const uint8_t len, const bool noACK);

bool transportDataAvailable(void);

uint8_t transportReceive(void* data);

void transportSetAddress(const uint8_t address);

uint8_t transportGetAddress(void);

bool transportSanityCheck(void);

void transportPowerDown(void);

void transportPowerUp(void);

void transportSleep(void);

void transportStandBy(void);

int16_t transportGetSendingRSSI(void);

int16_t transportGetReceivingRSSI(void);

int16_t transportGetSendingSNR(void);

int16_t transportGetReceivingSNR(void);

int16_t transportGetTxPowerPercent(void);

int16_t transportGetTxPowerLevel(void);

bool transportSetTxPowerPercent(const uint8_t powerPercent);
