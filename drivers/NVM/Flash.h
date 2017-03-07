#pragma once

#ifdef NRF5
#include "drivers/NRF5/Flash.cpp"
#else
#error "Unsupported platform."
#endif
