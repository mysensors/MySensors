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
#ifndef _ds1305_h__
#define _ds1305_h__

#include <spi.h>

#ifdef __cplusplus
extern "C"{
#endif

// these should be defined to the operation used to select/deselect the DS1305
void select_ds1305(void);
void deselect_ds1305(void);

#define DS1305_WRITE 0x80
#define DS1305_TIME 0x00
#define DS1305_ALARM0 0x07
#define DS1305_ALARM1 0x0B
#define DS1305_CONTROL 0x0F
#define DS1305_STATUS 0x010
#define DS1305_CHARGER 0x11
#define DS1305_USERRAM 0x20

#define DS1305_ALARM_SET 0x80

// send the address and a byte and returns the byte returned by the DS1305
unsigned char ds1305_transfer(unsigned char address, unsigned char data);
// write a block of bytes - will add 0x80 to the address
void ds1305_write_block(unsigned char address, unsigned char *data, int length);
// read a block of bytes
void ds1305_read_block(unsigned char address, unsigned char *data, int length);

typedef struct _DS1305_DATETIME
{
  unsigned char seconds;
  unsigned char minutes;
  unsigned char hours;
  unsigned char dayofweek;
  unsigned char date;
  unsigned char month;
  unsigned char year;
} DS1305_DATETIME;

/* set/get the current time/date passed the address of a DS1305_DATETIME struct
   To set the current time/date to "20:11:32 Sunday 29/07/2009"
   DS1305_DATETIME current = {0x32, 0x11, 0x20, 0x01, 0x29, 0x05, 0x09};
   set_time(&current);
*/
#define set_time(datetime) ds1305_write_block(DS1305_TIME, (unsigned char *)datetime, 7)
#define get_time(datetime)  ds1305_read_block(DS1305_TIME, (unsigned char *)datetime, 7)

/* set/get the time of alarm0/alarm1 passed the address of a DS1305_DATETIME struct,
   although only the seconds/minutes/hours/dayofweek are sent/received
*/
#define set_alarm0(time) ds1305_write_block(DS1305_ALARM0, (unsigned char *)time, 4)
#define get_alarm0(time)  ds1305_read_block(DS1305_ALARM0, (unsigned char *)time, 4)
#define set_alarm1(time) ds1305_write_block(DS1305_ALARM1, (unsigned char *)time, 4)
#define get_alarm1(time)  ds1305_read_block(DS1305_ALARM1, (unsigned char *)time, 4)

// flags used in set_control/get_control
#define DS1305_EOSC  0x80 // enable oscillator, note this is inverse of ^EOSC bit value on chip
#define DS1305_WP 0x40    // write protect
#define DS1305_INTCN 0x04 // enable interrupts
#define DS1305_AIE1 0x02  // alarm0 interrupt enable
#define DS1305_AIE0 0x01  // alarm1 interrupt enable

/* get/set the control register.  To enable  the oscillator, turn on write protection 
   and enable all interrupts the following would be used:
   set_control(DS1305_EOSC | DS1305_WP | DS1305_INTCN | DS1305_AIE1 | DS1305_AIE0);
*/
#define set_control(data) ds1305_transfer(DS1305_CONTROL+DS1305_WRITE, (data) ^ DS1305_EOSC)
#define get_control(data) (ds1305_transfer(DS1305_CONTROL, 0xFF) ^ DS1305_EOSC)

// flags used in get_status
#define DS1305_IRQF0 0x01 // interrupt 0 request flag - current time has matched alarm 0
#define DS1305_IRQF1 0x02 // interrupt 1 request flag - current time has matched alarm 1
#define get_status(data) ds1305_transfer(DS1305_STATUS, 0xFF)

// flags used in set_charger/get_charger
#define DS1305_CHARGER_OFF 0x5C // no charge - don't combine with other values
#define DS1305_CHARGER_1D2K 0xA5 // 1 diode 2K resistor
#define DS1305_CHARGER_1D4K 0xA6 // 1 diode 4K resistor
#define DS1305_CHARGER_1D8K 0xA7 // 1 diode 8K resistor
#define DS1305_CHARGER_2D2K 0xA1 // 2 diode 2K resistor
#define DS1305_CHARGER_2D4K 0xA2 // 2 diode 4K resistor
#define DS1305_CHARGER_2D8K 0xA3 // 2 diode 8K resistor
/* get/set the charger register */
#define set_charger(data) ds1305_transfer(DS1305_CHARGE+DS1305_WRITE, data)
#define get_charger(data) ds1305_transfer(DS1305_CHARGE, 0xFF)

#ifdef __cplusplus
} // extern "C"
#endif

#endif
