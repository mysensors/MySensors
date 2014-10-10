
// ---------------------------------------------------------------------------
// Created/Adapted by Stephen Erisman 2013-07-06
// Copyright 2013 - Under creative commons license 3.0:
//        Attribution-ShareAlike CC BY-SA
//
// This software is furnished "as is", without technical support, and with no 
// warranty, express or implied, as to its usefulness for any purpose.
//
// @file LiquidCrystal_SR1W.h
// Connects a hd44780 LCD using 1 pin from the Arduino, via an 8-bit Latching
// ShiftRegister (SR1W from now on).
// 
// @brief 
// This is the 1 wire shift register interface class for the LCD library
//
// The functionality provided by this class and its base class is a superset of
// the original functionality of the Arduino LiquidCrystal library and can
// be used as such.
// See the LCD class for a full description of the API functions available.
//
// It works with a 8-bit latched, no-tristate, unidirectional SIPO (Serial-In-Parallel-Out)
// shift register, and an hd44780 LCD in 4-bit mode.
// The 74HC595 shift register has been tested.
//
//
// 1 Pin required from the Arduino:
// - Serial PIN:
//    The Serial PIN is wired directly to the shift register's Clock PIN and its
//    unaltered signal directly triggers the Clock on every LOW to HIGH transition.
//
//    Additionally, the Serial PIN is wired through a resistor capacitor (RC) filter to
//    the shift register's Data PIN.  During a quick transition of the Serial PIN the
//    RC filter will maintain the Data PIN's previous value because the capacitor isn't
//    given enough time to charge (or discharge) to the alternate state.  If the transition
//    is held for enough time, however, the RC capacitor will charge (or discharge) and the
//    value seen by the Data PIN will have changed state.
//
//    There are two circuit versions that behave differently for Latch, Enable, and Clear:
//
//  HW_CLEAR version:
//    In this version the shift register's Latch and LCD's Enable PINs are wired directly to
//    the shift register's Q'H output.  The shift register's /Clear PIN is then wired up
//    through two logic "gates": first QH and Q'H are AND'd together with a diode-resistor
//    "gate" the output of which is NAND'd with VCC using a resistor-NPN-resistor "gate".
//    So, /CLR = ((QH AND Q'H) NAND VCC).  We also put a capacitor on the NPN base to GND
//    to delay the signal a bit and allow the Latch and EN signals some extra time to trigger.
//    
//    This all fits together as follows:
//      1. We shift in a '1'.
//      2. Ws shift in the other 7 bits.
//      3. At this point the first '1' has been shifted into Q'H causing it to go HIGH.
//      4. When Q'H is HIGH it causes Latch and EN to also go HIGH.
//      5. When Latch transitions to HIGH it changes the shift register outputs to the bits
//         that were shifted in.
//      6. This causes QH to go HIGH (if it wasn't already).
//      7. Now that QH AND Q'H are both HIGH they causes the base capacitor to start charging.
//      8. When the capacitor has charged enough the transistor brings /CLR LOW.
//      8. This will cause /CLR to trigger and the shift register will be cleared
//         (NOTE: This doesn't change the latched outputs)
//      9. The clearing of the shift register causes Q'H to go LOW.
//      9. When Q'H is LOW it causes Latch and EN to also go LOW.
//      10. When EN transitions to LOW the LCD reads in the bits on the shift register pins
//          and does it's thing.
//      11. Now that Q'H is LOW the base capacitor starts discharging.
//      12. When the capacitor has discharged enough the transistor will stop sinking /CLR.
//      13. This will cause /CLR to be pulled back up to HIGH by the VCC pullup resistor
//          (it will stay that way until our next nibble/byte has been shifted in)
//      14. We are now ready for our next nibble/byte.
//
//
//  SW_CLEAR version:
//    In this version the Serial PIN is wired to the shift register's Latch and LCD's Enable
//    PINs through another RC filter.  These PINs are also wired through a diode (AND "gate")
//    tied to the shift register's Q'H output.  This combination means that the Latch and
//    Enable PINs will be held LOW as long as EITHER the Q'H or RC output is LOW.
//
//    This all fits together as follows:
//      1. We shift in a '1'.
//      2. We shift in the other 7 bits. (NOTE: We leave Serial PIN HIGH at the end of this)
//      3. At this point the first '1' has been shifted into Q'H causing it to go HIGH.
//         (NOTE: Up until this time Q'H has been LOW so the attached diode has been keeping
//          the Latch/EN pins LOW.)
//      4. Now that Q'H is HIGH it causes the attached diode to stop discharging the Latch/EN
//         capacitor.  We delay here for a while to make sure it is fully charged.
//      5. When the capacitor has charged enough Latch/EN will be HIGH
//      5. When Latch transitions to HIGH it changes the shift register outputs to what was
//         shifted in.
//      6. We now bring the Serial PIN LOW and wait for the Latch/EN capacitor to discharge.
//      7. When the capacitor has discharged enough Latch/EN will be LOW
//      8. When EN transitions to LOW the LCD reads in the bits on the shift register pins
//         and does it's thing.
//      9. We now shift in '0' 8 times (as quickly as possible).
//      10. If we keep the LOW to HIGH to LOW pulses short enough while shifting in the '0's
//          the Latch/EN capacitor won't have time to charge to a point where it will re-trigger
//          the Latch/EN pins.
//      11. Now Q'H will be LOW and the shift register has been cleared (NOTE: This doesn't
//          change the latched outputs.)
//      12. We now bring the Serial PIN HIGH again and wait for the Data capacitor to recharge.
//      13. When the Data capacitor has fully charged we are ready for our next nibble/byte.
//
//
// These designs incorporate (and merge) ideas originally found here (1-wire concept):
//   http://www.romanblack.com/shift1.htm
// and here (diode-resistor AND "gate" EN control):
//   http://www.rentron.com/Myke1.htm
// as well as introducing some new and original ideas (particularly how HW_CLEAR works).
//
// Because of its use of the diode AND "gate", the SW_CLEAR design allows for faster sending
// of data to the LCD compared to Roman's original design.  With the proposed 5uS delay (see
// notes below), a byte can be sent to the LCD in as little as 30 uS (plus overhead) when
// sending all 1's. This increases to as much as 190 uS (plus overhead) when sending all 0's.
// This is in comparison to Roman's estimate of around 3-4 mS to send a byte. So this
// implementation is 15-133 times faster for the cost of a single (1N4148 or similar) diode.
//
// The HW_CLEAR version is even faster as it can completely eliminate the clearSR() call as
// well as the delays that are needed to latch the data in the SW_CLEAR version.
//
//
// Default Shift Register Bits - Shifted MSB first:
// Bit #0 (QA) - not used
// Bit #1 (QB) - connects to LCD data input D7
// Bit #2 (QC) - connects to LCD data input D6
// Bit #3 (QD) - connects to LCD data input D5
// Bit #4 (QE) - connects to LCD data input D4
// Bit #5 (QF) - optional backlight control
// Bit #6 (QG) - connects to RS (Register Select) on the LCD
// Bit #7 (QH) - used for /CLR on the HW_CLEAR version (cannot be changed)
//        (Q'H) - used for Latch/EN (via the diode AND "gate") (cannot be changed)
//
// NOTE: Any of these can be changed around as needed EXCEPT Bit #7 (QH and Q'H).
//
//
// Circuit Types (for the 74HC595)
// -------------------------------
// The 74HC595 is a latching shift register.  See the explanations above for how these circuits
// work.
//
//
// HW_CLEAR version: (Faster but higher part count)
// ------------------------------------------------
//
//                         74HC595     (VCC)
//                       +----u----+     |        2.2nF
// (LCD D7)------------1-|QB    VCC|-16--+    +----||----(GND)
// (LCD D6)------------2-|QC     QA|-15       |
// (LCD D5)------------3-|QD    SER|-14-------+--[ Resistor ]--+
// (LCD D4)------------4-|QE    /OE|-13--(GND)     1.5k        |
// (BL Circuit)--------5-|QF    RCK|-12-----+                  |
//                       |         |         \                 |
// (LCD RS)------------6-|QG    SCK|-11-------)----------------+--(Serial PIN)
//                       |         |          |
//             +-------7-|QH   /CLR|-10-------)--+--[ Resistor ]--(VCC)
//             |         |         |         /   |       1k
//             |    +--8-|GND   Q'H|--9-----+    |
//             |    |    +---------+        |    |         (GND)--(LCD RW)
//             |    |      0.1uF            |     \
//             |  (GND)-----||----(VCC)     +------)--------------(LCD EN)
//             |                            |     /
//             |----|<|----+--[ Resistor ]--|    |
//                 diode   |       1k            C
//                         |                     |
//                         +-------------+---B-|> (NPN)
//                                       |       |
//                               (2.2nF) =       E
//                                       |       |
//                                     (GND)   (GND)
//
//
// SW_CLEAR version: (Lower part count but slower)
// -----------------------------------------------
//
//                         74HC595     (VCC)
//                       +----u----+     |          2.2nF
// (LCD D7)------------1-|QB    VCC|-16--+      +----||----(GND)
// (LCD D6)------------2-|QC     QA|-15         |
// (LCD D5)------------3-|QD    SER|-14---------+--[ Resistor ]--+
// (LCD D4)------------4-|QE    /OE|-13--(GND)         1.5k      |
// (BL Circuit)--------5-|QF    RCK|-12---------+                |
//                       |         |             \               |
// (LCD RS)------------6-|QG    SCK|-11-----------)--------------+--(Serial PIN)
//                     7-|QH   /CLR|-10--(VCC)   /               |
//                  +--8-|GND   Q'H|--9---|<|---+--[ Resistor ]--+
//                  |    +---------+     diode  |      1.5k
//                  |                           |
//                  |      0.1uF                |
//                (GND)-----||----(VCC)         +----||----(GND)
//                                              |   2.2nF
// (LCD EN)-------------------------------------+
// (LCD RW)--(GND)
//
//
// In either case the LCD RW pin is hardwired to GND meaning we will only be able to write
// to the LCD.
// Therefore, the Busy Flag (BF, data bit D7) is not able to be read and we have to make use
// of the minimum delay time constraints.  This isn't really a problem because it usually
// takes us longer to shift and latch the data than the minimum delay anyway.  For now, we
// simply keep track of our delays and add more delay at the end to get to at least 37 uS.
//
//
// Backlight Control Circuit
// -------------------------
// Since we are using the latching nature of the shift resiter we don't need the extra
// backlight circuitry that SR2W uses.  Keeping it around, however, would still work because
// the circuit just slows down the transitions to the mosfet a bit.
//
// Here are two more optimized versions that can be used.
//
//
// NPN Transistor version: (Cheaper but more power draw and higher part count)
// ---------------------------------------------------------------------------
//
//                (value depends on LCD, 100ohm is usually safe)
// (LCD BL anode)---[ resistor ]---(VCC)
//
// (LCD BL cathode)---------------+
//                                |
//                                C
//                                |
// (BL input)--[ Resistor ]---B-|> (NPN)
//                  1k            |
//                                E
//                                |
//                              (GND)
//
// NOTE: The Bate resistor is needed because the NPN is current fed.  For lower
//       power draw, try a 10k resistor.
//
//
// N-CH Mosfet version: (More costly but less power draw and lower part count)
// ---------------------------------------------------------------------------
//
//                (value depends on LCD, 100ohm is usually safe)
// (LCD BL anode)---[ resistor ]---(VCC)
//
// (LCD BL cathode)---------------+
//                                |
//                                D
//                                |
// (BL input)----------------G-|-< (2N7000 FET)
//                                |
//                                S
//                                |
//                              (GND)
//
// NOTE: Gate resistor not needed because the mosfet is voltage fed and only really
//       pulls current while switching.
//
// In either case, when the BL input is HIGH the LCD backlight will turn on.
//
//
// History
// 2013.07.31 serisman - fixed potential interrupt bug and made more performance optimizations
// 2013.07.10 serisman - more performance optimizations and modified the HW_CLEAR circuit a bit
// 2013.07.09 serisman - added an even faster version that performs the clear in hardware
// 2013.07.08 serisman - changed code to shift data MSB first to match SR2W
// 2013.07.07 serisman - major speed optimization
// 2013.07.06 serisman - created/modified from SR2W source to create SR1W
// @author  S. Erisman - arduino@serisman.com
// --------------------------------------------------------------------------------

