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


#define MY_WITH_LEDS_BLINKING
#define MY_INCLUSION_MODE_FEATURE
#define MY_INCLUSION_BUTTON_FEATURE

#define MY_DEFAULT_LED_BLINK_PERIOD 300

#define MY_INCLUSION_MODE_DURATION 60 // Number of minutes inclusion mode is enabled
#define MY_INCLUSION_MODE_BUTTON_PIN  3 // Digital pin used for inclusion mode button

#define MY_DEFAULT_ERR_LED_PIN 7  // Error led pin
#define MY_DEFAULT_RX_LED_PIN  8  // Receive led pin
#define MY_DEFAULT_TX_LED_PIN  9  // the PCB, on board LED

#define MY_RADIO_CE_PIN        5  // radio chip enable
#define MY_RADIO_SPI_SS_PIN    6  // radio SPI serial select

#include <MySensor.h>


#define MY_IP_PORT 5003        // The port you want to open 
#ifndef MY_IP_ADDRESS_DHCP
// Gateway IP address if not using DHCP
IPAddress myIp (192, 168, 178, 66);  // Configure your static ip-address here    COMPILE ERROR HERE? Use Arduino IDE 1.5.7 or later!
#endif /* IP_ADDRESS_DHCP */

// NRFRF24L01 radio driver (set low transmit power by default) 
MyTransportNRF24 transport(RADIO_CE_PIN, RADIO_SPI_SS_PIN, RF24_PA_LEVEL_GW);  
//MyTransportRFM69 transport;

// Message signing driver (signer needed if MY_SIGNING_FEATURE is turned on in MyConfig.h)
//MySigningNone signer;
//MySigningAtsha204Soft signer;
//MySigningAtsha204 signer;

// Hardware profile
MyHwATMega328 hw;

// Use default controller protocol
MyProtocolDefault protocol;

// The MAC address can be anything you want but should be unique on your network.
// Newer boards have a MAC address printed on the underside of the PCB, which you can (optionally) use.
// Note that most of the Ardunio examples use  "DEAD BEEF FEED" for the MAC address.
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };  // DEAD BEEF FEED

// Controller IP
IPAddress controllerIP = IPAddress(192, 168, 178, 254);

// Gateway transport driver
#ifdef IP_ADDRESS_DHCP
MyGatewayTransportEthernet ctrlTransport(protocol, mac, /* gw port */ CONTROLLER_PORT, IP_RENEWAL_INTERVAL, controllerIP, CONTROLLER_PORT);
#else
MyGatewayTransportEthernet ctrlTransport(protocol, mac, myIp, /* gw port */ CONTROLLER_PORT, controllerIP, CONTROLLER_PORT);
#endif


// Construct MyGateway library (signer needed if MY_SIGNING_FEATURE is turned on in MyConfig.h)
// To use LEDs blinking, uncomment MY_LEDS_BLINKING_FEATURE in MyConfig.h
MyGateway gw(ctrlTransport, transport, hw /*, signer*/);

void setup()
{
  gw.begin(NULL);
}

void loop() {
  gw.process();
}

