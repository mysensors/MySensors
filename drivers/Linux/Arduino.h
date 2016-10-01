#ifndef Arduino_h
#define Arduino_h

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <string>
#include <algorithm>
#include "stdlib_noniso.h"

#ifdef LINUX_ARCH_RASPBERRYPI
	#include "rpi_util.h"
	using namespace rpi_util;
#endif

#define PI 3.1415926535897932384626433832795
#define HALF_PI 1.5707963267948966192313216916398
#define TWO_PI 6.283185307179586476925286766559
#define DEG_TO_RAD 0.017453292519943295769236907684886
#define RAD_TO_DEG 57.295779513082320876798154814105
#define EULER 2.718281828459045235360287471352

#define constrain(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))
#define radians(deg) ((deg)*DEG_TO_RAD)
#define degrees(rad) ((rad)*RAD_TO_DEG)
#define sq(x) ((x)*(x))
#define round(x) ((x)>=0?(long)((x)+0.5):(long)((x)-0.5))

#define lowByte(w) ((uint8_t) ((w) & 0xff))
#define highByte(w) ((uint8_t) ((w) >> 8))

#define bitRead(value, bit) (((value) >> (bit)) & 0x01)
#define bitSet(value, bit) ((value) |= (1UL << (bit)))
#define bitClear(value, bit) ((value) &= ~(1UL << (bit)))
#define bitWrite(value, bit, bitvalue) (bitvalue ? bitSet(value, bit) : bitClear(value, bit))

#define bit(b) (1UL << (b))

#define GET_MACRO(_0, _1, _2, NAME, ...) NAME
#define random(...) GET_MACRO(_0, ##__VA_ARGS__, randMinMax, randMax, rand)(__VA_ARGS__)

#ifndef delay
	#define delay _delay_ms
#endif

using std::string;
using std::min;
using std::max;
using std::abs;

typedef bool boolean;
typedef uint8_t byte;
typedef string String;

void yield(void);
unsigned long millis(void);
void _delay_ms(unsigned int millis);
void randomSeed(unsigned long seed);
long randMax(long howbig);
long randMinMax(long howsmall, long howbig);

#endif
