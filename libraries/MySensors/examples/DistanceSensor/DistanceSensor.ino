#include <SPI.h>
#include <EEPROM.h>  
#include <RF24.h>
#include <MySensor.h>  
#include <NewPing.h>

#define CHILD_ID 1
#define TRIGGER_PIN  6  // Arduino pin tied to trigger pin on the ultrasonic sensor.
#define ECHO_PIN     5  // Arduino pin tied to echo pin on the ultrasonic sensor.
#define MAX_DISTANCE 300 // Maximum distance we want to ping for (in centimeters). Maximum sensor distance is rated at 400-500cm.
unsigned long SLEEP_TIME = 5; // Sleep time between reads (in seconds)

MySensor gw;
NewPing sonar(TRIGGER_PIN, ECHO_PIN, MAX_DISTANCE); // NewPing setup of pins and maximum distance.
MyMessage msg(CHILD_ID, V_DISTANCE);
int lastDist;
boolean metric = true; 

void setup()  
{ 
  gw.begin();

  // Send the sketch version information to the gateway and Controller
  gw.sendSketchInfo("Distance Sensor", "1.0");

  // Register all sensors to gw (they will be created as child devices)
  gw.present(CHILD_ID, S_DISTANCE);
  boolean metric = gw.getConfig().isMetric;
}

void loop()      
{     
  int dist = metric?sonar.ping_cm():sonar.ping_in();

  Serial.print("Ping: ");
  Serial.print(dist); // Convert ping time to distance in cm and print result (0 = outside set distance range)
  Serial.println(metric?" cm":" in");


  if (dist != lastDist) {
      gw.send(msg.set(dist));
      lastDist = dist;
  }

  delay(300); //delay to allow serial to fully print before sleep
  gw.sleep(SLEEP_TIME*1000);
}


