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
 * Contribution by Ivo Pullens (ESP8266 support)
 * 
 * DESCRIPTION
 * The EthernetGateway sends data received from sensors to the WiFi link. 
 * The gateway also accepts input on ethernet interface, which is then sent out to the radio network.
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
 * The ESP8266 however requires different wiring:
 * nRF24L01+  ESP8266
 * VCC        VCC
 * CE         GPIO4          
 * CSN/CS     GPIO15
 * SCK        GPIO14
 * MISO       GPIO12
 * MOSI       GPIO13
 *            
 * Not all ESP8266 modules have all pins available on their external interface.
 * This code has been tested on an ESP-12 module.
 * The ESP8266 requires a certain pin configuration to download code, and another one to run code:
 * - Connect REST (reset) via 10K pullup resistor to VCC, and via switch to GND ('reset switch')
 * - Connect GPIO15 via 10K pulldown resistor to GND
 * - Connect CH_PD via 10K resistor to VCC
 * - Connect GPIO2 via 10K resistor to VCC
 * - Connect GPIO0 via 10K resistor to VCC, and via switch to GND ('bootload switch')
 * 
  * Inclusion mode button:
 * - Connect GPIO5 via switch to GND ('inclusion switch')
 * 
 * Hardware SHA204 signing is currently not supported!
 *
 * Make sure to fill in your ssid and WiFi password below for ssid & pass.
 */
#define NO_PORTB_PINCHANGES 

#include <SPI.h>  

#include <MySigningNone.h> 
#include <MySigningAtsha204Soft.h>
#include <MyTransportNRF24.h>
#include <MyTransportRFM69.h>
#include <EEPROM.h>
#include <MyHwESP8266.h>
#include <ESP8266WiFi.h>

#include <MyParserSerial.h>  
#include <MySensor.h>  
#include <stdarg.h>
#include "GatewayUtil.h"

const char *ssid =  "MySSID";    // cannot be longer than 32 characters!
const char *pass =  "MyVerySecretPassword"; //

#define INCLUSION_MODE_TIME 1 // Number of minutes inclusion mode is enabled
#define INCLUSION_MODE_PIN  5 // Digital pin used for inclusion mode button

#define RADIO_CE_PIN        4   // radio chip enable
#define RADIO_SPI_SS_PIN    15  // radio SPI serial select

#ifdef WITH_LEDS_BLINKING
#define RADIO_ERROR_LED_PIN 7  // Error led pin
#define RADIO_RX_LED_PIN    8  // Receive led pin
#define RADIO_TX_LED_PIN    9  // the PCB, on board LED
#endif


// NRFRF24L01 radio driver (set low transmit power by default) 
MyTransportNRF24 transport(RADIO_CE_PIN, RADIO_SPI_SS_PIN, RF24_PA_LEVEL_GW);
//MyTransportRFM69 transport;


// Message signing driver (signer needed if MY_SIGNING_FEATURE is turned on in MyConfig.h)
#ifdef MY_SIGNING_FEATURE
MySigningNone signer;
//MySigningAtsha204Soft signer;
#endif

// Hardware profile 
MyHwESP8266 hw;

// Construct MySensors library (signer needed if MY_SIGNING_FEATURE is turned on in MyConfig.h)
// To use LEDs blinking, uncomment WITH_LEDS_BLINKING in MyConfig.h
MySensor gw(transport, hw
#ifdef MY_SIGNING_FEATURE
    , signer
#endif
#ifdef WITH_LEDS_BLINKING
  , RADIO_RX_LED_PIN, RADIO_TX_LED_PIN, RADIO_ERROR_LED_PIN
#endif
  );
  

#define IP_PORT 5003         // The port you want to open 
#define MAX_SRV_CLIENTS 5    // how many clients should be able to telnet to this ESP8266

// a R/W server on the port
static WiFiServer server(IP_PORT);
static WiFiClient clients[MAX_SRV_CLIENTS];
static bool clientsConnected[MAX_SRV_CLIENTS];
static inputBuffer inputString[MAX_SRV_CLIENTS];

