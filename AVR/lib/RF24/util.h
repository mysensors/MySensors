/*
 Copyright (C) 2012 jaseg <s@jaseg.de>

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 version 3 as published by the Free Software Foundation.
 */

#ifndef __UTIL_H__
#define __UTIL_H__
#include <avr/io.h>

void uart_puthex_nibble(uint8_t nibble);
void uart_puthex(uint8_t data);
void uart_puthex_16(uint16_t data);
void uart_puthex_32(uint32_t data);
void uart_puthex_flip_16(uint16_t data);
void uart_puthex_flip_32(uint32_t data);
void uart_putdec(uint8_t data);
inline uint8_t min(uint8_t a, uint8_t b){
    return a>b?b:a;
}

#endif//__UTIL_H__
