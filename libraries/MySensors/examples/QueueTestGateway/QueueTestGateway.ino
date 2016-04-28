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

#define MY_RADIO_NRF24
#define MY_RF24_CHANNEL  (100)
#define MY_RF24_DATARATE (RF24_1MBPS)
#define MY_GATEWAY_SERIAL
#define MY_LEDS_BLINKING_FEATURE
#define MY_DEFAULT_LED_BLINK_PERIOD 50
#define MY_WITH_LEDS_BLINKING_INVERSE
#define MY_RF24_IRQ_PIN  (2)

#define MY_DEFAULT_ERR_LED_PIN A0  // Error led pin
#define MY_DEFAULT_RX_LED_PIN  A1  // Receive led pin
#define MY_DEFAULT_TX_LED_PIN  A2  // the PCB, on board LED

//#define MY_DEBUG
//#define MY_DEBUG_VERBOSE_RF24

#include <SPI.h>
#include <MySensor.h>  

void setup() { 
}

void presentation() {
}


void receive(const MyMessage &message)
{
  static uint16_t prevCount = 0;
  uint16_t count = message.getUInt();
  if ((count-1) != prevCount)
  {
    Serial.print(F("Mismatch: prev="));
    Serial.print(prevCount);
    Serial.print(F("  new="));
    Serial.println(count);
  }
  prevCount = count;
}

const uint8_t maxCount = 100;

void loop() { 
  static uint8_t count = 0;
  ++count;
  if (count++ > maxCount)
  {
    count = 0;
    delay(1000);
  }
}





