/***
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 * 
 * DESCRIPTION
 * This sketch provides a Dimmable LED Light using PWM and based Henrik Ekblad 
 * <henrik.ekblad@gmail.com> Vera Arduino Sensor project.  
 * Developed by Bruce Lacey, inspired by Hek's MySensor's example sketches.
 * 
 * The circuit uses a MOSFET for Pulse-Wave-Modulation to dim the attached LED or LED strip.  
 * The MOSFET Gate pin is connected to Arduino pin 3 (LED_PIN), the MOSFET Drain pin is connected
 * to the LED negative terminal and the MOSFET Source pin is connected to ground.  
 *
 * This sketch is extensible to support more than one MOSFET/PWM dimmer per circuit.
 *
 * REVISION HISTORY
 * Version 1.0 - February 15, 2014 - Bruce Lacey
 * Version 1.1 - August 13, 2014 - Converted to 1.4 (hek) 
 *
 ***/
#define SN "DimmableLED"
#define SV "1.1"

#include <MySensor.h> 
#include <SPI.h>

#define LED_PIN 3      // Arduino pin attached to MOSFET Gate pin
#define FADE_DELAY 10  // Delay in ms for each percentage fade up/down (10ms = 1s full-range dim)

MySensor gw;

static int currentLevel = 0;  // Current dim level...
MyMessage dimmerMsg(0, V_DIMMER);
MyMessage lightMsg(0, V_LIGHT);


/***
 * Dimmable LED initialization method
 */
void setup()  
{ 
  Serial.println( SN ); 
  gw.begin( incomingMessage );
  
  // Register the LED Dimmable Light with the gateway
  gw.present( 0, S_DIMMER );
  
  gw.sendSketchInfo(SN, SV);
  // Pull the gateway's current dim level - restore light level upon sendor node power-up
  gw.request( 0, V_DIMMER );
}

/***
 *  Dimmable LED main processing loop 
 */
void loop() 
{
  gw.process();
}



void incomingMessage(const MyMessage &message) {
  if (message.type == V_LIGHT || message.type == V_DIMMER) {
    
    //  Retrieve the power or dim level from the incoming request message
    int requestedLevel = atoi( message.data );
    
    // Adjust incoming level if this is a V_LIGHT variable update [0 == off, 1 == on]
    requestedLevel *= ( message.type == V_LIGHT ? 100 : 1 );
    
    // Clip incoming level to valid range of 0 to 100
    requestedLevel = requestedLevel > 100 ? 100 : requestedLevel;
    requestedLevel = requestedLevel < 0   ? 0   : requestedLevel;
    
    Serial.print( "Changing level to " );
    Serial.print( requestedLevel );
    Serial.print( ", from " ); 
    Serial.println( currentLevel );

    fadeToLevel( requestedLevel );
    
    // Inform the gateway of the current DimmableLED's SwitchPower1 and LoadLevelStatus value...
    gw.send(lightMsg.set(currentLevel > 0 ? 1 : 0));

    // hek comment: Is this really nessesary?
    gw.send( dimmerMsg.set(currentLevel) );

    
    }
}

/***
 *  This method provides a graceful fade up/down effect
 */
void fadeToLevel( int toLevel ) {

  int delta = ( toLevel - currentLevel ) < 0 ? -1 : 1;
  
  while ( currentLevel != toLevel ) {
    currentLevel += delta;
    analogWrite( LED_PIN, (int)(currentLevel / 100. * 255) );
    delay( FADE_DELAY );
  }
}

