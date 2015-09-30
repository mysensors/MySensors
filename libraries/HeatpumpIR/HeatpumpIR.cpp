#include <HeatpumpIR.h>

HeatpumpIR::HeatpumpIR()
{
}

// This is a virtual function, i.e. never called
void HeatpumpIR::send(IRSender&, byte, byte, byte, byte, byte, byte)
{
}

// Heatpump model and info getters
const char PROGMEM* HeatpumpIR::model()
{
  return _model;
}

const char PROGMEM* HeatpumpIR::info()
{
  return _info;
}