#ifndef _LIQUIDCRYSTAL_SR1W_
#define _LIQUIDCRYSTAL_SR1W_

#include <inttypes.h>
#include "LCD.h"
#include "FastIO.h"

// 1-wire SR timing constants
// ---------------------------------------------------------------------------

// NOTE:
//  The 1.5k resistor (1.2k - 1.8k with a 20% tolerance)
//   takes between 2.376uS and 4.36uS to fully charge or discharge
//	 the 2.2n capacitor (1.98n - 2.42n with a 10% tolerance).
//	We round this up to a 5uS delay to provide an additional safety margin.

#define SR1W_DELAY_US		5
#define SR1W_DELAY()		{ delayMicroseconds(SR1W_DELAY_US); numDelays++; }

// 1-wire SR output bit constants
// ---------------------------------------------------------------------------

#define SR1W_UNUSED_MASK	0x01	// Set unused bit(s) to '1' as they are slightly faster to clock in.
#define SR1W_D7_MASK		0x02
#define SR1W_D6_MASK		0x04
#define SR1W_D5_MASK		0x08
#define SR1W_D4_MASK		0x10
#define SR1W_BL_MASK		0x20
#define SR1W_RS_MASK		0x40
#define SR1W_EN_MASK		0x80	// This cannot be changed. It has to be the first thing shifted in.

