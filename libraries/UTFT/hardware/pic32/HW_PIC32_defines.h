// AquaLEDSource All in One Super Screw Shield
// -------------------------------------------
// Uncomment the following line if you are using this shield
//#define AQUALED_SHIELD 1
//
// For this shield: RS=82, WR=83, CS=84, RST=85 (Standard for chipKit Max32)
//**************************************************************************

// *** Hardwarespecific defines ***
#define cbi(reg, bitmask) (*(reg + 1)) = bitmask
#define sbi(reg, bitmask) (*(reg + 2)) = bitmask
#define pulse_high(reg, bitmask) sbi(reg, bitmask); cbi(reg, bitmask);
#define pulse_low(reg, bitmask) cbi(reg, bitmask); sbi(reg, bitmask);

#define cport(port, data) port &= data
#define sport(port, data) port |= data

#define swap(type, i, j) {type t = i; i = j; j = t;}

#define fontbyte(x) cfont.font[x]  

#define PROGMEM
#define regtype volatile uint32_t
#define regsize uint16_t
#define bitmapdatatype unsigned short*

