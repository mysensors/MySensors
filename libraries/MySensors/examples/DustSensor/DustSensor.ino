/*
  Arduino Dust Sensort

  connect the sensor as follows :

  VCC       >>> 5V
  A         >>> A0
  GND       >>> GND

  Based on: http://www.dfrobot.com/wiki/index.php/Sharp_GP2Y1010AU 
  Authors: Cyrille Médard de Chardon (serialC), Christophe Trefois (Trefex)
  Contribution: epierre
  COnverted to 1.4 by Henrik Ekblad
  
  The dust sensor used (see purchase guide for latest link):
  http://rover.ebay.com/rover/1/711-53200-19255-0/1?icep_ff3=2&pub=5575069610&toolid=10001&campid=5337433187&customid=&icep_item=171259125886&ipn=psmain&icep_vectorid=229466&kwid=902099&mtid=824&kw=lg
  
*/

#include <MySensor.h>  
#include <SPI.h>

#define CHILD_ID_DUST 0
#define DUST_SENSOR_ANALOG_PIN 1

unsigned long SLEEP_TIME = 30*1000; // Sleep time between reads (in milliseconds)
//VARIABLES
int val = 0;           // variable to store the value coming from the sensor
float valDUST =0.0;
float lastDUST =0.0;
int samplingTime = 280;
int deltaTime = 40;
int sleepTime = 9680;
float voMeasured = 0;
float calcVoltage = 0;
float dustDensity = 0;

MySensor gw;
MyMessage dustMsg(CHILD_ID_DUST, V_DUST_LEVEL);

void setup()  
{
  gw.begin();

  // Send the sketch version information to the gateway and Controller
  gw.sendSketchInfo("Dust Sensor", "1.1");

  // Register all sensors to gateway (they will be created as child devices)
  gw.present(CHILD_ID_DUST, S_DUST);  
   
}

void loop()      
{    
  uint16_t voMeasured = analogRead(DUST_SENSOR_ANALOG_PIN);// Get DUST value

  // 0 - 5V mapped to 0 - 1023 integer values
  // recover voltage
  calcVoltage = voMeasured * (5.0 / 1024.0);

  // linear eqaution taken from http://www.howmuchsnow.com/arduino/airquality/
  // Chris Nafis (c) 2012
  dustDensity = (0.17 * calcVoltage - 0.1)*1000;
 
  Serial.print("Raw Signal Value (0-1023): ");
  Serial.print(voMeasured);
  
  Serial.print(" - Voltage: ");
  Serial.print(calcVoltage);
  
  Serial.print(" - Dust Density: ");
  Serial.println(dustDensity); // unit: ug/m3
 
  if (ceil(dustDensity) != lastDUST) {
      gw.send(dustMsg.set((int)ceil(dustDensity)));
      lastDUST = ceil(dustDensity);
  }
 
  gw.sleep(SLEEP_TIME);
}
