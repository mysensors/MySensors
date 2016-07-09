/* An Alternative Software Serial Library
 * http://www.pjrc.com/teensy/td_libs_AltSoftSerial.html
 * Copyright (c) 2014 PJRC.COM, LLC, Paul Stoffregen, paul@pjrc.com
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

// Version 1.2: Support Teensy 3.x
//
// Version 1.1: Improve performance in receiver code
//
// Version 1.0: Initial Release


#include "AltSoftSerial.h"
#include "config/AltSoftSerial_Boards.h"
#include "config/AltSoftSerial_Timers.h"

/****************************************/
/**          Initialization            **/
/****************************************/

static uint16_t ticks_per_bit=0;
bool AltSoftSerial::timing_error=false;

static uint8_t rx_state;
static uint8_t rx_byte;
static uint8_t rx_bit = 0;
static uint16_t rx_target;
static uint16_t rx_stop_ticks=0;
static volatile uint8_t rx_buffer_head;
static volatile uint8_t rx_buffer_tail;
#define RX_BUFFER_SIZE 80
static volatile uint8_t rx_buffer[RX_BUFFER_SIZE];

static volatile uint8_t tx_state=0;
static uint8_t tx_byte;
static uint8_t tx_bit;
static volatile uint8_t tx_buffer_head;
static volatile uint8_t tx_buffer_tail;
#define TX_BUFFER_SIZE 68
static volatile uint8_t tx_buffer[RX_BUFFER_SIZE];


#ifndef INPUT_PULLUP
#define INPUT_PULLUP INPUT
#endif

void AltSoftSerial::init(uint32_t cycles_per_bit)
{
	if (cycles_per_bit < 7085) {
		CONFIG_TIMER_NOPRESCALE();
	} else {
		cycles_per_bit /= 8;
		if (cycles_per_bit < 7085) {
			CONFIG_TIMER_PRESCALE_8();
		} else {
			return; // minimum 283 baud at 16 MHz clock
		}
	}
	ticks_per_bit = cycles_per_bit;
	rx_stop_ticks = cycles_per_bit * 37 / 4;
	pinMode(INPUT_CAPTURE_PIN, INPUT_PULLUP);
	digitalWrite(OUTPUT_COMPARE_A_PIN, HIGH);
	pinMode(OUTPUT_COMPARE_A_PIN, OUTPUT);
	rx_state = 0;
	rx_buffer_head = 0;
	rx_buffer_tail = 0;
	tx_state = 0;
	tx_buffer_head = 0;
	tx_buffer_tail = 0;
	ENABLE_INT_INPUT_CAPTURE();
}

void AltSoftSerial::end(void)
{
	DISABLE_INT_COMPARE_B();
	DISABLE_INT_INPUT_CAPTURE();
	flushInput();
	flushOutput();
	DISABLE_INT_COMPARE_A();
	// TODO: restore timer to original settings?
}


/****************************************/
/**           Transmission             **/
/****************************************/

void AltSoftSerial::writeByte(uint8_t b)
{
	uint8_t intr_state, head;

	head = tx_buffer_head + 1;
	if (head >= TX_BUFFER_SIZE) head = 0;
	while (tx_buffer_tail == head) ; // wait until space in buffer
	intr_state = SREG;
	cli();
	if (tx_state) {
		tx_buffer[head] = b;
		tx_buffer_head = head;
	} else {
		tx_state = 1;
		tx_byte = b;
		tx_bit = 0;
		ENABLE_INT_COMPARE_A();
		CONFIG_MATCH_CLEAR();
		SET_COMPARE_A(GET_TIMER_COUNT() + 16);
	}
	SREG = intr_state;
}


ISR(COMPARE_A_INTERRUPT)
{
	uint8_t state, byte, bit, head, tail;
	uint16_t target;

	state = tx_state;
	byte = tx_byte;
	target = GET_COMPARE_A();
	while (state < 9) {
		target += ticks_per_bit;
		bit = byte & 1;
		byte >>= 1;
		state++;
		if (bit != tx_bit) {
			if (bit) {
				CONFIG_MATCH_SET();
			} else {
				CONFIG_MATCH_CLEAR();
			}
			SET_COMPARE_A(target);
			tx_bit = bit;
			tx_byte = byte;
			tx_state = state;
			// TODO: how to detect timing_error?
			return;
		}
	}
	if (state == 9) {
		tx_state = 10;
		CONFIG_MATCH_SET();
		SET_COMPARE_A(target + ticks_per_bit);
		return;
	}
	head = tx_buffer_head;
	tail = tx_buffer_tail;
	if (head == tail) {
		tx_state = 0;
		CONFIG_MATCH_NORMAL();
		DISABLE_INT_COMPARE_A();
	} else {
		tx_state = 1;
		if (++tail >= TX_BUFFER_SIZE) tail = 0;
		tx_buffer_tail = tail;
		tx_byte = tx_buffer[tail];
		tx_bit = 0;
		CONFIG_MATCH_CLEAR();
		SET_COMPARE_A(target + ticks_per_bit);
		// TODO: how to detect timing_error?
	}
}

