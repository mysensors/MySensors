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

#include <MySigningNone.h>
#include <MyTransportRFM69.h>
#include <MyTransportNRF24.h>
#include <MyHwATMega328.h>
#include <MySigningAtsha204Soft.h>
#include <MySigningAtsha204.h>

#include <SPI.h>  
#include <MyGatewayTransportSerial.h>  
#include <MyProtocolDefault.h>  
#include <MyGateway.h>  

#define INCLUSION_MODE_TIME 60 // Number of seconds inclusion mode is enabled
#define INCLUSION_MODE_PIN  3 // Digital pin used for inclusion mode button
#define RADIO_ERROR_LED_PIN 4  // Error led pin
#define RADIO_RX_LED_PIN    6  // Receive led pin
#define RADIO_TX_LED_PIN    5  // the PCB, on board LED

// NRFRF24L01 radio driver (set low transmit power by default) 
MyTransportNRF24 transport(RF24_CE_PIN, RF24_CS_PIN, RF24_PA_LEVEL_GW);
//MyTransportRFM69 transport;

// Message signing driver (signer needed if MY_SIGNING_FEATURE is turned on in MyConfig.h)
//MySigningNone signer;
//MySigningAtsha204Soft signer;
//MySigningAtsha204 signer;

// Hardware profile 
MyHwATMega328 hw;

// Controller Transport profile
MyProtocolDefault protocol;
MyGatewayTransportSerial ctrlTransport(protocol);

// Construct MySensors library (signer needed if MY_SIGNING_FEATURE is turned on in MyConfig.h)
// To use LEDs blinking, uncomment MY_LEDS_BLINKING_FEATURE in MyConfig.h
#ifdef MY_LEDS_BLINKING_FEATURE
MyGateway gw(ctrlTransport, transport, hw /*, signer*/, RADIO_RX_LED_PIN, RADIO_TX_LED_PIN, RADIO_ERROR_LED_PIN);
#else
MyGateway gw(ctrlTransport, transport, hw /*, signer*/);
#endif

void setup()  
{ 
  gw.begin(NULL
  #ifdef MY_INCLUSION_MODE_FEATURE
				, INCLUSION_MODE_TIME
  #ifdef MY_INCLUSION_BUTTON_FEATURE
				, INCLUSION_MODE_PIN
  #endif
  #endif
  );

}

void loop()  
{ 
  gw.process();
}





