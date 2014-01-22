/**
* First stab at a test suite for spi lib.
*/
#include <spi.h>

void reportError(int line, char *message, uint8_t expected, uint8_t actual)
{
  Serial.print("Test "); Serial.print(message);
  Serial.print(" failed at line "); Serial.print(line-3);
  Serial.print(", expected="); Serial.print(expected, BIN);
  Serial.print(", actual="); Serial.println(actual, BIN);
}

#define assertEquals(message, expected, actual) \
  if (expected != actual) { reportError(__LINE__, message, expected, actual); return; }


#define test_setup_spi(message, eSPCR, eSPSR, mode, dord, interrupt, clock) \
    setup_spi(mode, dord, interrupt, clock); \
    assertEquals(message, eSPCR, SPCR); \
    assertEquals(message, eSPSR, SPSR & 0x01);


void test_registers()
{
  // top 4 bits of SPCR - enabled, interrupt, lsb/msb, master/slave
  disable_spi();
  assertEquals("0", 0x00, SPCR);
  
  test_setup_spi("1", 0xE0, 0x00, SPI_MODE_0, SPI_LSB, SPI_INTERRUPT, SPI_SLAVE);
  test_setup_spi("2", 0x60, 0x00, SPI_MODE_0, SPI_LSB, SPI_NO_INTERRUPT, SPI_SLAVE);
  test_setup_spi("3", 0x40, 0x00, SPI_MODE_0, SPI_MSB, SPI_NO_INTERRUPT, SPI_SLAVE);

  // CPOL/CPHA bits - timing mode
  test_setup_spi("4", 0x40, 0x00, SPI_MODE_0, SPI_MSB, SPI_NO_INTERRUPT, SPI_SLAVE);
  test_setup_spi("5", 0x44, 0x00, SPI_MODE_1, SPI_MSB, SPI_NO_INTERRUPT, SPI_SLAVE);
  test_setup_spi("6", 0x48, 0x00, SPI_MODE_2, SPI_MSB, SPI_NO_INTERRUPT, SPI_SLAVE);
  test_setup_spi("7", 0x4C, 0x00, SPI_MODE_3, SPI_MSB, SPI_NO_INTERRUPT, SPI_SLAVE);

  // clock 
  test_setup_spi("9", 0xE0, 1, SPI_MODE_0, SPI_LSB, SPI_INTERRUPT, SPI_MSTR_CLK2);
  test_setup_spi("10", 0xE0, 0, SPI_MODE_0, SPI_LSB, SPI_INTERRUPT, SPI_MSTR_CLK4);
  test_setup_spi("11", 0xE1, 1, SPI_MODE_0, SPI_LSB, SPI_INTERRUPT, SPI_MSTR_CLK8);
  test_setup_spi("12", 0xE1, 0, SPI_MODE_0, SPI_LSB, SPI_INTERRUPT, SPI_MSTR_CLK16);
  test_setup_spi("13", 0xE2, 1, SPI_MODE_0, SPI_LSB, SPI_INTERRUPT, SPI_MSTR_CLK32);
  test_setup_spi("14", 0xE2, 0, SPI_MODE_0, SPI_LSB, SPI_INTERRUPT, SPI_MSTR_CLK64);
  test_setup_spi("15", 0xE3, 0, SPI_MODE_0, SPI_LSB, SPI_INTERRUPT, SPI_MSTR_CLK128);
}
 
void setup()
{
  Serial.begin(19200);
}

void loop()
{
  delay(5000);
  Serial.println("Running tests");
  test_registers();
  Serial.println("Run tests");
  while(true);
}
