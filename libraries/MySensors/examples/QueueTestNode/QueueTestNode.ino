#include <Arduino.h>
#include <Wire.h>

#define MY_RADIO_NRF24
#define MY_RF24_CHANNEL  (100)
#define MY_NODE_ID       (200)
#define MY_RF24_CE_PIN   (9)
#define MY_RF24_CS_PIN   (10)
//#define MY_RF24_IRQ_PIN  (2)
#define MY_RF24_DATARATE (RF24_1MBPS)
#define MY_DEBUG
//#define MY_DEBUG_VERBOSE_RF24

#include <SPI.h>
#include <MySensor.h>

#undef debug
#ifndef NODEBUG
#define debug(x)   Serial.print(x)
#define debugln(x) Serial.println(x)
#else
#define debug(x)
#define debugln(x)
#endif

void presentation()
{
  sendSketchInfo("Test", "1.0");
  debug("ID: ");  debugln(getNodeId());

  present(0, S_CUSTOM);
}

void receive(const MyMessage &message)
{
  //  debug("Message "); debugln(message.type);
}

void setup()
{
  debugln(F("---- Starting sensor ----"));
}

void loop()
{
  static uint16_t count = 0;
  send( MyMessage(0,  V_VAR1).set(count) );
  ++count;

  delay(100);
}

