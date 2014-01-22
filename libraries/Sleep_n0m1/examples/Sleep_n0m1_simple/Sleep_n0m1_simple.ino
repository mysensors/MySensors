#include <Sleep_n0m1.h>

Sleep sleep;
unsigned long sleepTime; //how long you want the arduino to sleep



void setup()
{
   
   Serial.begin(9600);
   sleepTime = 50000; //set sleep time in ms, max sleep time is 49.7 days
   
}

void loop()
{
  
  delay(100); ////delays are just for serial print, without serial they can be removed
  Serial.println("execute your code here");

  Serial.print("sleeping for ");
  Serial.println(sleepTime); 
  delay(100); //delay to allow serial to fully print before sleep
    
  sleep.pwrDownMode(); //set sleep mode
  sleep.sleepDelay(sleepTime); //sleep for: sleepTime
 
 
  
}
