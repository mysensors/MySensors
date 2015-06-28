//
// Sample of using Async reading of Dallas Temperature Sensors
// 
#include <OneWire.h>
#include <DallasTemperature.h>

// Data wire is plugged into port 2 on the Arduino
#define ONE_WIRE_BUS 2

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

DeviceAddress tempDeviceAddress;

int  resolution = 12;
unsigned long lastTempRequest = 0;
int  delayInMillis = 0;
float temperature = 0.0;
int  idle = 0;
//
// SETUP
//
void setup(void)
{
  Serial.begin(115200);
  Serial.println("Dallas Temperature Control Library - Async Demo");
  Serial.print("Library Version: ");
  Serial.println(DALLASTEMPLIBVERSION);
  Serial.println("\n");

  sensors.begin();
  sensors.getAddress(tempDeviceAddress, 0);
  sensors.setResolution(tempDeviceAddress, resolution);
  
  sensors.setWaitForConversion(false);
  sensors.requestTemperatures();
  delayInMillis = 750 / (1 << (12 - resolution)); 
  lastTempRequest = millis(); 
  
  pinMode(13, OUTPUT); 
}

void loop(void)
{ 
  
  if (millis() - lastTempRequest >= delayInMillis) // waited long enough??
  {
    digitalWrite(13, LOW);
    Serial.print(" Temperature: ");
    temperature = sensors.getTempCByIndex(0);
    Serial.println(temperature, resolution - 8); 
    Serial.print("  Resolution: ");
    Serial.println(resolution); 
    Serial.print("Idle counter: ");
    Serial.println(idle);     
    Serial.println(); 
    
    idle = 0; 
        
    // immediately after fetching the temperature we request a new sample 
	// in the async modus
    // for the demo we let the resolution change to show differences
    resolution++;
    if (resolution > 12) resolution = 9;
    
    sensors.setResolution(tempDeviceAddress, resolution);
    sensors.requestTemperatures(); 
    delayInMillis = 750 / (1 << (12 - resolution));
    lastTempRequest = millis(); 
  }
  
  digitalWrite(13, HIGH);
  // we can do usefull things here 
  // for the demo we just count the idle time in millis
  delay(1);
  idle++;
}