#define SR1W_ATOMIC_WRITE_LOW(reg, mask)	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { *reg &= ~mask; }
#define SR1W_ATOMIC_WRITE_HIGH(reg, mask)	ATOMIC_BLOCK(ATOMIC_RESTORESTATE) { *reg |= mask; }


typedef enum { SW_CLEAR, HW_CLEAR } t_sr1w_circuitType;

class LiquidCrystal_SR1W : public LCD
{
public:
   /*!
    @method     
    @abstract   LCD 1 wire SHIFT REGISTER constructor.
    @discussion Defines the pin assignments that connect to the shift register.
    The constructor does not initialize the LCD. Assuming 1 line 8 pixel high 
    font.
    
    @param srdata[in]       Arduino pin for shift register.
    @param circuitType[in]  optionally select an alternate circuit type
    @param blpol[in]        optional backlight polarity (default = POSITIVE)
    */
   LiquidCrystal_SR1W (uint8_t srdata, t_sr1w_circuitType circuitType, 
                       t_backlighPol blpol = POSITIVE);
   
   /*!
    @function
    @abstract   Send a particular value to the LCD.
    @discussion Sends a particular value to the LCD for writing to the LCD or
    as an LCD command using the shift register.
    
    Users should never call this method.
    
    @param      value[in] Value to send to the LCD.
    @param      mode[in]  DATA=8bit data, COMMAND=8bit cmd, FOUR_BITS=4bit cmd
    the LCD.
    */
   virtual void send(uint8_t value, uint8_t mode);
   
   
   /*!
    @function
    @abstract   Switch-on/off the LCD backlight.
    @discussion Switch-on/off the LCD backlight.
    The setBacklightPin has to be called before setting the backlight for
    this method to work. @see setBacklightPin.
    
    @param      mode[in] backlight mode (0 off, non-zero on)
    */
   void setBacklight ( uint8_t mode );
   
private:
   
   /*!
    @method     
    @abstract   Initializes the LCD pin allocation
    @discussion Initializes the LCD pin allocation and configuration.
    */
   void init ( uint8_t srdata, t_sr1w_circuitType circuitType, t_backlighPol blpol, 
               uint8_t lines, uint8_t font );
   
   /*!
    @method     
    @abstract Clears the shift register to ensure the Latch/Enable pins aren't 
    triggered accidentally.
    */
   uint8_t clearSR ();
   
   /*!
    * @method
    * @abstract takes care of shifting and the enable pulse
    */
   uint8_t loadSR (uint8_t val);
   
   fio_register _srRegister; // Serial PIN
   fio_bit _srMask;
   
   t_sr1w_circuitType _circuitType;
   
   uint8_t _blPolarity;
   uint8_t _blMask;
};
#endif