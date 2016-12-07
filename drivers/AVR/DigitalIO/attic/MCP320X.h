#ifndef MCP320X_h
#define MCP320X_h
#include <DigitalIO.h>
//==============================================================================
template<uint8_t CsPin, uint8_t ClkPin, uint8_t DoutPin, uint8_t UsecDelay = 0>
/** MCP3201 class */
class MCP3201
{
public:
	//----------------------------------------------------------------------------
	/** Set pin modes and initial levels. */
	void begin()
	{
		fastPinMode(ClkPin, 1);
		fastDigitalWrite(ClkPin, 0);
		fastPinMode(CsPin, 1);
		fastDigitalWrite(CsPin, 1);
		fastPinMode(DoutPin, 0);
	}
	//----------------------------------------------------------------------------
	/** Read MCP3201 12-bit ADC
	 * \return 16-bit value read.
	 */
	inline __attribute__((always_inline))
	uint16_t read()
	{
		fastDigitalWrite(CsPin, 0);
		// Start sample.
		mcpAdcDummy(3);
		// Extra sample time.
		if (UsecDelay) {
			delayMicroseconds(UsecDelay);
		}
		// End sample.
		mcpAdcDummy(3);
		// Null bit.
		mcpAdcDummy(3);
		uint16_t v = 0;
		readBit(v, 11);
		readBit(v, 10);
		readBit(v,  9);
		readBit(v,  8);
		readBit(v,  7);
		readBit(v,  6);
		readBit(v,  5);
		readBit(v,  4);
		readBit(v,  3);
		readBit(v,  2);
		readBit(v,  1);
		readBit(v,  0);
		fastDigitalWrite(CsPin, 1);
		return v;
	}
private:
	//----------------------------------------------------------------------------
	/** delay n nops
	 * \param[in] n nops to delay, must be a constant so compiler optimizes if().
	 */
	inline __attribute__((always_inline))
	void delayCycles(uint8_t n)
	{
		if (n & 1) {
			asm volatile("nop\n\t");
		}
		if (n & 2) {
			asm volatile("nop\n\t" "nop\n\t");
		}
		if (n & 4) {
			asm volatile("nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t");
		}
	}
	//----------------------------------------------------------------------------
	/** clock for don't care bits
	 *\param[in] delayBefore if true, delay before raising clock*/
	inline __attribute__((always_inline))
	void mcpAdcDummy(uint8_t delayBefore)
	{
		delayCycles(delayBefore);
		fastDigitalWrite(ClkPin, 1);
		delayCycles(3);
		fastDigitalWrite(ClkPin, 0);
	}
	//----------------------------------------------------------------------------
	inline __attribute__((always_inline))
	void readBit(uint16_t &v, uint8_t b, uint8_t delayRead = 2)
	{
		delayCycles(3);
		fastDigitalWrite(ClkPin, 1);
		delayCycles(delayRead);
		if (fastDigitalRead(DoutPin)) {
			v |= (1 << b);
		}
		fastDigitalWrite(ClkPin, 0);
	}
};
//==============================================================================
template<uint8_t CsPin, uint8_t ClkPin,
         uint8_t DoutPin, uint8_t DinPin, uint8_t UsecDelay = 0>
/** MCP3202 class */
class MCP3202
{
public:
	//----------------------------------------------------------------------------
	/** Set pin modes and initial levels. */
	void begin()
	{
		fastPinMode(ClkPin, 1);
		fastDigitalWrite(ClkPin, 0);
		fastPinMode(CsPin, 1);
		fastDigitalWrite(CsPin, 1);
		fastPinMode(DoutPin, 0);
		fastPinMode(DinPin, 1);
	}
	//----------------------------------------------------------------------------
	/** Read MCP3202 12-bit ADC
	 * \return 16-bit value read.
	 */
	inline __attribute__((always_inline))
	uint16_t read(uint8_t config)
	{
		uint16_t v = 0;
		fastDigitalWrite(CsPin, 0);
		// Start bit.
		writeBit(true);
		// Mode bit.
		writeBit(config & 2);
		// Channel Selection and start sample.
		writeBit(config & 1);
		// Extra sample time.
		if (UsecDelay) {
			delayMicroseconds(UsecDelay);
		}
		// MSB first format and end sample.
		writeBit(true);
		// Null bit.
		writeBit(true);
		// Data bits.
		readBit(v, 11);
		readBit(v, 10);
		readBit(v,  9);
		readBit(v,  8);
		readBit(v,  7);
		readBit(v,  6);
		readBit(v,  5);
		readBit(v,  4);
		readBit(v,  3);
		readBit(v,  2);
		readBit(v,  1);
		readBit(v,  0);
		fastDigitalWrite(CsPin, 1);
		return v;
	}
private:
	//----------------------------------------------------------------------------
	/** delay n nops
	 * \param[in] n nops to delay, must be a constant so compiler optimizes if().
	 */
	inline __attribute__((always_inline))
	void delayCycles(uint8_t n)
	{
		if (n & 1) {
			asm volatile("nop\n\t");
		}
		if (n & 2) {
			asm volatile("nop\n\t" "nop\n\t");
		}
		if (n & 4) {
			asm volatile("nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t");
		}
	}
	//----------------------------------------------------------------------------
	/** clock for don't care bits
	 *\param[in] delayBefore if true, delay before raising clock*/
	inline __attribute__((always_inline))
	void writeBit(bool value)
	{
		fastDigitalWrite(DinPin, value);
		fastDigitalWrite(ClkPin, 1);
		delayCycles(2);
		fastDigitalWrite(ClkPin, 0);
	}
	//----------------------------------------------------------------------------
	inline __attribute__((always_inline))
	void readBit(uint16_t &v, uint8_t b, uint8_t delayRead = 1)
	{
		delayCycles(2);
		fastDigitalWrite(ClkPin, 1);
		delayCycles(delayRead);
		if (fastDigitalRead(DoutPin)) {
			v |= (1 << b);
		}
		fastDigitalWrite(ClkPin, 0);
	}
};
//==============================================================================
template<uint8_t CsPin, uint8_t ClkPin,
         uint8_t DoutPin, uint8_t DinPin, uint8_t UsecDelay = 0>
