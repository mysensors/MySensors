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
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <util/delay.h>

#include <ds1305.h>

// following two methods required by DS1305 library to select/deselect device
void select_ds1305(void)
{
  PORTD |= (1<<PD4);
}

void deselect_ds1305(void)
{
  PORTD &= ~(1<<PD4);
}

// flash led that's connected to pin PD1
void flash_led(void)
{
  DDRD |= (1<<PD1);
  for (int i=0; i<10; i++) {
    PORTD ^= (1<<PD1);
    _delay_ms(50);
  }
}

// called when pin INT0 goes from 1 to 0
ISR(INT0_vect)
{
  // get alarm value to clear alarm interrupt flag on DS1305
  DS1305_DATETIME alarm;
  get_alarm0(&alarm);
  flash_led();
}

int main(void)
{
  // flash LED at start to indicate were about to start
  flash_led();
  flash_led();
  
  // pin used to enable DS1305
  DDRD = (1<<PD4);
  
  // Make sure ADC is unselected and setup spi
  deselect_ds1305();
  setup_spi(SPI_MODE_1, SPI_MSB, SPI_NO_INTERRUPT, SPI_MSTR_CLK16);
  set_control(0);

  // set current date/time to Thursday 06/05/09 20:32:30
  DS1305_DATETIME current = {0x30, 0x32, 0x20, 0x05, 0x06, 0x05, 0x09};
  set_time(&current);
  
  // change the value of alarm to the required time/granularity:
  // raise alarm every day (at 20:32:00):
  // DS1305_DATETIME alarm = {0x00, 0x32, 0x20, 0x00 | DS1305_ALARM_SET};
  // raise alarm every minute (when seconds = 00):
  // DS1305_DATETIME alarm = {0x00, 0x32 | DS1305_ALARM_SET,
  //		   0x20 | DS1305_ALARM_SET, 0x05 | DS1305_ALARM_SET};
  // raise alarm every second:
  DS1305_DATETIME alarm = {0x55 | DS1305_ALARM_SET, 0x32 | DS1305_ALARM_SET,
  			   0x20 | DS1305_ALARM_SET, 0x05 | DS1305_ALARM_SET};
  set_alarm0(&alarm);
  // turn on timer and make alarm0 lower INT0 pin when alarm is triggered
  set_control(DS1305_EOSC | DS1305_INTCN | DS1305_AIE0);

  // raise interrupt when INT0 pin falls (pin PD0 at AT90usbXXX, pin PD2 on ATmegaXXX (arduino pin 2))
  // the code in ISR(INT0_vect) above will be called
  EICRA = (1<<ISC01) | (0<<ISC00);
  // enable interrupts
  EIMSK = (1<<INT0);
  sei();

  while(1);
}
