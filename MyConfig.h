/*
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in RAM or EEPROM which keeps track of the
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
 */
/**
 * @file MyConfig.h
 * @ingroup MyConfigGrp
 *
 * @brief MySensors specific configuration flags.
 *
 * Set these in your sketch before including MySensors.h to customize the library to your needs.
 * If the sketch does not define these flags, they will get default values where applicable.
 */
#ifndef MyConfig_h
#define MyConfig_h
#include <stdint.h>

/**
 * @defgroup SerialDebugGrpPub Serial and debugging
 * @ingroup MyConfigGrp
 * @brief These options control serial and debugging features and functionalities in the library.
 * @{
 */
/**
 * @def MY_DEBUG
 * @brief Define MY_DEBUG to show debug prints.
 *
 * This option will add a lot to the size of the final sketch but is helpful to see what is actually
 * is happening during development.
 *
 * @note Values in parenthesis indicate default values which will be used if you have not defined
 * the flag in your sketch.
 */
//#define MY_DEBUG

/**
 * @def MY_SPECIAL_DEBUG
 * @brief Define MY_SPECIAL_DEBUG to enable support for I_DEBUG messages.
 *
 * I_DEBUG messages are sent from the controller to the node, which responds with the requested
 * data. The request can be one of the following:
 * - 'R': routing info (only repeaters): received msg XXYY (as stream), where XX is the node and YY
 *   the routing node
 * - 'V': CPU voltage
 * - 'F': CPU frequency
 * - 'M': free memory
 * - 'E': clear MySensors EEPROM area and reboot (i.e. "factory" reset)
 */
//#define MY_SPECIAL_DEBUG

/**
 * @def MY_DISABLED_SERIAL
 * @brief Define MY_DISABLED_SERIAL if you want to use the UART TX/RX pins as normal I/O pins.
 *
 * @note When defined, if you want to use the pins as a UART, you need to handle initialization and
 * configuration yourself.
 */
//#define MY_DISABLED_SERIAL

/**
 * @def MY_SPLASH_SCREEN_DISABLED
 * @ingroup memorysavings
 * @brief If defined, will disable the MySensors splash screen.
 *
 * @note This saves 120 bytes of flash.
 */
//#define MY_SPLASH_SCREEN_DISABLED

#ifdef MY_DISABLED_SERIAL
#undef MY_DEBUG
#endif

/**
 * @def MY_BAUD_RATE
 * @brief Serial output baud rate (debug prints and serial gateway speed).
 *
 * The baud rate configured here must match the baud rate at the "other" end.
 *
 * @warning Depending on your target device and clock speed, certain baud rates might not work well.
 */
#ifndef MY_BAUD_RATE
#define MY_BAUD_RATE (115200ul)
#endif

/**
 * @def MY_SERIAL_OUTPUT_SIZE
 * @brief Maximum characters for serial output.
 *
 * If you are running extreamely low on memory, reducing this size might just save your day.
 */
#ifndef MY_SERIAL_OUTPUT_SIZE
#define MY_SERIAL_OUTPUT_SIZE (120u)
#endif
/** @}*/ // End of SerialDebugGrpPub group

/**
 * @defgroup RadioSettingGrpPub Radio selection
 * @ingroup MyConfigGrp
 * @brief These options control what radio type to use and various radio specific customisations.
 * @{
 */

// Define or uncomment MY_OTA_USE_I2C_EEPROM below if you want I2C EEPROM instead
// of a SPI flash. Used EEPROM needs to be large enough, an 24(L)C256 will do as minimum.
// HW I2C assumed. This will exclude the SPI flash code.
// Note that you also need an updated DualOptiboot supporting I2C EEPROM!
//#define MY_OTA_USE_I2C_EEPROM

#ifdef MY_OTA_USE_I2C_EEPROM
// I2C address of EEPROM. Wire will shift this left, i.e. 0x50->0xA0
#ifndef MY_OTA_I2C_ADDR
#define MY_OTA_I2C_ADDR		0x50
#endif
#endif

/**
 * @defgroup RS485SettingGrpPub RS485
 * @ingroup RadioSettingGrpPub
 * @brief These options are specific to the RS485 wired transport.
 * @{
 */

/**
 * @def MY_RS485
 * @brief Define this to use the RS485 wired transport for sensor network communication.
 */
//#define MY_RS485

/**
 * @def MY_RS485_BAUD_RATE
 * @brief The RS485 BAUD rate.
 */
#ifndef MY_RS485_BAUD_RATE
#define MY_RS485_BAUD_RATE (9600)
#endif

/**
 * @def MY_RS485_MAX_MESSAGE_LENGTH
 * @brief The maximum message length used for RS485.
 */
#ifndef MY_RS485_MAX_MESSAGE_LENGTH
#define MY_RS485_MAX_MESSAGE_LENGTH (40)
#endif

/**
 * @def MY_RS485_DE_PIN
 * @brief RS485 driver enable pin.
 */
//#define MY_RS485_DE_PIN (2)

/**
 * @def MY_RS485_HWSERIAL
 * @brief Define this if RS485 is connected to a hardware serial port.
 */
//#define MY_RS485_HWSERIAL (Serial1)
/** @}*/ // End of RS485SettingGrpPub group

/**
 * @defgroup RF24SettingGrpPub RF24
 * @ingroup RadioSettingGrpPub
 * @brief These options are specific to the RF24 family of wireless transport modules.
 *
 * The following chips are supported by this driver:
 * | Vendor                   | Chip
 * |--------------------------|----------
 * | Nordic Semiconductor     | nRF24L01
 * |                          | nRF24L01+
 * | Beken Corporation        | BK2401
 * |                          | BK2421
 * |                          | BK2491
 * | Hope Microelectronics    | RFM70
 * |                          | RFM73
 * | Panchip Microelectronics | XN297
 * | Silicon Labs(?)          | SI24R1
 * @{
 */
// legacy
#ifdef MY_RADIO_NRF24
//MY_RADIO_NRF24 is deprecated
#define MY_RADIO_RF24
#endif

/**
 * @def MY_RADIO_RF24
 * @brief Define this to use a RF24-based radio transport for sensor network communication.
 */
//#define MY_RADIO_RF24

/**
 * @def MY_RF24_ENABLE_ENCRYPTION
 * @brief Define this to enable software based %AES encryption.
 *
 * All nodes and gateway must have this enabled, and all must be personalized with the same %AES
 * key.
 * @see @ref personalization
 *
 * @warning This driver always sets the initialization vector to 0 so encryption is weak.
 */
//#define MY_RF24_ENABLE_ENCRYPTION

/**
 * @def MY_DEBUG_VERBOSE_RF24
 * @brief Define this for verbose debug prints related to the RF24 driver.
 */
//#define MY_DEBUG_VERBOSE_RF24

/**
 * @def MY_RF24_SPI_SPEED
 * @brief Define this if you need to run the SPI clock at a different frequency than the default.
 *
 * Default nRF24L01+ SPI speed, 2MHz should be safe for nRF24L01+ clones.
 */
#ifndef MY_RF24_SPI_SPEED
#define MY_RF24_SPI_SPEED (2*1000000ul)
#endif

/**
 * @def MY_RF24_CE_PIN
 * @brief Define this to change the chip enable pin from the default.
 */
#ifndef MY_RF24_CE_PIN
#define MY_RF24_CE_PIN (DEFAULT_RF24_CE_PIN)
#endif

/**
 * @def MY_RF24_CS_PIN
 * @brief Define this to change the chip select pin from the default.
 */
#ifndef MY_RF24_CS_PIN
#define MY_RF24_CS_PIN (DEFAULT_RF24_CS_PIN)
#endif

/**
 * @def MY_RF24_IRQ_PIN
 * @brief Define this to use the IRQ pin of the RF24 module (optional).
 */
//#define MY_RF24_IRQ_PIN (2)

/**
 * @def MY_RF24_POWER_PIN
 * @brief Define this to use the NRF24 power pin (optional).
 */
//#define MY_RF24_POWER_PIN (3)

/**
 * @def MY_RX_MESSAGE_BUFFER_FEATURE
 * @brief This enables the receiving buffer feature.
 *
 * This feature is currently not supported for anything but RF24.
 * Require @ref MY_RF24_IRQ_PIN to be set.
 */
//#define MY_RX_MESSAGE_BUFFER_FEATURE

/**
 * @def MY_RX_MESSAGE_BUFFER_SIZE
 * @brief Define this to change the incoming message buffer size from the default.
 *
 * Require @ref MY_RX_MESSAGE_BUFFER_FEATURE to be set.
 */
