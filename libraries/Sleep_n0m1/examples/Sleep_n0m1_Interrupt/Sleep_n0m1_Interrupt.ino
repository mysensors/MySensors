#include <Sleep_n0m1.h>

Sleep sleep;

void setup()
{
   
   Serial.begin(9600);
   
   
}

void loop()
{
  
  delay(100); ////delays are just for serial print, without serial they can be removed
  Serial.println("execute your code here");

  Serial.print("Sleeping Till Interrupt");
 
  delay(100); //delay to allow serial to fully print before sleep
    
  sleep.pwrDownMode(); //set sleep mode
  sleep.sleepInterrupt(0,FALLING); //sleep for: sleepTime
 
 
  
}
