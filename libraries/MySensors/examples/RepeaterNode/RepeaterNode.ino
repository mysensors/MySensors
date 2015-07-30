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
 * Example sketch showing how to create a node thay repeates messages
 * from nodes far from gateway back to gateway. 
 * It is important that nodes that has enabled repeater mode calls  
 * gw.process() frequently. Repeaters should never sleep. 
 */

#include <MySensor.h>
#include <SPI.h>

MySensor gw;

void setup()  
{  
  // The third argument enables repeater mode.
  gw.begin(NULL, AUTO, true);

  //Send the sensor node sketch version information to the gateway
  gw.sendSketchInfo("Repeater Node", "1.0");
}

void loop() 
{
  // By calling process() you route messages in the background
  gw.process();
}

