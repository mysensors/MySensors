#include <Sleep_n0m1.h>
#include <SPI.h>
#include <EEPROM.h>  
#include <DallasTemperature.h>
#include <OneWire.h>
#include <RF24.h>
#include <Sensor.h>  

#define ONE_WIRE_BUS 3 // Pin where dallase sensor is connected 
#define MAX_ATTACHED_DS18B20 16
unsigned long SLEEP_TIME = 30; // Sleep time between reads (in seconds)

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
Sensor gw;
Sleep sleep;
float lastTemperature[MAX_ATTACHED_DS18B20];
int numSensors=0;
boolean metric = true; 

void setup()  
{ 
  sensors.begin();
  gw.begin(); 

  // Send the sketch version information to the gateway and Controller
  gw.sendSketchInfo("Temperature Sensor", "1.0");

  // Fetch the number of attached sensors  
  numSensors = sensors.getDeviceCount();
  // Register all sensors to gw (they will be created as child devices)
  for (int i=0; i<numSensors && i<MAX_ATTACHED_DS18B20; i++) {   
     gw.sendSensorPresentation(i, S_TEMP);
  }
  metric = gw.isMetricSystem();
}

void loop()     
{     
  sensors.requestTemperatures(); // Fetch temperatures from Dallas
  delay(100);
  for (int i=0; i<numSensors && i<MAX_ATTACHED_DS18B20; i++) {
    // Fetch and round temperature to one decimal
    float temperature = static_cast<float>(static_cast<int>((metric?sensors.getTempCByIndex(i):sensors.getTempFByIndex(i)) * 10.)) / 10.;
    // Only send data if temperature has changed and no error
    if (lastTemperature[i] != temperature && temperature != -127.00) {
      gw.powerUp(); // Powerup introduces a small delay (which is missing in radio.write powerup)
      // Send variable (using registered shortcut) to gw
      gw.sendVariable(i, V_TEMP, temperature, 1);
      lastTemperature[i]=temperature;
    }
  }
  // Power down the radio.  Note that the radio will get powered back up
  // on the next write() call.
  delay(500);
  gw.powerDown();
  sleep.pwrDownMode(); //set sleep mode
  sleep.sleepDelay(SLEEP_TIME * 1000); //sleep for: sleepTime 
}


