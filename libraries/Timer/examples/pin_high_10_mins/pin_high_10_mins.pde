#include "Timer.h"

Timer t;
int pin = 13;

void setup()
{
  pinMode(pin, OUTPUT);
  t.pulse(pin, 10 * 1000, HIGH); // 10 seconds
  // t.pulse(pin, 10 * 60 * 1000, HIGH); // 10 minutes  
}

void loop()
{
  t.update();
}

