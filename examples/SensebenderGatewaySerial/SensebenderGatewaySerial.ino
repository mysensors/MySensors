/**
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
* DESCRIPTION
* The ArduinoGateway prints data received from sensors on the serial link.
* The gateway accepts input on serial which will be sent out on radio network.
*
* This GW code is designed for Sensebender GateWay / (Arduino Zero variant)
*
* Wire connections (OPTIONAL):
* - Inclusion button should be connected to SW2
*
* LEDs on board (default assignments):
* - Orange: USB RX/TX - Blink when receiving / transmitting on USB CDC device
* - Yellow: RX  - Blink fast on radio message received. In inclusion mode will blink fast only on presentation received
* - Green : TX  - Blink fast on radio message transmitted. In inclusion mode will blink slowly
* - Red   : ERR - Fast blink on error during transmission error or receive crc error
* - Blue  : free - (use with LED_BLUE macro)
*
*/

#define SKETCH_VERSION "0.2"
// Enable debug prints to serial monitor
#define MY_DEBUG

// Enable and select radio type attached
#define MY_RADIO_RF24
//#define MY_RADIO_NRF5_ESB
//#define MY_RADIO_RFM69
//#define MY_RADIO_RFM95

// Set LOW transmit power level as default, if you have an amplified NRF-module and
// power your radio separately with a good regulator you can turn up PA level.
#define MY_RF24_PA_LEVEL RF24_PA_HIGH

// Enable serial gateway
#define MY_GATEWAY_SERIAL

// Define a lower baud rate for Arduinos running on 8 MHz (Arduino Pro Mini 3.3V & Sensebender)
#if F_CPU == 8000000L
#define MY_BAUD_RATE 38400
#endif

// Enable inclusion mode
#define MY_INCLUSION_MODE_FEATURE
// Enable Inclusion mode button on gateway
#define MY_INCLUSION_BUTTON_FEATURE

// Inverses behavior of inclusion button (if using external pullup)
//#define MY_INCLUSION_BUTTON_EXTERNAL_PULLUP

// Set inclusion mode duration (in seconds)
#define MY_INCLUSION_MODE_DURATION 60
// Digital pin used for inclusion mode button
//#define MY_INCLUSION_MODE_BUTTON_PIN  3

// Set blinking period
#define MY_DEFAULT_LED_BLINK_PERIOD 300

// Inverses the behavior of leds
//#define MY_WITH_LEDS_BLINKING_INVERSE

// Flash leds on rx/tx/err
// Uncomment to override default HW configurations
//#define MY_DEFAULT_ERR_LED_PIN 4  // Error led pin
//#define MY_DEFAULT_RX_LED_PIN  6  // Receive led pin
//#define MY_DEFAULT_TX_LED_PIN  5  // the PCB, on board LED

#include <MySensors.h>
#include <SD.h>
#include <drivers/ATSHA204/ATSHA204.cpp>

Sd2Card card;

#define EEPROM_VERIFICATION_ADDRESS 0x01

static uint8_t num_of_leds = 5;
static uint8_t leds[] = {LED_BLUE, LED_RED, LED_GREEN, LED_YELLOW, LED_ORANGE};

void setup()
{
	// Setup locally attached sensors
}

void presentation()
{
	// Present locally attached sensors
}

void loop()
{
	// Send locally attached sensor data here
}


