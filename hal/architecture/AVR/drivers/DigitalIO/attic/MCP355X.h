#ifndef MCP355X_h
#define MCP355X_h
#include <DigitalIO.h>
// Overflow values.
const int32_t MCP355X_OVH =  2097152L;
const int32_t MCP355X_OVL = -2097153L;
const int32_t MCP355X_ERR = 0X80000000;
const uint8_t MCP355X_TO_MS = 100;
//==============================================================================
template<uint8_t CsPin, uint8_t SckPin, uint8_t SdoPin>
/** MCP355X class */
class MCP355X
{
public:
	//----------------------------------------------------------------------------
	/** Set pin modes and initial levels. */
	void begin(bool singleMode = true)
	{
		m_singleMode = singleMode;
		fastPinMode(SdoPin, 0);
		fastPinMode(SckPin, 1);
		fastDigitalWrite(SckPin, 1);
		fastPinMode(CsPin, 1);
		fastDigitalWrite(CsPin, 1);
		if (!m_singleMode) {
			// Wait for conversion to complete.
			delay(100);
			// Start continuous conversion mode.
			fastDigitalWrite(CsPin, 0);
		}
	}
	//----------------------------------------------------------------------------
	/** Read MCP355X 22-bit ADC
	 * \return 22-bit value.
	 */
	inline __attribute__((always_inline))
	int32_t read()
	{
		uint8_t t = 0;
		uint8_t v3 = 0;
		if (m_singleMode) {
			// Start conversion.
			fastDigitalWrite(CsPin, 0);
			// Delay at least 10 usec to avoid RDY glitch on exit from Shutdown.
			delay(1);
			// Toggle CsPin to indicate single conversion mode.
			fastDigitalWrite(CsPin, 1);
			// Wait for conversion to complete.
			while (1) {
				delay(1);
				fastDigitalWrite(CsPin, 0);
				// Delay while RDY settles.
				delayCycles(4);
				if (!fastDigitalRead(SdoPin)) {
					break;
				}
				fastDigitalWrite(CsPin, 1);
				if (t++ > MCP355X_TO_MS) {
					return MCP355X_ERR;
				}
			}
		} else {
			while (1) {
				if (!fastDigitalRead(SdoPin)) {
					break;
				}
				delay(1);
				if (t++ > MCP355X_TO_MS) {
					return MCP355X_ERR;
				}
			}
		}
		uint8_t v2 = readByte();
		uint8_t v1 = readByte();
		uint8_t v0 = readByte();

		// The 25th falling edge of SCK changes SDO/RDY from Data mode to RDY mode.
		uint8_t dummy = 0;
		readBit(dummy, 0);

		if (m_singleMode) {
			fastDigitalWrite(CsPin, 1);
		}
		if ((v2 & 0XE0) == 0X20) {
			// Negative in range so extend sign bit.
			v2 |= 0XC0;
			v3 = 0XFF;
		} else if (v2 & 0X40) {
			// Overflow high.  Cause value to be >= MCP355X_OVH.
			if (v2 & 0X20) {
				v2 &= 0X3F;
			}
		} else if (v2 & 0X80) {
			// Overflow low.  Cause value to be <= MCP355X_OVH.
			if ((v2 & 0X20) == 0) {
				v2 |= 0X40;
			}
			v3 = 0XFF;
		}
		uint16_t v_high = (v3 << 8) | v2;
		uint16_t v_low = (v1 << 8) | v0;
		return ((uint32_t)v_high << 16) | v_low;
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
	// Default delay yields about 2 MHz clock.
	inline __attribute__((always_inline))
	void readBit(uint8_t &v, uint8_t b, uint8_t delayRead = 0)
	{
		fastDigitalWrite(SckPin, 0);
		delayCycles(2 + delayRead);
		fastDigitalWrite(SckPin, 1);
		if (fastDigitalRead(SdoPin)) {
			v |= (1 << b);
		}
		delayCycles(delayRead);
	}
	//----------------------------------------------------------------------------
	inline __attribute__((always_inline))
	uint8_t readByte()
	{
		uint8_t v = 0;
		readBit(v, 7);
		readBit(v, 6);
		readBit(v, 5);
		readBit(v, 4);
		readBit(v, 3);
		readBit(v, 2);
		readBit(v, 1);
		readBit(v, 0);
		return v;
	}
	//----------------------------------------------------------------------------
	bool m_singleMode;
};
#endif  // MCP355X_h