#ifdef MY_RX_MESSAGE_BUFFER_FEATURE
#ifndef MY_RX_MESSAGE_BUFFER_SIZE
#define MY_RX_MESSAGE_BUFFER_SIZE (20)
#endif
#endif

/**
 * @def MY_RF24_PA_LEVEL
 * @brief Default RF24 PA level. Override in sketch if needed.
 *
 * - RF24_PA_LOW = -18dBm
 * - RF24_PA_MID = -12dBm
 * - RF24_PA_HIGH = -6dBm
 * - RF24_PA_MAX = 0dBm
 */
#ifndef MY_RF24_PA_LEVEL
#define MY_RF24_PA_LEVEL (RF24_PA_HIGH)
#endif

/**
 * @def MY_RF24_CHANNEL
 * @brief RF channel for the sensor net, 0-125.
 *
 * Frequencies: 2400 Mhz - 2525 Mhz
 *
 * Channels: 126
 * @see https://www.nordicsemi.com/eng/nordic/download_resource/8765/2/42877161/2726
 *
 * - 0 => 2400 Mhz (RF24 channel 1)
 * - 1 => 2401 Mhz (RF24 channel 2)
 * - 76 => 2476 Mhz (RF24 channel 77)
 * - 83 => 2483 Mhz (RF24 channel 84)
 * - 124 => 2524 Mhz (RF24 channel 125)
 * - 125 => 2525 Mhz (RF24 channel 126)
 *
 * In some countries there might be limitations, in Germany for example only the range
 * 2400,0 - 2483,5 Mhz is allowed.
 * @see http://www.bundesnetzagentur.de/SharedDocs/Downloads/DE/Sachgebiete/Telekommunikation/Unternehmen_Institutionen/Frequenzen/Allgemeinzuteilungen/2013_10_WLAN_2,4GHz_pdf.pdf
 */
#ifndef MY_RF24_CHANNEL
#define MY_RF24_CHANNEL (76)
#endif

/**
 * @def MY_RF24_DATARATE
 * @brief RF24 datarate.
 *
 * - RF24_250KBPS for 250kbs
 * - RF24_1MBPS for 1Mbps
 * - RF24_2MBPS for 2Mbps.
 *
 * @note nRF24L01, BK2401, BK2421, BK2491 and XN297 does not support RF24_250KBPS
 * @note BK2401 does not support RF24_2MBPS
 */
#ifndef MY_RF24_DATARATE
#define MY_RF24_DATARATE (RF24_250KBPS)
#endif

/**
 * @def MY_RF24_BASE_RADIO_ID
 * @brief RF24 radio network identifier.
 *
 * This acts as base value for sensor nodeId addresses. Change this (or channel) if you have more
 * than one sensor network.
 */
#ifndef MY_RF24_BASE_RADIO_ID
#define MY_RF24_BASE_RADIO_ID 0x00,0xFC,0xE1,0xA8,0xA8
#endif

/**
 * @def MY_RF24_ADDR_WIDTH
 * @brief RF24 base address width.
 */
#ifndef MY_RF24_ADDR_WIDTH
#define MY_RF24_ADDR_WIDTH (5)
#endif
/** @}*/ // End of RF24SettingGrpPub group

/**
 * @defgroup NRF5SettingGrpPub nRF5
 * @ingroup RadioSettingGrpPub
 * @brief These options are specific to the nRF5 (with Enhanced ShockBurst) family of wireless
 * transport modules.
 *
 * The nRF5 driver is OTA compatible with the RF24 driver.
 *
 * The following chips are supported by this driver:
 * - nRF51x22
 * - nRF52822
 *
 * @{
 */

/**
 * @def MY_NRF5_ESB_ENABLE_ENCRYPTION
 * @brief Define this to enable software based (RF24 compatible) %AES encryption.
 *
 * All nodes and gateway must have this enabled, and all must be personalized with the same %AES
 * key.
 * @see @ref personalization
 *
 * @warning This driver always sets the initialization vector to 0 so encryption is weak.
 */
//#define MY_NRF5_ESB_ENABLE_ENCRYPTION

/**
 * @def MY_DEBUG_VERBOSE_NRF5_ESB
 * @brief Define this for verbose debug prints related to the nRF5 driver.
 */
//#define MY_DEBUG_VERBOSE_NRF5_ESB

/**
 * @def MY_NRF5_ESB_PA_LEVEL
 * @brief Default nRF5 PA level. Override in sketch if needed.
 *
 * - NRF5_PA_MIN = -40dBm
 * - NRF5_PA_LOW = -16dBm
 * - NRF5_PA_HIGH = 0dBm
 * - NRF5_PA_MAX = 4dBm
 */
#ifndef MY_NRF5_ESB_PA_LEVEL
#define MY_NRF5_ESB_PA_LEVEL (NRF5_PA_MAX)
#endif

/**
 * @def MY_NRF5_ESB_CHANNEL
 * @brief RF channel for the sensor net, 0-125.
 *
 * Frequencies: 2400 Mhz - 2525 Mhz
 *
 * Channels: 126
 * @see https://www.nordicsemi.com/eng/nordic/download_resource/8765/2/42877161/2726
 *
 * - 0 => 2400 Mhz (RF24 channel 1)
 * - 1 => 2401 Mhz (RF24 channel 2)
 * - 76 => 2476 Mhz (RF24 channel 77)
 * - 83 => 2483 Mhz (RF24 channel 84)
 * - 124 => 2524 Mhz (RF24 channel 125)
 * - 125 => 2525 Mhz (RF24 channel 126)
 *
 * In some countries there might be limitations, in Germany for example only the range
 * 2400,0 - 2483,5 Mhz is allowed.
 * @see http://www.bundesnetzagentur.de/SharedDocs/Downloads/DE/Sachgebiete/Telekommunikation/Unternehmen_Institutionen/Frequenzen/Allgemeinzuteilungen/2013_10_WLAN_2,4GHz_pdf.pdf
 */
#ifndef MY_NRF5_ESB_CHANNEL
#define MY_NRF5_ESB_CHANNEL (76)
#endif

/**
 * @def MY_NRF5_ESB_MODE
 * @brief nRF5 mode.
 *
 * - NRF5_250KBPS for 250kbs
 * - NRF5_1MBPS for 1Mbps
 * - NRF5_2MBPS for 2Mbps.
 */
#ifndef MY_NRF5_ESB_MODE
#define MY_NRF5_ESB_MODE (NRF5_250KBPS)
#endif

/**
 * @def MY_NRF5_ESB_BASE_RADIO_ID
 * @brief nRF5 radio network identifier.
 *
 * This acts as base value for sensor nodeId addresses. Change this (or channel) if you have more
 * than one sensor network.
 */
#ifndef MY_NRF5_ESB_BASE_RADIO_ID
#define MY_NRF5_ESB_BASE_RADIO_ID 0x00, 0xFC, 0xE1, 0xA8, 0xA8
#endif

/**
 * @def MY_NRF5_ESB_ADDR_WIDTH
 * @brief nRF5 base address width.
 */
#ifndef MY_NRF5_ESB_ADDR_WIDTH
#define MY_NRF5_ESB_ADDR_WIDTH (5)
#endif

/**
 * @def MY_NRF5_ESB_RX_BUFFER_SIZE
 * @brief Declare the amount of incoming messages that can be buffered at driver level.
 */
#ifndef MY_NRF5_ESB_RX_BUFFER_SIZE
#define MY_NRF5_ESB_RX_BUFFER_SIZE (20)
#endif

/**
 * @def MY_NRF5_ESB_REVERSE_ACK
 * @brief Switch to SI24R1 or faked nRF24L01+ compatible ACK mode.
 */
//#define MY_NRF5_ESB_REVERSE_ACK
/** @}*/ // End of NRF5SettingGrpPub group

/**
 * @defgroup RFM69SettingGrpPub RFM69
 * @ingroup RadioSettingGrpPub
 * @brief These options are specific to the %RFM69 family of wireless transport modules.
 *
 * The following chips are supported by this driver:
 * - Semtech sx1231
 *
 * If using the HW variant of the %RFM69 module, define @ref MY_IS_RFM69HW.
 * @{
 */

/**
 * @def MY_DEBUG_VERBOSE_RFM69
 * @brief Define this for verbose debug prints related to the %RFM69 driver.
 */
//#define MY_DEBUG_VERBOSE_RFM69

/**
 * @def MY_DEBUG_VERBOSE_RFM69_REGISTERS
 * @brief Define this for verbose dumping of the %RFM69 registers.
 */
//#define MY_DEBUG_VERBOSE_RFM69_REGISTERS

/**
 * @def MY_RFM69_NEW_DRIVER
 * @brief Define this to enable the improved %RFM69 driver.
 *
 * @note  This driver is not compatible with the old (=default) %RFM69 driver.
 */
