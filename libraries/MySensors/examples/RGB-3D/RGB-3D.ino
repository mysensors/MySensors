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
 * Version 1.0 - Created by vil1driver
 * 
 * DESCRIPTION
 * RGB led strip controled with three dimmers + one On/Off for run/stop rgb color cycle :p
 * 
 */
 
#define SN   "RGB Led strip 3D"
#define SV   "v1"

// Load mysensors library	
#include <MySensor.h>	
// Load Serial Peripheral Interface library  
#include <SPI.h>

// Arduino pin attached to MOSFET Gate pin
#define RED_PIN 3 	
#define GREEN_PIN 5
#define BLUE_PIN 6

MySensor gw;	

// Define message name and type to send sensor info
MyMessage RedStatus(RED_PIN, V_DIMMER);		
MyMessage GreenStatus(GREEN_PIN, V_DIMMER);
MyMessage BlueStatus(BLUE_PIN, V_DIMMER);
MyMessage Status(1, V_DIMMER);
MyMessage rgbShowState(0, V_LIGHT);
    
// Serial.print translate sensor id to sensor name
char color[][6] = {"","","","RED","","GREEN","BLUE"};	
   
// Vars for rgbShow function
int redval = 0;
int greenval = 0;
int blueval = 0;
long time=0;
int isShow;
     
void setup() 
{
  // Initializes the sensor node (Callback function for incoming messages, node id, is repeater)
  gw.begin(incomingMessage, 31, true);		
       
  // Present sketch (name, version)
  gw.sendSketchInfo(SN, SV);				
       
  // Register sensors (id, type, description, ack back)
  gw.present(RED_PIN, S_DIMMER, "present RED light", false);
  gw.present(GREEN_PIN, S_DIMMER, "present GREEN light", false);
  gw.present(BLUE_PIN, S_DIMMER, "present BLUE light", false);
  gw.present(0, S_LIGHT, "present Show button", false);

  // Define pin mode (pin number, type)
  pinMode(RED_PIN, OUTPUT);		
  pinMode(GREEN_PIN, OUTPUT);
  pinMode(BLUE_PIN, OUTPUT);

  // Correct saved RGB value for first start
  gw.saveState(RED_PIN, constrain((int8_t)gw.loadState(RED_PIN), 0, 100)); 
  gw.saveState(GREEN_PIN, constrain((int8_t)gw.loadState(GREEN_PIN), 0, 100)); 
  gw.saveState(BLUE_PIN, constrain((int8_t)gw.loadState(BLUE_PIN), 0, 100)); 
             
  // Get value from eeprom and write to output
  analogWrite(RED_PIN, 255 * gw.loadState(RED_PIN) / 100); 		
  analogWrite(GREEN_PIN, 255 * gw.loadState(GREEN_PIN) / 100);
  analogWrite(BLUE_PIN, 255 * gw.loadState(BLUE_PIN) / 100);
         
  // Write some debug info
  Serial.print("Load from eeprom RED: "); 
  Serial.print(gw.loadState(RED_PIN)); 
  Serial.println("%"); 
  Serial.print("Load from eeprom GREEN: "); 
  Serial.print(gw.loadState(GREEN_PIN)); 
  Serial.println("%"); 
  Serial.print("Load from eeprom BLUE: "); 
  Serial.print(gw.loadState(BLUE_PIN)); 
  Serial.println("%");  
  
  // Send RGB value to controler (request ack back: true/false)
  Serial.println("Send eeprom value to controler"); 
  gw.send( RedStatus.set(gw.loadState(RED_PIN)), false );    
  gw.send( GreenStatus.set(gw.loadState(GREEN_PIN)), false );
  gw.send( BlueStatus.set(gw.loadState(BLUE_PIN)), false );
  
  // Correct RGB show state for first start and load it (set to 'On' at first start)
  gw.saveState(0, constrain((int8_t)gw.loadState(0), 0, 1));
  isShow=gw.loadState(0);
       
  // Send RGB show state to controler (request ack back: true/false)
  gw.send( rgbShowState.set(isShow), false);
  
  if (isShow==1){Serial.println("RGB show running..."); }
  Serial.println("Ready to receive messages...");  
}

void loop()
{
  // Process incoming messages (like config and light state from controller)
  gw.process();		
      
  // Run RGB show if is set
  if (isShow==1)
  {
    	rgbShow();
  	analogWrite(RED_PIN, redval);
  	analogWrite(GREEN_PIN, greenval);
  	analogWrite(BLUE_PIN, blueval);
   } 
}


