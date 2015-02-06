#ifndef MYSBootloaderHW_H
#define MYSBootloaderHW_H

#include <stdlib.h>

// hardware



#ifdef USE_PRESCALER
#define F_CPU_DIV	clock_div_4
#else
#define F_CPU_DIV	clock_div_1
#endif

#define F_CPU_REAL	F_CPU / (1 << F_CPU_DIV)
#define DELAY_M		125 * (1 << F_CPU_DIV)
#define BAUD_SETTING (( (F_CPU + BAUD_RATE * 4L) / ((BAUD_RATE * 8L))) - 1 )
#define BAUD_ACTUAL (F_CPU/(8 * ((BAUD_SETTING)+1)))
#define BAUD_ERROR (( 100*(BAUD_RATE - BAUD_ACTUAL) ) / BAUD_RATE)
#define UART_SRA UCSR0A
#define UART_SRB UCSR0B
#define UART_SRC UCSR0C
#define UART_SRL UBRR0L
#define UART_UDR UDR0

// boolean definition

typedef uint8_t bool;
typedef uint8_t boolean;
#define false 	0x0
#define true 	0x1
#define LOW 	0x0
#define HIGH 	0x1

// Watchdog definitions and functions

#define WATCHDOG_OFF    (0)
#define WATCHDOG_16MS   (_BV(WDE))
#define WATCHDOG_32MS   (_BV(WDP0) | _BV(WDE))
#define WATCHDOG_64MS   (_BV(WDP1) | _BV(WDE))
#define WATCHDOG_125MS  (_BV(WDP1) | _BV(WDP0) | _BV(WDE))
#define WATCHDOG_250MS  (_BV(WDP2) | _BV(WDE))
#define WATCHDOG_500MS  (_BV(WDP2) | _BV(WDP0) | _BV(WDE))
#define WATCHDOG_1S     (_BV(WDP2) | _BV(WDP1) | _BV(WDE))
#define WATCHDOG_2S     (_BV(WDP2) | _BV(WDP1) | _BV(WDP0) | _BV(WDE))
#define WATCHDOG_4S     (_BV(WDP3) | _BV(WDE))
#define WATCHDOG_8S     (_BV(WDP3) | _BV(WDP0) | _BV(WDE))

inline void watchdogReset() {
	asm volatile ("wdr");
}

void watchdogConfig(uint8_t x) {
	WDTCSR = _BV(WDCE) | _BV(WDE);
	WDTCSR = x;
}

// delay procedures

inline static void delayu() {
	asm volatile ("nop");
}

static void delaym(uint8_t value) {
	for (uint8_t outer = 0; outer < value; outer++) {
		for (uint16_t inner = 0; inner < DELAY_M; inner++) {
				delayu();	
		}
	}			
}

// SPI communication

#define SPI_DDR		DDRB
#define SPI_PORT	PORTB
#define SPI_PIN		PINB
#define	SPI_SCLK	5		// Arduino Pin 13 <-> Bit 5 of port B
#define	SPI_MISO	4		// Arduino Pin 12 <-> Bit 4 of port B
#define	SPI_MOSI	3		// Arduino Pin 11 <-> Bit 3 of port B
#define	SPI_CSN		2		// Arduino Pin 10 <-> Bit 2 of port B
#define	SPI_CE		1		// Arduino Pin  9 <-> Bit 1 of port B
#define csnlow() SPI_PORT &= ~_BV(SPI_CSN)//;delaym(1)			// not necessary
#define csnhigh() SPI_PORT |= _BV(SPI_CSN)//;delaym(1)			// not necessary
#define celow() SPI_PORT &= ~_BV(SPI_CE)//;delaym(1)			// not necessary
#define cehigh() SPI_PORT |= _BV(SPI_CE)//;delaym(1)			// minimum puls width 10us

static void SPIinit() {
	// set pin mode: MOSI,SCLK,CE = OUTPUT, MISO = INPUT
	SPI_DDR = _BV(SPI_MOSI) | _BV(SPI_SCLK) | _BV(SPI_CE) | _BV(SPI_CSN) | ~_BV(SPI_MISO);
}

static uint8_t SPItransfer(uint8_t value) {
	for(uint8_t i = 0 ;i < 8;i++) {
		if (value & 0x80) SPI_PORT |= _BV(SPI_MOSI); else SPI_PORT &= ~_BV(SPI_MOSI);
		delayu();
		value <<= 1;
		SPI_PORT |= _BV(SPI_SCLK);
		delayu();
		value |= ((SPI_PIN >> SPI_MISO) & 0x01);
		SPI_PORT &= ~_BV(SPI_SCLK);
		delayu();
	}
	return value;	
}


// UART debug

static void uart_init() {
	//Double speed mode USART0
	UART_SRA = _BV(U2X0);		
	UART_SRB = _BV(RXEN0) | _BV(TXEN0);
	UART_SRC = _BV(UCSZ00) | _BV(UCSZ01);
	UART_SRL = (uint8_t)( (F_CPU + BAUD_RATE * 4L) / (BAUD_RATE * 8L) - 1 );
}

void putch(char ch) {
	while (!(UART_SRA & _BV(UDRE0)));
	UART_UDR = ch;
}

uint8_t getch() {
	uint8_t ch;
	// wait until char received
	while(!(UART_SRA & _BV(RXC0)));
	// framing error?
	if (!(UART_SRA & _BV(FE0))) {
		watchdogReset();
	}
	ch = UART_UDR;
	return ch;
}

static void put_string(char *s) {
	while (*s)
	putch(*s++);
}

static void put_int(uint8_t i) {
	char s[5];
	itoa( i, s, 10 );
	put_string( s );
}


#endif // MYSBootloaderHW_H
