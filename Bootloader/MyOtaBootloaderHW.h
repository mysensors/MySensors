#ifndef MyOtaBootloaderHW_H
#define MyOtaBootloaderHW_H

#include <stdlib.h>
#include <avr/power.h>

//#define LED_DEBUG
//#define UART_DEBUG

// hardware
// ********

#define F_CPU_DIV	clock_div_4
#define F_CPU_REAL	F_CPU / (1 << F_CPU_DIV)
#define UART_BAUD	9600UL
#define DELAY_M		125 * (1 << F_CPU_DIV)

// boolean definition
// ******************

typedef uint8_t bool;
typedef uint8_t boolean;
#define false 	0x0
#define true 	0x1
#define LOW 	0x0
#define HIGH 	0x1

// delay
// *****

static void delayu() {
	asm("nop"); 
}

static void delaym(uint8_t t) {
	uint8_t d;
	uint16_t a;
	for (d = 0; d < t; d++)
		for (a = 0; a < DELAY_M; a++)
			asm("nop"); 
}

// LED debug
// *********

#ifdef LED_DEBUG
#define LED_DDR			DDRD
#define LED_PORT		PORTD
#define	LED_RED			6		// Arduino Pin 6 <-> Bit 6 of port D
#define	LED_GREEN		7		// Arduino Pin 7 <-> Bit 7 of port D
#define	LED_MRED		(1 << LED_RED)
#define	LED_MGREEN		(1 << LED_GREEN)
#define	LED_MBOTH		(LED_MRED | LED_MGREEN)

static void led_init() {
	LED_DDR |= LED_MBOTH;
	LED_PORT &= ~LED_MBOTH;
}

static void blink(uint8_t mask, uint16_t time) {
	LED_PORT |= mask;
	if (time > 0)
	{
		delaym(time);
		LED_PORT &= ~mask;
		delaym(time);
	}
}
#endif
  
// UART debug
// **********

#ifdef UART_DEBUG
static void uart_init()
{
	UCSR0A = _BV(U2X0); //Double speed mode USART0
	UCSR0B = _BV(RXEN0) | _BV(TXEN0);
	UCSR0C = _BV(UCSZ00) | _BV(UCSZ01);
	UBRR0L = (uint8_t)( (F_CPU_REAL + UART_BAUD * 4L) / (UART_BAUD * 8L) - 1 );
}

static void uart_putc(char ch) {
	while (!(UCSR0A & _BV(UDRE0)));
	UDR0 = ch;
}
  
static void uart_puts(char *s)
{
	while (*s)
		uart_putc(*s++);
}

static void uart_puti(uint8_t i)
{
	char s[5];
	itoa( i, s, 10 );
	uart_puts( s );
}
#endif

// SPI communication
// *****************

#define SPI_DDR		DDRB
#define SPI_PORT	PORTB
#define SPI_PIN		PINB
#define	SPI_SCLK	5		// Arduino Pin 13 <-> Bit 5 of port B
#define	SPI_MISO	4		// Arduino Pin 12 <-> Bit 4 of port B
#define	SPI_MOSI	3		// Arduino Pin 11 <-> Bit 3 of port B
#define	SPI_CSN		2		// Arduino Pin 10 <-> Bit 2 of port B
#define	SPI_CE		1		// Arduino Pin  9 <-> Bit 1 of port B

#define csnlow() SPI_PORT &= ~(1<<SPI_CSN);delaym(1)
#define csnhigh() SPI_PORT |= (1<<SPI_CSN);delaym(1)
#define celow() SPI_PORT &= ~(1<<SPI_CE);delaym(1)
#define cehigh() SPI_PORT |= (1<<SPI_CE);delaym(1)

static void SPIinit()
{
	SPI_DDR |= (1 << SPI_MOSI) + (1 << SPI_SCLK) + (1 << SPI_CE) + (1 << SPI_CSN);
	SPI_DDR &= ~(1 << SPI_MISO);
	SPI_PORT &= ~((1 << SPI_MOSI) + (1 << SPI_SCLK) + (1 << SPI_CE));
	SPI_PORT |= (1 << SPI_CSN);
}

static uint8_t SPItransfer(uint8_t value)
{
	uint8_t i;
	for(i = 0 ;i < 8;i++) {
		delayu();
		if ((value & 0x80) == 0x00)
			SPI_PORT &= ~(1 << SPI_MOSI);
		else
			SPI_PORT |= (1 << SPI_MOSI);
		value = (value << 1);    // shift next bit into MSB..
		delayu();
		SPI_PORT |= (1 << SPI_SCLK);
		value |= ((SPI_PIN >> SPI_MISO) & 0x01);     // capture current MISO bit
		delayu();
		SPI_PORT &= ~(1 << SPI_SCLK);
		delayu();
	}
	return value;
}

static uint8_t spiTrans(uint8_t cmd){
  uint8_t status;
  csnlow();
  status = SPItransfer( cmd );
  csnhigh();
  return status;
}

#endif // MyOtaBootloaderHW_H
