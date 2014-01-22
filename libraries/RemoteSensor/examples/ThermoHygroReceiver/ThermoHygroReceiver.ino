/*
 * This sketch receives and decodes data from a 433MHz thermo/hygro weather sensor.
 * The received data (temperature, humidity, channel) is echo
 *
 * Setup:
 * - Connect digital output of a 433MHz receiver to digital pin 2 of Arduino
 * - Enable the serial monitor at 115200 baud. 
 *
 */

#include <SensorReceiver.h>

void setup() {
  Serial.begin(115200);

  // Init the receiver on interrupt pin 0 (digital pin 2).
  // Set the callback to function "showTempHumi", which is called
  // whenever valid sensor data has been received.
  SensorReceiver::init(0, showTempHumi);
}

void loop() {
  // Empty! However, you can do other stuff here if you like.
}

void showTempHumi(byte *data) {
  // is data a ThermoHygro-device?
  if ((data[3] & 0x1f) == 0x1e) {
    // Yes!

    byte channel, randomId;
    int temp;
    byte humidity;

    // Decode the data
    SensorReceiver::decodeThermoHygro(data, channel, randomId, temp, humidity);

    // Print temperature. Note: temp is 10x the actual temperature!
    Serial.print("Temperature: ");
    Serial.print(temp / 10); // units
    Serial.print('.');
    Serial.print(temp % 10); // decimal

    // Print humidity
    Serial.print(" deg, Humidity: ");
    Serial.print(humidity);
    Serial.print("% REL");

    // Print channel
    Serial.print(", Channel: ");
    Serial.println(channel, DEC);   
  }
}

