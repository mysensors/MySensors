  /*
 * Copyright (C) 2013-2014 Henrik Ekblad <henrik.ekblad@gmail.com>
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
 * - RX/TX/ERR leds need to be connected between +5V (anode) and digital pin 6/5/4 with resistor 270-330R in a series
 *
 * LEDs (OPTIONAL):
 * - RX (green) - blink fast on radio message recieved. In inclusion mode will blink fast only on presentation recieved
 * - TX (yellow) - blink fast on radio message transmitted. In inclusion mode will blink slowly
 * - ERR (red) - fast blink on error during transmission error or recieve crc error  
 */

#define NO_PORTB_PINCHANGES  

#include <MySigningDriverNone.h>
#include <MyRFDriverNRF24.h>
#include <SPI.h>  
#include <MyParserSerial.h>  
#include <MySensor.h>  
#include <stdarg.h>
#include <MsTimer2.h>
#include <PinChangeInt.h>
#include "GatewayUtil.h"

#define INCLUSION_MODE_TIME 1 // Number of minutes inclusion mode is enabled
#define INCLUSION_MODE_PIN  3 // Digital pin used for inclusion mode button
#define RADIO_ERROR_LED_PIN 4  // Error led pin
#define RADIO_RX_LED_PIN    6  // Receive led pin
#define RADIO_TX_LED_PIN    5  // the PCB, on board LED


// NRFRF24L01 radio driver (set low transmit power by default) 
MyRFDriverNRF24 radio(RF24_CE_PIN, RF24_CS_PIN, RF24_PA_LEVEL_GW);  
// Message signing driver (none default)
MySigningDriverNone signer;
// Construct MySensors library
MySensor gw(radio, signer);

char inputString[MAX_RECEIVE_LENGTH] = "";    // A string to hold incoming commands from serial/ethernet interface
int inputPos = 0;
boolean commandComplete = false;  // whether the string is complete

void parseAndSend(char *commandBuffer);

void output(const char *fmt, ... ) {
   va_list args;
   va_start (args, fmt );
   vsnprintf_P(serialBuffer, MAX_SEND_LENGTH, fmt, args);
   va_end (args);
   Serial.print(serialBuffer);
}

  
void setup()  
{ 
  gw.begin(incomingMessage, 0, true, 0);

  setupGateway(RADIO_RX_LED_PIN, RADIO_TX_LED_PIN, RADIO_ERROR_LED_PIN, INCLUSION_MODE_PIN, INCLUSION_MODE_TIME, output);  

  // Add led timer interrupt
  MsTimer2::set(300, ledTimersInterrupt);
  MsTimer2::start();

  // Add interrupt for inclusion button to pin
  PCintPort::attachInterrupt(pinInclusion, startInclusionInterrupt, RISING);


  // Send startup log message on serial
  serial(PSTR("0;0;%d;0;%d;Gateway startup complete.\n"),  C_INTERNAL, I_GATEWAY_READY);
}

void loop()  
{ 
  gw.process();

  checkButtonTriggeredInclusion();
  checkInclusionFinished();
  
  if (commandComplete) {
    // A command wass issued from serial interface
    // We will now try to send it to the actuator
    parseAndSend(gw, inputString);
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



