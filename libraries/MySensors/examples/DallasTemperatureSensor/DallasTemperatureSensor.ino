// Example sketch showing how to send in OneWire temperature readings
#include <MySensor.h>  
#include <Sleep_n0m1.h>
#include <SPI.h>
#include <DallasTemperature.h>
#include <OneWire.h>
#include <RF24.h>

#define ONE_WIRE_BUS 3 // Pin where dallase sensor is connected 
#define MAX_ATTACHED_DS18B20 16
unsigned long SLEEP_TIME = 30; // Sleep time between reads (in seconds)
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
MySensor gw;
Sleep sleep;
float lastTemperature[MAX_ATTACHED_DS18B20];
int numSensors=0;
boolean receivedConfig = false;
boolean metric = true; 
MyMessage out;

void setup()  
{ 
  // Startup OneWire 
  sensors.begin();

  // Startup and initialize MySensors library. Set callback for incoming messages. 
  gw.begin(); 

  // Send the sketch version information to the gateway and Controller
  gw.sendSketchInfo("Temperature Sensor", "1.0");

  // Fetch the number of attached temperature sensors  
  numSensors = sensors.getDeviceCount();

  // Present all sensors to controller
  for (int i=0; i<numSensors && i<MAX_ATTACHED_DS18B20; i++) {   
     gw.present(i, S_TEMP);
  }

    // Initialize outgoing message to always send temperature
  out.type = V_TEMP;
}


void loop()     
{     
  // Process incoming messages (like config from server)
  gw.process(); 

  // Fetch temperatures from Dallas sensors
  sensors.requestTemperatures(); 

  // Read temperatures and send them to controller 
  for (int i=0; i<numSensors && i<MAX_ATTACHED_DS18B20; i++) {
 
    // Fetch and round temperature to one decimal
    float temperature = static_cast<float>(static_cast<int>((gw.getConfig().isMetric?sensors.getTempCByIndex(i):sensors.getTempFByIndex(i)) * 10.)) / 10.;
 
    // Only send data if temperature has changed and no error
    if (lastTemperature[i] != temperature && temperature != -127.00) {
 
      // Send in the new temperature
      gw.send(out.setSensor(i)->set(temperature,1));
      lastTemperature[i]=temperature;
    }
  }
  // Power down the radio and sleep Arduino to save batteries.  
  delay(500);
  gw.powerDown();
  sleep.pwrDownMode(); //set sleep mode
  sleep.sleepDelay(SLEEP_TIME * 1000); //sleep for: sleepTime 
}



