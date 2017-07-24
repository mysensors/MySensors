/**
   The MySensors Arduino library handles the wireless radio link and protocol
   between your home built sensors/actuators and HA controller of choice.
   The sensors forms a self healing radio network with optional repeaters. Each
   repeater and gateway builds a routing tables in EEPROM which keeps track of the
   network topology allowing messages to be routed to nodes.
   Created by mboyer85
   Full contributor list: https://github.com/mysensors/Arduino/graphs/contributors
   Documentation: http://www.mysensors.org
   Support Forum: http://forum.mysensors.org
   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as published by the Free Software Foundation.
 *******************************
   DESCRIPTION
   Example sketch showing how to send PH readings back to the controller
*/

// Enable debug prints
#define MY_DEBUG

// Enable and select radio type attached 
#define MY_RADIO_NRF24
//#define MY_RADIO_RFM69
//#define MY_RS485

// Define a lower baud rate for Arduino's running on 8 MHz (Arduino Pro Mini 3.3V & SenseBender)
#if F_CPU == 8000000L
#define MY_BAUD_RATE 38400
#endif
 
#include <SPI.h>
#include <MySensors.h>  
#include <DallasTemperature.h>
#include <OneWire.h>

#define MAX_ATTACHED_DS18B20 6

#define ONE_WIRE_BUS 4 // Pin where dallase sensor is connected 

#define SENSOR_TEMP_OFFSET 0

#define CHILD_ID_PH 10
#define ArrayLenth 10               // times of collection
#define COMPARE_PH 0 //only send PH if different
#define PH_SENSOR_ANALOG_PIN A0         // pH meter Analog output to Arduino Analog Input 0
#define LED_DIGITAL_PIN 13
#define Offset 0.00           //deviation compensate

unsigned long int avgValue;  //Store the average value of the sensor feedback
float b;
int buf[10],temp;

unsigned long SLEEP_TIME = 30000;
bool metric = true;
bool receivedConfig = false;
int numSensors=0;

float lastPH;

OneWire oneWire(ONE_WIRE_BUS); // Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
DallasTemperature sensors(&oneWire); // Pass the oneWire reference to Dallas Temperature. 

MyMessage msgDallas(0,V_TEMP);
// Initialize PH message
MyMessage msg(CHILD_ID_PH, V_PH);

void before()
{
  // Startup up the OneWire library
  sensors.begin();
}

void setup()
{
  //Setup your PH sensor here (I2C,Serial,Phidget...)
  Serial.begin(MY_BAUD_RATE);
  // requestTemperatures() will not block current thread
  sensors.setWaitForConversion(false);    
}

void presentation()
{
  // Send the sketch version information to the gateway and Controller
  sendSketchInfo("PHandDallasSensor", "1.1");

 // Fetch the number of attached temperature sensors  
  numSensors = sensors.getDeviceCount();

  // Present all sensors to controller
  for (int i=0; i<numSensors && i<MAX_ATTACHED_DS18B20; i++) {   
     present(i, S_TEMP);
  }
   
  
  present(CHILD_ID_PH, S_WATER_QUALITY);

  metric = getControllerConfig().isMetric;

}

void loop()
{

  #ifdef MY_DEBUG
  Serial.print("Sensors: ");
  Serial.println(numSensors);
  #endif


  // Fetch temperatures from Dallas sensors
  sensors.requestTemperatures();

  // query conversion time and sleep until conversion completed
  int16_t conversionTime = sensors.millisToWaitForConversion(sensors.getResolution());
  // sleep() call can be replaced by wait() call if node need to process incoming messages (or if node is repeater)
  sleep(conversionTime);

// Read temperatures and send them to controller 
  for (int i=0; i<numSensors && i<MAX_ATTACHED_DS18B20; i++) {
 
    // Fetch and round temperature to one decimal
    float temperature = static_cast<float>(static_cast<int>((getControllerConfig().isMetric?sensors.getTempCByIndex(i):sensors.getTempFByIndex(i)) * 10.)) / 10.;
 
  // Send in the new temperature
  send(msgDallas.setSensor(i).set(temperature,1));
      #ifdef MY_DEBUG
      Serial.print("Dallas");
      Serial.print(i);
      Serial.print(" :");
      Serial.println(temperature);
      #endif

//  sleep(SLEEP_TIME);
  }


    float ph = getPH();
    
    #if COMPARE_PH == 1
      if (lastPH != ph) {
    #endif
    
    // Send in the new PH value
    #ifdef MY_DEBUG
    Serial.print("PH: ");
    Serial.println(ph);
    #endif
    send(msg.set(ph, 1));
    // Save new PH value for next compare
    lastPH = ph;

#if COMPARE_PH == 1
  }
#endif

  sleep(SLEEP_TIME);
}


float getPH()
{
  //query your PH sensor here (I2C,Serial,Phidget...)
  for(int i=0;i<10;i++)       //Get 10 sample value from the sensor for smooth the value
  { 
    #ifdef MY_DEBUG
    Serial.print("PH Analog PIN: ");
    Serial.println(analogRead(PH_SENSOR_ANALOG_PIN));
    #endif
    buf[i]=analogRead(PH_SENSOR_ANALOG_PIN);
    delay(10);
  }
  for(int i=0;i<9;i++)        //sort the analog from small to large
  {
    for(int j=i+1;j<10;j++)
    {
      if(buf[i]>buf[j])
      {
        temp=buf[i];
        buf[i]=buf[j];
        buf[j]=temp;
      }
    }
  }
  avgValue=0;
  for(int i=2;i<8;i++)                      //take the average value of 6 center sample
    avgValue+=buf[i];
  float phValue=(float)avgValue*5.0/1024/6; //convert the analog into millivolt
  phValue=3.5*phValue;                      //convert the millivolt into pH value
  #ifdef MY_DEBUG
  Serial.print("PH: ");
  Serial.println(phValue);
  #endif
  return phValue;
}
