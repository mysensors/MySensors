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
 *
 *******************************
 *
 * REVISION HISTORY
 * Version 1.0 - Henrik Ekblad
 *
 * DESCRIPTION
 * The ESP8266 MQTT gateway sends radio network (or locally attached sensors) data to your MQTT broker.
 * The node also listens to MY_MQTT_TOPIC_PREFIX and sends out those messages to the radio network
 *
 * LED purposes:
 * - To use the feature, uncomment any of the MY_DEFAULT_xx_LED_PINs in your sketch
 * - RX (green) - blink fast on radio message received. In inclusion mode will blink fast only on presentation received
 * - TX (yellow) - blink fast on radio message transmitted. In inclusion mode will blink slowly
 * - ERR (red) - fast blink on error during transmission error or receive crc error
 *
 * See https://www.mysensors.org/build/connect_radio for wiring instructions.
 *
 * If you are using a "barebone" ESP8266, see
 * https://www.mysensors.org/build/esp8266_gateway#wiring-for-barebone-esp8266
 *
 * Inclusion mode button:
 * - Connect GPIO5 (=D1) via switch to GND ('inclusion switch')
 *
 * Hardware SHA204 signing is currently not supported!
 *
 * Make sure to fill in your ssid and WiFi password below for ssid & pass.
 *
 ********************************
 *
 * SSL support by Eric Grammatico. You should have an updated version of MyGatewayTransportMQTTClient.cpp.
 * Please see: https://forum.mysensors.org/topic/11941/esp8266-mqtt-gateway-ssl-connection
 *
 * The following constants have to be defined from the gateway code:
 * MY_GATEWAY_ESP8266_SECURE   In place of MY_GATEWAY_ESP8266 to go to secure connexions.
 * MY_MQTT_CA_CERTx            Up to three root Certificates Authorities could be defined
 *                              to validate the mqtt server' certificate. The most secure.
 *                              MY_MQTT_CA_CERT is deprecated and MY_MQTT_CA_CERT1 should
 *                              be used instead.
 * MY_MQTT_FINGERPRINT         Alternatively, the mqtt server' certificate finger print
 *                              could be used. Less secure and less convenient as you'll
 *                              have to update the fingerprint each time the mqtt server'
 *                              certificate is updated
 *                              If neither MY_MQTT_CA_CERT1 nor MY_MQTT_FINGERPRINT are
 *                              defined, insecure connexion will be established. The mqtt
 *                              server' certificate will not be validated.
 * MY_MQTT_CLIENT_CERT         The mqtt server may require client certificate for
 * MY_MQTT_CLIENT_KEY            authentication.
 *
 * The certs.h file holds the mqtt server' fingerprint and root Certificate Authorities and
 * client certificate and key. This a sample how to populate MY_MQTT_CA_CERTx, MY_MQTT_FINGERPRINT,
 * MY_MQTT_CLIENT_CERT and MY_MQTT_CLIENT_KEY.
 */

// Imports certificates and client key
#include "certs.h"

/**********************************
 * MySensors node configuration
 */

// General settings
#define SKETCH_NAME "MySensorsMQTTGW_Secure"
#define SKETCH_VERSION "0.6"
#define MY_DEBUG
#define MY_NODE_ID 1

// Use a bit lower baudrate for serial prints on ESP8266 than default in MyConfig.h
#define MY_BAUD_RATE 9600

// Enables and select radio type (if attached)
#define MY_RADIO_RF24
//#define MY_RF24_PA_LEVEL RF24_PA_LOW

//#define MY_RADIO_RFM69
//#define MY_RADIO_RFM95

/**************
 * Secured connexion with ESP8266
 */
#define MY_GATEWAY_ESP8266_SECURE
//** Set WIFI SSID and password
#define MY_WIFI_SSID "ssid"
#define MY_WIFI_PASSWORD "password"
//** Set the hostname for the WiFi Client. This is the hostname
//   passed to the DHCP server if not static.
#define MY_HOSTNAME "esp8266-gw"
// Enable MY_IP_ADDRESS here if you want a static ip address (no DHCP)
//#define MY_IP_ADDRESS 192,168,178,87

// If using static ip you can define Gateway and Subnet address as well
//#define MY_IP_GATEWAY_ADDRESS 192,168,178,1
//#define MY_IP_SUBNET_ADDRESS 255,255,255,0

//** Certificate Authorities. One or two should be enough
#define MY_MQTT_CA_CERT1 cert_isrgrootx1_Authority
#define MY_MQTT_CA_CERT2 cert_isrgrootx2_Authority
//#define MY_MQTT_CA_CERT3 cert_letsEncryptR3_Authority

//** Server certificate validation with its fingerprint
//   less secure and less convenient than with Certificate
//   Authorities as server certificates are updated often.
//   Will not be used if MY_MQTT_CA_CERT1 defined.
#define MY_MQTT_FINGERPRINT mqtt_fingerprint

//** The mqtt server may require client certificate for
//   authentication.
#define MY_MQTT_CLIENT_CERT cert_client
#define MY_MQTT_CLIENT_KEY key_client


/**************
 * MQTT_CLIENT configuration
 */
#define MY_GATEWAY_MQTT_CLIENT

//** MQTT broker if using URL instead of ip address.
//   should correspond to the CN field in the mqtt server'
//   certificate.
#define MY_CONTROLLER_URL_ADDRESS mqtt_host

//** The MQTT broker port to open
#define MY_PORT mqtt_port

//** Enable these if your MQTT broker requires username/password
//#define MY_MQTT_USER "<mqtt-user>"
//#define MY_MQTT_PASSWORD "<mqtt-passwd>"
//** Set MQTT client id
//#define MY_MQTT_CLIENT_ID "<mqtt-userID>"

//** Set this node's subscribe and publish topic prefix
#define MY_MQTT_PUBLISH_TOPIC_PREFIX "esp8266-gw/out"
#define MY_MQTT_SUBSCRIBE_TOPIC_PREFIX "esp8266-gw/in"


// Enable inclusion mode
//#define MY_INCLUSION_MODE_FEATURE
// Enable Inclusion mode button on gateway
//#define MY_INCLUSION_BUTTON_FEATURE
// Set inclusion mode duration (in seconds)
//#define MY_INCLUSION_MODE_DURATION 60
// Digital pin used for inclusion mode button
//#define MY_INCLUSION_MODE_BUTTON_PIN D1

// Set blinking period
//#define MY_DEFAULT_LED_BLINK_PERIOD 300

// Flash leds on rx/tx/err
//#define MY_DEFAULT_ERR_LED_PIN 16  // Error led pin
//#define MY_DEFAULT_RX_LED_PIN  16  // Receive led pin
//#define MY_DEFAULT_TX_LED_PIN  16  // the PCB, on board LED

#include <MySensors.h>

void setup()
{

	// In order to speed up certs and keys verifications
	system_update_cpu_freq(160);

	// Setup locally attached sensors
}

void presentation()
{
	// Present locally attached sensors here
}

void loop()
{
	// Send locally attached sensors data here
}

