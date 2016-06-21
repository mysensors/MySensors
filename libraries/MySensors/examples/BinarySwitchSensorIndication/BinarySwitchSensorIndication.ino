/**
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2016 Sensnology AB
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
 * DESCRIPTION
 *
 * Simple binary switch example 
 * Connect button or door/window reed switch between 
 * digitial I/O pin 3 (BUTTON_PIN below) and GND.
 * A NeoPixel LED will be used to provide visual feedback of the sensor status.
 * http://www.mysensors.org/build/binary
 */


// Enable debug prints to serial monitor
#define MY_DEBUG 

// Enable and select radio type attached
#define MY_RADIO_NRF24
//#define MY_RADIO_RFM69

#include <Adafruit_NeoPixel.h>
#include <MsTimer2.h>
#include <SPI.h>
#include <MySensors.h>
#include <Bounce2.h>

#define LED_CYCLE_MS    30   // Update rate of LEDs, in [ms].
#define NEO_PIN         6    // NeoPixels input pin
#define CHILD_ID        3
#define BUTTON_PIN      3    // Arduino Digital I/O pin for button/reed switch

Bounce debouncer = Bounce(); 
int oldValue=-1;

static Adafruit_NeoPixel pixels = Adafruit_NeoPixel(1, NEO_PIN, NEO_GRB + NEO_KHZ800);
    
// Change to V_LIGHT if you use S_LIGHT in presentation below
MyMessage msg(CHILD_ID,V_TRIPPED);

#define BLINK_TIMES(x)  ((x)<<4)

static uint8_t ledR = 0;
static uint8_t ledG = 0;
static uint8_t ledB = 0;

void updateLed(void)
{
  // Update the pattern (setting leds).
  if (ledR > 0) --ledR;
  if (ledG > 0) --ledG;
  if (ledB > 0) --ledB;
  // Determine of each color if it should be on or off.
  uint8_t r = ((ledR & 0x0F) < 6) ? 0 : 255;
  uint8_t g = ((ledG & 0x0F) < 6) ? 0 : 255;
  uint8_t b = ((ledB & 0x0F) < 6) ? 0 : 255;
  pixels.setPixelColor(0, r, g, b); 
  pixels.show();
}

void before()
{
    // Initialize the NeoPixel chain
    pixels.begin();
    pixels.show();

    // Configure timer interrupt
    MsTimer2::set(LED_CYCLE_MS, updateLed);
    MsTimer2::start();
}

void setup()  
{  
  // Setup the button
  pinMode(BUTTON_PIN,INPUT);
  // Activate internal pull-up
  digitalWrite(BUTTON_PIN,HIGH);
  
  // After setting up the button, setup debouncer
  debouncer.attach(BUTTON_PIN);
  debouncer.interval(5);
}

void presentation() {
  // Register binary input sensor to gw (they will be created as child devices)
  // You can use S_DOOR, S_MOTION or S_LIGHT here depending on your usage. 
  // If S_LIGHT is used, remember to update variable type you send in. See "msg" above.
  present(CHILD_ID, S_DOOR);  
}

void indication( const indication_t ind )
{
    if ((INDICATION_TX == ind) || (INDICATION_GW_TX == ind))
    {
        // Blink Blue 1x
        ledB = BLINK_TIMES(1);
        Serial.println(F("TX"));
    } else if ((INDICATION_RX == ind) || (INDICATION_GW_RX == ind))
    {
        // Blink Blue 1x
        ledB = BLINK_TIMES(1);
        Serial.println(F("RX"));
    } else if (INDICATION_FIND_PARENT == ind)
    {
        // Blink Green 5x
        ledG = BLINK_TIMES(5);
        Serial.println(F("JOIN"));
    } else if (INDICATION_GOT_PARENT == ind)
    {
        // Green off
        ledG = BLINK_TIMES(0);
        Serial.println(F("JOINED"));
    } else if (ind > INDICATION_ERR_START)
    {
        // Blink Red, depending on the error number
        ledR = BLINK_TIMES(ind - INDICATION_ERR_START);
        Serial.print(F("ERROR")); Serial.println(ind - INDICATION_ERR_START);
    }
}

//  Check if digital input has changed and send in new value
void loop() 
{
  debouncer.update();
  // Get the update value
  int value = debouncer.read();
 
  if (value != oldValue) {
     // Send in the new value
     send(msg.set(value==HIGH ? 1 : 0));
     oldValue = value;
  }
} 

