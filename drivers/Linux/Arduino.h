#ifndef Arduino_h
#define Arduino_h

#include <stdio.h>
#include <stdint.h>

#ifdef __cplusplus
	#include <algorithm>

	using namespace std;

	extern "C" {
#endif

void yield(void);

#define PI 3.1415926535897932384626433832795
#define HALF_PI 1.5707963267948966192313216916398
#define TWO_PI 6.283185307179586476925286766559
#define DEG_TO_RAD 0.017453292519943295769236907684886
#define RAD_TO_DEG 57.295779513082320876798154814105
#define EULER 2.718281828459045235360287471352

#ifndef __cplusplus
	#define min(a,b) ((a)<(b)?(a):(b))
	#define max(a,b) ((a)>(b)?(a):(b))
	#define abs(x) ((x)>0?(x):-(x))
	#define round(x) ((x)>=0?(long)((x)+0.5):(long)((x)-0.5))
#endif

#define constrain(amt,low,high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))
#define radians(deg) ((deg)*DEG_TO_RAD)
#define degrees(rad) ((rad)*RAD_TO_DEG)
#define sq(x) ((x)*(x))

#define lowByte(w) ((uint8_t) ((w) & 0xff))
#define highByte(w) ((uint8_t) ((w) >> 8))

#define bitRead(value, bit) (((value) >> (bit)) & 0x01)
#define bitSet(value, bit) ((value) |= (1UL << (bit)))
#define bitClear(value, bit) ((value) &= ~(1UL << (bit)))
#define bitWrite(value, bit, bitvalue) (bitvalue ? bitSet(value, bit) : bitClear(value, bit))

#define bit(b) (1UL << (b))

typedef bool boolean;
typedef uint8_t byte;

char *itoa(int value, char* result, int base);
char *ltoa(long value, char* result, int base);
char *ultoa(long num, char *str, int radix);
char *utoa(int num, char *str, int radix);
char *dtostrf(float f, int width, int decimals, char *result);
unsigned long millis(void);
void delay(unsigned int millis);

#ifdef __cplusplus
	}
#endif

#endif
