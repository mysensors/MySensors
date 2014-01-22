#include <spi.h>

// MAX6675 connected to pin D9
#define MAX6675_SELECT_PIN 8
#define DESELECT_MAX6675 digitalWrite(MAX6675_SELECT_PIN, HIGH)
#define SELECT_MAX6675 digitalWrite(MAX6675_SELECT_PIN, LOW)

void setup()
{
  pinMode(MAX6675_SELECT_PIN, OUTPUT);
  DESELECT_MAX6675;
  setup_spi(SPI_MODE_1, SPI_MSB, SPI_NO_INTERRUPT, SPI_MSTR_CLK8);
  Serial.begin(19200);
}

void loop()
{
  delay(500);

  // select the device, wait > 100nS, read two bytes, deselect
  SELECT_MAX6675;
  delayMicroseconds(1);
  unsigned char highByte = send_spi(0);
  unsigned char lowByte = send_spi(0);
  DESELECT_MAX6675;
  
  // if bit 3 is high thermocouple is unconnected
  if (lowByte & (1<<2)) {
    Serial.print("Not connected "); 
    Serial.print(highByte, HEX); Serial.print(" ");
    Serial.println(lowByte, HEX);
  } else {
    // temperature value is in bits 6-0 of highByte and 7-2 of lowByte
    short value = (highByte << 5 | lowByte>>3);
    // 1 bit is 0.25 degree 'divide' by 4 to show degrees
    Serial.print(value/4); Serial.print("."); Serial.println(value%4 *25);
  }
}
