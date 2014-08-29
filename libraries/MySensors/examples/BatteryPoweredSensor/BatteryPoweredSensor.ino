// This is an example that demonstrates how to report the battery level for a sensor
// Instructions for measuring battery capacity on A0 are available in the follwoing forum
// thread: http://forum.micasaverde.com/index.php/topic,20078.0.html

#include <SPI.h>
#include <MySensor.h>

int BATTERY_SENSE_PIN = A0;  // select the input pin for the battery sense point

MySensor gw;
unsigned long SLEEP_TIME = 900000;  // sleep time between reads (seconds * 1000 milliseconds)
int oldBatteryPcnt = 0;

void setup()  
{
   // use the 1.1 V internal reference
   analogReference(INTERNAL);
   gw.begin();

   // Send the sketch version information to the gateway and Controller
   gw.sendSketchInfo("Battery Meter", "1.0");
}

void loop()
{
   // get the battery Voltage
   int sensorValue = analogRead(BATTERY_SENSE_PIN);
   Serial.println(sensorValue);
   
   // 1M, 470K divider across battery and using internal ADC ref of 1.1V
   // Sense point is bypassed with 0.1 uF cap to reduce noise at that point
   // ((1e6+470e3)/470e3)*1.1 = Vmax = 3.44 Volts
   // 3.44/1023 = Volts per bit = 0.003363075
   float batteryV  = sensorValue * 0.003363075;
   int batteryPcnt = sensorValue / 10;

   Serial.print("Battery Voltage: ");
   Serial.print(batteryV);
   Serial.println(" V");

   Serial.print("Battery percent: ");
   Serial.print(batteryPcnt);
   Serial.println(" %");

   if (oldBatteryPcnt != batteryPcnt) {
     // Power up radio after sleep
     gw.sendBatteryLevel(batteryPcnt);
     oldBatteryPcnt = batteryPcnt;
   }
   gw.sleep(SLEEP_TIME);
}
