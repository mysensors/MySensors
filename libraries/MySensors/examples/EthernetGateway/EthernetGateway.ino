/**
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2015 Sensnology AB
 * Full contributor list: https://github.com/mysensors/Arduino/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 *******************************
 *
 * REVISION HISTORY
 * Version 1.0 - Henrik EKblad
 * Contribution by a-lurker and Anticimex,
 * Contribution by Norbert Truchsess <norbert.truchsess@t-online.de>
 * Contribution by Tomas Hozza <thozza@gmail.com>
 *
 *
 * DESCRIPTION
 * The EthernetGateway sends data received from sensors to the ethernet link.
 * The gateway also accepts input on ethernet interface, which is then sent out to the radio network.
 *
 * The GW code is designed for Arduino 328p / 16MHz.  ATmega168 does not have enough memory to run this program.
 *
 *
 * COMPILING WIZNET (W5100) ETHERNET MODULE
 * > Edit MyConfig.h in (libraries\MySensors\) to enable softspi (remove // before "#define SOFTSPI").
 *
 * COMPILING ENC28J60 ETHERNET MODULE
 * > Use Arduino IDE 1.5.7 (or later)
 * > Disable DEBUG in Sensor.h before compiling this sketch. Othervise the sketch will probably not fit in program space when downloading.
 * > Remove Ethernet.h include below and include UIPEthernet.h
 * > Remove DigitalIO include
 * Note that I had to disable UDP and DHCP support in uipethernet-conf.h to reduce space. (which means you have to choose a static IP for that module)
 *
 * VERA CONFIGURATION:
 * Enter "ip-number:port" in the ip-field of the Arduino GW device. This will temporarily override any serial configuration for the Vera plugin.
 * E.g. If you want to use the defualt values in this sketch enter: 192.168.178.66:5003
 *
 * LED purposes:
 * - To use the feature, uncomment WITH_LEDS_BLINKING in MyConfig.h
 * - RX (green) - blink fast on radio message recieved. In inclusion mode will blink fast only on presentation recieved
 * - TX (yellow) - blink fast on radio message transmitted. In inclusion mode will blink slowly
 * - ERR (red) - fast blink on error during transmission error or recieve crc error
 *
 * See http://www.mysensors.org/build/ethernet_gateway for wiring instructions.
 *
 */

// Enable debug prints to console
#define MY_DEBUG 

// This node has a NR24L01+ radio attached
#define MY_RADIO_NRF24 


// W5100 Ethernet in client mode (node opens conmection to controller)
#define MY_GATEWAY_W5100_CLIENT
// W5100 Ethernet in client mode (controller opens conmection to node)
//#define MY_GATEWAY_W5100_SERVER

// Set default gateway protocol (http://www.mysensors.org/download/serial_api_15)
#define MY_GATEWAY_PROTOCOL_DEFAULT

// Enable Soft SPI for radio (note different radio wiring required)
// Useful for W5100 which sometimes have hard time co-operate with radio on the same spi bus.
//#define MY_SOFTSPI
//#define MY_SOFT_SPI_SCK_PIN 14
//#define MY_SOFT_SPI_MISO_PIN 16
//#define MY_SOFT_SPI_MOSI_PIN 15
         

// Enable to use UDP          
#define MY_USE_UDP

#define MY_IP_ADDRESS 192,168,178,66   // If this is disabled, DHCP is used to retrieve address
// Renewal period if using DHCP
//#define MY_IP_RENEWAL_INTERVAL 60000
// The port to keep open on node server mode / or port to contact in client mode
#define MY_PORT 5003      

// Controller ip address (client mode)
#define MY_CONTROLLER_IP_ADDRESS 192, 168, 178, 254   
// The port you want to contact on controller (client mode)
#define MY_CONTROLLER_PORT 5003                
 
// The MAC address can be anything you want but should be unique on your network.
// Newer boards have a MAC address printed on the underside of the PCB, which you can (optionally) use.
// Note that most of the Ardunio examples use  "DEAD BEEF FEED" for the MAC address.
#define MY_MAC_ADDRESS 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED

// Flash leds on rx/tx/err
#define MY_WITH_LEDS_BLINKING
// Set blinking period
#define MY_DEFAULT_LED_BLINK_PERIOD 300

// Enable inclusion mode
#define MY_INCLUSION_MODE_FEATURE
// Enable Inclusion mode button on gateway
#define MY_INCLUSION_BUTTON_FEATURE
// Set inclusion mode duration (in seconds)
#define MY_INCLUSION_MODE_DURATION 60 
// Digital pin used for inclusion mode button
#define MY_INCLUSION_MODE_BUTTON_PIN  3 

#define MY_DEFAULT_ERR_LED_PIN 7  // Error led pin
#define MY_DEFAULT_RX_LED_PIN  8  // Receive led pin
#define MY_DEFAULT_TX_LED_PIN  9  // the PCB, on board LED

#define MY_RADIO_CE_PIN        5  // radio chip enable
#define MY_RADIO_SPI_SS_PIN    6  // radio SPI serial select

#include <MySensor.h>


void setup()
{
  dataCallback();
}

void loop() {
}

