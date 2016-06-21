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
 * Example sketch showing how to interface with a GPS sensor. 
 * A GPS is also an excellent atomic time source.
 * http://www.mysensors.org/build/gps
 */ 

// Enable debug prints to serial monitor
#define MY_DEBUG 

// Enable and select radio type attached
#define MY_RADIO_NRF24
//#define MY_RADIO_RFM69

// GPS position send interval (in millisectonds)
#define GPS_SEND_INTERVAL 10000 

// The child id used for the gps sensor
#define CHILD_ID_GPS 1

// This is where the pin TX pin of your GPS sensor is connected to the arduino
static const int GPS_PIN = A0;

// GPS Baud rate (note this is not the same as your serial montor baudrate). Most GPS modules uses 9600 bps.
static const uint32_t GPSBaud = 9600;

// Offset hours adjustment from gps time (UTC)
const int offset = 1;   

#include <SPI.h>
#include <Time.h>
#include <MySensors.h>
#include <TinyGPS++.h>
#include <SoftwareSerial.h>

// TinyGPS++ is used for parsing serial gps data
TinyGPSPlus gps;

MyMessage msg(CHILD_ID_GPS, V_POSITION);

// The serial connection to the GPS device
// A5 pin can be left unconnected 
SoftwareSerial ss(GPS_PIN, A5); 

// Last time GPS position was sent to controller
unsigned long lastGPSSent = 0;

// Some buffers 
char latBuf[11];
char lngBuf[11];   
char altBuf[6];
char payload[30];
char sz[64];


void setup() {
  // Set baudrate form gps communication
  ss.begin(GPSBaud);
}


void present() {
  // Send the sketch version information to the gateway and Controller
  sendSketchInfo("GPS Sensor", "1.0");

  // Register all sensors to gateway (they will be created as child devices)
  present(CHILD_ID_GPS, S_GPS);
}

void loop()
{
  unsigned long currentTime = millis();

  // Evaluate if it is time to send a new position
  bool timeToSend = currentTime - lastGPSSent > GPS_SEND_INTERVAL;

  // Read gps data  
  while (ss.available()) 
    gps.encode(ss.read());
  
  if (timeToSend) {
    // Sync gps time with Arduino
    updateTime();

    // Send current gps location
    if (gps.location.isValid() && gps.altitude.isValid()) {
      // Build position and altitude string to send
      dtostrf(gps.location.lat(), 1, 6, latBuf);
      dtostrf(gps.location.lng(), 1, 6, lngBuf);
      dtostrf(gps.altitude.meters(), 1, 0, altBuf);
      sprintf(payload, "%s;%s;%s", latBuf, lngBuf, altBuf);

      Serial.print(F("Position: "));
      Serial.println(payload);

      send(msg.set(payload));

      Serial.print(F("GPS Time: "));
      sprintf(sz, "%02d/%02d/%02d %02d:%02d:%02d", month(), day(), year(), hour(), minute(), second());
      Serial.println(sz);

      
    } else {
      if (millis() > 5000 && gps.charsProcessed() < 10)
        Serial.println(F("No GPS data received: check wiring"));
      else
        Serial.println(F("No GPS data received yet..."));
    }
    lastGPSSent = currentTime;
  }
}

void updateTime()
{
  TinyGPSDate d = gps.date;
  TinyGPSTime t = gps.time;
  if (d.isValid() && t.isValid()) {
    // set the Time to the latest GPS reading if less then 0.2 seconds old
    setTime(t.hour(), t.minute(), t.second(), d.day(), d.month(), d.year());
    adjustTime(offset * SECS_PER_HOUR);
    return;
  } 
  Serial.println(F("Unable to adjust time from GPS"));
}


