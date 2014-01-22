#include <Sleep_n0m1.h>

int count = 0;
Sleep sleep;
unsigned long sleepTime; //how long you want the arduino to sleep



void setup()
{
   
   Serial.begin(9600);
   sleepTime = 50000; //set sleep time in ms, max sleep time is 49.7 days
   
}

void loop()
{
  delay(100); //delay to allow serial output to be ready after wake up
  Serial.print("Awake for ");
  Serial.print(count);
  Serial.println("sec");

  count++;
  delay(1000);                           // waits for a second
  
  if(count >= 3)
  {
    count = 0;
    Serial.print("sleeping ");
    Serial.println(sleepTime); 
    delay(100); //delay to allow serial to fully print before sleep
    
    sleep.pwrDownMode(); //set sleep mode
    sleep.sleepDelay(sleepTime); //sleep for: sleepTime
  }
 
  
}
