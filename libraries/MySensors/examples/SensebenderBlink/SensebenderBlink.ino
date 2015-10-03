// Sensebender Micro board
// Simple blinking FW to test OTA updates

// Enable debug prints to serial monitor
#define MY_DEBUG 

// Enable repeater feature
#define MY_REPEATER_FEATURE

// Enable and select radio type attached
#define MY_RADIO_NRF24
//#define MY_RADIO_RFM69

#include <SPI.h>
#include <MySensor.h>

#define SketchName "Sensebender Blink"
#define SketchVersion "20150721"
#define NODE_ID AUTO
#define CHILD_ID_COUNTER 0

#define LED_PIN A2
#define BLINK_INTERVAL 500
#define REPORT_TIME 5000L

unsigned long lastUpdate = 0;
boolean LED_STATUS = false;

MyMessage msgCounter(CHILD_ID_COUNTER, V_VAR1);

void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LED_STATUS);
}

void presentation()  {
  sendSketchInfo(SketchName, SketchVersion);  
  present(CHILD_ID_COUNTER, S_CUSTOM,"Millis()"); 
}

void loop() {
  wait(BLINK_INTERVAL);
  LED_STATUS ^= true;
  digitalWrite(LED_PIN, LED_STATUS);
  
  if (millis()-lastUpdate > REPORT_TIME) {
    send(msgCounter.set( millis() ));
    lastUpdate = millis();
  }
}  
  

