#include <OneWire.h>
#include <DallasTemperature.h>

// Data wire is plugged into port 2 on the Arduino
#define ONE_WIRE_BUS 2

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

void setup(void)
{
  // start serial port
  Serial.begin(115200);
  Serial.println("Dallas Temperature Control Library - Async Demo");
  Serial.println("\nDemo shows the difference in length of the call\n\n");

  // Start up the library
  sensors.begin();
}

void loop(void)
{ 
  // Request temperature conversion (traditional)
  Serial.println("Before blocking requestForConversion");
  unsigned long start = millis();    

  sensors.requestTemperatures();

  unsigned long stop = millis();
  Serial.println("After blocking requestForConversion");
  Serial.print("Time used: ");
  Serial.println(stop - start);
  
  // get temperature
  Serial.print("Temperature: ");
  Serial.println(sensors.getTempCByIndex(0));  
  Serial.println("\n");
  
  // Request temperature conversion - non-blocking / async
  Serial.println("Before NON-blocking/async requestForConversion");
  start = millis();       
  sensors.setWaitForConversion(false);  // makes it async
  sensors.requestTemperatures();
  sensors.setWaitForConversion(true);
  stop = millis();
  Serial.println("After NON-blocking/async requestForConversion");
  Serial.print("Time used: ");
  Serial.println(stop - start); 
  
  
  // 9 bit resolution by default 
  // Note the programmer is responsible for the right delay
  // we could do something usefull here instead of the delay
  int resolution = 9;
  delay(750/ (1 << (12-resolution)));
  
  // get temperature
  Serial.print("Temperature: ");
  Serial.println(sensors.getTempCByIndex(0));  
  Serial.println("\n\n\n\n");  
  
  delay(5000);
}
