#include <spi.h>

#define SELECT_ADC digitalWrite(8, LOW);
#define DESELECT_ADC  digitalWrite(8, HIGH);

unsigned short read_adc(void)
{
  // select ADC wait 100microseconds then read two bytes
  SELECT_ADC;
  unsigned char one = send_spi(0xFF);
  unsigned char two = send_spi(0xFF);
  DESELECT_ADC;
  // 12 bits of ADC value is bottom 5 bits of first
  // byte and top 7 bits of second, move into 16 bit int
  return ((0x1F & one) << 7) | (two >> 1);
}

void setup()
{
  pinMode(8, OUTPUT);
  Serial.begin(19200);
  // make sure ADC is not selected and setup spi
  DESELECT_ADC;
  setup_spi(SPI_MODE_0, SPI_MSB, SPI_NO_INTERRUPT, SPI_MSTR_CLK16);
}

void loop()
{
  unsigned short num = read_adc();
  Serial.println(num);
  delay(1000);
}
