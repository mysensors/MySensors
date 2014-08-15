// 				MyMQTT Gateway 0.1b
//
// Created by Daniel Wiegert <daniel.wiegert@gmail.com>
// Based on Mysensors Ethernet Gateway by Henrik Ekblad <henrik.ekblad@gmail.com>
// http://www.mysensors.org
//
// Requires MySensors lib 1.4b
//
// Don't forget to look at the definitions in MyMQTT.h!
// Don't forget to configure Radio pins, IP and MAC-address!
//
// Address-layout is : [MQTT_BROKER_PREFIX]/[NodeID]/[SensorID]/Light
// NodeID and SensorID is Decimal number.
// Last segment is translation of the sensor type, look inside MyMQTT.cpp for
// the definitions. User can change this to their needs.
//
// Features:
// Supports automatic nodeID delegation
// Recieve sketchname and version Example: (openhab item)
//String sketch20 "Node 20 Sketch name [%s]"			(sketch,name,a)	{mqtt="<[mysensor:MyMQTT/20/255/Sketch_name:state:default]"}
//String sketch21 "Node 21 Sketch name [%s]"			(sketch,name,a)	{mqtt="<[mysensor:MyMQTT/21/255/Sketch_name:state:default]"}
// ...
//
// Todo:
// DOCUMENTATION...
// Special commands : clear or set EEPROM Values
// Special commands : Reboot
// ...
/*
Sketch uses 23,666 bytes (77%) of program storage space. Maximum is 30,720 bytes.
Global variables use 766 bytes (37%) of dynamic memory, leaving 1,282 bytes for local variables. Maximum is 2,048 bytes.
*/

#include <SPI.h>
#include <MyMQTT.h>
#include <Ethernet.h>
#include <MsTimer2.h>


// Use this for IBOARD modded to use standard MISO/MOSI/SCK More information see;
// http://forum.mysensors.org/topic/224/iboard-cheap-single-board-ethernet-arduino-with-radio/5
//#define RADIO_CE_PIN        3		// radio chip enable
//#define RADIO_SPI_SS_PIN    8		// radio SPI serial select
//#define RADIO_ERROR_LED_PIN A2		// Error led pin
//#define RADIO_RX_LED_PIN    A1		// Receive led pin
//#define RADIO_TX_LED_PIN    A0		// the PCB, on board LED

//Use this for default configured pro mini / nano etc :
#define RADIO_CE_PIN        5		// radio chip enable
#define RADIO_SPI_SS_PIN    6		// radio SPI serial select
#define RADIO_ERROR_LED_PIN 7		// Error led pin
#define RADIO_RX_LED_PIN    8		// Receive led pin
#define RADIO_TX_LED_PIN    9		// the PCB, on board LED

#define IP_PORT 1883                    // MQTT Listening port
IPAddress myIp ( 192, 168, 0, 234 );    // Configure your static ip-address here
byte mac[] = { 0x02, 0xDE, 0xAD, 0x00, 0x00, 0x42 };  // Mac-address - Change this!
// * NOTE: Keep first byte at x2, x6, xA or xE (replace x with any hex value) for using Locally Administered Address Ranges.

EthernetServer server = EthernetServer(IP_PORT);
MyMQTT gw(RADIO_CE_PIN, RADIO_SPI_SS_PIN);
// Uncomment this constructor if you have leds and include button attached to your gateway
// MyMQTT gw(RADIO_CE_PIN, RADIO_SPI_SS_PIN, RADIO_RX_LED_PIN, RADIO_TX_LED_PIN, RADIO_ERROR_LED_PIN);

void processEthernetMessages() {
  char inputString[MQTT_MAX_PACKET_SIZE] = "";
  int inputSize = 0;
  EthernetClient client = server.available();
  if (client) {
    while (client.available()) {
      char inChar = client.read();
      inputString[inputSize] = inChar;
      inputSize++;
    }
    gw.processMQTTMessage(inputString, inputSize);
  }
}

void writeEthernet(char *writeBuffer, int *writeSize) {
  server.write(writeBuffer, *writeSize); // Should this really be *writeSize?
}

void ledTimersInterrupt() {
  gw.ledTimersInterrupt();
}

int main(void) {
  init();
  Ethernet.begin(mac, myIp);
  delay(1000);   // Wait for Ethernet to get configured.
  gw.begin(RF24_PA_LEVEL_GW, RF24_CHANNEL, RF24_DATARATE, writeEthernet);
  server.begin();
  if (gw.isLedMode()) {
    // Add led timer interrupt
    MsTimer2::set(300, ledTimersInterrupt);
    MsTimer2::start();
  }
  while (1) {
    processEthernetMessages();
    gw.processRadioMessage();
  }
}
