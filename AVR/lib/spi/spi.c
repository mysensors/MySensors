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

#include "spi.h"

#ifdef __cplusplus
extern "C"{
#endif

#ifdef __ARDUINO__
#include <wiring.h>
#endif

#ifndef SPI_SOFT_DRIVER

// hardware driver

void setup_spi(uint8_t mode, int dord, int interrupt, uint8_t clock)
{
  // specify pin directions for SPI pins on port B
  if (clock == SPI_SLAVE) { // if slave SS and SCK is input
    DDRB &= ~(1<<SPI_MOSI_PIN); // input
    DDRB |= (1<<SPI_MISO_PIN); // output
    DDRB &= ~(1<<SPI_SS_PIN); // input
    DDRB &= ~(1<<SPI_SCK_PIN);// input
  } else {
    DDRB |= (1<<SPI_MOSI_PIN); // output
    DDRB &= ~(1<<SPI_MISO_PIN); // input
    DDRB |= (1<<SPI_SCK_PIN);// output
    DDRB |= (1<<SPI_SS_PIN);// output
  }
  SPCR = ((interrupt ? 1 : 0)<<SPIE) // interrupt enabled
    | (1<<SPE) // enable SPI
    | (dord<<DORD) // LSB or MSB
    | (((clock != SPI_SLAVE) ? 1 : 0) <<MSTR) // Slave or Master
    | (((mode & 0x02) == 2) << CPOL) // clock timing mode CPOL
    | (((mode & 0x01)) << CPHA) // clock timing mode CPHA
    | (((clock & 0x02) == 2) << SPR1) // cpu clock divisor SPR1
    | ((clock & 0x01) << SPR0); // cpu clock divisor SPR0
  SPSR = (((clock & 0x04) == 4) << SPI2X); // clock divisor SPI2X
}

void disable_spi()
{
  SPCR = 0;
}

uint8_t send_spi(uint8_t out)
{
  SPDR = out;
  while (!(SPSR & (1<<SPIF)));
  return SPDR;
}

uint8_t received_from_spi(uint8_t data)
{
  SPDR = data;
  return SPDR;
}

#else

// software driver
void setup_spi(uint8_t mode, int dord, int interrupt, uint8_t clock)
{
	if(mode) if(dord) if (interrupt) if(clock) {}
	// specify pin directions for SPI pins on port B
	if (clock == SPI_SLAVE) { // if slave SS and SCK is input
		SPI_DDR &= ~(1<<SPI_MOSI_PIN); // input
		SPI_DDR |= (1<<SPI_MISO_PIN); // output
//		SPI_DDR &= ~(1<<SPI_SS_PIN); // input
		SPI_DDR &= ~(1<<SPI_SCK_PIN);// input
	} else {
		SPI_DDR |= (1<<SPI_MOSI_PIN); // output
		SPI_DDR &= ~(1<<SPI_MISO_PIN); // input
		SPI_DDR |= (1<<SPI_SCK_PIN);// output
//		SPI_DDR |= (1<<SPI_SS_PIN);// output
	}
}

void disable_spi()
{
}

uint8_t send_spi(uint8_t tx)
{
    uint8_t i = 0;
    uint8_t rx = 0;

	SPI_PORT &= ~(1 << SPI_SCK_PIN);
_delay_us(50);

    //nrf24_sck_digitalWrite(LOW);

    for(i=0;i<8;i++)
    {
	    if(tx & (1<<(7-i)))
	    {
			SPI_PORT |= (1 << SPI_MOSI_PIN);
		    //nrf24_mosi_digitalWrite(HIGH);
	    }
	    else
	    {
			SPI_PORT &= ~(1 << SPI_MOSI_PIN);
		    //nrf24_mosi_digitalWrite(LOW);
	    }

		SPI_PORT |= (1 << SPI_SCK_PIN);
	    //nrf24_sck_digitalWrite(HIGH);
_delay_us(50);

	    rx = rx << 1;
	    if(SPI_PIN & (1 << SPI_MISO_PIN))
	    //if(nrf24_miso_digitalRead())
	    {
		    rx |= 0x01;
	    }

	    //nrf24_sck_digitalWrite(LOW);
		SPI_PORT &= ~(1 << SPI_SCK_PIN);

    }
_delay_us(50);

    return rx;
}

//uint8_t received_from_spi(uint8_t data)
//{
//}

#endif
