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
 * Version 1.0 - Created by Henrik Ekblad
 * 
 * DESCRIPTION
 * Parking sensor using a neopixel led ring and distance sensor (HC-SR04).
 * Configure the digital pins used for distance sensor and neopixels below.
 * NOTE! Remeber to feed leds and distance sensor serparatly from your Arduino. 
 * It will probably not survive feeding more than a couple of LEDs. You 
 * can also adjust intesity below to reduce the power requirements.
 * 
 * Sends parking status to the controller as a DOOR sensor if SEND_STATUS_TO_CONTROLLER 
 * is defined below. You can also use this _standalone_ without any radio by 
 * removing the SEND_STATUS_TO_CONTROLLER define.
 */

#define SEND_STATUS_TO_CONTROLLER  // Put a comment on this line for standalone mode

#include <Adafruit_NeoPixel.h>
#include <NewPing.h>

#ifdef SEND_STATUS_TO_CONTROLLER
#include <SPI.h>
#include <MySensor.h>
#endif

#define NEO_PIN      4 // NeoPixels input pin

#define TRIGGER_PIN  6  // Arduino pin tied to trigger pin on the ultrasonic sensor.
#define ECHO_PIN     5  // Arduino pin tied to echo pin on the ultrasonic sensor.

#define NUMPIXELS      24 // Number of nexpixels in ring/strip
#define MAX_INTESITY   20  // Intesity of leds (in percentage). Remeber more intesity requires more power.

// The maximum rated measuring range for the HC-SR04 is about 400-500cm.
#define MAX_DISTANCE 100 // Max distance we want to start indicating green (in cm)
#define PANIC_DISTANCE 5 // Mix distance we red warning indication should be active (in cm)
#define PARKED_DISTANCE 20 // Distance when "parked signal" should be sent to controller (in cm)

#define PARK_OFF_TIMEOUT 20000 // Number of milliseconds until turning off light when parked.

// Note that for older NeoPixel strips you might need to change the third parameter--see the strandtest
// example for more information on possible values.

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, NEO_PIN, NEO_GRB + NEO_KHZ400);

NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE); // NewPing setup of pins and maximum distance.

#ifdef SEND_STATUS_TO_CONTROLLER
#define CHILD_ID 1
MySensor gw;
MyMessage msg(CHILD_ID,V_TRIPPED);
#endif
unsigned long sendInterval = 5000;  // Send park status at maximum every 5 second.
unsigned long lastSend;

int oldParkedStatus=-1;

unsigned long blinkInterval = 100; // blink interval (milliseconds)
unsigned long lastBlinkPeriod;
bool blinkColor = true;

// To make a fading motion on the led ring/tape we only move one pixel/distDebounce time
unsigned long distDebounce = 30; 
unsigned long lastDebouncePeriod;
int numLightPixels=0;
int skipZero=0;

void setup() {
  Serial.begin(115200);
  Serial.println("Starting distance sensor");
  pixels.begin(); // This initializes the NeoPixel library.
  Serial.println("Neopixels initialized");
#ifdef SEND_STATUS_TO_CONTROLLER
  gw.begin();
  gw.sendSketchInfo("Parking Sensor", "1.0");
  gw.present(CHILD_ID, S_DOOR, "Parking Status");
#endif
}

void loop() {
  unsigned long now = millis();
  
  int fullDist = sonar.ping_cm();
//  Serial.println(fullDist);
  int displayDist = min(fullDist, MAX_DISTANCE);
  if (displayDist == 0 && skipZero<10) {
    // Try to filter zero readings
    skipZero++;
    return;
  }
  // Check if it is time to alter the leds
  if (now-lastDebouncePeriod > distDebounce) {
    lastDebouncePeriod = now;

    // Update parked status
    int parked = displayDist != 0 && displayDist<PARKED_DISTANCE;
    if (parked != oldParkedStatus && now-lastSend > sendInterval) {
      if (parked)
        Serial.println("Car Parked");
      else
        Serial.println("Car Gone");
#ifdef SEND_STATUS_TO_CONTROLLER
      gw.send(msg.set(parked)); 
#endif
      oldParkedStatus = parked;
      lastSend = now;
    }

    if (parked && now-lastSend > PARK_OFF_TIMEOUT) {
      // We've been parked for a while now. Turn off all pixels
      for(int i=0;i<NUMPIXELS;i++){
        pixels.setPixelColor(i, pixels.Color(0,0,0)); 
      }
    } else {
      if (displayDist == 0) {
        // No reading from sensor, assume no object found
        numLightPixels--;
      } else {
        skipZero = 0;
        int newLightPixels = NUMPIXELS - (NUMPIXELS*(displayDist-PANIC_DISTANCE)/MAX_DISTANCE);
        if (newLightPixels>numLightPixels) {
          // Fast raise
          numLightPixels += max((newLightPixels - numLightPixels) / 2, 1);
        } else if (newLightPixels<numLightPixels) {
          // Slow decent
          numLightPixels--;
        }
      }
  
      if (numLightPixels>=NUMPIXELS) {
        // Do some intense red blinking 
        if (now-lastBlinkPeriod > blinkInterval) {
          blinkColor = !blinkColor;
          lastBlinkPeriod = now;
        }
        for(int i=0;i<numLightPixels;i++){
          pixels.setPixelColor(i, pixels.Color(blinkColor?255*MAX_INTESITY/100:0,0,0)); 
        }              
      } else {
        for(int i=0;i<numLightPixels;i++){
          int r = 255 * i/NUMPIXELS;
          int g = 255 - r;     
          // pixels.Color takes RGB values, from 0,0,0 up to 255,255,255
          pixels.setPixelColor(i, pixels.Color(r*MAX_INTESITY/100,g*MAX_INTESITY/100,0)); 
        }
        // Turn off the rest
        for(int i=numLightPixels;i<NUMPIXELS;i++){
          pixels.setPixelColor(i, pixels.Color(0,0,0)); 
        }
      }
    }
    pixels.show(); // This sends the updated pixel color to the hardware.
  }
}
