
/*
 * Copyright (C) 2013 Henrik Ekblad <henrik.ekblad@gmail.com>
 * 
 * Contribution by a-lurker and Anticimex
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 * 
 * DESCRIPTION
 * The EthernetGateway sends data received from sensors to the ethernet link. 
 * The gateway also accepts input on ethernet interface, which is then sent out to the radio network.
 *
 * The GW code is designed for Arduino 328p / 16MHz.  ATmega168 does not have enough memory to run this program.
 * 
 * COMPILING ENC28J60
 * > Use Arduino IDE 1.5.7 (or later) 
 * > Disable DEBUG in Sensor.h before compiling this sketch. Othervise the sketch will probably not fit in program space when downloading. 
 * > Remove Ethernet.h include below and inlcude UIPEthernet.h 
 * COMPILING WizNET (W5100)
 * > Remove UIPEthernet include below and include Ethernet.h.  
 *
 * Note that I had to disable UDP and DHCP support in uipethernet-conf.h to reduce space. (which meas you ave to choose a static IP for that module)

 * VERA CONFIGURATION:
 * Enter "ip-number:port" in the ip-field of the Arduino GW device. This will temporarily override any serial configuration for the Vera plugin. 
 * E.g. If you want to use the defualt values in this sketch enter: 192.168.178.66:5003
 *
 * LED purposes:
 * - RX (green) - blink fast on radio message recieved. In inclusion mode will blink fast only on presentation recieved
 * - TX (yellow) - blink fast on radio message transmitted. In inclusion mode will blink slowly
 * - ERR (red) - fast blink on error during transmission error or recieve crc error  
 * 
 *  ----------- Connection guide ---------------------------------------------------------------------------
 *  13  Radio & Ethernet SPI SCK          
 *  12  Radio & Ethernet SPI MISO (SO)
 *  11  Radio & Ethernet SPI MOSI (SI)
 *  10  Ethernet SPI Slave Select (CS)    Pin 10, the SS pin, must be an o/p to put SPI in master mode
 *  9   Radio TX LED using on board LED   (optional)  +5V -> LED -> 270-330 Ohm resistor -> pin 9.
 *  8   Radio RX LED                      (optional)  +5V -> LED -> 270-330 Ohm resistor -> pin 8.
 *  7   Radio error LED                   (optional)  +5V -> LED -> 270-330 Ohm resistor -> pin 7.
 *  6   Radio SPI Slave Select
 *  5   Radio Chip Enable
 *  4   Ethernet SPI Enable               (optional if using a shield/module that manages SPI_EN signal)
 *  3   Inclusion mode button             (optional) GND-> Button -> Pin3
 *  2   Radio IRQ pin                     (optional) 
 * ----------------------------------------------------------------------------------------------------------- 
 * Powering: both NRF24l01 radio and Ethernet(ENC28J60) uses 3.3V
 */
#define NO_PORTB_PINCHANGES 

#include <SPI.h>  
#include <MySensor.h>  
#include <stdarg.h>
#include <MsTimer2.h>
#include <PinChangeInt.h>
#include "GatewayUtil.h"

#ifndef MYSENSORS_ETHERNET_MQTT_GATEWAY
#error Please switch to MYSENSORS_ETHERNET_MQTT_GATEWAY in MyConfig.h
#endif

// Use this if you have attached a Ethernet ENC28J60 shields  
#include <UIPEthernet.h>  

// Use this fo WizNET module and Arduino Ethernet Shield 
//#include <Ethernet.h>   


#define INCLUSION_MODE_TIME 1 // Number of minutes inclusion mode is enabled
#define INCLUSION_MODE_PIN  3 // Digital pin used for inclusion mode button

#define W5100_SPI_EN        4  // Ethernet SPI enable (where available)
#define RADIO_CE_PIN        5  // radio chip enable
#define RADIO_SPI_SS_PIN    6  // radio SPI serial select

#define RADIO_ERROR_LED_PIN 7  // Error led pin
#define RADIO_RX_LED_PIN    8  // Receive led pin
#define RADIO_TX_LED_PIN    9  // the PCB, on board LED


MySensor gw;


#define IP_PORT 5003        // The port you want to open 
IPAddress myIp (192, 168, 178, 66);  // Configure your static ip-address here    COMPILE ERROR HERE? Use Arduino IDE 1.5.7 or later!

// The MAC address can be anything you want but should be unique on your network.
// Newer boards have a MAC address printed on the underside of the PCB, which you can (optionally) use.
// Note that most of the Ardunio examples use  "DEAD BEEF FEED" for the MAC address.
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED };  // DEAD BEEF FEED

// a R/W server on the port
EthernetServer server = EthernetServer(IP_PORT);


char inputString[MAX_RECEIVE_LENGTH] = "";    // A string to hold incoming commands from serial/ethernet interface
int inputPos = 0;

void w5100_spi_en(boolean enable)
{
#ifdef W5100_SPI_EN
  if (enable)
  {
    // Pull up pin
    pinMode(W5100_SPI_EN, INPUT);
    digitalWrite(W5100_SPI_EN, HIGH);
  }
  else
  {
    // Ground pin
    pinMode(W5100_SPI_EN, OUTPUT);
    digitalWrite(W5100_SPI_EN, LOW);
  }
#endif
}

void output(const char *fmt, ... ) {
   va_list args;
   va_start (args, fmt );
   vsnprintf_P(serialBuffer, MAX_SEND_LENGTH, fmt, args);
   va_end (args);
   Serial.print(serialBuffer);
   w5100_spi_en(true);
   server.write(serialBuffer);
   w5100_spi_en(false);
}

void setup()  
{ 
  w5100_spi_en(false);
  // Initialize gateway at maximum PA level, channel 70 and callback for write operations 
  gw.begin(incomingMessage, 0, true, 0);
  w5100_spi_en(true);

  Ethernet.begin(mac, myIp);
  setupGateway(RADIO_RX_LED_PIN, RADIO_TX_LED_PIN, RADIO_ERROR_LED_PIN, INCLUSION_MODE_PIN, INCLUSION_MODE_TIME, output);

  // Add led timer interrupt
  MsTimer2::set(300, ledTimersInterrupt);
  MsTimer2::start();

  // Add interrupt for inclusion button to pin
  PCintPort::attachInterrupt(pinInclusion, startInclusionInterrupt, RISING);

  // give the Ethernet interface a second to initialize
  delay(1000);
  
  // start listening for clients
  server.begin();
  w5100_spi_en(false);

  //output(PSTR("0;0;%d;0;%d;Gateway startup complete.\n"),  C_INTERNAL, I_GATEWAY_READY);

}


void loop()
{
  gw.process();  
  
  
  checkButtonTriggeredInclusion();
  checkInclusionFinished();

  // if an incoming client connects, there will be
  // bytes available to read via the client object
  w5100_spi_en(true);
  EthernetClient client = server.available();

  if (client) {

    // if got 1 or more bytes
      if (client.available()) {
         // read the bytes incoming from the client
         char inChar = client.read();
         if (inputPos<MAX_RECEIVE_LENGTH-1) { 
           // if newline then command is complete
           if (inChar == '\n') {  
             Serial.println("Finished");
              // a command was issued by the client
              // we will now try to send it to the actuator
              inputString[inputPos] = 0;

              // echo the string to the serial port
              Serial.print(inputString);

              w5100_spi_en(false);
              parseAndSend(gw, inputString);

              // clear the string:
              inputPos = 0;
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
  w5100_spi_en(false);
}


