/*
  Copyright (c) 2014-2015 Arduino LLC.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "variant.h"

/*
 * Pins descriptions
 */
const PinDescription g_APinDescription[]=
{

  /*
  +------------+------------------+--------+-----------------+--------+-----------------------+---------+---------+--------+--------+----------+----------+
  | Pin number |  MysX pin        |  PIN   | Notes           | Peri.A |     Peripheral B      | Perip.C | Perip.D | Peri.E | Peri.F | Periph.G | Periph.H |
  |            |                  |        |                 |   EIC  | ADC |  AC | PTC | DAC | SERCOMx | SERCOMx |  TCCx  |  TCCx  |    COM   | AC/GLCK  |
  |            |                  |        |                 |(EXTINT)|(AIN)|(AIN)|     |     | (x/PAD) | (x/PAD) | (x/WO) | (x/WO) |          |          |
  +------------+------------------+--------+-----------------+--------+-----+-----+-----+-----+---------+---------+--------+--------+----------+----------+
  | 00         | D1_DFM           |  PA10  |                 |   10   |  10 |     | X02 |     |   0/02  |   2/02  |*TCC1/0 | TCC0/2 | I2S/SCK0 | GCLK_IO4 |
  | 01         | D2_DTM           |  PA08  |                 |   NMI  |  16 |     | X00 |     |   0/00  |   2/00  |*TCC0/0 | TCC1/2 | I2S/SD1  |          |
  | 02         | D3_INT           |  PA05  |                 |    5   |  05 |  01 | Y03 |     |         |   0/01  |*TCC0/1 |        |          |          |
  | 03         | D4_INT           |  PA07  |                 |    7   |  07 |  03 | Y05 |     |         |   0/03  |*TCC1/1 |        | I2S/SD0  |          |
  | 04         | D5_PWM           |  PA04  |                 |    4   |  04 |  00 | Y02 |     |         |   0/00  |*TCC0/0 | TCC0/4 |          |          |
  | 05         | D6_PWM           |  PA06  |                 |    6   |  06 |  02 | Y04 |     |         |   0/02  |*TCC1/0 |        |          |          |
  | 06         | D7_SCL           |  PA23  |                 |    7   |     |     | X11 |     |   3/01  |   5/01  |  TC4/1 |*TCC0/5 | USB/SOF  | GCLK_IO7 |
  | 07         | D8_SDA           |  PA22  |                 |    6   |     |     | X10 |     |   3/00  |   5/00  |  TC4/0 |*TCC0/4 |          | GCLK_IO6 |
  | 08         | D9_A3            |  PB02  |                 |    2   |  10 |     | Y08 |     |         |   5/00  |  TC6/0 |        |          |          |
  | 09         | D10_A4           |  PB03  |                 |    3   |  11 |     | X09 |     |         |   5/01  |  TC6/1 |        |          |          |
  | 10         | D11_MOSI         |  PB30  |                 |   14   |     |     |     |     |         |   5/00  | TCC0/0 | TCC1/2 |          |          |
  | 11         | D12_MISO         |  PB22  |                 |    6   |     |     |     |     |         |   5/02  |  TC7/0 |        |          | GCLK_IO0 |
  | 12         | D13_SCK          |  PB31  |                 |   15   |     |     |     |     |         |   5/01  | TCC0/1 | TCC1/3 |          |          |
  | 13         | D14_CS           |  PB23  |                 |    7   |     |     |     |     |         |   5/03  |  TC7/1 |        |          | GCLK_IO1 |
  | 14         | A1               |  PA02  |                 |    2   |  00 |     | Y00 | VOUT|         |         |        |        |          |          |
  | 15         | A2               |  PA03  |                 |    3   |  01 |     | Y01 |     |         |         |        |        |          |          |
  +------------+------------------+--------+-----------------+--------+-----+-----+-----+-----+---------+---------+--------+--------+----------+----------+
  */

  { PORTA, 10, PIO_SERCOM,     (PIN_ATTR_DIGITAL),                          No_ADC_Channel, NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_11 },
  { PORTA,  8, PIO_SERCOM,     (PIN_ATTR_DIGITAL),                          No_ADC_Channel, NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_NMI },
  { PORTA,  5, PIO_DIGITAL,    (PIN_ATTR_DIGITAL),                          No_ADC_Channel, NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_14 },
  { PORTA,  7, PIO_DIGITAL,    (PIN_ATTR_DIGITAL),                          No_ADC_Channel, PWM0_CH1, TCC0_CH1, EXTERNAL_INT_9 },
  { PORTA,  4, PIO_TIMER,      (PIN_ATTR_DIGITAL|PIN_ATTR_PWM|PIN_ATTR_TIMER), No_ADC_Channel, PWM0_CH0, TCC0_CH0, EXTERNAL_INT_NMI},
  { PORTA,  6, PIO_TIMER,      (PIN_ATTR_DIGITAL|PIN_ATTR_PWM|PIN_ATTR_TIMER), No_ADC_Channel, PWM3_CH1, TC3_CH1, EXTERNAL_INT_1},
  { PORTA, 23, PIO_SERCOM,     (PIN_ATTR_DIGITAL),                             No_ADC_Channel, NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_6 },
  { PORTA, 22, PIO_SERCOM,     (PIN_ATTR_DIGITAL),                          No_ADC_Channel, NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_7 },
  { PORTB,  2, PIO_DIGITAL,    (PIN_ATTR_ANALOG|PIN_ATTR_DIGITAL),          ADC_Channel10,  NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_2},
  { PORTB,  3, PIO_DIGITAL,    (PIN_ATTR_ANALOG|PIN_ATTR_DIGITAL),          ADC_Channel11,  NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_3},
  { PORTB, 30, PIO_SERCOM_ALT, (PIN_ATTR_DIGITAL),                          No_ADC_Channel, NOT_ON_PWM, NOT_ON_TIMER, NOT_AN_INTERRUPT },
  { PORTB, 22, PIO_SERCOM_ALT, (PIN_ATTR_DIGITAL),                          No_ADC_Channel, NOT_ON_PWM, NOT_ON_TIMER, NOT_AN_INTERRUPT },
  { PORTB, 31, PIO_SERCOM_ALT, (PIN_ATTR_DIGITAL),                          No_ADC_Channel, NOT_ON_PWM, NOT_ON_TIMER, NOT_AN_INTERRUPT },
  { PORTB, 23, PIO_DIGITAL,    (PIN_ATTR_DIGITAL),                          No_ADC_Channel, NOT_ON_PWM, NOT_ON_TIMER, NOT_AN_INTERRUPT },
  { PORTA,  2, PIO_ANALOG,     (PIN_ATTR_ANALOG),                           ADC_Channel0,   NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_1},
  { PORTA,  3, PIO_ANALOG,     (PIN_ATTR_ANALOG),                           ADC_Channel1,   NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_3},

  /*
  +-------- ----+------------------+--------+-----------------+--------+-----------------------+---------+---------+--------+--------+----------+----------+
  | Pin number |  Internal pins   |  PIN   | Notes           | Peri.A |     Peripheral B      | Perip.C | Perip.D | Peri.E | Peri.F | Periph.G | Periph.H |
  |            |                  |        |                 |   EIC  | ADC |  AC | PTC | DAC | SERCOMx | SERCOMx |  TCCx  |  TCCx  |    COM   | AC/GLCK  |
  |            |  LED INDICATORS  |        |                 |(EXTINT)|(AIN)|(AIN)|     |     | (x/PAD) | (x/PAD) | (x/WO) | (x/WO) |          |          |
  +------------+------------------+--------+-----------------+--------+-----+-----+-----+-----+---------+---------+--------+--------+----------+----------+
  | 16         | LED_1            |  PA19  |                 |   10   |     |     | X02 |     |   0/02  |   2/02  |*TCC1/0 | TCC0/2 | I2S/SCK0 | GCLK_IO4 |
  | 17         | LED_2            |  PA20  |                 |   NMI  |     |     | X00 |     |   0/00  |   2/00  |*TCC0/0 | TCC1/2 | I2S/SD1  |          |
  | 18         | LED_3            |  PA21  |                 |   05   |  05 |  01 | Y03 |     |         |   0/01  |*TCC0/1 |        |          |          |
  | 19         | LED_4            |  PB16  |                 |   06   |  07 |  03 | Y05 |     |         |   0/03  |*TCC1/1 |        | I2S/SD0  |          |
  | 20         | LED_5            |  PB17  |                 |  *10   |     |     |     |     |         |   4/02  |* TC5/0 | TCC0/4 | I2S/MCK1 | GCLK_IO4 |
  +------------+------------------+--------+-----------------+--------+-----+-----+-----+-----+---------+---------+--------+--------+----------+----------+
  */

  { PORTA, 19, PIO_OUTPUT, (PIN_ATTR_DIGITAL),                              No_ADC_Channel, NOT_ON_PWM, NOT_ON_TIMER, NOT_AN_INTERRUPT },
  { PORTA, 20, PIO_OUTPUT, (PIN_ATTR_DIGITAL),                              No_ADC_Channel, NOT_ON_PWM, NOT_ON_TIMER, NOT_AN_INTERRUPT },
  { PORTA, 21, PIO_OUTPUT, (PIN_ATTR_DIGITAL),                              No_ADC_Channel, NOT_ON_PWM, NOT_ON_TIMER, NOT_AN_INTERRUPT },
  { PORTB, 16, PIO_OUTPUT, (PIN_ATTR_DIGITAL),                              No_ADC_Channel, NOT_ON_PWM, NOT_ON_TIMER, NOT_AN_INTERRUPT },
  { PORTB, 17, PIO_OUTPUT, (PIN_ATTR_DIGITAL),                              No_ADC_Channel, NOT_ON_PWM, NOT_ON_TIMER, NOT_AN_INTERRUPT },

 /*
  +------------+------------------+--------+-----------------+--------+-----------------------+---------+---------+--------+--------+----------+----------+
  | Pin number |  Internal pins   |  PIN   | Notes           | Peri.A |     Peripheral B      | Perip.C | Perip.D | Peri.E | Peri.F | Periph.G | Periph.H |
  |            |                  |        |                 |   EIC  | ADC |  AC | PTC | DAC | SERCOMx | SERCOMx |  TCCx  |  TCCx  |    COM   | AC/GLCK  |
  |            |  Peripherals     |        |                 |(EXTINT)|(AIN)|(AIN)|     |     | (x/PAD) | (x/PAD) | (x/WO) | (x/WO) |          |          |
  +------------+------------------+--------+-----------------+--------+-----+-----+-----+-----+---------+---------+--------+--------+----------+----------+
  | 21         |  NET_MOSI        |  PB12  |                 |   12   |     |     | X12 |     |   4/00  |         |  TC4/0 | TCC0/6 | I2S/FS1  | GCLK_IO6 |
  | 22         |  NET_MISO        |  PB14  |                 |   14   |     |     | X14 |     |   4/02  |         |  TC5/0 |        |          | GCLK_IO0 |
  | 23         |  NET_SCK         |  PB13  |                 |   13   |     |     | X13 |     |   4/01  |         |  TC4/1 | TCC0/7 |          | GCLK_IO7 |
  | 24         |  NET_SS          |  PB15  |                 |   15   |     |     | X15 |     |   4/03  |         |  TC5/1 |        |          | GCLK_IO1 |
  | 25         |  NET_RST         |  PB11  |                 |   11   |     |     |     |     |         |    4/3  |  TC5/0 | TCC0/5 | I2S/SCK1 | GCLK_IO5 |
  | 26         |  MOSI            |  PA16  |                 |   12   |     |     | X12 |     |   1/00  |         |  TC4/0 | TCC0/6 | I2S/FS1  | GCLK_IO6 |
  | 27         |  MISO            |  PA18  |                 |   14   |     |     | X14 |     |   1/02  |         |  TC5/0 |        |          | GCLK_IO0 |
  | 28         |  SCK             |  PA17  |                 |   13   |     |     | X13 |     |   1/01  |         |  TC4/1 | TCC0/7 |          | GCLK_IO7 |
  | 29         |  CS_NRF          |  PA09  |                 |    9   |  17 |     | X01 |     |   0/01  |    2/1  | TCC0/1 | TCC1/3 | I2S/MCK  |          |
  | 30         |  CS_RFM          |  PA12  |                 |   12   |     |     |     |     |   2/00  |    4/0  | TCC2/0 | TCC0/6 |          | AC/CMP0  |
  | 31         |  IRQ_NRF         |  PA14  |                 |   14   |     |     |     |     |   2/02  |    4/2  |  TC3/0 | TCC0/4 |          | GCLK_IO0 |
  | 32         |  IRQ_RFM         |  PA13  |                 |   13   |     |     |     |     |   2/01  |    4/1  | TCC2/1 | TCC0/7 |          | AC/CMP1  |
  | 33         |  CS_SDCARD       |  PA27  |                 |   15   |     |     |     |     |         |         |        |        |          | GCLK_IO0 |
  | 34         |  CE_NRF          |  PA28  |                 |    8   |     |     |     |     |         |         |        |        |          | GCLK_IO0 |
  | 35         |  SDCARD_DETECT   |  PB07  |                 |    7   |     |     |     |     |         |         |        |        |          |          |
  +------------+------------------+--------+-----------------+--------+-----+-----+-----+-----+---------+---------+--------+--------+----------+----------+
  */

  { PORTB, 12, PIO_SERCOM_ALT, (PIN_ATTR_NONE),                             No_ADC_Channel, NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_NONE }, // MOSI: SERCOM4/PAD[0]
  { PORTB, 14, PIO_SERCOM_ALT, (PIN_ATTR_NONE),                             No_ADC_Channel, NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_NONE }, // MISO: SERCOM4/PAD[2]
  { PORTB, 13, PIO_SERCOM_ALT, (PIN_ATTR_NONE),                             No_ADC_Channel, NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_NONE }, // SS: SERCOM4/PAD[3]
  { PORTB, 15, PIO_SERCOM_ALT, (PIN_ATTR_NONE),                             No_ADC_Channel, NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_NONE }, // SCK: SERCOM1/PAD[1]
  { PORTB, 11, PIO_OUTPUT,     (PIN_ATTR_DIGITAL),                          No_ADC_Channel, NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_NONE }, // Reset line
  { PORTA, 16, PIO_SERCOM,     (PIN_ATTR_NONE),                             No_ADC_Channel, NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_NONE }, // MOSI: SERCOM4/PAD[0]
  { PORTA, 18, PIO_SERCOM,     (PIN_ATTR_NONE),                             No_ADC_Channel, NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_NONE }, // MISO: SERCOM4/PAD[2]
  { PORTA, 17, PIO_SERCOM,     (PIN_ATTR_NONE),                             No_ADC_Channel, NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_NONE }, // SS: SERCOM4/PAD[3]
  { PORTA,  9, PIO_OUTPUT,     (PIN_ATTR_DIGITAL),                          No_ADC_Channel, NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_9 }, // Reset line
  { PORTA, 12, PIO_OUTPUT,     (PIN_ATTR_DIGITAL),                          No_ADC_Channel, NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_12 }, // Reset line
  { PORTA, 14, PIO_DIGITAL,    (PIN_ATTR_DIGITAL),                          No_ADC_Channel, NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_14 }, // Reset line
  { PORTA, 13, PIO_DIGITAL,    (PIN_ATTR_DIGITAL),                          No_ADC_Channel, NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_13 }, // Reset line
  { PORTA, 27, PIO_OUTPUT,     (PIN_ATTR_DIGITAL),                          No_ADC_Channel, NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_NONE }, // Reset line
  { PORTA, 28, PIO_OUTPUT,     (PIN_ATTR_DIGITAL),                          No_ADC_Channel, NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_NONE }, // Reset line
  { PORTB,  7, PIO_DIGITAL,    (PIN_ATTR_DIGITAL),                          No_ADC_Channel, NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_7 }, // Reset line

  /*
   +------------+------------------+--------+-----------------+--------+-----------------------+---------+---------+--------+--------+----------+----------+
   | Pin number |  Internal pins   |  PIN   | Notes           | Peri.A |     Peripheral B      | Perip.C | Perip.D | Peri.E | Peri.F | Periph.G | Periph.H |
   |            |                  |        |                 |   EIC  | ADC |  AC | PTC | DAC | SERCOMx | SERCOMx |  TCCx  |  TCCx  |    COM   | AC/GLCK  |
   |            |  Peripherals     |        |                 |(EXTINT)|(AIN)|(AIN)|     |     | (x/PAD) | (x/PAD) | (x/WO) | (x/WO) |          |          |
   +------------+------------------+--------+-----------------+--------+-----+-----+-----+-----+---------+---------+--------+--------+----------+----------+
   | 36         | BAT_DET          |  PB08  |                 |    8   |   2 |     | Y14 |     |         |   4/0   |  TC4/0 |        |          |          |
   | 37         | SUPPLY_DET       |  PB09  |                 |    9   |   3 |     | Y15 |     |         |   4/1   |  TC4/1 |        |          |          |
   | 38         | TESTMODE         |  PB04  |                 |    4   |  12 |     | Y10 |     |         |         |        |        |          | GCLK_IO7 |
   | 39         | BOOTMODE         |  PA15  |                 |   15   |     |     |     |     |   2/03  |   4/3   |  TC3/1 | TCC0/5 |          | GCLK_IO1 |
   | 39         | INCL_SW          |  PB01  |                 |    1   |   9 |     | Y07 |     |         |   5/3   |  TC7/1 |        |          | GCLK_IO1 |
   | 39         | SWC1             |  PB05  |                 |    5   |  13 |     | Y11 |     |         |         |        |        |          |          |
   | 39         | SWC2             |  PB06  |                 |    6   |  14 |     | Y12 |     |         |         |        |        |          |          |
   +------------+------------------+--------+-----------------+--------+-----+-----+-----+-----+---------+---------+--------+--------+----------+----------+
   */

  { PORTB,  8, PIO_ANALOG,  PIN_ATTR_ANALOG,                              ADC_Channel2, NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_NONE },
  { PORTB,  9, PIO_ANALOG,  PIN_ATTR_ANALOG,                              ADC_Channel3, NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_NONE },
  { PORTB,  4, PIO_DIGITAL, PIN_ATTR_DIGITAL,                             No_ADC_Channel, NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_4  },
  { PORTA, 15, PIO_DIGITAL, PIN_ATTR_DIGITAL,                             No_ADC_Channel, NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_15 },
  { PORTB,  1, PIO_DIGITAL, PIN_ATTR_DIGITAL,                             No_ADC_Channel, NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_1  },
  { PORTB,  5, PIO_DIGITAL, PIN_ATTR_DIGITAL,                             No_ADC_Channel, NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_5  },
  { PORTA,  6, PIO_DIGITAL, PIN_ATTR_DIGITAL,                             No_ADC_Channel, NOT_ON_PWM, NOT_ON_TIMER, EXTERNAL_INT_6  }
};

const void* g_apTCInstances[TCC_INST_NUM+TC_INST_NUM]={ TCC0, TCC1, TCC2, TC3, TC4, TC5 } ;

// Multi-serial objects instantiation
SERCOM sercom0( SERCOM0 ) ;
SERCOM sercom1( SERCOM1 ) ;
SERCOM sercom2( SERCOM2 ) ;
SERCOM sercom3( SERCOM3 ) ;
SERCOM sercom4( SERCOM4 ) ;
SERCOM sercom5( SERCOM5 ) ;

Uart Serial1( &sercom0, PIN_SERIAL1_RX, PIN_SERIAL1_TX, PAD_SERIAL1_RX, PAD_SERIAL1_TX ) ;

void SERCOM0_Handler()
{
  Serial1.IrqHandler();
}
