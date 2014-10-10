
// Example sketch showing how to request time from controller. 
// Created by Henrik Ekblad <henrik.ekblad@mysensors.org>

#include <SPI.h>
#include <MySensor.h>  
#include <Time.h>  

MySensor gw;
boolean timeReceived = false;
unsigned long lastUpdate=0, lastRequest=0;

void setup()  
{  
  gw.begin();

  // Send the sketch version information to the gateway and Controller
  gw.sendSketchInfo("Clock", "1.0");

  // Request time from controller. 
  gw.requestTime(receiveTime);  
}

// This is called when a new time value was received
void receiveTime(unsigned long time) {
  // Ok, set incoming time 
  setTime(time);
  timeReceived = true;
}
 
void loop()     
{     
  unsigned long now = millis();
  gw.process();
  
   // If no time has been received yet, request it every 10 second from controller
  // When time has been received, request update every hour
  if ((!timeReceived && now-lastRequest > 10*1000)
    || (timeReceived && now-lastRequest > 60*1000*60)) {
    // Request time from controller. 
    Serial.println("requesting time");
    gw.requestTime(receiveTime);  
    lastRequest = now;
  }
  
  // Print time every second
  if (timeReceived && now-lastUpdate > 1000) {
    Serial.print(hour());
    printDigits(minute());
    printDigits(second());
    Serial.print(" ");
    Serial.print(day());
    Serial.print(" ");
    Serial.print(month());
    Serial.print(" ");
    Serial.print(year()); 
    Serial.println(); 
    lastUpdate = now;
  }
}

void printDigits(int digits){
  // utility function for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if(digits < 10)
    Serial.print('0');
  Serial.print(digits);
}


