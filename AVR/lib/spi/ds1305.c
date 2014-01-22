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

#include <ds1305.h>

#ifdef __cplusplus
extern "C"{
#endif

unsigned char ds1305_transfer(unsigned char address, unsigned char data)
{
  select_ds1305();
  send_spi(address);
  unsigned char out = send_spi(data);
  deselect_ds1305();
  return out;
}

void ds1305_write_block(unsigned char address, unsigned char *data, int length)
{
  select_ds1305();
  send_spi(address+DS1305_WRITE);
  int i;
  for (i=0; i<length; i++) {
    send_spi(*data++);
  }
  deselect_ds1305();
}

void ds1305_read_block(unsigned char address, unsigned char *data, int length)
{
  select_ds1305();
  send_spi(address);
  int i;
  for (i=0; i<length; i++) {
    *(data+i) = send_spi(0xFF);
  }
  deselect_ds1305();
}

#ifdef __cplusplus
} // extern "C"
#endif

