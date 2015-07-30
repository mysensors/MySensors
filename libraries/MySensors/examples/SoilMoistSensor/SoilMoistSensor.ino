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
 * Version 1.0 - Henrik Ekblad
 * 
 * DESCRIPTION
 * Example sketch sending soil moisture alarm to controller 
 * http://www.mysensors.org/build/moisture
 */
 
#include <SPI.h>
#include <MySensor.h>  

#define DIGITAL_INPUT_SOIL_SENSOR 3   // Digital input did you attach your soil sensor.  
#define INTERRUPT DIGITAL_INPUT_SOIL_SENSOR-2 // Usually the interrupt = pin -2 (on uno/nano anyway)
#define CHILD_ID 0   // Id of the sensor child

MySensor gw;
MyMessage msg(CHILD_ID, V_TRIPPED);
int lastSoilValue = -1;

void setup()  
{ 
  gw.begin();

  // Send the sketch version information to the gateway and Controller
  gw.sendSketchInfo("Soil Moisture Sensor", "1.0");
  // sets the soil sensor digital pin as input
  pinMode(DIGITAL_INPUT_SOIL_SENSOR, INPUT);      
  // Register all sensors to gw (they will be created as child devices)  
  gw.present(CHILD_ID, S_MOTION);
}
 
void loop()     
{     
  // Read digital soil value
  int soilValue = digitalRead(DIGITAL_INPUT_SOIL_SENSOR); // 1 = Not triggered, 0 = In soil with water 
  if (soilValue != lastSoilValue) {
    Serial.println(soilValue);
    gw.send(msg.set(soilValue==0?1:0));  // Send the inverse to gw as tripped should be when no water in soil
    lastSoilValue = soilValue;
  }
  // Power down the radio and arduino until digital input changes.
  gw.sleep(INTERRUPT,CHANGE);
}


