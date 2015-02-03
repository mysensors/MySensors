/*
  Single_Temp.pde - Example using the MAX6675 Library.
  Created by Ryan McLaughlin <ryanjmclaughlin@gmail.com>

  This work is licensed under a Creative Commons Attribution-ShareAlike 3.0 Unported License.
  http://creativecommons.org/licenses/by-sa/3.0/
*/

#include <MAX6675.h>

int LED1 = 9;             // Status LED Pin
int CS = 10;             // CS pin on MAX6675
int SO = 12;              // SO pin of MAX6675
int SCK = 13;             // SCK pin of MAX6675
int units = 2;            // Units to readout temp (0 = raw, 1 = ˚C, 2 = ˚F)
float temperature = 0.0;  // Temperature output variable


// Initialize the MAX6675 Library for our chip
MAX6675 temp(CS,SO,SCK,units);


// Setup Serial output and LED Pin  
// MAX6675 Library already sets pin modes for MAX6675 chip!
void setup() {
  Serial.begin(9600);
  pinMode(LED1, OUTPUT);
}

void loop() {
	// Read the temp from the MAX6675
	temperature = temp.read_temp();

	if(temperature < 0) {                   
		// If there is an error with the TC, temperature will be < 0
		Serial.print("Thermocouple Error on CS");
		Serial.println( temperature ); 
		digitalWrite(LED1, HIGH);
	} else {
		Serial.print("Current Temperature: ");
		Serial.println( temperature ); 
		digitalWrite(LED1, LOW);
	}
 
	// Wait one second before reading again
	delay(1000);	
}
