/* code was modified by george spanos <spaniakos@gmail.com>
 * 16/12/14
 */

#ifndef __AES_CONFIG_H__
#define __AES_CONFIG_H__

#if  (defined(__linux) || defined(linux)) && !defined(__ARDUINO_X86__)

  #define AES_LINUX

  #include <stdint.h>
  #include <stdio.h>
  #include <stdlib.h>
  #include <string.h>
  #include <sys/time.h>
  #include <unistd.h> 
#else
  #include <Arduino.h>
#endif

#include <stdint.h>
#include <string.h>

#if defined(__ARDUINO_X86__) || (defined (__linux) || defined (linux))
	#undef PROGMEM
	#define PROGMEM __attribute__(( section(".progmem.data") ))
	#define pgm_read_byte(p) (*(p))
	typedef unsigned char byte;
	#define printf_P printf
	#define PSTR(x) (x)
#elif defined(ARDUINO_ARCH_ESP8266)
	#include <pgmspace.h>
#elif defined(ARDUINO_ARCH_SAMD)
	#define printf_P printf
#else
	#include <avr/pgmspace.h>
#endif

#define N_ROW                   4
#define N_COL                   4
#define N_BLOCK   (N_ROW * N_COL)
#define N_MAX_ROUNDS           14
#define KEY_SCHEDULE_BYTES ((N_MAX_ROUNDS + 1) * N_BLOCK)
#define AES_SUCCESS (0)
#define AES_FAILURE (-1)

#endif
