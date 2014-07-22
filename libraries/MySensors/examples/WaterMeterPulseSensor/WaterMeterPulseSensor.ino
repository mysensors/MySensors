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

#include <SPI.h>
#include <MySensor.h>  

#define DIGITAL_INPUT_SENSOR 3                  // The digital input you attached your sensor.  (Only 2 and 3 generates interrupt!)
#define PULSE_FACTOR 1000                       // Nummber of blinks per m3 of your meter (One rotation/liter)
#define SLEEP_MODE false                        // flowvalue can only be reported when sleep mode is false.
#define MAX_FLOW 40                             // Max flow (l/min) value to report. This filetrs outliers.
#define INTERRUPT DIGITAL_INPUT_SENSOR-2        // Usually the interrupt = pin -2 (on uno/nano anyway)
#define CHILD_ID 5                              // Id of the sensor child
unsigned long SEND_FREQUENCY = 20000;           // Minimum time between send (in milliseconds). We don't want to spam the gateway.

MySensor gw;
 
double ppl = ((double)PULSE_FACTOR)/1000;        // Pulses per liter

volatile unsigned long pulseCount = 0;   
volatile unsigned long lastBlink = 0;
volatile double flow = 0;  
boolean pcReceived = false;
unsigned long oldPulseCount = 0;
unsigned long newBlink = 0;   
double oldflow = 0;
double volume;                     
double oldvolume;
unsigned long lastSend;
unsigned long lastPulse;
unsigned long currentTime;
boolean metric;
MyMessage flowMsg(CHILD_ID,V_FLOW);
MyMessage volumeMsg(CHILD_ID,V_VOLUME);
MyMessage pcMsg(CHILD_ID,V_VAR1);


void setup()  
{  
  gw.begin(incomingMessage); 

  // Send the sketch version information to the gateway and Controller
  gw.sendSketchInfo("Water Meter", "1.0");

  // Register this device as Waterflow sensor
  gw.present(CHILD_ID, S_WATER);       

  // Fetch last known pulse count value from gw
  gw.request(CHILD_ID, V_VAR1);

  attachInterrupt(INTERRUPT, onPulse, RISING);
  lastSend = millis();
}


void loop()     
{ 
  gw.process();
  currentTime = millis();
	
    // Only send values at a maximum frequency or woken up from sleep
  bool sendTime = currentTime - lastSend > SEND_FREQUENCY;
  if (pcReceived && (SLEEP_MODE || sendTime)) {
    // New flow value has been calculated  
    if (!SLEEP_MODE && flow != oldflow) {
      // Check that we dont get unresonable large flow value. 
      // could hapen when long wraps or false interrupt triggered
      if (flow<((unsigned long)MAX_FLOW)) {
        gw.send(flowMsg.set(flow, 2));                   // Send flow value to gw
      }  
      //Serial.print("l/min:");
      //Serial.println(flow);
      oldflow = flow;
    }
  
    // No Pulse count in 2min 
    if(currentTime - lastPulse > 120000){
      flow = 0;
    } 
  
  
    // Pulse count has changed
    if (pulseCount != oldPulseCount) {
      gw.send(pcMsg.set(pulseCount));                  // Send  volumevalue to gw VAR1
      double volume = ((double)pulseCount/((double)PULSE_FACTOR));     
      oldPulseCount = pulseCount;
      if (volume != oldvolume) {
        gw.send(volumeMsg.set(volume, 3));               // Send volume value to gw
        oldvolume = volume;
      } 
    }
    lastSend = currentTime;
  } else if (sendTime) {
   // No count received. Try requesting it again
    gw.request(CHILD_ID, V_VAR1);
  }
  
  if (SLEEP_MODE) {
    gw.sleep(SEND_FREQUENCY);
  }
}

void incomingMessage(const MyMessage &message) {
  if (message.type==V_VAR1) {  
    pulseCount = oldPulseCount = message.getLong();
    Serial.print("Received last pulse count from gw:");
    Serial.println(pulseCount);
    pcReceived = true;
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
  }
  pulseCount++; 
}