#define ARRAY_SIZE(x)  (sizeof(x)/sizeof(x[0]))


void output(const char *fmt, ... )
{
  char serialBuffer[MAX_SEND_LENGTH];
  va_list args;
  va_start (args, fmt );
  vsnprintf_P(serialBuffer, MAX_SEND_LENGTH, fmt, args);
  va_end (args);
  Serial.print(serialBuffer);
  for (uint8_t i = 0; i < ARRAY_SIZE(clients); i++)
  {
    if (clients[i] && clients[i].connected())
    {
//       Serial.print("Client "); Serial.print(i); Serial.println(" write");
       clients[i].write((uint8_t*)serialBuffer, strlen(serialBuffer));
    }
  }
}

void setup()  
{ 
  // Setup console
  hw_init();

  Serial.println(); Serial.println();
  Serial.println("ESP8266 MySensors Gateway");
  Serial.print("Connecting to "); Serial.println(ssid);

  (void)WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected!");
  Serial.print("IP: "); Serial.println(WiFi.localIP());
  Serial.flush();
  
  setupGateway(INCLUSION_MODE_PIN, INCLUSION_MODE_TIME, output);

  // Initialize gateway at maximum PA level, channel 70 and callback for write operations 
  gw.begin(incomingMessage, 0, true, 0);
  
  // start listening for clients
  server.begin();
  server.setNoDelay(true);  
}


void loop() {
  gw.process();  
  
  checkButtonTriggeredInclusion();
  checkInclusionFinished();

  // Go over list of clients and stop any that are no longer connected.
  // If the server has a new client connection it will be assigned to a free slot.
  bool allSlotsOccupied = true;
  for (uint8_t i = 0; i < ARRAY_SIZE(clients); i++)
  {
    if (!clients[i].connected())
    {
      if (clientsConnected[i])
      {
        Serial.print("Client "); Serial.print(i); Serial.println(" disconnected");
        clients[i].stop();
      }
      //check if there are any new clients
      if (server.hasClient())
      {
        clients[i] = server.available();
        inputString[i].idx = 0;
        Serial.print("Client "); Serial.print(i); Serial.println(" connected"); 
        output(PSTR("0;0;%d;0;%d;Gateway startup complete.\n"),  C_INTERNAL, I_GATEWAY_READY);
      }
    }
    bool connected = clients[i].connected();
    clientsConnected[i] = connected;
    allSlotsOccupied &= connected;
  }
  if (allSlotsOccupied && server.hasClient())
  {
    //no free/disconnected spot so reject
    Serial.println("No free slot available");
    WiFiClient c = server.available();
    c.stop();
  }
  
  // Loop over clients connect and read available data
  for (uint8_t i = 0; i < ARRAY_SIZE(clients); i++)
  {
    while(clients[i].connected() && clients[i].available())
    {
      char inChar = clients[i].read();
      if ( inputString[i].idx < MAX_RECEIVE_LENGTH - 1 )
      { 
        // if newline then command is complete
        if (inChar == '\n')
        {  
          // a command was issued by the client
          // we will now try to send it to the actuator
          inputString[i].string[inputString[i].idx] = 0;
    
          // echo the string to the serial port
          Serial.print("Client "); Serial.print(i); Serial.print(": "); Serial.println(inputString[i].string);
    
          parseAndSend(gw, inputString[i].string);
    
          // clear the string:
          inputString[i].idx = 0;
          // Finished with this client's message. Next loop() we'll see if there's more to read.
          break;
        } else {  
         // add it to the inputString:
         inputString[i].string[inputString[i].idx++] = inChar;
        }
      } else {
        // Incoming message too long. Throw away 
        Serial.print("Client "); Serial.print(i); Serial.println(": Message too long");
        inputString[i].idx = 0;
        // Finished with this client's message. Next loop() we'll see if there's more to read.
        break;
      }
    }
  }
}



