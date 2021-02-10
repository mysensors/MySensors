/*
   The MySensors Arduino library handles the wireless radio link and protocol
   between your home built sensors/actuators and HA controller of choice.
   The sensors forms a self healing radio network with optional repeaters. Each
   repeater and gateway builds a routing tables in EEPROM which keeps track of the
   network topology allowing messages to be routed to nodes.

   Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
   Copyright (C) 2013-2020 Sensnology AB
   Full contributor list: https://github.com/mysensors/MySensors/graphs/contributors

   Documentation: http://www.mysensors.org
   Support Forum: http://forum.mysensors.org

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as published by the Free Software Foundation.

 *******************************

   DESCRIPTION

   This is an example that demonstrates how to report the battery level for a sensor
   using the internal measurement method.
   Instructions for measuring battery capacity are available here:
   http://www.mysensors.org/build/battery

*/

// Enable debug prints to serial monitor
#define MY_DEBUG

// Enable and select radio type attached
#define MY_RADIO_RF24
//#define MY_RADIO_NRF5_ESB
//#define MY_RADIO_RFM69
//#define MY_RADIO_RFM95

#include <MySensors.h>

uint32_t SLEEP_TIME = 900000;  // sleep time between reads (seconds * 1000 milliseconds)
int oldBatteryPcnt = 0;
#define FULL_BATTERY 3 // 3V for 2xAA alkaline. Adjust if you use a different battery setup.

void setup()
{
}

void presentation()
{
  // Send the sketch version information to the gateway and Controller
  sendSketchInfo("Battery Meter", "1.0");
}

void loop()
{
  // get the battery Voltage
  long batteryMillivolts = readVcc();
  int batteryPcnt = batteryMillivolts / FULL_BATTERY / 1000.0 * 100 + 0.5;
#ifdef MY_DEBUG
  Serial.print("Battery Voltage: ");
  Serial.println(batteryMillivolts / 1000.0);
  Serial.print("V");
  Serial.print("Battery percent: ");
  Serial.print(batteryPcnt);
  Serial.println(" %");
#endif

  if (oldBatteryPcnt != batteryPcnt) {
    sendBatteryLevel(batteryPcnt);
    oldBatteryPcnt = batteryPcnt;
  }
  sleep(SLEEP_TIME);
}

long readVcc() {
  // Read 1.1V reference against AVcc
  // set the reference to Vcc and the measurement to the internal 1.1V reference
#if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
  ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
  ADMUX = _BV(MUX5) | _BV(MUX0);
#elif defined (__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
  ADMUX = _BV(MUX3) | _BV(MUX2);
#else
  ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
#endif

  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Start conversion
  while (bit_is_set(ADCSRA, ADSC)); // measuring

  uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH
  uint8_t high = ADCH; // unlocks both

  long result = (high << 8) | low;

  result = 1125300L / result; // Calculate Vcc (in mV); 1125300 = 1.1*1023*1000
  return result; // Vcc in millivolts
}
