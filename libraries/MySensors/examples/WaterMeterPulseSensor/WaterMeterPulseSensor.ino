//
// Use this sensor to measure volume and flow of your house watermeter.
// You need to set the correct pulsefactor of your meter (pulses per m3).
// The sensor starts by fetching current volume reading from gateway (VAR 1).
// Reports both volume and flow back to gateway.
//
// Unfortunately millis() won't increment when the Arduino is in 
// sleepmode. So we cannot make this sensor sleep if we also want  
// to calculate/report flow.
//
// Sensor on pin 3


#include <Relay.h>
#include <Sleep_n0m1.h>
#include <SPI.h>
#include <EEPROM.h>  
#include <RF24.h>
#include <Sensor.h>  

#define DIGITAL_INPUT_SENSOR 3                  // The digital input you attached your sensor.  (Only 2 and 3 generates interrupt!)
#define PULSE_FACTOR 1000                       // Nummber of blinks per m3 of your meter (One rotation/liter)
#define SLEEP_MODE false                        // flowvalue can only be reported when sleep mode is false.
#define MAX_FLOW 40                             // Max flow (l/min) value to report. This filetrs outliers.
#define INTERRUPT DIGITAL_INPUT_SENSOR-2        // Usually the interrupt = pin -2 (on uno/nano anyway)
#define CHILD_ID 5                              // Id of the sensor child
unsigned long SEND_FREQUENCY = 20;              // Minimum time between send (in seconds). We don't want to spam the gateway.

Sensor gw;
Sleep sleep;                         
 
double ppl = ((double)PULSE_FACTOR)/1000;        // Pulses per liter

volatile unsigned long pulseCount = 0;   
volatile unsigned long lastBlink = 0;
volatile double flow = 0;   
unsigned long oldPulseCount = 0;
unsigned long newBlink = 0;   
double oldflow = 0;
double volume;                     
double oldvolume;
unsigned long lastSend;
unsigned long lastPulse;
unsigned long currentTime;
boolean metric;

void setup()  
{  
  gw.begin(); 

  // Send the sketch version information to the gateway and Controller
  gw.sendSketchInfo("Water Meter", "1.0");

  // Register this device as Waterflow sensor
  gw.sendSensorPresentation(CHILD_ID, S_WATER);       

  // Fetch last known pulse count value from gw
  pulseCount = oldPulseCount = atol(gw.getStatus(CHILD_ID, V_VAR1));
  //Serial.print("Last pulse count from gw:");
  //Serial.println(pulseCount);
  attachInterrupt(INTERRUPT, onPulse, RISING);
  lastSend = millis();
}


void loop()     
{ 
    currentTime = millis();
	
    // Only send values at a maximum frequency or woken up from sleep
  if (SLEEP_MODE || currentTime - lastSend > 1000*SEND_FREQUENCY) {
    // New flow value has been calculated  
    if (!SLEEP_MODE && flow != oldflow) {
      // Check that we dont get unresonable large flow value. 
      // could hapen when long wraps or false interrupt triggered
      if (flow<((unsigned long)MAX_FLOW)) {
        gw.sendVariable(CHILD_ID, V_FLOW, flow, 2);                   // Send flow value to gw
      }  
      //Serial.print("l/min:");
      //Serial.println(flow);
      oldflow = flow;
    }
  
    // No Pulse count in 2min 
	
	//Serial.print("currentTime");
	//Serial.println(currentTime);
	//Serial.print("lastPulse");
	//Serial.println(lastPulse);
 
    if(currentTime - lastPulse > 120000){
		flow = 0;
	} 
  
  
    // Pulse count has changed
    if (pulseCount != oldPulseCount) {
      gw.sendVariable(CHILD_ID, V_VAR1, pulseCount);                  // Send  volumevalue to gw VAR1
      double volume = ((double)pulseCount/((double)PULSE_FACTOR));     
      oldPulseCount = pulseCount;
      //Serial.print("Pulse count:");
      //Serial.println(pulseCount);
      if (volume != oldvolume) {
        gw.sendVariable(CHILD_ID, V_VOLUME, volume, 3);               // Send volume value to gw
        //Serial.print("m3:");
        //Serial.println(volume, 3);
        oldvolume = volume;
      } 
    }
    lastSend = currentTime;
  }
  
  if (SLEEP_MODE) {
    delay(300);                                                       //delay to allow serial to fully print before sleep
    gw.powerDown();
    sleep.pwrDownMode();                                              //set sleep mode									
    sleep.sleepDelay(SEND_FREQUENCY * 1000);                          //sleep for: sleepTime
  }
}


void onPulse()     
{ 
  if (!SLEEP_MODE) {
    unsigned long newBlink = micros();   
    unsigned long interval = newBlink-lastBlink;
	lastPulse = millis();
    if (interval<500000L) {
      // Sometimes we get interrupt on RISING,  500000 = 0.5sek debounce ( max 120 l/min)
      return;   
    }
    
    flow = (60000000.0 /interval) / ppl;
    lastBlink = newBlink;
	  //Serial.println(flow, 4);
  }
  pulseCount++; 
}



