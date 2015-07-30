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
 * Version 1.0 - idefix
 * 
 * DESCRIPTION
 * Arduino BH1750FVI Light sensor
 * communicate using I2C Protocol
 * this library enable 2 slave device addresses
 * Main address  0x23
 * secondary address 0x5C
 * connect the sensor as follows :
 *
 *   VCC  >>> 5V
 *   Gnd  >>> Gnd
 *   ADDR >>> NC or GND  
 *   SCL  >>> A5
 *   SDA  >>> A4
 * http://www.mysensors.org/build/light
 */

#include <SPI.h>
#include <MySensor.h>  
#include <BH1750.h>
#include <Wire.h> 

#define CHILD_ID_LIGHT 0
unsigned long SLEEP_TIME = 30000; // Sleep time between reads (in milliseconds)

BH1750 lightSensor;
MySensor gw;

// V_LIGHT_LEVEL should only be used for uncalibrated light level 0-100%.
// If your controller supports the new V_LEVEL variable, use this instead for
// transmitting LUX light level.
MyMessage msg(CHILD_ID_LIGHT, V_LIGHT_LEVEL);
// MyMessage msg(CHILD_ID_LIGHT, V_LEVEL);  
uint16_t lastlux;

void setup()  
{ 
  gw.begin();

  // Send the sketch version information to the gateway and Controller
  gw.sendSketchInfo("Light Lux Sensor", "1.0");

  // Register all sensors to gateway (they will be created as child devices)
  gw.present(CHILD_ID_LIGHT, S_LIGHT_LEVEL);
  
  lightSensor.begin();
}

void loop()      
{     
  uint16_t lux = lightSensor.readLightLevel();// Get Lux value
  Serial.println(lux);
  if (lux != lastlux) {
      gw.send(msg.set(lux));
      lastlux = lux;
  }
  
  gw.sleep(SLEEP_TIME);
}