void preHwInit()
{

	pinMode(MY_SWC1, INPUT_PULLUP);
	pinMode(MY_SWC2, INPUT_PULLUP);
	if (digitalRead(MY_SWC1) && digitalRead(MY_SWC2)) {
		return;
	}

	uint8_t tests = 0;

	for (int i=0; i< num_of_leds; i++) {
		pinMode(leds[i], OUTPUT);
	}
	if (digitalRead(MY_SWC1)) {
		uint8_t led_state = 0;
		while (!Serial) {
			digitalWrite(LED_BLUE, led_state);
			led_state ^= 0x01;
			delay(500);
		} // Wait for USB to be connected, before spewing out data.
	}
	digitalWrite(LED_BLUE, LOW);
	if (Serial) {
		Serial.println("Sensebender GateWay test routine");
		Serial.print("MySensors core version : ");
		Serial.println(MYSENSORS_LIBRARY_VERSION);
		Serial.print("GateWay sketch version : ");
		Serial.println(SKETCH_VERSION);
		Serial.println("----------------------------------");
		Serial.println();
	}
	if (testSha204()) {
		digitalWrite(LED_GREEN, HIGH);
		tests++;
	}
	if (testSDCard()) {
		digitalWrite(LED_YELLOW, HIGH);
		tests++;
	}

	if (testEEProm()) {
		digitalWrite(LED_ORANGE, HIGH);
		tests++;
	}
	if (testAnalog()) {
		digitalWrite(LED_BLUE, HIGH);
		tests++;
	}
	if (tests == 4) {
		while(1) {
			for (int i=0; i<num_of_leds; i++) {
				digitalWrite(leds[i], HIGH);
				delay(200);
				digitalWrite(leds[i], LOW);
			}
		}
	} else {
		while (1) {
			digitalWrite(LED_RED, HIGH);
			delay(200);
			digitalWrite(LED_RED, LOW);
			delay(200);
		}
	}

}

bool testSha204()
{
	uint8_t rx_buffer[SHA204_RSP_SIZE_MAX];
	uint8_t ret_code;
	if (Serial) {
		Serial.print("- > SHA204 ");
	}
	atsha204_init(MY_SIGNING_ATSHA204_PIN);
	ret_code = atsha204_wakeup(rx_buffer);

	if (ret_code == SHA204_SUCCESS) {
		ret_code = atsha204_getSerialNumber(rx_buffer);
		if (ret_code != SHA204_SUCCESS) {
			if (Serial) {
				Serial.println(F("Failed to obtain device serial number. Response: "));
			}
			Serial.println(ret_code, HEX);
		} else {
			if (Serial) {
				Serial.print(F("Ok (serial : "));
				for (int i=0; i<9; i++) {
					if (rx_buffer[i] < 0x10) {
						Serial.print('0'); // Because Serial.print does not 0-pad HEX
					}
					Serial.print(rx_buffer[i], HEX);
				}
				Serial.println(")");
			}
			return true;
		}
	} else {
		if (Serial) {
			Serial.println(F("Failed to wakeup SHA204"));
		}
	}
	return false;
}

bool testSDCard()
{
	if (Serial) {
		Serial.print("- > SD CARD ");
	}
	if (!card.init(SPI_HALF_SPEED, MY_SDCARD_CS)) {
		if (Serial) {
			Serial.println("SD CARD did not initialize!");
		}
	} else {
		if (Serial) {
			Serial.print("SD Card initialized correct! - ");
			Serial.print("type detected : ");
			switch(card.type()) {
			case SD_CARD_TYPE_SD1:
				Serial.println("SD1");
				break;
			case SD_CARD_TYPE_SD2:
				Serial.println("SD2");
				break;
			case SD_CARD_TYPE_SDHC:
				Serial.println("SDHC");
				break;
			default:
				Serial.println("Unknown");
			}
		}
		return true;
	}
	return false;
}

bool testEEProm()
{
	uint8_t eeprom_d1, eeprom_d2;
	SerialUSB.print(" -> EEPROM ");
	eeprom_d1 = hwReadConfig(EEPROM_VERIFICATION_ADDRESS);
	delay(500);
	eeprom_d1 = ~eeprom_d1; // invert the bits
	hwWriteConfig(EEPROM_VERIFICATION_ADDRESS, eeprom_d1);
	delay(500);
	eeprom_d2 = hwReadConfig(EEPROM_VERIFICATION_ADDRESS);
	if (eeprom_d1 == eeprom_d2) {
		SerialUSB.println("PASSED");
		hwWriteConfig(EEPROM_VERIFICATION_ADDRESS, ~eeprom_d1);
		return true;
	}
	SerialUSB.println("FAILED!");
	return false;
}

bool testAnalog()
{
	int bat_detect = analogRead(MY_BAT_DETECT);
	Serial.print("-> analog : ");
	Serial.print(bat_detect);
	if (bat_detect < 400 || bat_detect > 650) {
		Serial.println(" Failed");
		return false;
	}
	Serial.println(" Passed");
	return true;
}
