#include <time.h>
#include <sys/time.h>
#include <stdlib.h>
#include "Arduino.h"

// For millis()
static unsigned long millis_at_start = 0;

void yield(void) {}

unsigned long millis(void)
{
	timeval curTime;

	if (millis_at_start == 0) {
		gettimeofday(&curTime, NULL);
		millis_at_start = curTime.tv_sec;
	}

	gettimeofday(&curTime, NULL);
	return ((curTime.tv_sec - millis_at_start) * 1000) + (curTime.tv_usec / 1000);
}

unsigned long micros()
{
	timeval curTime;

	if (millis_at_start == 0) {
		gettimeofday(&curTime, NULL);
		millis_at_start = curTime.tv_sec;
	}

	gettimeofday(&curTime, NULL);
	return ((curTime.tv_sec - millis_at_start) * 1000000) + (curTime.tv_usec);
}

void _delay_ms(unsigned int millis)
{
	struct timespec sleeper;

	sleeper.tv_sec  = (time_t)(millis / 1000);
	sleeper.tv_nsec = (long)(millis % 1000) * 1000000;
	nanosleep(&sleeper, NULL);
}

void randomSeed(unsigned long seed)
{
	if (seed != 0) {
		srand(seed);
	}
}

long randMax(long howbig)
{
	if (howbig == 0) {
		return 0;
	}
	return rand() % howbig;
}

long randMinMax(long howsmall, long howbig)
{
	if (howsmall >= howbig) {
		return howsmall;
	}
	long diff = howbig - howsmall;
	return randMax(diff) + howsmall;
}
