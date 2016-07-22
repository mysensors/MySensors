#include <Vcc.h>
#include <LowPower.h>

const float VccMin        = 2.0*0.6;  // Minimum expected Vcc level, in Volts. Example for 2xAA Alkaline.
const float VccMax        = 2.0*1.5;  // Maximum expected Vcc level, in Volts. Example for 2xAA Alkaline.
const float VccCorrection = 1.0/1.0;  // Measured Vcc by multimeter divided by reported Vcc

Vcc vcc(VccCorrection);

void setup()
{
  Serial.begin(115200);
}

void loop()
{  
  float v = vcc.Read_Volts();
  Serial.print("VCC = ");
  Serial.print(v);
  Serial.println(" Volts");

  float p = vcc.Read_Perc(VccMin, VccMax);
  Serial.print("VCC = ");
  Serial.print(p);
  Serial.println(" %");

  delay(200); //delay to allow serial to fully print before sleep

  LowPower.powerDown(SLEEP_2S, ADC_OFF, BOD_OFF);
}
