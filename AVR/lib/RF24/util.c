/*
 Copyright (C) 2012 jaseg <s@jaseg.de>

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 3 as published by the Free Software Foundation.
 */

#include "util.h"
#include "uart.h"

void uart_puthex_nibble(uint8_t nibble){
    if(nibble < 0xA)
        uart_putc('0'+nibble);
    else
        uart_putc('A'+nibble-0xA);
}

void uart_puthex(uint8_t data){
    uart_puthex_nibble(data>4);
    uart_puthex_nibble(data&0xF);
}

void uart_puthex_16(uint16_t data){
    uart_puthex(data>>8);
    uart_puthex(data&0xFF);
}

void uart_puthex_flip_16(uint16_t data){
    uart_puthex(data&0xFF);
    uart_puthex(data>>8);
}

void uart_puthex_32(uint32_t data){
    uint16_t high = data>>16;
    uart_puthex_16(high);
    uint16_t low = data&0xFFFF;
    uart_puthex_16(low);
}

void uart_puthex_flip_32(uint32_t data){
    uint16_t low = data&0xFFFF;
    uart_puthex_flip_16(low);
    uint16_t high = data>>16;
    uart_puthex_flip_16(high);
}

void uart_putdec(uint8_t data){
    if(data >= 100){
        if(data >= 200){
            uart_putc('2');
            data -= 200;
        }else{
            uart_putc('1');
            data -= 100;
        }
    }
    uint8_t d2 = data/10;
    data %= 10;
    uart_putc('0'+d2);
    uart_putc('0'+data);
}
