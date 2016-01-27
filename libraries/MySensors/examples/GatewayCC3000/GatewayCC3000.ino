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
 * DESCRIPTION
 * The ArduinoGateway prints data received from sensors on the serial link. 
 * The gateway accepts input on seral which will be sent out on radio network.
 *
 * The GW code is designed for Arduino Nano 328p / 16MHz
 *
 * Wire connections (OPTIONAL):
 * - Inclusion button should be connected between digital pin 3 and GND  
 * - RX/TX/ERR leds need to be connected between +5V (anode) and digital pin 6/5/4 with resistor 270-330R in a series
 *
 * LEDs (OPTIONAL):
 * - To use the feature, uncomment MY_LEDS_BLINKING_FEATURE in MyConfig.h
 * - RX (green) - blink fast on radio message recieved. In inclusion mode will blink fast only on presentation recieved
 * - TX (yellow) - blink fast on radio message transmitted. In inclusion mode will blink slowly
 * - ERR (red) - fast blink on error during transmission error or recieve crc error 
 * 
 */

// Enable debug prints to serial monitor
#define MY_DEBUG 

//Set the CE and CSN pins for the radio 
#define MY_RF24_CE_PIN 4
#define MY_RF24_CS_PIN 5

// Enable and select radio type attached
#define MY_RADIO_NRF24
//#define MY_RADIO_RFM69

// Set LOW transmit power level as default, if you have an amplified NRF-module and
// power your radio separately with a good regulator you can turn up PA level. 
#define MY_RF24_PA_LEVEL RF24_PA_LOW

// Enable CC3000 gateway
#define MY_GATEWAY_CC3000
#include <Adafruit_CC3000.h>
#include <Adafruit_CC3000_Server.h>

//change to match your network
#define MY_CC3000_SSID "MySSID"
#define MY_CC3000_PASSWORD "MyVerySecretPassword"
#define MY_WLAN_SECURITY   WLAN_SEC_WPA2
//default port and max clients. 
#define MY_PORT 5003  
#define MY_GATEWAY_MAX_CLIENTS 2

// Flash leds on rx/tx/err
#define MY_LEDS_BLINKING_FEATURE
// Set blinking period
#define MY_DEFAULT_LED_BLINK_PERIOD 300

// Enable inclusion mode
#define MY_INCLUSION_MODE_FEATURE
// Enable Inclusion mode button on gateway
#define MY_INCLUSION_BUTTON_FEATURE

// Inverses behavior of inclusion button (if using external pullup)
//#define MY_INCLUSION_BUTTON_EXTERNAL_PULLUP

// Set inclusion mode duration (in seconds)
#define MY_INCLUSION_MODE_DURATION 60 

#define MY_INCLUSION_MODE_BUTTON_PIN  A3 // Digital pin used for inclusion mode button
#define MY_DEFAULT_ERR_LED_PIN A0  // Error led pin
#define MY_DEFAULT_RX_LED_PIN  A1  // Receive led pin
#define MY_DEFAULT_TX_LED_PIN  A2  // the PCB, on board LED

//Set the CE and CSN and IRQ pins for the Wifi Module 
#define MY_CC3000_CS 6
#define MY_CC3000_VBAT 7
#define MY_CC3000_IRQ 3

#include <SPI.h>
#include <MySensor.h>  

void setup() { 
  // Setup locally attached sensors
}

void presentation() {
 // Present locally attached sensors 
}

void loop() { 
  // Send locally attached sensor data here 
}





