/*
 * Copyright (C) 2013 Henrik Ekblad <henrik.ekblad@gmail.com>
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 * 
 * DESCRIPTION
 * The ArduinoGateway prints data received from sensors on the serial link. 
 * The gateway accepts input on seral which will be sent out on radio network.
 *
 * The GW code is designed for Arduino Nano 328p / 16MHz
 *
 * Wire connections (OPTIONAL):
 * - Inclusion button should be connected between digital pin 3 and GND
 * - RX/TX/ERR leds need to be connected between +5V (anode) and digital ping 6/5/4 with resistor 270-330R in a series
 *
 * LED purposes:
 * - RX (green) - blink fast on radio message recieved. In inclusion mode will blink fast only on presentation recieved
 * - TX (yellow) - blink fast on radio message transmitted. In inclusion mode will blink slowly
 * - ERR (red) - fast blink on error during transmission error or recieve crc error  
 */

#include <SPI.h>  
#include <EEPROM.h>  
#include <RF24.h>
#include <MsTimer2.h>
#include <PinChangeInt.h>
#include <Gateway.h>  
#include <stdarg.h>
#include <avr/progmem.h>

#define INCLUSION_MODE_TIME 1 // Number of minutes inclusion mode is enabled
#define INCLUSION_MODE_PIN 3 // Digital pin used for inclusion mode button


// No blink or button functionality. Use the vanilla constructor.
//Gateway gw;

// To start gateway with include button and led blinking functionality enabled use this instead!
Gateway gw(9, 10, INCLUSION_MODE_TIME, INCLUSION_MODE_PIN,  6, 5, 4);


char inputString[MAX_RECEIVE_LENGTH] = "";    // A string to hold incoming commands from serial/ethernet interface
int inputPos = 0;
boolean commandComplete = false;  // whether the string is complete

void setup()  
{ 
  gw.begin();

  // C++ classes and interrupts really sucks. Need to attach interrupt 
  // outside thw Gateway class due to language shortcomings! Gah! 

  if (gw.isLedMode()) {
    // Add led timer interrupt
    MsTimer2::set(300, ledTimersInterrupt);
    MsTimer2::start();
    // Add interrupt for inclustion button to pin 
    PCintPort::attachInterrupt(INCLUSION_MODE_PIN, startInclusionInterrupt, RISING);
  }
}

void loop()  
{ 
  gw.processRadioMessage();   
  checkSerialInput();
  
}


void startInclusionInterrupt() {
  gw.startInclusionInterrupt();
}

void ledTimersInterrupt() {
  gw.ledTimersInterrupt();
}

void checkSerialInput() {
  if (commandComplete) {
    // A command wass issued from serial interface
    // We will now try to send it to the actuator
    gw.parseAndSend(inputString);
    commandComplete = false;  
    inputPos = 0;
  }
}

/*
  SerialEvent occurs whenever a new data comes in the
 hardware serial RX.  This routine is run between each
 time loop() runs, so using delay inside loop can delay
 response.  Multiple bytes of data may be available.
 */
void serialEvent() {
  while (Serial.available()) {
    // get the new byte:
    char inChar = (char)Serial.read(); 
    // if the incoming character is a newline, set a flag
    // so the main loop can do something about it:
    if (inputPos<MAX_RECEIVE_LENGTH-1 && !commandComplete) { 
      if (inChar == '\n') {
        inputString[inputPos] = 0;
        commandComplete = true;
      } else {
        // add it to the inputString:
        inputString[inputPos] = inChar;
        inputPos++;
      }
    } else {
       // Incoming message too long. Throw away 
        inputPos = 0;
    }
  }
}


