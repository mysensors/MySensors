/*
    Class to send IR signals using the Arduino PWM
*/
#ifndef IRSender_h
#define IRSender_h

#include <Arduino.h>

class IRSender
{
  public:
    IRSender(byte pin);
    void setFrequency(int frequency);
    void sendIRByte(byte sendByte, int bitMarkLength, int zeroSpaceLength, int oneSpaceLength);
    byte bitReverse(byte x);
    void space(int spaceLength);
    void mark(int markLength);

  private:
    byte _pin;
};

#endif
