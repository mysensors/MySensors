/***
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 * 
 * DESCRIPTION
 * This sketch provides an example how to implement a Dimmable Light 
 * It is pure virtual and it logs messages to the serial output
 * It can be used as a base sketch for actual hardware.
 * Stores the last light state and level in eeprom.
 * 
 * Developed by GizMoCuz (Domoticz)
 *
 * REVISION HISTORY
 * Version 1.0 - January 30, 2015
 *
 ***/
 
#include <SPI.h>
#include <MySensor.h>  

#define CHILD_ID_LIGHT 1

#define EPROM_LIGHT_STATE 1
#define EPROM_DIMMER_LEVEL 2

#define LIGHT_OFF 0
#define LIGHT_ON 1

#define SN "Dimable Light"
#define SV "1.0"

int LastLightState=LIGHT_OFF;
int LastDimValue=100;

MySensor gw;
MyMessage lightMsg(CHILD_ID_LIGHT, V_LIGHT);
MyMessage dimmerMsg(CHILD_ID_LIGHT, V_DIMMER);

void setup()  
{ 
  gw.begin(incomingMessage, AUTO, false);

  // Send the Sketch Version Information to the Gateway
  gw.sendSketchInfo(SN, SV);

  gw.present(CHILD_ID_LIGHT, S_DIMMER );

  //Retreive our last light state from the eprom
  int LightState=gw.loadState(EPROM_LIGHT_STATE); 
  if (LightState<=1) {
    LastLightState=LightState;
    int DimValue=gw.loadState(EPROM_DIMMER_LEVEL); 
    if ((DimValue>0)&&(DimValue<=100)) {
      //There should be no Dim value of 0, this would mean LIGHT_OFF
      LastDimValue=DimValue;
    }
  }

  //Here you actualy switch on/off the light with the last known dim level
  SetCurrentState2Hardware();
  
  Serial.println( "Node ready to receive messages..." );  
}

void loop()      
{
  //delay(1000);  // Removed by hek 
  // Process incoming messages (like config and light state from controller)
  gw.process();
}

void incomingMessage(const MyMessage &message)
{
  if (message.type == V_LIGHT) {
    Serial.println( "V_LIGHT command received..." );
    
    int lstate= atoi( message.data );
    if ((lstate<0)||(lstate>1)) {
      Serial.println( "V_LIGHT data invalid (should be 0/1)" );
      return;
    }
    LastLightState=lstate;
    gw.saveState(EPROM_LIGHT_STATE, LastLightState);
    
    if ((LastLightState==LIGHT_ON)&&(LastDimValue==0)) {
       //In the case that the Light State = On, but the dimmer value is zero,
       //then something (probably the controller) did something wrong,
       //for the Dim value to 100%
      LastDimValue=100;
      gw.saveState(EPROM_DIMMER_LEVEL, LastDimValue);
    }
    
    //When receiving a V_LIGHT command we switch the light between OFF and the last received dimmer value
    //This means if you previously set the lights dimmer value to 50%, and turn the light ON
    //it will do so at 50%
  }
  else if (message.type == V_DIMMER) {
    Serial.println( "V_DIMMER command received..." );  
    int dimvalue= atoi( message.data );
    if ((dimvalue<0)||(dimvalue>100)) {
      Serial.println( "V_DIMMER data invalid (should be 0..100)" );
      return;
    }
    if (dimvalue==0) {
      LastLightState=LIGHT_OFF;
    }
    else {
      LastLightState=LIGHT_ON;
      LastDimValue=dimvalue;
      gw.saveState(EPROM_DIMMER_LEVEL, LastDimValue);
    }
  }
  else {
    Serial.println( "Invalid command received..." );  
    return;
  }

  //Here you set the actual light state/level
  SetCurrentState2Hardware();
}

void SetCurrentState2Hardware()
{
  if (LastLightState==LIGHT_OFF) {
     Serial.println( "Light state: OFF" );
  }
  else {
     Serial.print( "Light state: ON, Level: " );
     Serial.println( LastDimValue );
  }

  //Send current state to the controller
  SendCurrentState2Controller();
}

void SendCurrentState2Controller()
{
  if ((LastLightState==LIGHT_OFF)||(LastDimValue==0)) {
    gw.send(dimmerMsg.set(0));
  }
  else {
    gw.send(dimmerMsg.set(LastDimValue));
  }
}