//#define MY_RFM69_NEW_DRIVER

/**
 * @def MY_RFM69_FREQUENCY
 * @brief The frequency to use.
 *
 * - RFM69_433MHZ
 * - RFM69_868MHZ
 * - RFM69_915MHZ
 * - Custom frequency in Hz (new %RFM69 driver only)
 * @see MY_RFM69_NEW_DRIVER
 *
 * This must match the hardware version of the %RFM69 radio.
 * Additional information: https://en.wikipedia.org/wiki/Short_Range_Devices
 */
#ifndef MY_RFM69_FREQUENCY
#define MY_RFM69_FREQUENCY (RFM69_868MHZ)
#endif

/**
 * @def MY_IS_RFM69HW
 * @brief Define this if you are using the RFM69HW model.
 */
//#define MY_IS_RFM69HW

/**
 * @def MY_RFM69HW
 * @brief Set to true if @ref MY_IS_RFM69HW is set.
 *
 * @todo Mark this internals
 */
#ifdef MY_IS_RFM69HW
#define MY_RFM69HW true
#else
#define MY_RFM69HW false
#endif

/**
 * @def MY_RFM69_TX_POWER_DBM
 * @brief Set TX power level, default 5dBm (overriden if ATC mode enabled).
 */
#ifndef MY_RFM69_TX_POWER_DBM
#define MY_RFM69_TX_POWER_DBM (5)
#endif

/**
 * @def MY_RFM69_ATC_TARGET_RSSI_DBM
 * @brief Target RSSI level (in dBm) for RFM69 ATC mode.
 */
#ifndef MY_RFM69_ATC_TARGET_RSSI_DBM
#define MY_RFM69_ATC_TARGET_RSSI_DBM (-80)
#endif

/**
 * @def MY_RFM69_ATC_MODE_DISABLED
 * @brief Define to disable ATC mode of RFM69 driver.
 */
//#define MY_RFM69_ATC_MODE_DISABLED

/**
 * @def MY_RFM69_MAX_POWER_LEVEL_DBM
 * @brief Set max TX power in dBm if local legislation requires this
 *
 * - 1mW = 0dBm
 * - 10mW = 10dBm
 * - 25mW = 14dBm
 * - 100mW = 20dBm
 *
 * See here: https://en.wikipedia.org/wiki/Short_Range_Devices
 */
//#define MY_RFM69_MAX_POWER_LEVEL_DBM (10u)

/**
 * @def MY_RFM69_NETWORKID
 * @brief RFM69 Network ID. Use the same for all nodes that will talk to each other.
 */
#ifndef MY_RFM69_NETWORKID
#define MY_RFM69_NETWORKID (100)
#endif

/**
 * @def MY_RFM69_RST_PIN
 * @brief Define this to use the %RFM69 reset pin (optional).
 */
//#define MY_RFM69_RST_PIN (9)

#ifdef MY_RF69_RESET
// legacy, older board files
#define MY_RFM69_RST_PIN MY_RF69_RESET
#endif

/**
 * @def MY_RFM69_POWER_PIN
 * @brief Define this to use the %RFM69 power pin (optional).
 */
//#define MY_RFM69_POWER_PIN (3)

/**
 * @def MY_RFM69_IRQ_PIN
 * @brief Define this to use the %RFM69 IRQ pin (optional).
 */
#ifndef MY_RFM69_IRQ_PIN
#ifdef MY_RF69_IRQ_PIN
// legacy, older board files
#define MY_RFM69_IRQ_PIN MY_RF69_IRQ_PIN
#else
#define MY_RFM69_IRQ_PIN DEFAULT_RFM69_IRQ_PIN
#endif
#endif

/**
 * @def MY_RFM69_IRQ_NUM
 * @brief %RFM69 IRQ number.
 */
#ifndef MY_RFM69_IRQ_NUM
#ifdef MY_RF69_IRQ_NUM
// legacy, older board files
#define MY_RFM69_IRQ_NUM MY_RF69_IRQ_NUM
#else
#define MY_RFM69_IRQ_NUM DEFAULT_RFM69_IRQ_NUM
#endif
#endif

/**
 * @def MY_RFM69_CS_PIN
 * @brief RFM69 SPI chip select pin.
 */
#ifndef MY_RFM69_CS_PIN
#ifdef MY_RF69_SPI_CS
// legacy, older board files
#define MY_RFM69_CS_PIN MY_RF69_SPI_CS
#else
#define MY_RFM69_CS_PIN DEFAULT_RFM69_CS_PIN
#endif
#endif

/**
 * @def MY_RFM69_SPI_SPEED
 * @brief Set to overrule default RFM69 SPI speed.
 */
#ifndef MY_RFM69_SPI_SPEED
#define MY_RFM69_SPI_SPEED (4*1000000ul)	// datasheet says 10Mhz max.
#endif

/**
 * @def MY_RFM69_ENABLE_ENCRYPTION
 * @brief Define this to enable %AES encryption in the %RFM69 module.
 *
 * All nodes and gateway must have this enabled, and all must be personalized with the same %AES
 * key.
 * @see @ref personalization
 */
//#define MY_RFM69_ENABLE_ENCRYPTION

/**
 * @def MY_RFM69_ENABLE_LISTENMODE
 * @brief Define this if you need listenmode, or skip it to save memory
 */
//#define MY_RFM69_ENABLE_LISTENMODE

#if defined(MY_RFM69_ENABLE_LISTENMODE) && !defined(MY_RFM69_DEFAULT_LISTEN_RX_US)
// By default, receive for 256uS in listen mode and idle for ~1s
#define MY_RFM69_DEFAULT_LISTEN_RX_US	(256)
#endif

#if defined(MY_RFM69_ENABLE_LISTENMODE) && !defined(MY_RFM69_DEFAULT_LISTEN_IDLE_US)
// By default, receive for 256uS in listen mode and idle for ~1s
#define  MY_RFM69_DEFAULT_LISTEN_IDLE_US (1*1000000ul)
#endif

#if !defined(MY_RFM69_BITRATE_MSB) && !defined(MY_RFM69_BITRATE_LSB)
/**
 * @def MY_RFM69_BITRATE_MSB
 * @brief %RFM69 bit rate (most significant bits)
 *
 * Bitrate between the transmitter and the receiver must be better than 6.5.
 * Refer to RFM69registers_old.h (L.153) or RFM69registers_new.h (L.154) for settings or
 * http://www.semtech.com/apps/filedown/down.php?file=sx1231.pdf
 * @note RFM69_FOSC(Hz)/MSB_LSBVALUE = Bitrate in kbits
 *
 */
#define MY_RFM69_BITRATE_MSB (RFM69_BITRATEMSB_55555)
/**
 * @def MY_RFM69_BITRATE_LSB
 * @brief %RFM69 bit rate (least significant bits)
 *
 * Bitrate between the transmitter and the receiver must be better than 6.5.
 * Refer to RFM69registers_old.h (L.153) or RFM69registers_new.h (L.154) for settings or
 * http://www.semtech.com/apps/filedown/down.php?file=sx1231.pdf
 * @note RFM69_FOSC(Hz)/MSB_LSBVALUE = Bitrate in kbits
 */
#define MY_RFM69_BITRATE_LSB (RFM69_BITRATELSB_55555)
#endif
/** @}*/ // End of RFM69SettingGrpPub group

/**
 * @defgroup RFM95SettingGrpPub RFM95
 * @ingroup RadioSettingGrpPub
 * @brief These options are specific to the %RFM95 family of wireless transport modules.
 *
 * The following chips are supported by this driver:
 * - Semtech sx1276
 * @{
 */

/**
 * @def MY_DEBUG_VERBOSE_RFM95
 * @brief Define this for verbose debug prints related to the RFM95 driver.
 */
//#define MY_DEBUG_VERBOSE_RFM95

/**
 * @def MY_RFM95_FREQUENCY
 * @brief The frequency to use.
 *
 * - RFM95_169MHZ
 * - RFM95_315MHZ
 * - RFM95_434MHZ
 * - RFM95_868MHZ
 * - RFM95_915MHZ
 * - Custom frequency in Hz
 *
 * This must match the hardware version of the RFM95 radio.
 * Additional information: https://en.wikipedia.org/wiki/Short_Range_Devices
 */
#ifndef MY_RFM95_FREQUENCY
#define MY_RFM95_FREQUENCY (RFM95_868MHZ)
#endif

