/*
  Vera Arduino UVM-30A

  connect the sensor as follows :

  +   >>> 5V
  -   >>> GND
  out >>> A0     
  
  Contribution: epierre, bulldoglowell
  Converted to 1.4 by Henrik Ekblad

  License: Attribution-NonCommercial-ShareAlike 3.0 Unported (CC BY-NC-SA 3.0)
 
*/

#include <MySensor.h>  
#include <SPI.h>

#define CHILD_ID_UV 0
#define UV_SENSOR_ANALOG_PIN 0
unsigned long SLEEP_TIME = 30*1000; // Sleep time between reads (in milliseconds)

MySensor gw;
MyMessage uvMsg(CHILD_ID_UV, V_UV);
int lastUV = -1;
int uvIndexValue [13] = { 50, 227, 318, 408, 503, 606, 696, 795, 881, 976, 1079, 1170, 3000};
int uvIndex;

void setup()  
{ 
  gw.begin();

  // Send the sketch version information to the gateway and Controller
  gw.sendSketchInfo("UV Sensor", "1.1");

  // Register all sensors to gateway (they will be created as child devices)
  gw.present(CHILD_ID_UV, S_UV);

}

void loop()      
{     
  uint16_t uv = analogRead(0);// Get UV value
  Serial.print("Uv reading: ");
  Serial.println(uv);
  for (int i = 0; i < 13; i++)
  {
    if (uv <= uvIndexValue[i]) 
    {
      uvIndex = i;
      break;
    }
  }
  Serial.print("Uv index: ");
  Serial.println(uvIndex);

  if (uvIndex != lastUV) {
      gw.send(uvMsg.set(uvIndex));
      lastUV = uvIndex;
  }
  
  gw.sleep(SLEEP_TIME);
}
