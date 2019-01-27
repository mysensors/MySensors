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

#ifndef MyTransportHAL_h
#define MyTransportHAL_h

#define INVALID_SNR         ((int16_t)-256)	//!< INVALID_SNR
#define INVALID_RSSI        ((int16_t)-256)	//!< INVALID_RSSI
#define INVALID_PERCENT     ((int16_t)-100)	//!< INVALID_PERCENT
#define INVALID_LEVEL       ((int16_t)-256)	//!< INVALID_LEVEL

#if defined(MY_RX_MESSAGE_BUFFER_FEATURE)
#if defined(MY_RADIO_NRF5_ESB)
#error Receive message buffering not supported for NRF5 radio! Please define MY_NRF5_RX_BUFFER_SIZE
#endif
#if defined(MY_RADIO_RFM69)
#error Receive message buffering not supported for RFM69!
#endif
#if defined(MY_RADIO_RFM95)
#error Receive message buffering not supported for RFM95!
#endif
#if defined(MY_RS485)
#error Receive message buffering not supported for RS485!
#endif
#elif defined(MY_RX_MESSAGE_BUFFER_SIZE)
#error Receive message buffering requires message buffering feature enabled!
#endif

/**
* @brief Signal report selector
*/
typedef enum {
	SR_RX_RSSI,            //!< SR_RX_RSSI
	SR_TX_RSSI,            //!< SR_TX_RSSI
	SR_RX_SNR,             //!< SR_RX_SNR
	SR_TX_SNR,             //!< SR_TX_SNR
	SR_TX_POWER_LEVEL,     //!< SR_TX_POWER_LEVEL
	SR_TX_POWER_PERCENT,   //!< SR_TX_POWER_PERCENT
	SR_UPLINK_QUALITY,     //!< SR_UPLINK_QUALITY
	SR_NOT_DEFINED         //!< SR_NOT_DEFINED
} signalReport_t;


/**
* @brief Initialize transport HW
* @return true if initialization successful
*/
bool transportInit(void);
/**
* @brief Set node address
*/
void transportSetAddress(const uint8_t address);
/**
* @brief Retrieve node address
*/
uint8_t transportGetAddress(void) __attribute__((unused));
/**
* @brief Send message
* @param to recipient
* @param data message to be sent
* @param len length of message (header + payload)
* @param noACK do not wait for ACK
* @return true if message sent successfully
*/
bool transportSend(const uint8_t to, const void *data, const uint8_t len,
                   const bool noACK = false);
/**
* @brief Verify if RX FIFO has pending messages
* @return true if message available in RX FIFO
*/
bool transportAvailable(void);
/**
* @brief Sanity check for transport: is transport HW still responsive?
* @return true if transport HW is ok
*/
bool transportSanityCheck(void);
/**
* @brief Receive message from FIFO
* @return length of received message (header + payload)
*/
uint8_t transportReceive(void *data);
/**
* @brief Power down transport HW (if corresponding MY_XYZ_POWER_PIN defined)
*/
void transportPowerDown(void);
/**
* @brief Power up transport HW (if corresponding MY_XYZ_POWER_PIN defined)
*/
void transportPowerUp(void);
/**
* @brief Set transport HW to sleep (no power down)
*/
void transportSleep(void);
/**
* @brief Set transport HW to standby
*/
void transportStandBy(void);
/**
* @brief transportGetSendingRSSI
* @return RSSI of outgoing message (via ACK packet)
*/
int16_t transportGetSendingRSSI(void);
/**
* @brief transportGetReceivingRSSI
* @return RSSI of incoming message
*/
int16_t transportGetReceivingRSSI(void);
/**
* @brief transportGetSendingSNR
* @return SNR of outgoing message (via ACK packet)
*/
int16_t transportGetSendingSNR(void);
/**
* @brief transportGetReceivingSNR
* @return SNR of incoming message
*/
int16_t transportGetReceivingSNR(void);
/**
* @brief transportGetTxPowerPercent
* @return TX power level in percent
*/
int16_t transportGetTxPowerPercent(void);
/**
* @brief transportSetTxPowerPercent
* @param powerPercent power level in percent
* @return True if power level set
*/
bool transportSetTxPowerPercent(const uint8_t powerPercent) __attribute__((unused));
/**
* @brief transportGetTxPowerLevel
* @return TX power in dBm
*/
int16_t transportGetTxPowerLevel(void);

#endif // MyTransportHAL_h
