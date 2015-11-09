1.0 by Simon Monk
Library as downloaded 02Feb2012 22:55 UTC from http://srmonk.blogspot.com/2012/01/arduino-timer-library.html

1.1 by Jack Christensen
Changed data types of variables and functions:
 o event types and indexes changed from int to int8_t.
 o periods and durations changed from lon to unsigned long.
 o update() and stop() functions typed as void, since they return nothing.
 o pin numbers and pin values changed from int to uint8_t, this agrees with digitalWrite().
 o added return values to Timer::pulse() and Timer::oscillate(uint8_t, unsigned long, uint8_t).
 o changed test in Event::update() to use subtraction to avoid rollover issues.
 o Updated keywords.txt file to include all functions.

1.2 by Damian Philipp
 o Added a range check to Timer::stop() to avoid memory corruption.
 o Added constants to <Timer.h>: 
    - NO_TIMER_AVAILABLE: Signals that while an event was to be queued, no free timer could be found.
    - TIMER_NOT_AN_EVENT: Can be used to flag a variable that *might* contain a timer ID as
      *not* containing a timer ID
 o Replaced a bunch of magic numbers in <Timer.cpp> by the above constants
 o Added several comments
 o Added Timer::pulseImmediate(). pulseImmediate sets the pin to the specified value for the given
   duration. After the duration, the pin is set to !value.
   
1.3 by Jack Christensen
 o Added "blink2" example illustrating flashing two LEDs at different rates.
 o 19Oct2013: This is the last v1.x release. It will continue to be available on GitHub
   as a branch named v1.3. Future development will continue with Sandy Walsh's v2.0 which
   can pass context (timer ID, etc.) to the callback functions.