/**
 * @def MY_RFM95_MODEM_CONFIGRUATION
 * @brief RFM95 modem configuration.
 *
 * BW = Bandwidth in kHz
 * CR = Error correction code
 * SF = Spreading factor, chips / symbol
 *
 * | CONFIG                 | BW    | CR  | SF   | Comment
 * |------------------------|-------|-----|------|-----------------------------
 * | RFM95_BW125CR45SF128   | 125   | 4/5 | 128  | Default, medium range
 * | RFM95_BW500CR45SF128   | 500   | 4/5 | 128  | Fast, short range
 * | RFM95_BW31_25CR48SF512 | 31.25 | 4/8 | 512  | Slow, long range
 * | RFM95_BW125CR48SF4096  | 125   | 4/8 | 4096 | Slow, long range
 *
 */
#ifndef MY_RFM95_MODEM_CONFIGRUATION
#define MY_RFM95_MODEM_CONFIGRUATION RFM95_BW125CR45SF128
#endif

/**
 * @def MY_RFM95_RST_PIN
 * @brief Define this to use the RFM95 reset pin (optional).
 */
//#define MY_RFM95_RST_PIN (9)

/**
 * @def MY_RFM95_POWER_PIN
 * @brief Define this to use the RFM95 power pin (optional).
 */
//#define MY_RFM95_POWER_PIN (3)

/**
 * @def MY_RFM95_IRQ_PIN
 * @brief Define this to use the RFM95 IRQ pin (optional).
 */
#ifndef MY_RFM95_IRQ_PIN
#define MY_RFM95_IRQ_PIN DEFAULT_RFM95_IRQ_PIN
#endif

/**
 * @def MY_RFM95_IRQ_NUM
 * @brief RFM95 IRQ number.
 */
#ifndef MY_RFM95_IRQ_NUM
#define MY_RFM95_IRQ_NUM DEFAULT_RFM95_IRQ_NUM
#endif

/**
 * @def MY_RFM95_CS_PIN
 * @brief RFM95 SPI chip select pin.
 */
#ifndef MY_RFM95_CS_PIN
#define MY_RFM95_CS_PIN DEFAULT_RFM95_CS_PIN
#endif

/**
 * @def MY_RFM95_SPI_SPEED
 * @brief Set to overrule default RFM95 SPI speed.
 */
#ifndef MY_RFM95_SPI_SPEED
#define MY_RFM95_SPI_SPEED (4*1000000ul)
#endif

/**
 * @def MY_RFM95_TX_POWER_DBM
 * @brief Set TX power level, default 13dBm (overriden if ATC mode enabled)
 *
 * See here https://en.wikipedia.org/wiki/Short_Range_Devices
 */
#ifndef MY_RFM95_TX_POWER_DBM
#define MY_RFM95_TX_POWER_DBM (13u)	// 20mW
#endif

/**
 * @def MY_RFM95_ATC_MODE_DISABLED
 * @brief Define to disable ATC mode of RFM95 driver.
 */
//#define MY_RFM95_ATC_MODE_DISABLED

/**
 * @def MY_RFM95_ATC_TARGET_RSSI
 * @brief Target RSSI level (in dBm) for RFM95 ATC mode
 */
#ifndef MY_RFM95_ATC_TARGET_RSSI
#define MY_RFM95_ATC_TARGET_RSSI (-60)
#endif

/**
 * @def MY_RFM95_MAX_POWER_LEVEL_DBM
 * @brief Set max TX power in dBm if local legislation requires this
 *
 * - 1mW = 0dBm
 * - 10mW = 10dBm
 * - 25mW = 14dBm
 * - 100mW = 20dBm
 *
 * See here: https://en.wikipedia.org/wiki/Short_Range_Devices
 */
//#define MY_RFM95_MAX_POWER_LEVEL_DBM (10u)

/**
 * @def MY_RFM95_TCXO
 * @brief Enable to force your radio to use an external frequency source (e.g. TCXO, if present).
 *
 * This allows for better stability using SF 9 to 12.
 */
//#define MY_RFM95_TCXO
/** @}*/ // End of RFM95SettingGrpPub group

/**
 * @defgroup SoftSpiSettingGrpPub Soft SPI
 * @ingroup RadioSettingGrpPub
 * @brief These options are specific the soft SPI driver for certain radio transport drivers.
 *
 * The following transport drivers supported by this driver:
 * - The RF24 driver @see RF24SettingGrpPub
 * - The new %RFM69 driver @see RFM69SettingGrpPub @see MY_RFM69_NEW_DRIVER
 * - The RFM95 driver @see RFM95SettingGrpPub
 * @{
 */

/**
 * @def MY_SOFTSPI
 * @brief Define this to use a software based SPI driver which allows more freedom in pin selection
 * for the (supported) radio module.
 */
//#define MY_SOFTSPI

/**
 * @def MY_SOFT_SPI_SCK_PIN
 * @brief Soft SPI SCK pin.
 */
#ifndef MY_SOFT_SPI_SCK_PIN
#define MY_SOFT_SPI_SCK_PIN (14)
#endif

/**
 * @def MY_SOFT_SPI_MISO_PIN
 * @brief Soft SPI MISO pin.
 */
#ifndef MY_SOFT_SPI_MISO_PIN
#define MY_SOFT_SPI_MISO_PIN (16)
#endif

/**
 * @def MY_SOFT_SPI_MOSI_PIN
 * @brief Soft SPI MOSI pin.
 */
#ifndef MY_SOFT_SPI_MOSI_PIN
#define MY_SOFT_SPI_MOSI_PIN (15)
#endif
/** @}*/ // End of SoftSpiSettingGrpPub group

/** @}*/ // End of RadioSettingGrpPub group

/**
 * @defgroup RoutingNodeSettingGrpPub Routing and node
 * @ingroup MyConfigGrp
 * @brief These options control message routing and node configurations.
 * @{
 */
/**
 * @def MY_DISABLE_RAM_ROUTING_TABLE_FEATURE
 * @ingroup memorysavings
 * @brief If defined, routing table will not be kept in RAM.
 * @see MY_RAM_ROUTING_TABLE_FEATURE
 */
/**
 * @def MY_RAM_ROUTING_TABLE_FEATURE
 * @brief If enabled, the routing table is kept in RAM (if memory allows) and saved in regular
 *        intervals.
 * @note Enabled by default on most platforms, but on AVR only for atmega1280, atmega1284 and
 *       atmega2560.
 * @see MY_DISABLE_RAM_ROUTING_TABLE_FEATURE
 */
#ifndef MY_DISABLE_RAM_ROUTING_TABLE_FEATURE
#define MY_RAM_ROUTING_TABLE_FEATURE
#endif

/**
 * @def MY_ROUTING_TABLE_SAVE_INTERVAL_MS
 * @brief Interval to dump content of routing table to EEPROM
 */
#ifndef MY_ROUTING_TABLE_SAVE_INTERVAL_MS
#define MY_ROUTING_TABLE_SAVE_INTERVAL_MS (30*60*1000ul)
#endif

/**
 * @def MY_REPEATER_FEATURE
 * @brief Enables repeater functionality (relays messages from other nodes)
 * @note Repeaters need to be constantly kept awake to be useful. They are therefore not suitable
 *       for battery powered operation.
 */
//#define MY_REPEATER_FEATURE
#if defined(MY_REPEATER_FEATURE)
#define MY_TRANSPORT_SANITY_CHECK
#endif

/**
 * @def MY_PASSIVE_NODE
 * @brief If enabled, the node operates fully autonomously, i.e. messages are sent without ACKing.
 *
 * @note All transport-related checks and safety-mechanisms are disabled.
 * @note Requires that @ref MY_NODE_ID is set, @ref MY_PARENT_NODE_ID and
 *       @ref MY_PARENT_NODE_IS_STATIC are optional.
 * @note Singing, registration, and OTA FW update are disabled.
 */
//#define MY_PASSIVE_NODE

/**
 * @def MY_NODE_ID
 * @brief Node id defaults to AUTO (tries to fetch id from controller).
 */
#ifndef MY_NODE_ID
#define MY_NODE_ID (AUTO)
#endif

/**
 * @def MY_PARENT_NODE_ID
 * @brief Node parent defaults to AUTO (tries to find a parent automatically).
 */
#ifndef MY_PARENT_NODE_ID
#define MY_PARENT_NODE_ID (AUTO)
#endif

/**
 * @def MY_PARENT_NODE_IS_STATIC
 * @brief Define MY_PARENT_NODE_IS_STATIC to disable fall back if parent node fails
 */
//#define MY_PARENT_NODE_IS_STATIC

/**
 * @def MY_TRANSPORT_SANITY_CHECK
 * @brief If defined, will cause node to check transport in regular intervals to detect HW issues
 *        and re-initialize in case of failure.
 * @note This feature is enabled for all repeater nodes (incl. GW)
 */
