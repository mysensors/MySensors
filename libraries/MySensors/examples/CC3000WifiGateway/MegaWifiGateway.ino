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
#define NO_PORTB_PINCHANGES 

//#include <DigitalIO.h>     // This include can be removed when using UIPEthernet module  
#include <SPI.h>  

#include <MySigningNone.h>
#include <MyTransportRFM69.h>
#include <MyTransportNRF24.h>
#include <MyHwATMega328.h>


#include <MyParserSerial.h>  
#include <MySensor.h>  
#include <stdarg.h>
#include <PinChangeInt.h>
#include "GatewayUtil.h"

//wifi module stuff
//#include <string.h>
//#include "utility/debug.h"
#include <Adafruit_CC3000.h>
//#include <ccspi.h>

#define INCLUSION_MODE_TIME 1 // Number of minutes inclusion mode is enabled
#define INCLUSION_MODE_PIN  22 // Digital pin used for inclusion mode button

#define RADIO_CE_PIN        5  // radio chip enable
#define RADIO_SPI_SS_PIN    6  // radio SPI serial select

#define RADIO_ERROR_LED_PIN 23  // Error led pin
#define RADIO_RX_LED_PIN    24  // Receive led pin
#define RADIO_TX_LED_PIN    25  // the PCB, on board LED

//WiFi module settings and defines
// These are the interrupt and control pins
#define ADAFRUIT_CC3000_IRQ   3  // MUST be an interrupt pin!
// These can be any two pins
#define ADAFRUIT_CC3000_VBAT  4
#define ADAFRUIT_CC3000_CS    10
// Use hardware SPI for the remaining pins
Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT,
                                         SPI_CLOCK_DIVIDER); // you can change this clock speed but DI

#define WLAN_SSID       "EKG-DevMain"        // cannot be longer than 32 characters!
#define WLAN_PASS       "BigSackLunch"
// Security can be WLAN_SEC_UNSEC, WLAN_SEC_WEP, WLAN_SEC_WPA or WLAN_SEC_WPA2
#define WLAN_SECURITY   WLAN_SEC_WPA2


// NRFRF24L01 radio driver (set low transmit power by default) 
MyTransportNRF24 transport(5, 6, RF24_PA_LEVEL_GW);  

// Hardware profile 
MyHwATMega328 hw;

// Construct MySensors library (signer needed if MY_SIGNING_FEATURE is turned on in MyConfig.h)
// To use LEDs blinking, uncomment WITH_LEDS_BLINKING in MyConfig.h
#ifdef WITH_LEDS_BLINKING
MySensor gw(transport, hw /*, signer*/, RADIO_RX_LED_PIN, RADIO_TX_LED_PIN, RADIO_ERROR_LED_PIN);
#else
MySensor gw(transport, hw /*, signer*/);
#endif


#define IP_PORT 5003        // The port you want to open 

// a R/W server on the port
Adafruit_CC3000_Server server = Adafruit_CC3000_Server(IP_PORT);
Adafruit_CC3000_ClientRef client = new Adafruit_CC3000_Client();

char inputString[MAX_RECEIVE_LENGTH] = "";    // A string to hold incoming commands from serial/ethernet interface
int inputPos = 0;
bool sentReady = false;

void output(const char *fmt, ... ) {
   va_list args;
   va_start (args, fmt );
   vsnprintf_P(serialBuffer, MAX_SEND_LENGTH, fmt, args);
   va_end (args);
   Serial.print(serialBuffer);
   server.write(serialBuffer);
}

boolean cc300_Connect() {
  /* Initialise the module */
  Serial.println(F("Initializing Wifi Module..."));
  if (!cc3000.begin())
  {
    while(1);
  }
  Serial.println(F("Done."));

  /* Connect to supplied network */
  Serial.println(F("Connecting to Network..."));
  if (!cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY)) {
    while(1);
  }
  Serial.println(F("Done."));
  
  /* Wait for DHCP to complete */  
  Serial.println(F("Getting IP address from DHCP...")); 
  while (!cc3000.checkDHCP())
  {
    delay(100); // ToDo: Insert a DHCP timeout!
  }  
  Serial.println(F("Done."));
  delay(100);
  Serial.println(F("Connected!"));
}

void reset_server(){
   Serial.println(F("========================================================"));
   Serial.println(F("Resetting Server...."));
   client.stop();
   Serial.println(F("Server Reset."));
   Serial.println(F("========================================================"));
}
int connector;

void setup()  
{ 
  Serial.begin(115200);
  //start the gateway.
  Serial.println(F("Starting Gateway"));
  delay(500);
  gw.begin(incomingMessage, 0, true, 0);
  setupGateway(INCLUSION_MODE_PIN, INCLUSION_MODE_TIME, output);
  
  // Add interrupt for inclusion button to pin
  PCintPort::attachInterrupt(pinInclusion, startInclusionInterrupt, RISING); 
  
  /* Connect to wifi */
  delay(1000);
  cc300_Connect();
  delay(5000);
   
  // start listening for clients
  server.begin();
 
  // This next line should not need to be printed in setup. 
  //output(PSTR("0;0;%d;0;%d;Gateway startup complete.\n"),  C_INTERNAL, I_GATEWAY_READY);
  Serial.println(F("Setup Complete!"));
connector = 0;
}


void loop() {
  gw.process();    
  checkButtonTriggeredInclusion();
  checkInclusionFinished();
  
  Adafruit_CC3000_ClientRef newclient = server.available();
  // if a new client connects make sure to dispose any previous existing sockets
  if (newclient) {
      if (client != newclient) {
       client.stop();
       client = newclient;
       Serial.println(F("========================================================"));
       Serial.println(F("Got new client. "));
       Serial.println(F("Disconnecting any previous client and resetting gateway."));
       output(PSTR("0;0;%d;0;%d;Gateway startup complete.\n"),  C_INTERNAL, I_GATEWAY_READY);
       Serial.println(F("========================================================"));
       connector = 1;
     }
   }

   //kill the client so someone else can reconnect. this makes sure we can always reconnect if the controller goes controller goes down for some reason.
   if (!client.connected()) {
    //if its the first run 
    if(connector == 0){
      Serial.println(F("no client"));
      }  
         
    else{
      Serial.println(F("Disconnected."));
      client.stop();
    }
   }


   if (client) {
     if (!client.connected()) {
      Serial.println(F("Disconnecting client"));
       client.stop();
     } else if (client.available()) { 
       // read the bytes incoming from the client
       char inChar = client.read();
       if (inputPos<MAX_RECEIVE_LENGTH-1) { 
         // if newline then command is complete
         if (inChar == '\n') {  
           Serial.println("Finished");
            // a command was issued by the client
            // we will now try to send it to the actuator
            inputString[inputPos] = 0;

            //change inputString into something we can wrap text around.
            String inString = inputString;         
            Serial.println("Sending " + inString + " To Gateway");
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
  
}



