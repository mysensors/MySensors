/*

  Arduino soild mosture based on gypsum sensor/resistive sensor to avoid electric catalyse in soil
  Required to interface the sensor: 2 * 4.7kOhm + 2 * 1N4148

Sensor and calibration:
	DIY: See http://vanderleevineyard.com/1/category/vinduino/1.html
	Built: Davis / Watermark 200SS 
		http://www.cooking-hacks.com/watermark-soil-moisture-sensor?_bksrc=item2item&_bkloc=product
		http://www.irrometer.com/pdf/supportmaterial/sensors/voltage-WM-chart.pdf
		cb (centibar) http://www.irrometer.com/basics.html
			0-10 Saturated Soil. Occurs for a day or two after irrigation 
			10-20 Soil is adequately wet (except coarse sands which are drying out at this range) 
			30-60 Usual range to irrigate or water (except heavy clay soils). 
			60-100 Usual range to irrigate heavy clay soils 
			100-200 Soil is becoming dangerously dry for maximum production. Proceed with caution. 
  
 Connection:
	D6, D7: alternative powering to avoid sensor degradation
	A0, A1: alternative resistance mesuring

  Contributor: epierre

  Based on:	
   "Vinduino" portable soil moisture sensor code V3.00
   Date December 31, 2012
   Reinier van der Lee and Theodore Kaskalis
   www.vanderleevineyard.com
 */

// Copyright (C) 2015, Reinier van der Lee
// www.vanderleevineyard.com

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

#include <math.h>       // Conversion equation from resistance to %
#include <SPI.h>
#include <MySensor.h>

// Setting up format for reading 3 soil sensors
#define NUM_READS 10    // Number of sensor reads for filtering
#define CHILD_ID 0

MySensor gw;  // Arduino initialization
MyMessage msg(CHILD_ID, V_HUM);  
unsigned long SLEEP_TIME = 30000; // Sleep time between reads (in milliseconds)

long buffer[NUM_READS];
int index;

typedef struct {        // Structure to be used in percentage and resistance values matrix to be filtered (have to be in pairs)
  int moisture;
  long resistance;
} values;

const long knownResistor = 4700;  // Constant value of known resistor in Ohms

int supplyVoltage;                // Measured supply voltage
int sensorVoltage;                // Measured sensor voltage

values valueOf[NUM_READS];        // Calculated moisture percentages and resistances to be sorted and filtered

int i;                            // Simple index variable

void setup() {
  // initialize serial communications at 9600 bps:
  Serial.begin(115200);
  gw.begin();
  gw.sendSketchInfo("Soil Moisture Sensor Reverse Polarity", "1.0");
  gw.present(CHILD_ID, S_HUM);  
  // initialize the digital pins as an output.
  // Pin 6,7 is for sensor 1
  // initialize the digital pin as an output.
  // Pin 6 is sense resistor voltage supply 1
  pinMode(6, OUTPUT);    

  // initialize the digital pin as an output.
  // Pin 7 is sense resistor voltage supply 2
  pinMode(7, OUTPUT);   


}

void loop() {

	measure(1,6,7,1);
	Serial.print ("\t");
	Serial.println (average());
	long read1 = average();

	measure(1,7,6,0);
	Serial.print ("\t");
	Serial.println (average());
	long read2= average();

	long sensor1 = (read1 + read2)/2;

	Serial.print ("resistance bias =" );
	Serial.println (read1-read2);
	Serial.print ("sensor bias compensated value = ");
	Serial.println (sensor1);
	Serial.println ();
	
	//send back the values
	gw.send(msg.set((long int)ceil(sensor1)));
	// delay until next measurement (msec)
    gw.sleep(SLEEP_TIME);
}

void measure (int sensor, int phase_b, int phase_a, int analog_input)
{
  // read sensor, filter, and calculate resistance value
  // Noise filter: median filter

  for (i=0; i<NUM_READS; i++) {

    // Read 1 pair of voltage values
    digitalWrite(phase_a, HIGH);                 // set the voltage supply on
    delayMicroseconds(25);
    supplyVoltage = analogRead(analog_input);   // read the supply voltage
    delayMicroseconds(25);
    digitalWrite(phase_a, LOW);                  // set the voltage supply off
    delay(1);

    digitalWrite(phase_b, HIGH);                 // set the voltage supply on
    delayMicroseconds(25);
    sensorVoltage = analogRead(analog_input);   // read the sensor voltage
    delayMicroseconds(25);
    digitalWrite(phase_b, LOW);                  // set the voltage supply off

    // Calculate resistance
    // the 0.5 add-term is used to round to the nearest integer
    // Tip: no need to transform 0-1023 voltage value to 0-5 range, due to following fraction
    long resistance = (knownResistor * (supplyVoltage - sensorVoltage ) / sensorVoltage) ;

    delay(1);
    addReading(resistance);
    Serial.print (resistance);
    Serial.print ("\t");
  }
}



// Averaging algorithm
void addReading(long resistance){
  buffer[index] = resistance;
  index++;
  if (index >= NUM_READS) index = 0;
}

long average(){
  long sum = 0;
  for (int i = 0; i < NUM_READS; i++){
    sum += buffer[i];
  }
  return (long)(sum / NUM_READS);
}