//#define MY_TRANSPORT_SANITY_CHECK

/**
 * @def MY_TRANSPORT_SANITY_CHECK_INTERVAL_MS
 * @brief Interval (in ms) for transport sanity checks
 */
#ifndef MY_TRANSPORT_SANITY_CHECK_INTERVAL_MS
#define MY_TRANSPORT_SANITY_CHECK_INTERVAL_MS (15*60*1000ul)
#endif
/**
 * @def MY_TRANSPORT_DISCOVERY_INTERVAL_MS
 * @brief This is a gateway-only feature: Interval (in ms) to issue network discovery checks
 */
#ifndef MY_TRANSPORT_DISCOVERY_INTERVAL_MS
#define MY_TRANSPORT_DISCOVERY_INTERVAL_MS (20*60*1000ul)
#endif

/**
 *@def MY_TRANSPORT_UPLINK_CHECK_DISABLED
 *@brief If defined, disables uplink check to GW during transport initialisation
 */
//#define MY_TRANSPORT_UPLINK_CHECK_DISABLED

/**
 *@def MY_TRANSPORT_MAX_TX_FAILURES
 *@brief Define to override max. consecutive TX failures until SNP is initiated
 */
//#define MY_TRANSPORT_MAX_TX_FAILURES (10u)

/**
 * @def MY_TRANSPORT_WAIT_READY_MS
 * @brief Timeout in ms until transport is ready during startup, set to 0 for no timeout
 */
#ifndef MY_TRANSPORT_WAIT_READY_MS
#define MY_TRANSPORT_WAIT_READY_MS (0ul)
#endif

/**
 * @def MY_DISABLE_SIGNAL_REPORT
 * @ingroup memorysavings
 * @brief If defined, signal report functionality will be unavailable.
 * @see MY_SIGNAL_REPORT_ENABLED
 */
/**
 * @def MY_SIGNAL_REPORT_ENABLED
 * @brief Enables signal report functionality.
 * @note Enabled by default. This feature adds ~1kB code to the sketch.
 * @see MY_DISABLE_SIGNAL_REPORT
 */
#ifndef MY_DISABLE_SIGNAL_REPORT
#define MY_SIGNAL_REPORT_ENABLED
#endif
/** @}*/ // End of RoutingNodeSettingGrpPub group

/**
 * @defgroup RegistrationSettingGrpPub Node registration
 * @ingroup MyConfigGrp
 * @brief These options control node registration configurations.
 * @{
 */
/**
 * @def MY_REGISTRATION_FEATURE
 * @brief If enabled, node has to register to GW/controller before being allowed to send sensor
 *        data.
 * @note Enabled by default.
 */
#define MY_REGISTRATION_FEATURE

/**
 * @def MY_REGISTRATION_RETRIES
 * @brief Number of registration retries if no reply received from GW/controller.
 */

#ifndef MY_REGISTRATION_RETRIES
#define MY_REGISTRATION_RETRIES (3u)
#endif

/**
* @def MY_REGISTRATION_DEFAULT
* @brief Node registration default - this applies if no registration response is received from
 *       controller.
*/
#define MY_REGISTRATION_DEFAULT (true)

/**
 * @def MY_REGISTRATION_CONTROLLER
 * @brief If defined, node registration request has to be handled by controller
 */
//#define MY_REGISTRATION_CONTROLLER
/** @}*/ // End of RegistrationSettingGrpPub group

/**
 * @defgroup CoreSettingGrpPub Core
 * @ingroup MyConfigGrp
 * @brief These options control the library core configurations.
 * @{
 */
/**
 * @def MY_CORE_ONLY
 * @brief Define this if you want to use core functions without loading the framework.
 */
//#define MY_CORE_ONLY

/**
 * @def MY_CORE_COMPATIBILITY_CHECK
 * @brief If defined, library compatibility is checked during node registration.
 *        Incompatible libraries are unable to send sensor data.
 */
#define MY_CORE_COMPATIBILITY_CHECK
/** @}*/ // End of CoreSettingGrpPub group

/**
 * @defgroup SleepSettingGrpPub Sleep
 * @ingroup MyConfigGrp
 * @brief These options control sleep configurations.
 * @{
 */
/**
 * @def MY_SLEEP_TRANSPORT_RECONNECT_TIMEOUT_MS
 * @brief Timeout (in ms) to re-establish link if node is send to sleep and transport is not ready.
 */
#ifndef MY_SLEEP_TRANSPORT_RECONNECT_TIMEOUT_MS
#define MY_SLEEP_TRANSPORT_RECONNECT_TIMEOUT_MS (10*1000ul)
#endif

/**
 * @def MY_SMART_SLEEP_WAIT_DURATION_MS
 * @brief The wait period (in ms) before going to sleep when using smartSleep-functions.
 *
 * This period has to be long enough for controller to be able to send out
 * potential buffered messages.
 */
#ifndef MY_SMART_SLEEP_WAIT_DURATION_MS
#define MY_SMART_SLEEP_WAIT_DURATION_MS (500ul)
#endif
/** @}*/ // End of SleepSettingGrpPub group

/**
 * @defgroup OTASettingGrpPub Over The Air firmware
 * @ingroup MyConfigGrp
 * @brief These options control OTA firmware configurations.
 * @{
 */

/**
 * @def MY_OTA_FIRMWARE_FEATURE
 * @brief Define this in sketch to allow safe over-the-air firmware updates.
 *
 * This feature requires external flash and the DualOptiBoot boot-loader.
 * @note You can still have OTA FW updates without external flash but it
 *       requires the MYSBootloader and you must not define this flag.
 */
//#define MY_OTA_FIRMWARE_FEATURE

/**
 * @def MY_OTA_FLASH_SS
 * @brief Slave select pin for external flash used for OTA.
 */
#ifndef MY_OTA_FLASH_SS
#define MY_OTA_FLASH_SS (8)
#endif

/**
 * @def MY_OTA_FLASH_JDECID
 * @brief Flash JDECID used for OTA. Use (0x00) if unknown.
 */
#ifndef MY_OTA_FLASH_JDECID
#define MY_OTA_FLASH_JDECID (0x1F65)
#endif

/**
 * @def MY_DISABLE_REMOTE_RESET
 * @brief Disables over-the-air reset of node
 */
//#define MY_DISABLE_REMOTE_RESET
/** @}*/ // End of OTASettingGrpPub group

/**
 * @defgroup GatewaySettingGrpPub Gateway
 * @ingroup MyConfigGrp
 * @brief These options control gateway specific configurations.
 * @{
 */
/**
 * @def MY_GATEWAY_MAX_RECEIVE_LENGTH
 * @brief Max buffersize needed for messages coming from controller.
 */
#ifndef MY_GATEWAY_MAX_RECEIVE_LENGTH
#define MY_GATEWAY_MAX_RECEIVE_LENGTH (100u)
#endif

/**
 * @def MY_GATEWAY_MAX_SEND_LENGTH
 * @brief Max buffer size when sending messages.
 */
#ifndef MY_GATEWAY_MAX_SEND_LENGTH
#define MY_GATEWAY_MAX_SEND_LENGTH (120u)
#endif

/**
 * @def MY_GATEWAY_MAX_CLIENTS
 * @brief Max number of parallel clients (sever mode).
 */
#ifndef MY_GATEWAY_MAX_CLIENTS
#define MY_GATEWAY_MAX_CLIENTS (1u)
#endif

/**
 * @def MY_INCLUSION_MODE_FEATURE
 * @brief Define this to enable the inclusion mode feature.
 */
//#define MY_INCLUSION_MODE_FEATURE

/**
 * @def MY_INCLUSION_BUTTON_FEATURE
 * @brief Enables inclusion-mode button feature on the gateway device.
 *
 * With this defined, you can put the GW in inclusion mode by pressing a button attached to the GW.
 */
//#define MY_INCLUSION_BUTTON_FEATURE

// Disable inclusion mode button if inclusion mode feature is not enabled
#ifndef MY_INCLUSION_MODE_FEATURE
#undef MY_INCLUSION_BUTTON_FEATURE
#endif

/**
 * @def MY_INCLUSION_MODE_BUTTON_PIN
 * @brief The default input pin used for the inclusion mode button.
 */
#ifndef MY_INCLUSION_MODE_BUTTON_PIN
#if defined(ARDUINO_ARCH_ESP8266)
#define MY_INCLUSION_MODE_BUTTON_PIN	(5)
#else
#define MY_INCLUSION_MODE_BUTTON_PIN	(3)
#endif
#endif

/**
 * @def MY_INCLUSION_MODE_DURATION
 * @brief Number of seconds inclusion mode should be enabled.
 */
