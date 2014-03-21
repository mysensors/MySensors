
// Example sketch showing how to request time from vera
// and how to set a cutom variable on a sensor

#include <SPI.h>
#include <EEPROM.h>  
#include <RF24.h>
#include <Sensor.h>  
#include <Time.h>  

#define CHILD_ID 0   // Id of the sensor child

Sensor gw;


void setup()  
{  
  gw.begin();

  // Send the sketch version information to the gateway and Controller
  gw.sendSketchInfo("Clock", "1.0");

  // Register any sensortype. This example we just create a motion sensor.
  gw.sendSensorPresentation(CHILD_ID, S_MOTION);

  setSyncProvider(getTime);  
  setSyncInterval(3600);  // get time from gw every hour
}

unsigned long  getTime() {
  return gw.getTime(); // Vera is time provider
}
 
void loop()     
{     
//  long time = gw.getTime();
//  setTime(time);  
 
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
   
   // Ok now send time back to gw and store in in Variable1 
   // of the motion sensor device
   gw.sendVariable(CHILD_ID, V_VAR1, minute()); 
       
   // wait 10 seconds...
   delay(10000);     
}

void printDigits(int digits){
  // utility function for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if(digits < 10)
    Serial.print('0');
  Serial.print(digits);
}


