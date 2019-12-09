/**
   The MySensors Arduino library handles the wireless radio link and protocol
   between your home built sensors/actuators and HA controller of choice.
   The sensors forms a self healing radio network with optional repeaters. Each
   repeater and gateway builds a routing tables in EEPROM which keeps track of the
   network topology allowing messages to be routed to nodes.

   Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
   Copyright (C) 2013-2019 Sensnology AB
   Full contributor list: https://github.com/mysensors/Arduino/graphs/contributors

   Documentation: http://www.mysensors.org
   Support Forum: http://forum.mysensors.org

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as published by the Free Software Foundation.

 *******************************

   REVISION HISTORY
   Version 1.0 - tekka
   Contribution by Berk

   DESCRIPTION
   The ESP32 gateway sends data received from sensors to the WiFi link.
   The gateway also accepts input on ethernet interface, which is then sent out to the radio network.

   Make sure to fill in your ssid and WiFi password below.
*/

// Enable debug prints to serial monitor
#define MY_DEBUG

// Enables and select radio type (if attached)
#define MY_RADIO_RF24
//#define MY_RADIO_RFM69
//#define MY_RADIO_RFM95

#define MY_GATEWAY_ESP32

#define MY_WIFI_SSID "MySSID"
#define MY_WIFI_PASSWORD "MyVerySecretPassword"

// Set the hostname for the WiFi Client. This is the hostname
// passed to the DHCP server if not static.
#define MY_HOSTNAME "ESP32_GW"

// Enable MY_IP_ADDRESS here if you want a static ip address (no DHCP)
//#define MY_IP_ADDRESS 192,168,1,100

// If using static ip you can define Gateway and Subnet address as well
//#define MY_IP_GATEWAY_ADDRESS 192,168,1,1
//#define MY_IP_SUBNET_ADDRESS 255,255,255,0

// The port to keep open on node server mode
#define MY_PORT 5003

// How many clients should be able to connect to this gateway (default 1)
#define MY_GATEWAY_MAX_CLIENTS 2// Set blinking period


// Advanced Gateway Options
//#define MY_DEFAULT_LED_BLINK_PERIOD 50

// Flash leds on rx/tx/err
// Led pins used if blinking feature is enabled above
//#define MY_DEFAULT_ERR_LED_PIN 32  // Transfer data error led pin
//#define MY_DEFAULT_RX_LED_PIN  25  // Receive Data led pin
//#define MY_DEFAULT_TX_LED_PIN  27  // Transmit Data led pin

//#define MY_WITH_LEDS_BLINKING_INVERSE  // At the time of Error, Receive, Transmit the pin is at a high level

#include <ArduinoOTA.h>
#include <MySensors.h>

void setup()
{
	// Setup locally attached sensors
	ArduinoOTA.onStart([]() {
		Serial.println("Start updating");
	});
	ArduinoOTA.onEnd([]() {
		Serial.println("\nEnd updating");
	});
	ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
		Serial.printf("OTA Progress: %u%%\r", (progress / (total / 100)));
	});
	ArduinoOTA.onError([](ota_error_t error) {
		Serial.printf("Error[%u]: ", error);
		if (error == OTA_AUTH_ERROR) {
			Serial.println("Auth Failed");
		} else if (error == OTA_BEGIN_ERROR) {
			Serial.println("Begin Failed");
		} else if (error == OTA_CONNECT_ERROR) {
			Serial.println("Connect Failed");
		} else if (error == OTA_RECEIVE_ERROR) {
			Serial.println("Receive Failed");
		} else if (error == OTA_END_ERROR) {
			Serial.println("End Failed");
		}
	});
	ArduinoOTA.begin();



	// Setup locally attached sensors
}

void presentation()
{
	// Present locally attached sensors here
}

void loop()
{
	// Send locally attached sensors data here

	ArduinoOTA.handle();
}