#ifndef MY_INCLUSION_MODE_DURATION
#define MY_INCLUSION_MODE_DURATION (60)
#endif

/**
 * @def MY_INCLUSION_BUTTON_EXTERNAL_PULLUP
 * @brief Define this to change the default state for @ref MY_INCLUSION_BUTTON_PRESSED.
 */
/**
 * @def MY_INCLUSION_BUTTON_PRESSED
 * @brief The logical level indicating a pressed inclusion mode button.
 *
 * If @ref MY_INCLUSION_BUTTON_EXTERNAL_PULLUP is defined, this defaults to HIGH.
 */
#if defined(MY_INCLUSION_BUTTON_EXTERNAL_PULLUP)
#define MY_INCLUSION_BUTTON_PRESSED (HIGH)
#else
#define MY_INCLUSION_BUTTON_PRESSED (LOW)
#endif

/**************************************
* Ethernet Gateway Transport Defaults
***************************************/
/**
 * @def MY_GATEWAY_W5100
 * @brief Define this for Ethernet GW based on the W5100 module.
 * @def MY_GATEWAY_ENC28J60
 * @brief Define this for Ethernet GW based on the ENC28J60 module.
 * @def MY_GATEWAY_ESP8266
 * @brief Define this for Ethernet GW based on the ESP8266.
 * @def MY_GATEWAY_LINUX
 * @brief Define this for Ethernet GW based on Linux.
 */
// The gateway options available
//#define MY_GATEWAY_W5100
//#define MY_GATEWAY_ENC28J60
//#define MY_GATEWAY_ESP8266
//#define MY_GATEWAY_LINUX

/**
 * @def MY_PORT
 * @brief The Ethernet TCP/UDP port to open on controller or gateway.
 */
#ifndef MY_PORT
#ifdef MY_GATEWAY_MQTT_CLIENT
#define MY_PORT 1883
#else
#define MY_PORT 5003
#endif
#endif

/**
 * @def MY_MQTT_CLIENT_PUBLISH_RETAIN
 * @brief Enables MQTT client to set the retain flag when publishing specific messages.
 */
//#define MY_MQTT_CLIENT_PUBLISH_RETAIN

/**
 * @def MY_IP_ADDRESS
 * @brief Static ip address of gateway (if this is not defined, DHCP will be used).
 */
//#define MY_IP_ADDRESS 192,168,178,66

/**
 * @def MY_USE_UDP
 * @brief Enables UDP mode for Ethernet gateway.
 * @note This is not supported on ENC28J60 and Linux based GWs.
 */
//#define MY_USE_UDP

/**
 * @def MY_IP_RENEWAL_INTERVAL_MS
 * @brief DHCP, default renewal setting in milliseconds.
 */
#ifndef MY_IP_RENEWAL_INTERVAL_MS
#define MY_IP_RENEWAL_INTERVAL_MS (60*1000ul)
#endif

/**
 * @def MY_MAC_ADDRESS
 * @brief Ethernet MAC address.
 * @note This needs to be unique on the network.
 */
#ifndef MY_MAC_ADDRESS
#define MY_MAC_ADDRESS 0xDE,0xAD,0xBE,0xEF,0xFE,0xED
#endif

/**
 * @def MY_CONTROLLER_IP_ADDRESS
 * @brief If this is defined, gateway will act as a client trying to contact controller on
 *        @ref MY_PORT using this IP address.
 *
 * If left un-defined, gateway acts as server allowing incoming connections.
 */
//#define MY_CONTROLLER_IP_ADDRESS 192,168,178,254
/** @}*/ // End of GatewaySettingGrpPub group

/**
 * @defgroup lEDSettingGrpPub LED
 * @ingroup MyConfigGrp
 * @brief These options control LED specific configurations.
 * @{
 */
/**
 * @def MY_DEFAULT_ERR_LED_PIN
 * @brief Define this with a value that correspond to your placement of the error indication LED.
 *
 * @note This is optional.
 * @note On some platforms (for example sensebender GW) the hardware definitions can enable the LED
 *       by default. That default can be overridden by defining this flag.
 */
//#define MY_DEFAULT_ERR_LED_PIN (6)

/**
 * @def MY_DEFAULT_TX_LED_PIN
 * @brief Define this with a value that correspond to your placement of the TX indication LED.
 *
 * @note This is optional.
 * @note On some platforms (for example sensebender GW) the hardware definitions can enable the LED
 *       by default. That default can be overridden by defining this flag.
 */
//#define MY_DEFAULT_TX_LED_PIN (7)

/**
 * @def MY_DEFAULT_RX_LED_PIN
 * @brief Define this with a value that correspond to your placement of the RX indication LED.
 *
 * @note This is optional.
 * @note On some platforms (for example sensebender GW) the hardware definitions can enable the LED
 *       by default. That default can be overridden by defining this flag.
 */
//#define MY_DEFAULT_RX_LED_PIN (8)

/**
 * @def MY_WITH_LEDS_BLINKING_INVERSE
 * @brief Define this to inverse the LED blinking.
 *
 * When defined LEDs are normally turned on and switches off when blinking.
 */
//#define MY_WITH_LEDS_BLINKING_INVERSE

/**
 * @def MY_INDICATION_HANDLER
 * @brief Define to use own indication handler.
 */
//#define MY_INDICATION_HANDLER

/**
* @def MY_DEFAULT_LED_BLINK_PERIOD
* @brief Default LEDs blinking period in milliseconds.
*/
#ifndef MY_DEFAULT_LED_BLINK_PERIOD
#define MY_DEFAULT_LED_BLINK_PERIOD 300
#endif
/** @}*/ // End of lEDSettingGrpPub group

/**
 * @defgroup SecuritySettingGrpPub Security
 * @ingroup MyConfigGrp
 * @brief These options control security related configurations.
 *
 * Note that some encryption configurations are on a per-radio basis as these are specific to each
 * radio.
 *
 * @see MY_RF24_ENABLE_ENCRYPTION, MY_RFM69_ENABLE_ENCRYPTION, MY_NRF5_ESB_ENABLE_ENCRYPTION
 * @{
 */
/**
 * @defgroup SigningSettingGrpPub Signing
 * @ingroup SecuritySettingGrpPub
 * @brief These options control signing related configurations.
 *
 * @see MySigninggrpPub
 * @{
 */
/**
* @def MY_DEBUG_VERBOSE_SIGNING
* @brief Define this for verbose debug prints related to signing.
*/
//#define MY_DEBUG_VERBOSE_SIGNING

/**
 * @def MY_SIGNING_SIMPLE_PASSWD
 * @brief Enables SW backed signing functionality in library and uses provided password as key.
 *
 * This flag will enable signing, signature requests and encryption. It has to be identical on ALL
 * nodes in the network.
 *
 * Whitelisting is supported and serial will be the first 8 characters of the password, the ninth
 * character will be the node ID (to make each node have a unique serial).
 *
 * As with the regular signing modes, whitelisting is only activated if a whitelist is specified in
 * the sketch.
 *
 * No personalization is required for this mode.
 *
 * It is allowed to set @ref MY_SIGNING_WEAK_SECURITY for deployment purposes in this mode as it is
 * with the regular software and ATSHA204A based modes.
 *
 * If the provided password is shorter than the size of the HMAC or %AES key, it will be null-padded
 * to accomodate the key size in question. A 32 character password is the maximum length. Any
 * password longer than that will be truncated.
 */
//#define MY_SIGNING_SIMPLE_PASSWD "MyInsecurePassword"
#if defined(MY_SIGNING_SIMPLE_PASSWD)
#define MY_SIGNING_SOFT
#define MY_SIGNING_REQUEST_SIGNATURES
#define MY_RF24_ENABLE_ENCRYPTION
#define MY_RFM69_ENABLE_ENCRYPTION
#define MY_NRF5_ESB_ENABLE_ENCRYPTION
#endif

/**
 * @def MY_SIGNING_ATSHA204
 * @brief Enables HW backed signing functionality in library.
 */
//#define MY_SIGNING_ATSHA204

/**
 * @def MY_SIGNING_SOFT
 * @brief Enables SW backed signing functionality in library.
 */
//#define MY_SIGNING_SOFT

/**
 * @def MY_SIGNING_REQUEST_SIGNATURES
 * @brief Enable this to inform gateway to sign all messages sent to this node.
 *
 * If used for a gateway, gateway will by default require signatures from ALL nodes. This behavior
 * can be disabled by weakening security.
 * @see MY_SIGNING_WEAK_SECURITY
 */
//#define MY_SIGNING_REQUEST_SIGNATURES

