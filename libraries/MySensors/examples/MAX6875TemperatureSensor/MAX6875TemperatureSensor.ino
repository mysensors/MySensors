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
 * Version 1.0 - MrLynx
 * 
 * DESCRIPTION
 * MAX6875 High temperature sensor with range: -200 degC - +1300 degC 
 * http://forum.mysensors.org/topic/641/max6675
 *
 * Connect MAX6675 - Arduino
 *     CS0 - 4
 *      SO - 3 
 *    SCLK - 5
 *     VCC - 5V
 *     GND - GND
 *
 */


#include <MySensor.h>
#include <MAX6675.h>
#include <SPI.h>
uint8_t CS0 = 4; // CS pin on MAX6675
uint8_t SO = 3; // SO pin of MAX6675
uint8_t SCLK = 5; // SCK pin of MAX6675
uint8_t units = 1; // Units to readout temp (0 = ˚F, 1 = ˚C)
float temperature = 0.0; // Temperature output variable
float lastTemperature;
unsigned long SLEEP_TIME = 30000;
boolean metric = true;
MySensor gw;

MyMessage msg(0, V_TEMP);

// Initialize the MAX6675 Library for our chip

MAX6675 temp0(CS0, SO, SCLK, units);

void setup()
{
  // Startup and initialize MySensors library. Set callback for incoming messages.
  gw.begin();

  // Send the sketch version information to the gateway and Controller
  gw.sendSketchInfo("Max6675 Temp Sensor", "1.0");

  // Present all sensors to controller
  gw.present(0, S_TEMP);
}

void loop()
{
  // Process incoming messages (like config from server)
  gw.process();

  temperature = temp0.read_temp(); // Read the temp

  if (temperature < 0) { // If there is an error with the TC, temperature will be < 0
    Serial.println("Thermocouple Error!!"); // There is a thermocouple error
  } else {
    Serial.print("Current Temperature: ");
    Serial.println( temperature ); // Print the temperature to Serial
    if (temperature != lastTemperature)
      gw.send(msg.setSensor(0).set(temperature, 1));
    lastTemperature = temperature;
  }

  gw.sleep(SLEEP_TIME);
}
