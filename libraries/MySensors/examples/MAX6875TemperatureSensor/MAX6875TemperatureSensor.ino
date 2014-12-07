// MAX6875 Hight temperature sensor with range: -200 degC - +1300 degC 
// Contribution by MrLynx
// http://forum.mysensors.org/topic/641/max6675
//
// Connect:
// MAX6675 - Arduino
//     CS0 - 4
//      SO - 3 
//    SCLK - 5
//     VCC - 5V
//     GND - GND

#include <MySensor.h>
#include <MAX6675.h>
#include <SPI.h>
uint8_t CS0 = 4; // CS pin on MAX6675
uint8_t SO = 3; // SO pin of MAX6675
uint8_t SCLK = 5; // SCK pin of MAX6675
uint8_t units = 1; // Units to readout temp (0 = ˚F, 1 = ˚C)
float temperature = 0.0; // Temperature output variable
float lastTemperature;
unsigned long SLEEP_TIME = 30000;
boolean metric = true;
MySensor gw;

MyMessage msg(0, V_TEMP);

// Initialize the MAX6675 Library for our chip

MAX6675 temp0(CS0, SO, SCLK, units);

void setup()
{
  // Startup and initialize MySensors library. Set callback for incoming messages.
  gw.begin();

  // Send the sketch version information to the gateway and Controller
  gw.sendSketchInfo("Max6675 Temp Sensor", "1.0");

  // Present all sensors to controller
  gw.present(0, S_TEMP);
}

void loop()
{
  // Process incoming messages (like config from server)
  gw.process();

  temperature = temp0.read_temp(); // Read the temp

  if (temperature < 0) { // If there is an error with the TC, temperature will be < 0
    Serial.println("Thermocouple Error!!"); // There is a thermocouple error
  } else {
    Serial.print("Current Temperature: ");
    Serial.println( temperature ); // Print the temperature to Serial
    if (temperature != lastTemperature)
      gw.send(msg.setSensor(0).set(temperature, 1));
    lastTemperature = temperature;
  }

  gw.sleep(SLEEP_TIME);
}
