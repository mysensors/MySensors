// Use this sensor to measure KWH and Watt of your house meeter
// You need to set the correct pulsefactor of your meeter (blinks per KWH).
// The sensor starts by fetching current KWH value from gateway.
// Reports both KWH and Watt back to gateway.
//
// Unfortunately millis() won't increment when the Arduino is in 
// sleepmode. So we cannot make this sensor sleep if we also want 
// to calculate/report watt-number.

#include <Sleep_n0m1.h>
#include <SPI.h>
#include <EEPROM.h>  
#include <RF24.h>
#include <Sensor.h>  

#define DIGITAL_INPUT_SENSOR 3  // The digital input you attached your light sensor.  (Only 2 and 3 generates interrupt!)
#define PULSE_FACTOR 1000       // Nummber of blinks per KWH of your meeter
#define SLEEP_MODE false        // Watt-value can only be reported when sleep mode is false.
#define MAX_WATT 10000          // Max watt value to report. This filetrs outliers.
#define INTERRUPT DIGITAL_INPUT_SENSOR-2 // Usually the interrupt = pin -2 (on uno/nano anyway)
#define CHILD_ID 1   // Id of the sensor child
unsigned long SEND_FREQUENCY = 20; // Minimum time between send (in seconds). We don't wnat to spam the gateway.

Sensor gw;
Sleep sleep;

//double kwhPerBlink = 1.0/((double)PULSE_FACTOR); 
double ppwh = ((double)PULSE_FACTOR)/1000; // Pulses per watt hour

volatile unsigned long pulseCount = 0;   
volatile unsigned long lastBlink = 0;
volatile unsigned long watt = 0;
unsigned long oldPulseCount = 0;   
unsigned long oldWatt = 0;
double oldKwh;
unsigned long lastSend;

void setup()  
{  
  gw.begin();

  // Send the sketch version information to the gateway and Controller
  gw.sendSketchInfo("Energy Meter", "1.0");

  // Register this device as power sensor
  gw.sendSensorPresentation(CHILD_ID, S_POWER);

  // Fetch last known pulse count value from gw
  pulseCount = oldPulseCount = atol(gw.getStatus(CHILD_ID, V_VAR1));
  Serial.print("Last pulse count from gw:");
  Serial.println(pulseCount);
  attachInterrupt(INTERRUPT, onPulse, RISING);
  lastSend=millis();
}


void loop()     
{ 
   unsigned long now = millis();
  // Only send values at a maximum frequency or woken up from sleep
  if (SLEEP_MODE || now - lastSend > 1000*SEND_FREQUENCY) {
    // New watt value has been calculated  
    if (!SLEEP_MODE && watt != oldWatt) {
      // Check that we dont get unresonable large watt value. 
      // could hapen when long wraps or false interrupt triggered
      if (watt<((unsigned long)MAX_WATT)) {
        gw.sendVariable(CHILD_ID, V_WATT, watt);  // Send watt value to gw 
      }  
      Serial.print("Watt:");
      Serial.println(watt);
      oldWatt = watt;
    }
  
    // Pulse cout has changed
    if (pulseCount != oldPulseCount) {
      gw.sendVariable(CHILD_ID, V_VAR1, pulseCount);  // Send kwh value to gw 
      double kwh = ((double)pulseCount/((double)PULSE_FACTOR));     
      oldPulseCount = pulseCount;
      //Serial.print("Pulse count:");
      //Serial.println(pulseCount);
      if (kwh != oldKwh) {
        gw.sendVariable(CHILD_ID, V_KWH, kwh, 4);  // Send kwh value to gw 
        //Serial.print("KWH:");
        //Serial.println(kwh);
        oldKwh = kwh;
      }
    }
    lastSend = now;
  }
  
  if (SLEEP_MODE) {
    delay(300); //delay to allow serial to fully print before sleep
    gw.powerDown();
    sleep.pwrDownMode(); //set sleep mode
    sleep.sleepDelay(SEND_FREQUENCY * 1000); //sleep for: sleepTime 
  }
}

void onPulse()     
{ 
  if (!SLEEP_MODE) {
    unsigned long newBlink = micros();  
    unsigned long interval = newBlink-lastBlink;
    if (interval<10000L) { // Sometimes we get interrupt on RISING
      return;
    }
    watt = (3600000000.0 /interval) / ppwh;
    lastBlink = newBlink;
  } 
  pulseCount++;
}



