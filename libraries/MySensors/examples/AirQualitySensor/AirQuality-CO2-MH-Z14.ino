/*

  MH-Z14 CO2 sensor

  Wiring:
    Pad 1, Pad 5: Vin (Voltage input 4.5V-6V) 
    Pad 2, Pad 3, Pad 12: GND 
    Pad 6: PWM output ==> pin 6

	From: http://davidegironi.blogspot.fr/2014/01/co2-meter-using-ndir-infrared-mh-z14.html
	  MH-Z14 has a PWM output, with a sensitivity range of 0ppm to 2000ppm CO2, an accurancy of ±200ppm.
	  The cycle is 1004ms±5%, given the duty cicle Th (pulse high), Tl is 1004-Th, we can convert it to CO2 value using the formula:
	  CO2ppm = 2000 * (Th - 2ms) /(Th + Tl - 4ms)
	From: http://airqualityegg.wikispaces.com/Sensor+Tests
	  - response time is less than 30 s
    - 3 minute warm up time
	datasheet: http://www.futurlec.com/Datasheet/Sensor/MH-Z14.pdf
	Contributor: epierre
*/


#include <MySensor.h>  
#include <SPI.h>

#define CHILD_ID_AIQ 0
#define AIQ_SENSOR_ANALOG_PIN 6

float valAIQ =0.0;
float lastAIQ =0.0;

MySensor gw;
MyMessage msg(CHILD_ID_AIQ, V_LEVEL);

void setup()  
{
  gw.begin();

  // Send the sketch version information to the gateway and Controller
  gw.sendSketchInfo("AIQ Sensor CO2 MH-Z14", "1.0");

  // Register all sensors to gateway (they will be created as child devices)
  gw.present(CHILD_ID_AIQ, S_AIR_QUALITY);  
  
  gw.sleep(30*1000);
  
  pinMode(AIQ_SENSOR_ANALOG_PIN, INPUT);
   
}

void loop() { 

  //unsigned long duration = pulseIn(AIQ_SENSOR_ANALOG_PIN, HIGH);
  while(digitalRead(AIQ_SENSOR_ANALOG_PIN) == HIGH) {;}
  //wait for the pin to go HIGH and measure HIGH time
  unsigned long duration = pulseIn(AIQ_SENSOR_ANALOG_PIN, HIGH);
  
  //Serial.print(duration/1000); Serial.println(" ms ");
  //from datasheet
	//CO2 ppm = 2000 * (Th - 2ms) / (Th + Tl - 4ms)
	//  given Tl + Th = 1004
	//        Tl = 1004 - Th
	//        = 2000 * (Th - 2ms) / (Th + 1004 - Th -4ms)
	//        = 2000 * (Th - 2ms) / 1000 = 2 * (Th - 2ms)
  long co2ppm = 2 * ((duration/1000) - 2);
  //Serial.print(co2ppm);
  if ((co2ppm != lastAIQ)&&(abs(co2ppm-lastAIQ)>=10)) {
      gw.send(msg.set((long)ceil(co2ppm)));
      lastAIQ = ceil(co2ppm);
  }
  
  //Serial.println();
  
  // Power down the radio.  Note that the radio will get powered back up
  // on the next write() call.
  gw.sleep(30*1003); //sleep for: sleepTime
}
