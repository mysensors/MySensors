#include <Sleep_n0m1.h>

Sleep sleep;
boolean abortSleep; //cancel sleep cycle
int sleepCycleCount; //the number of times awake then asleep
unsigned long sleepTime; //how long you want the arduino to sleep
 


void setup()
{
   
   Serial.begin(9600);
   abortSleep = false; //can be used to cancel the sleep cycle
   sleepTime = 5000; //set sleep time in ms, max sleep time is 49.7 days
   
}

void loop()
{
  if(abortSleep)
  {
    Serial.println("sleep no more!");
  }
  else
  {
    delay(100);
    Serial.print("sleeping for ");
    Serial.println(sleepTime); 
    delay(100); //delay to allow serial to fully print before sleep
  }
  
  sleepCycleCount++;  
  if(sleepCycleCount > 5)
  {
    abortSleep = true;
  }
   
  sleep.pwrDownMode(); //set sleep mode
  sleep.sleepDelay(sleepTime,abortSleep); //sleep for: sleepTime
 
 
  
}
