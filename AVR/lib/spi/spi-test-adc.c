/* 
 * Copyright (c) 2009 Andrew Smallbone <andrew@rocketnumbernine.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <avr/io.h>
#include <util/delay.h>
#include <spi.h>

#define SELECT_ADC PORTB &= ~(1<<PB4)
#define DESELECT_ADC PORTB |= (1<<PB4)

unsigned short read_adc(void)
{
  // select ADC wait 100 microseconds then read two bytes
  SELECT_ADC;
  _delay_us(100);
  unsigned char one = send_spi(0xFF);
  _delay_us(100);
  unsigned char two = send_spi(0xFF);
  DESELECT_ADC;
  // 12 bits of ADC value is bottom 5 bits of first
  // byte and top 7 bits of second, move into 16 bit int
  return ((0x1F & one) << 7) | (two >> 1);
}

int main(void)
{
  DDRB |= (1<<PB4); // chip select for ADC
  // use port D for leds
  DDRD = 0xFF;
  PORTD = 0x00;

  // make sure ADC is unselected and setup spi
  SELECT_ADC;
  setup_spi(SPI_MODE_0, SPI_MSB, SPI_NO_INTERRUPT, SPI_MSTR_CLK16);

  while (1) {
    unsigned int num = read_adc();
    PORTD = (1<< (num >> 9)); // use the top 3 bytes to turn on LED
    _delay_ms(1);
  }  
}

