// *** Hardwarespecific defines ***
#define cbi(reg, bitmask) *reg &= ~bitmask
#define sbi(reg, bitmask) *reg |= bitmask
#define rbi(reg, bitmask) ((*reg) & bitmask)

#define pulse_high(reg, bitmask) sbi(reg, bitmask); cbi(reg, bitmask);
#define pulse_low(reg, bitmask) cbi(reg, bitmask); sbi(reg, bitmask);

#define swap(type, i, j) {type t = i; i = j; j = t;}

#if defined(TEENSYDUINO) && TEENSYDUINO >= 117
  #define regtype volatile uint8_t
  #define regsize uint8_t
#else
  #define regtype volatile uint32_t
  #define regsize uint32_t
#endif
