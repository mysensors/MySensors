/*
    Class to send IR signals using the Arduino PWM
*/
#ifndef IRSender_h
#define IRSender_h

#include <Arduino.h>

class IRSender
{
  public:
    IRSender(uint8_t pin);
    void setFrequency(int frequency);
    void sendIRbyte(uint8_t sendByte, int bitMarkLength, int zeroSpaceLength, int oneSpaceLength);
    uint8_t bitReverse(uint8_t x);
    void space(int spaceLength);
    void mark(int markLength);

  private:
    uint8_t _pin;
};

#endif