/**
 * @def MY_SIGNING_WEAK_SECURITY
 * @brief Enable this to permit downgrade of security preferences and relaxed gateway signing
 *        requirements.
 *
 * Use this for evaluating security. It allows for gradual introduction of signing requirements in
 * a network. Nodes that present themselves as not requiering signing or whitelisting will be
 * cleared of this requirement at the receiving end. A gateway which require signatures will only do
 * so from nodes that in turn require signatures.
 *
 * When not set, any node that has presented themselves as a node that require signatures or
 * whitelisting, will be permanently remembered as doing so at the receiver until EEPROM is cleared
 * or the receiver is reconfigured with this flag set or has signing disabled alltogether.
 *
 * @warning This flag when set will weaken security significantly
 */
//#define MY_SIGNING_WEAK_SECURITY

/**
 * @def MY_VERIFICATION_TIMEOUT_MS
 * @brief Define a suitable timeout for a signature verification session
 *
 * Consider the turnaround from a nonce being generated to a signed message being received
 * which might vary, especially in networks with many hops.
 *
 * Shorter time gives less time for an attacker to figure a way to hijack the nonce and attempt to
 * brute force attack the node. Longer time permits more network hops and node or GW processing
 * time. 5s ought to be enough for anyone.
 */
#ifndef MY_VERIFICATION_TIMEOUT_MS
#define MY_VERIFICATION_TIMEOUT_MS (5*1000ul)
#endif

/**
 * @def MY_SIGNING_NODE_WHITELISTING
 * @brief Define to turn on whitelisting
 *
 * When defined, a verifying node will look up the sender in the whitelist and salt the received
 * signature with that information before validating the result. It will also inform GW (or other
 * node) through the signing presentation message about this requirement.
 *
 * The signing node will check the presentaiton lists to determine if the recipient require
 * whitelisting and salt the signature with it's unique signature and nodeId before transmitting
 * the signed message.
 *
 * It is legal to only have one node with a whitelist for this reason but it is not required.
 */
//#define MY_SIGNING_NODE_WHITELISTING {{.nodeId = GATEWAY_ADDRESS,.serial = {0x09,0x08,0x07,0x06,0x05,0x04,0x03,0x02,0x01}}}

/**
 * @def MY_SIGNING_ATSHA204_PIN
 * @brief Atsha204a default pin setting. Set it to match the pin the device is attached to.
 */
#ifndef MY_SIGNING_ATSHA204_PIN
#define MY_SIGNING_ATSHA204_PIN (17)
#endif

/**
 * @def MY_SIGNING_SOFT_RANDOMSEED_PIN
 * @brief Pin used for random seed generation in soft signing
 * @note Do not connect anything to this when soft signing is enabled, or the seed will be
 *       predictable.
 */
#ifndef MY_SIGNING_SOFT_RANDOMSEED_PIN
#define MY_SIGNING_SOFT_RANDOMSEED_PIN (7)
#endif

/**
 * @def MY_SIGNING_FEATURE
 * @ingroup internals
 * @brief Helper flag to indicate that some signing feature is enabled
 */
#if defined(MY_SIGNING_ATSHA204) || defined(MY_SIGNING_SOFT)
#define MY_SIGNING_FEATURE
#endif
/**
 * @def MY_ENCRYPTION_FEATURE
 * @ingroup internals
 * @brief Helper flag to indicate that some encryption feature is enabled
 */
#if defined(MY_RF24_ENABLE_ENCRYPTION) || defined(MY_RFM69_ENABLE_ENCRYPTION) || defined(MY_NRF5_ESB_ENABLE_ENCRYPTION)
#define MY_ENCRYPTION_FEATURE
#endif
/** @}*/ // End of SigningSettingGrpPub group

/**
 * @defgroup MyLockgrppub Node locknig
 * @ingroup MyConfig
 * @brief These options control node lock related configurations.
 *
 * This feature locks a node that suspect itself for being under some form of attack.
 *
 * This is achieved by having a counter stored in EEPROM which decrements when suspicious activity
 * is detected.
 *
 * If the counter reaches 0, the node will not work anymore and will transmit a @ref I_LOCKED
 * message to the gateway/controller with 30 minute intervals. Payload is a string with a reason for
 * the locking.
 *
 * The string is abbreviated to accomodate a signature. The following abbreviations exist at the
 * moment:
 * - LDB (Locked During Boot)
 * - TMNR (Too Many Nonce Requests)
 * - TMFV (Too Many Failed Verifications)
 *
 * Typically, the counter only decrements when suspicious activity happens in a row.
 * It is reset if legit traffic is present.
 *
 * Examples of malicious activity are:
 * - Repeatedly incorrectly checksummed OTA firmware
 * - Repeated requests for signing nonces without properly signed messages arriving
 * - Repeatedly failed signature verifications
 *
 * If counter reaches zero, node locks down and EEPROM has to be erased/reset to reactivate node.
 * Node can also be unlocked by grounding a pin.
 * @see MY_NODE_UNLOCK_PIN
 *
 * The size of the counter can be adjusted using @ref MY_NODE_LOCK_COUNTER_MAX.
 * @{
 */
/**
 * @def MY_NODE_LOCK_FEATURE
 * @brief Enable this to activate intrusion prevention mechanisms on the node.
 */
//#define MY_NODE_LOCK_FEATURE

/**
 * @def MY_NODE_UNLOCK_PIN
 * @brief By grounding this pin durig reset of a locked node, the node will unlock.
 *
 * If using a secure bootloader, grounding the pin is the only option to reactivate the node.
 * If using stock Android bootloader or a DualOptiBoot it is also possible to download a sketch
 * using serial protocol to erase EEPROM to unlock the node.
 */
#ifndef MY_NODE_UNLOCK_PIN
#define MY_NODE_UNLOCK_PIN (14)
#endif

/**
 * @def MY_NODE_LOCK_COUNTER_MAX
 * @brief Maximum accepted occurances of suspected malicious activity in a node.
 *
 * Counter decrements on reoccuring incidents but resets if legitimate behaviour is identified.
 */
#ifndef MY_NODE_LOCK_COUNTER_MAX
#define MY_NODE_LOCK_COUNTER_MAX (5)
#endif
/** @}*/ // Node lock group
/** @}*/ // End of SecuritySettingGrpPub group

/**
 * @defgroup PlatformSettingGrpPub Platform specifics
 * @ingroup MyConfigGrp
 * @brief These options control platform specific configurations.
 * @{
 */
/**
 * @defgroup ESP8266SettingGrpPub ESP8266
 * @ingroup PlatformSettingGrpPub
 * @brief These options control ESP8266 specific configurations.
 * @{
 */
/**
 * @def MY_ESP8266_SERIAL_MODE
 * @brief ESP8266 serial modes
 *
 * - SERIAL_FULL: Default mode.
 * - SERIAL_TX_ONLY: allows to use RX (GPIO3) as a general purpose input/output.
 * - SERIAL_RX_ONLY: allows to use TX (GPIO1) as a general purpose input/output.
 */
#ifndef MY_ESP8266_SERIAL_MODE
#define MY_ESP8266_SERIAL_MODE SERIAL_FULL
#endif
/** @}*/ // End of ESP8266SettingGrpPub group

/**
 * @defgroup LinuxSettingGrpPub Linux
 * @ingroup PlatformSettingGrpPub
 * @brief These options control Linux specific configurations.
 * @{
 */
/**
 * @def MY_LINUX_SERIAL_PORT
 * @brief Serial device port
 */
#ifndef MY_LINUX_SERIAL_PORT
#define MY_LINUX_SERIAL_PORT "/dev/ttyACM0"
#endif

/**
 * @def MY_LINUX_IS_SERIAL_PTY
 * @brief Set serial as a pseudo terminal.
 *
 * Enable this if you need to connect to a controller running on the same device.
 */
//#define MY_LINUX_IS_SERIAL_PTY

/**
 * @def MY_LINUX_SERIAL_PTY
 * @brief Symlink name for the PTY device.
 */
#ifndef MY_LINUX_SERIAL_PTY
#define MY_LINUX_SERIAL_PTY "/dev/ttyMySensorsGateway"
#endif

/**
 * @def MY_LINUX_SERIAL_GROUPNAME
 * @brief Grant access to the specified system group for the serial device.
 */
//#define MY_LINUX_SERIAL_GROUPNAME "tty"

/**
 * @def MY_LINUX_CONFIG_FILE
 * @brief Sets the filepath for the gateway config file.
 *
 * @note For now the configuration file is only used to store the emulated eeprom state.
 */
#ifndef MY_LINUX_CONFIG_FILE
#define MY_LINUX_CONFIG_FILE "/etc/mysensors.dat"
#endif
/** @}*/ // End of LinuxSettingGrpPub group
/** @}*/ // End of PlatformSettingGrpPub group

