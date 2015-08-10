/*
  Vera Arduino Vibration Sensor

  connect the sensor as follows :

  VCC       >>> 5V
  S         >>> D3
  GND       >>> GND

  Based on: http://www.dfrobot.com/wiki/index.php/DFRobot_Digital_Vibration_Sensor_V2_SKU:DFR0027
  Contribution: epierre

# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# version 2 as published by the Free Software Foundation.

 
*/

#include <MySensor.h>
#include <SPI.h>
#include <Wire.h>

#define CHILD_ID_VIBRATION 0
#define VIBRATION_SENSOR_DIGITAL_PIN 3
#define SensorLED     13

unsigned long SLEEP_TIME = 10; // Sleep time between reads (in seconds)
//VARIABLES
int val = 0;           // variable to store the value coming from the sensor
float valVIBRATION =0.0;
float lastVIBRATION =0.0;
unsigned char state = 0;

MySensor gw;
MyMessage vibrationMsg(CHILD_ID_VIBRATION, V_VAR1);


void setup()  
{
  gw.begin();

  // Send the sketch version information to the gateway and Controller
  gw.sendSketchInfo("VIBRATION Sensor", "1.0");

  // Register all sensors to gateway (they will be created as child devices)
  gw.present(CHILD_ID_VIBRATION, S_VIBRATION);

  
  pinMode(VIBRATION_SENSOR_DIGITAL_PIN, INPUT);
  attachInterrupt(1, blink, FALLING);// Trigger the blink function when the falling edge is detected
  pinMode(SensorLED, OUTPUT);  
}

void loop()      
{    
  
  if(state>=40){ // basically below 40 so ignire basic level
        gw.send(vibrationMsg.set(int(state)));
        state = 0;  
        digitalWrite(SensorLED,HIGH);
   }    else {
        state = 0;  
        digitalWrite(SensorLED,LOW);
  } 

  
  // Power down the radio.  Note that the radio will get powered back up
  // on the next write() call.
  delay(1000); //delay to allow serial to fully print before sleep

  gw.sleep(SLEEP_TIME * 1000); //sleep for: sleepTime
}

void blink()//Interrupts function
{
  state++;
}