void incomingMessage(const MyMessage &message)
{
  if (message.isAck())
  {
   	Serial.println("Got ack from gateway");
  }
  if (message.type == V_LIGHT)
  {
  	// Incoming on/off command sent from controller ("1" or "0")
  	int lightState = message.getString()[0] == '1';
  
  	// if receive RGB Show On commands, start the show
  	if (message.sensor==0 && lightState==1){ rgbShowOn();	}
        // if receive RGB Show Off commands, stop the show
  	else if (message.sensor==0 && lightState==0){	rgbShowOff();	}
       
  	// if receive RGB switch On command
  	else if (lightState==1)
  	{
  		// Write some debug info
          	Serial.print("Incoming change for ");
          	Serial.print(color[message.sensor]);
          	Serial.println(": On");
          	Serial.print("Load from eeprom: ");
          
  		if ( gw.loadState(message.sensor) == 0)
  		{
  			// Pick up last saved dimmer level from the eeprom
            		analogWrite(message.sensor, 255 * gw.loadState(10*message.sensor) / 100);
            		// Save loaded value to current
            		gw.saveState(message.sensor, gw.loadState(10*message.sensor));
            		Serial.print(gw.loadState(10*message.sensor)); 
            		Serial.println("%");
            		// Send value to controler
            		Serial.println("Send value to controler");
            		gw.send(Status.setSensor(message.sensor).set(gw.loadState(10*message.sensor)),false);
          	}
          	else
          	{
            		// Pick up last saved dimmer level from the eeprom
            		analogWrite(message.sensor, 255 * gw.loadState(message.sensor) / 100);
            		Serial.print(gw.loadState(message.sensor));
            		Serial.println("%"); 
            		// Send value to controler
            		Serial.println("Send value to controler");
            		gw.send(Status.setSensor(message.sensor).set(gw.loadState(message.sensor)),false);
          	} 
          	// Stop the show if it's running
          	if (isShow==1){ rgbShowStop(message.sensor); }
        }
  	// if recieve switch Off command
  	else if (lightState==0)
  	{
  		// Write output to 0 (Off)
          	analogWrite(message.sensor, 0);
          	// Save old value to eeprom if it'was not zero
          	if ( gw.loadState(message.sensor) != 0 )
          	{
            		gw.saveState(10*message.sensor, constrain((int8_t)gw.loadState(message.sensor), 0, 100)); 
          	}
          	// Save new value to eeprom
          	gw.saveState(message.sensor, 0); 
          	// Write some debug info
  		Serial.print("Incoming change for ");
  		Serial.print(color[message.sensor]);
  		Serial.print(": ");
  		Serial.println("Off");	
          	Serial.print("Store old value: ");
          	Serial.print(gw.loadState(10*message.sensor));  
          	Serial.println("%");
          	// Send value to controler
          	Serial.println("Send value to controler");
          	gw.send(Status.setSensor(message.sensor).set(gw.loadState(message.sensor)),false);
  		// Stop the show if it's running
  		if (isShow==1){ rgbShowStop(message.sensor); }
  	}
  }
  else if (message.type == V_DIMMER)
  {    
    	uint8_t incomingDimmerStatus = message.getByte();
    	// limits range of sensor values to between 0 and 100 
    	incomingDimmerStatus = constrain((int8_t)incomingDimmerStatus, 0, 100);
    	// Change Dimmer level
    	analogWrite(message.sensor, 255 * incomingDimmerStatus / 100);
    	//Save value to eeprom
    	gw.saveState(message.sensor, incomingDimmerStatus); 
    	// Write some debug info
    	Serial.print("Incoming change for ");
    	Serial.print(color[message.sensor]);
    	Serial.print(": ");
    	Serial.print(incomingDimmerStatus);
    	Serial.println("%");
        // Send value to controler
        Serial.println("Send value to controler");
        gw.send(Status.setSensor(message.sensor).set(gw.loadState(message.sensor)),false);
    	// Stop the show if it's running
    	if (isShow==1){ rgbShowStop(message.sensor); }
    }
}
   
void rgbShow()
{
  time = millis();
  redval = 128+250*cos(2*PI/300000*time);
  greenval = 128+250*cos(2*PI/300000*time-222);
  blueval = 128+250*cos(2*PI/300000*time-111);
  // limits range of sensor values to between 0 and 255 
  redval = constrain(redval, 0, 255);
  greenval = constrain(greenval, 0, 255);
  blueval = constrain(blueval, 0, 255);
}

void rgbShowOn()
{
  // define show On
  isShow=1;
  // Save state
  gw.saveState(0, 1); 
  // Write some debug info
  Serial.println("Show must go on");
}
   
void rgbShowOff()
{
  // define show Off
  isShow=0;
  // Save state
  gw.saveState(0, 0);
  // Save RGB value to eeprom
  gw.saveState(RED_PIN, 100 * redval / 255); 
  gw.saveState(GREEN_PIN, 100 * greenval / 255);
  gw.saveState(BLUE_PIN, 100 * blueval / 255);
  // Write some debug info
  Serial.println("Stop the show");
  // Send actual RGB value and state to controler and request ack back (true/false)
  Serial.println("Send eeprom value to controler"); 
  gw.send( RedStatus.set(gw.loadState(RED_PIN)), false );    
  gw.send( GreenStatus.set(gw.loadState(GREEN_PIN)), false );
  gw.send( BlueStatus.set(gw.loadState(BLUE_PIN)), false );
  gw.send( rgbShowState.set(0), false);
}

void rgbShowStop(int sensor)
{
   // define show Off
   isShow=0;
   // Save state
   gw.saveState(0, 0);
   // Write some debug info
   Serial.println("Stop the show");
   // Send actual RGB value and state to controler and request ack back (true/false)
   Serial.println("Send eeprom value to controler"); 
   if (sensor != RED_PIN)
   {
        gw.saveState(RED_PIN, 100 * redval / 255); 
        gw.send( RedStatus.set(gw.loadState(RED_PIN)), false );  
    }
    if (sensor != GREEN_PIN)
    {
        gw.saveState(GREEN_PIN, 100 * greenval / 255); 
        gw.send( GreenStatus.set(gw.loadState(GREEN_PIN)), false );
    }
    if (sensor != BLUE_PIN)
    {
        gw.saveState(BLUE_PIN, 100 * blueval / 255);
        gw.send( BlueStatus.set(gw.loadState(BLUE_PIN)), false );
    }
    gw.send( rgbShowState.set(0), false);
}
           
   
   