/*
* "Helper" definitions
*/

/*
 * Detect node type
 * MY_GATEWAY_FEATURE is set for gateway sketches.
 * MY_IS_GATEWAY is true when @ref MY_GATEWAY_FEATURE is set.
 * MY_NODE_TYPE contain a string describing the class of sketch/node (gateway/repeater/node).
 */
#if defined(MY_GATEWAY_SERIAL) || defined(MY_GATEWAY_W5100) || defined(MY_GATEWAY_ENC28J60) || defined(MY_GATEWAY_ESP8266) || defined(MY_GATEWAY_LINUX) || defined(MY_GATEWAY_MQTT_CLIENT)
#define MY_GATEWAY_FEATURE
#define MY_IS_GATEWAY (true)
#define MY_NODE_TYPE "GW"
#elif defined(MY_REPEATER_FEATURE)
#define MY_IS_GATEWAY (false)
#define MY_NODE_TYPE "REPEATER"
#elif defined(DOXYGEN)
#define MY_IS_GATEWAY //!< true when configuration indicate a gateway device, @todo Mark these internals
#define MY_NODE_TYPE  //!< "GW" for wateways, REPEATER" for repeaters, "NODE" for nodes, @todo Mark these internals
#else
#define MY_IS_GATEWAY (false)
#define MY_NODE_TYPE "NODE"
#endif

// DEBUG
#if defined(MY_DEBUG)
// standard debug output
#define MY_DEBUG_VERBOSE_CORE	//!< MY_DEBUG_VERBOSE_CORE
#define MY_DEBUG_VERBOSE_TRANSPORT	//!< MY_DEBUG_VERBOSE_TRANSPORT
#define MY_DEBUG_VERBOSE_OTA_UPDATE	//!< MY_DEBUG_VERBOSE_OTA_UPDATE
#endif

#if defined(MY_DEBUG) || defined(MY_DEBUG_VERBOSE_CORE) || defined(MY_DEBUG_VERBOSE_TRANSPORT) || defined(MY_DEBUG_VERBOSE_SIGNING) || defined(MY_DEBUG_VERBOSE_OTA_UPDATE) || defined(MY_DEBUG_VERBOSE_RF24) || defined(MY_DEBUG_VERBOSE_NRF5_ESB) || defined(MY_DEBUG_VERBOSE_RFM69) || defined(MY_DEBUG_VERBOSE_RFM95)
#define DEBUG_OUTPUT_ENABLED	//!< DEBUG_OUTPUT_ENABLED
#define DEBUG_OUTPUT(x,...)		hwDebugPrint(x, ##__VA_ARGS__)	//!< debug
#else
#define DEBUG_OUTPUT(x,...)								//!< debug NULL
#endif

// transport layer files
#define debug(x,...)			DEBUG_OUTPUT(x, ##__VA_ARGS__)	//!< debug

// temp. workaround for nRF5 verifier: redirect RF24 to NRF_ESB
#if defined(ARDUINO_ARCH_NRF5) && (defined(MY_RADIO_RF24) || defined(MY_RADIO_NRF24) )
#undef MY_RADIO_RF24
#undef MY_RADIO_NRF24
#define MY_RADIO_NRF5_ESB
#endif

// Enable sensor network "feature" if one of the transport types was enabled
#if defined(MY_RADIO_RF24) || defined(MY_RADIO_NRF5_ESB) || defined(MY_RADIO_RFM69) || defined(MY_RADIO_RFM95) || defined(MY_RS485)
#define MY_SENSOR_NETWORK
#endif

// LEDS
#if !defined(MY_DEFAULT_ERR_LED_PIN) && defined(MY_HW_ERR_LED_PIN)
#define MY_DEFAULT_ERR_LED_PIN MY_HW_ERR_LED_PIN
#endif

#if !defined(MY_DEFAULT_TX_LED_PIN) && defined(MY_HW_TX_LED_PIN)
#define MY_DEFAULT_TX_LED_PIN MY_HW_TX_LED_PIN
#endif

#if !defined(MY_DEFAULT_RX_LED_PIN) && defined(MY_HW_TX_LED_PIN)
#define MY_DEFAULT_RX_LED_PIN MY_HW_TX_LED_PIN
#endif

#endif	// MyConfig_h

// Doxygen specific constructs, not included when built normally
// This is used to enable disabled macros/definitions to be included in the documentation as well.
#if DOXYGEN
// debug
#define MY_DEBUG
#define MY_SPECIAL_DEBUG
#define MY_DISABLED_SERIAL
#define MY_SPLASH_SCREEN_DISABLED
// linux
#define MY_LINUX_SERIAL_GROUPNAME
#define MY_LINUX_IS_SERIAL_PTY
// inclusion mode
#define MY_INCLUSION_MODE_FEATURE
#define MY_INCLUSION_BUTTON_FEATURE
// transport
#define MY_PARENT_NODE_IS_STATIC
#define MY_REGISTRATION_CONTROLLER
#define MY_TRANSPORT_UPLINK_CHECK_DISABLED
#define MY_TRANSPORT_SANITY_CHECK
#define MY_NODE_LOCK_FEATURE
#define MY_REPEATER_FEATURE
#define MY_PASSIVE_NODE
#define MY_MQTT_CLIENT_PUBLISH_RETAIN
#define MY_DISABLE_SIGNAL_REPORT
// general
#define MY_WITH_LEDS_BLINKING_INVERSE
#define MY_INDICATION_HANDLER
#define MY_DISABLE_REMOTE_RESET
#define MY_DISABLE_RAM_ROUTING_TABLE_FEATURE
// core
#define MY_CORE_ONLY
// GW
#define MY_INCLUSION_BUTTON_EXTERNAL_PULLUP
#define MY_GATEWAY_W5100
#define MY_GATEWAY_ENC28J60
#define MY_GATEWAY_ESP8266
#define MY_GATEWAY_LINUX
#define MY_IP_ADDRESS 192,168,178,66
#define MY_USE_UDP
#define MY_CONTROLLER_IP_ADDRESS 192,168,178,254
// LED
#define MY_DEFAULT_ERR_LED_PIN
#define MY_DEFAULT_TX_LED_PIN
#define MY_DEFAULT_RX_LED_PIN
// signing
#define MY_SIGNING_SIMPLE_PASSWD "MyInsecurePassword"
#define MY_SIGNING_ATSHA204
#define MY_SIGNING_SOFT
#define MY_SIGNING_REQUEST_SIGNATURES
#define MY_SIGNING_WEAK_SECURITY
#define MY_SIGNING_NODE_WHITELISTING {{.nodeId = GATEWAY_ADDRESS,.serial = {0x09,0x08,0x07,0x06,0x05,0x04,0x03,0x02,0x01}}}
#define MY_DEBUG_VERBOSE_SIGNING
#define MY_SIGNING_FEATURE
#define MY_ENCRYPTION_FEATURE
// RS485
#define MY_RS485_HWSERIAL (Serial1)
// RF24
#define MY_RADIO_RF24
#define MY_DEBUG_VERBOSE_RF24
#define MY_RF24_POWER_PIN
#define MY_RF24_IRQ_PIN
#define MY_RF24_ENABLE_ENCRYPTION
#define MY_RX_MESSAGE_BUFFER_FEATURE
#define MY_RX_MESSAGE_BUFFER_SIZE
// NRF5_ESB
#define MY_NRF5_ESB_ENABLE_ENCRYPTION
#define MY_DEBUG_VERBOSE_NRF5_ESB
#define MY_NRF5_ESB_REVERSE_ACK
// RFM69
#define MY_IS_RFM69HW
#define MY_RFM69_NEW_DRIVER
#define MY_RFM69_POWER_PIN
#define MY_RFM69_ENABLE_ENCRYPTION
#define MY_RFM69_ATC_MODE_DISABLED
#define MY_RFM69_MAX_POWER_LEVEL_DBM
#define MY_RFM69_RST_PIN
#define MY_DEBUG_VERBOSE_RFM69
#define MY_DEBUG_VERBOSE_RFM69_REGISTERS
#define MY_RFM69_ENABLE_LISTENMODE
// RFM95
#define MY_DEBUG_VERBOSE_RFM95
#define MY_RFM95_ATC_MODE_DISABLED
#define MY_RFM95_RST_PIN
#define MY_RFM95_MODEM_CONFIGRUATION
#define MY_RFM95_POWER_PIN
#define MY_RFM95_TCXO
#define MY_RFM95_MAX_POWER_LEVEL_DBM
// SOFT-SPI
#define MY_SOFTSPI
#endif
/** @}*/ // End of MyConfig group
