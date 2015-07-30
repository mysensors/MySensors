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
 * DESCRIPTION
 *
 * This sketch clears radioId, relayId and other routing information in EEPROM back to factory default
 * 
 */

#include <SPI.h>
#include <EEPROM.h>  
#include <MySensor.h>  

void setup()  
{ 
  Serial.begin(BAUD_RATE);
  Serial.println("Started clearing. Please wait...");
  for (int i=0;i<512;i++) {
    EEPROM.write(i, 0xff);
  }
  Serial.println("Clering done. You're ready to go!");
}

void loop()      
{ 
  // Nothing to do here...
} 
