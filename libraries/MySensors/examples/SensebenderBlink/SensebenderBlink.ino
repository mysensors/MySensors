// Sensebender Micro board
// Simple blinking FW to test OTA updates

#include <MySensor.h>
#include <SPI.h>
#include "utility/SPIFlash.h"

#define SketchName "Sensebender Blink"
#define SketchVersion "20150721"
#define NODE_ID AUTO
#define CHILD_ID_COUNTER 0

#define LED_PIN A2
#define BLINK_INTERVAL 500
#define REPORT_TIME 5000L

unsigned long lastUpdate = 0;
boolean LED_STATUS = false;

SPIFlash flash(8, 0x1F65);
MySensor gw;
MyMessage msgCounter(CHILD_ID_COUNTER, V_VAR1);

void setup() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LED_STATUS);
  
  // repeater mode on
  gw.begin(NULL,NODE_ID,true);
  gw.sendSketchInfo(SketchName, SketchVersion);  
  
  gw.present(CHILD_ID_COUNTER, S_CUSTOM,"Millis()"); 
  
}

void loop() {
  gw.wait(BLINK_INTERVAL);
  LED_STATUS ^= true;
  digitalWrite(LED_PIN, LED_STATUS);
  
  if (millis()-lastUpdate > REPORT_TIME) {
    gw.send(msgCounter.set( millis() ));
    lastUpdate = millis();
  }
  
}  
  

