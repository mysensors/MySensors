#include <Sleep_n0m1.h>
#include <SPI.h>
#include <EEPROM.h>  
#include <RF24.h>
#include <Sensor.h>  
#include <NewPing.h>


// Set RADIO_ID to something unique in your sensor network (1-254)
// or set to AUTO if you want gw to assign a RADIO_ID for you.
#define RADIO_ID 8

#define TRIGGER_PIN  6  // Arduino pin tied to trigger pin on the ultrasonic sensor.
#define ECHO_PIN     5  // Arduino pin tied to echo pin on the ultrasonic sensor.
#define MAX_DISTANCE 300 // Maximum distance we want to ping for (in centimeters). Maximum sensor distance is rated at 400-500cm.

unsigned long SLEEP_TIME = 5; // Sleep time between reads (in seconds)

Sensor gw(9, 10);
NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE); // NewPing setup of pins and maximum distance.
Sleep sleep;

int lastDist;
boolean metric; 

void setup()  
{ 
  Serial.begin(BAUD_RATE);  // Used to type in characters
  gw.begin(RADIO_ID);

  // Register all sensors to gw (they will be created as child devices)
  gw.sendSensorPresentation(0, S_DISTANCE);
  metric = gw.isMetricSystem();
}

void loop()      
{     
  gw.powerUp(); // Power up radio
  int dist = metric?sonar.ping_cm():sonar.ping_in();

  Serial.print("Ping: ");
  Serial.print(dist); // Convert ping time to distance in cm and print result (0 = outside set distance range)
  Serial.println(metric?" cm":" in");


  if (dist != lastDist) {
      gw.sendVariable(0, V_DISTANCE, dist);
      lastDist = dist;
  }

  // Power down the radio.  Note that the radio will get powered back up
  // on the next write() call.
  delay(1000); //delay to allow serial to fully print before sleep
  gw.powerDown();
  sleep.pwrDownMode(); //set sleep mode
  sleep.sleepDelay(SLEEP_TIME * 1000); //sleep for: sleepTime 
}