void AltSoftSerial::flushOutput(void)
{
	while (tx_state) /* wait */ ;
}


/****************************************/
/**            Reception               **/
/****************************************/


ISR(CAPTURE_INTERRUPT)
{
	uint8_t state, bit, head;
	uint16_t capture, target;
	int16_t offset;

	capture = GET_INPUT_CAPTURE();
	bit = rx_bit;
	if (bit) {
		CONFIG_CAPTURE_FALLING_EDGE();
		rx_bit = 0;
	} else {
		CONFIG_CAPTURE_RISING_EDGE();
		rx_bit = 0x80;
	}
	state = rx_state;
	if (state == 0) {
		if (!bit) {
			SET_COMPARE_B(capture + rx_stop_ticks);
			ENABLE_INT_COMPARE_B();
			rx_target = capture + ticks_per_bit + ticks_per_bit/2;
			rx_state = 1;
		}
	} else {
		target = rx_target;
		while (1) {
			offset = capture - target;
			if (offset < 0) break;
			rx_byte = (rx_byte >> 1) | rx_bit;
			target += ticks_per_bit;
			state++;
			if (state >= 9) {
				DISABLE_INT_COMPARE_B();
				head = rx_buffer_head + 1;
				if (head >= RX_BUFFER_SIZE) head = 0;
				if (head != rx_buffer_tail) {
					rx_buffer[head] = rx_byte;
					rx_buffer_head = head;
				}
				CONFIG_CAPTURE_FALLING_EDGE();
				rx_bit = 0;
				rx_state = 0;
				return;
			}
		}
		rx_target = target;
		rx_state = state;
	}
	//if (GET_TIMER_COUNT() - capture > ticks_per_bit) AltSoftSerial::timing_error = true;
}

ISR(COMPARE_B_INTERRUPT)
{
	uint8_t head, state, bit;

	DISABLE_INT_COMPARE_B();
	CONFIG_CAPTURE_FALLING_EDGE();
	state = rx_state;
	bit = rx_bit ^ 0x80;
	while (state < 9) {
		rx_byte = (rx_byte >> 1) | bit;
		state++;
	}
	head = rx_buffer_head + 1;
	if (head >= RX_BUFFER_SIZE) head = 0;
	if (head != rx_buffer_tail) {
		rx_buffer[head] = rx_byte;
		rx_buffer_head = head;
	}
	rx_state = 0;
	CONFIG_CAPTURE_FALLING_EDGE();
	rx_bit = 0;
}



int AltSoftSerial::read(void)
{
	uint8_t head, tail, out;

	head = rx_buffer_head;
	tail = rx_buffer_tail;
	if (head == tail) return -1;
	if (++tail >= RX_BUFFER_SIZE) tail = 0;
	out = rx_buffer[tail];
	rx_buffer_tail = tail;
	return out;
}

int AltSoftSerial::peek(void)
{
	uint8_t head, tail;

	head = rx_buffer_head;
	tail = rx_buffer_tail;
	if (head == tail) return -1;
	return rx_buffer[tail];
}

int AltSoftSerial::available(void)
{
	uint8_t head, tail;

	head = rx_buffer_head;
	tail = rx_buffer_tail;
	if (head >= tail) return head - tail;
	return RX_BUFFER_SIZE + head - tail;
}

void AltSoftSerial::flushInput(void)
{
	rx_buffer_head = rx_buffer_tail;
}


#ifdef ALTSS_USE_FTM0
void ftm0_isr(void)
{
	uint32_t flags = FTM0_STATUS;
	FTM0_STATUS = 0;
	if (flags & (1<<5)) altss_capture_interrupt();
	if (flags & (1<<6)) altss_compare_a_interrupt();
	if (flags & (1<<0)) altss_compare_b_interrupt();
}
#endif