/** MCP3204 class */
class MCP3204
{
public:
	//----------------------------------------------------------------------------
	/** Set pin modes and initial levels. */
	void begin()
	{
		fastPinMode(ClkPin, 1);
		fastDigitalWrite(ClkPin, 0);
		fastPinMode(CsPin, 1);
		fastDigitalWrite(CsPin, 1);
		fastPinMode(DoutPin, 0);
		fastPinMode(DinPin, 1);
	}
	//----------------------------------------------------------------------------
	/** Read MCP3204 12-bit ADC
	 * \return 16-bit value read.
	 */
	inline __attribute__((always_inline))
	uint16_t read(uint8_t config)
	{
		uint16_t v = 0;
		fastDigitalWrite(CsPin, 0);
		// Start bit.
		writeBit(true);
		// Mode bit.
		writeBit(config & 8);
		// D2 channel bit.
		writeBit(config & 4);
		// D1 channel bit.
		writeBit(config & 2);
		// D0 channel bit and start sample.
		writeBit(config & 1);
		// Extra sample time.
		if (UsecDelay) {
			delayMicroseconds(UsecDelay);
		}
		// End sample cycle.
		writeBit(true);
		// Null bit.
		writeBit(true);
		// Data bits.
		readBit(v, 11);
		readBit(v, 10);
		readBit(v,  9);
		readBit(v,  8);
		readBit(v,  7);
		readBit(v,  6);
		readBit(v,  5);
		readBit(v,  4);
		readBit(v,  3);
		readBit(v,  2);
		readBit(v,  1);
		readBit(v,  0);
		fastDigitalWrite(CsPin, 1);
		return v;
	}
private:
	//----------------------------------------------------------------------------
	/** delay n nops
	 * \param[in] n nops to delay, must be a constant so compiler optimizes if().
	 */
	inline __attribute__((always_inline))
	void delayCycles(uint8_t n)
	{
		if (n & 1) {
			asm volatile("nop\n\t");
		}
		if (n & 2) {
			asm volatile("nop\n\t" "nop\n\t");
		}
		if (n & 4) {
			asm volatile("nop\n\t" "nop\n\t" "nop\n\t" "nop\n\t");
		}
	}
	//----------------------------------------------------------------------------
	/** clock for don't care bits
	 *\param[in] delayBefore if true, delay before raising clock*/
	inline __attribute__((always_inline))
	void writeBit(bool value)
	{
		fastDigitalWrite(DinPin, value);
		fastDigitalWrite(ClkPin, 1);
		delayCycles(2);
		fastDigitalWrite(ClkPin, 0);
	}
	//----------------------------------------------------------------------------
	inline __attribute__((always_inline))
	void readBit(uint16_t &v, uint8_t b, uint8_t delayRead = 1)
	{
		delayCycles(2);
		fastDigitalWrite(ClkPin, 1);
		delayCycles(delayRead);
		if (fastDigitalRead(DoutPin)) {
			v |= (1 << b);
		}
		fastDigitalWrite(ClkPin, 0);
	}
};
//==============================================================================
// MCP3204 and MCP3208 use the same code.
template<uint8_t CsPin, uint8_t ClkPin,
         uint8_t DoutPin, uint8_t DinPin, uint8_t UsecDelay = 0>
class MCP3208 : public MCP3204<CsPin, ClkPin, DoutPin, DinPin, UsecDelay>
{
};
#endif  // MCP320X_h