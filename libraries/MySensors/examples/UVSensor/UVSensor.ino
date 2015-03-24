#include <SPI.h>
#include <MySensor.h>  
/*
  Arduino UVM-30A

  connect the sensor as follows :

  +   >>> 5V
  -   >>> GND
  out >>> A0     
  
  Contribution: epierre, bulldoglowell, gizmocuz

  Index table taken from: http://www.elecrow.com/sensors-c-111/environment-c-111_112/uv-sensor-moduleuvm30a-p-716.html
  Because this table is pretty lineair, we can calculate a UVI with one decimal 

  License: Attribution-NonCommercial-ShareAlike 3.0 Unported (CC BY-NC-SA 3.0)
*/

#include <MySensor.h>  
#include <SPI.h>

#define UV_SENSOR_ANALOG_PIN 0

#define CHILD_ID_UV 0

unsigned long SLEEP_TIME = 30*1000; // Sleep time between reads (in milliseconds)

MySensor gw;
MyMessage uvMsg(CHILD_ID_UV, V_UV);

unsigned long lastSend =0; 
float uvIndex;
float lastUV = -1;
int uvIndexValue [12] = { 50, 227, 318, 408, 503, 606, 696, 795, 881, 976, 1079, 1170};

void setup()  
{ 
  gw.begin();

  // Send the sketch version information to the gateway and Controller
  gw.sendSketchInfo("UV Sensor", "1.2");

  // Register all sensors to gateway (they will be created as child devices)
  gw.present(CHILD_ID_UV, S_UV);
}

void loop()      
{
  unsigned long currentTime = millis();
  
  uint16_t uv = analogRead(UV_SENSOR_ANALOG_PIN);// Get UV value
  if (uv>1170)
    uv=1170;
    
  //Serial.print("UV Analog reading: ");
  //Serial.println(uv);
  
  int i;
  for (i = 0; i < 12; i++)
  {
    if (uv <= uvIndexValue[i]) 
    {
      uvIndex = i;
      break;
    }
  }
  
  //calculate 1 decimal if possible
  if (i>0) {
    float vRange=uvIndexValue[i]-uvIndexValue[i-1];
    float vCalc=uv-uvIndexValue[i-1];
    uvIndex+=(1.0/vRange)*vCalc-1.0;
  }

  //Serial.print("UVI: ");
  //Serial.println(uvIndex,2);

  //Send value to gateway if changed, or at least every 5 minutes
  if ((uvIndex != lastUV)||(currentTime-lastSend >= 5*60*1000)) {
      lastSend=currentTime;
      gw.send(uvMsg.set(uvIndex,2));
      lastUV = uvIndex;
  }
  
  gw.sleep(SLEEP_TIME);
}
