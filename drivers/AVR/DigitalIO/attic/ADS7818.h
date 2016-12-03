#ifndef ADS7818_h
#define ADS7871_h
//------------------------------------------------------------------------------
/** nop to tune soft SPI timing */
#define nop asm volatile ("nop\n\t")
//------------------------------------------------------------------------------
/** doxygen can't handle __attribute__ very well */
#define STATIC_ALWAYS_INLINE static inline __attribute__((always_inline))
//------------------------------------------------------------------------------
template<uint8_t ClkPin, uint8_t ConvPin, uint8_t DataPin>
/** ADS7818 class */
class ADS7818
{
public:
	//----------------------------------------------------------------------------
	/** Set pin modes and initial levels. */
	void begin()
	{
		fastPinMode(ClkPin, 1);
		fastDigitalWrite(ClkPin, 0);
		fastPinMode(ConvPin, 1);
		fastDigitalWrite(ConvPin, 1);
		fastPinMode(DataPin, 0);
	}
	//----------------------------------------------------------------------------
	/** Read ADS7818 12-bit ADC
	 * \return 16-bit value read.
	 */
	inline __attribute__((always_inline))
	uint16_t read()
	{
		uint16_t v = 0;
		fastDigitalWrite(ConvPin, 0);
		fastDigitalWrite(ClkPin, 1);
		fastDigitalWrite(ClkPin, 0);
		fastDigitalWrite(ConvPin, 1);
		fastDigitalWrite(ClkPin, 1);
		fastDigitalWrite(ClkPin, 0);

		readBitFast16(v,  11);
		readBitFast16(v,  10);
		readBitFast16(v,  9);
		readBitFast16(v,  8);
		readBitFast16(v,  7);
		readBitFast16(v,  6);
		readBitFast16(v,  5);
		readBitFast16(v,  4);
		readBitFast16(v,  3);
		readBitFast16(v,  2);
		readBitFast16(v,  1);
		readBitFast16(v,  0);
		return  v;
	}
private:
	//----------------------------------------------------------------------------
	inline __attribute__((always_inline))
	void readBitFast16(uint16_t &v, uint8_t b)
	{
		fastDigitalWrite(ClkPin, 1);
		if (fastDigitalRead(DataPin)) {
			v |= (1 << b);
		}
		fastDigitalWrite(ClkPin, 0);
	}
};
#if 0
/** initialize I/O ports */
STATIC_ALWAYS_INLINE void adcBegin()
{
	fastPinMode(ADC_CLK_PIN, 1);
	fastDigitalWrite(ADC_CLK_PIN, 0);
	fastPinMode(ADC_CONV_PIN, 1);
	fastDigitalWrite(ADC_CONV_PIN, 1);
	fastPinMode(ADC_DATA_PIN, 0);
}
//------------------------------------------------------------------------------
/** clock for don't care bits
 *\param[in] delayBefore if true, delay before raising clock*/
STATIC_ALWAYS_INLINE void fastDummy(bool first)
{
	if (!first) {
		nop;
	}
	fastDigitalWrite(ADC_CLK_PIN, 1);
	nop;
	fastDigitalWrite(ADC_CLK_PIN, 0);
}
//------------------------------------------------------------------------------
/** read next bit fast as possible
 * \param[in] v word to receive bit
 * \param[in] b bit number to be set.  v |= (1 << b) if next bit is high.
 */
STATIC_ALWAYS_INLINE void readBitFast16(uint16_t &v, uint8_t b)
{
	fastDigitalWrite(ADC_CLK_PIN, 1);
	if (fastDigitalRead(ADC_DATA_PIN)) {
		v |= (1 << b);
	}
	fastDigitalWrite(ADC_CLK_PIN, 0);
}
//------------------------------------------------------------------------------
/** Read AD7680 16-bit ADC in less than 8 microseconds
 *  cs is chip select pin
 */
STATIC_ALWAYS_INLINE uint16_t adcRead(bool centered = false)
{
	uint16_t v = 0;
	fastDigitalWrite(ADC_CONV_PIN, 0);
	fastDigitalWrite(ADC_CLK_PIN, 1);
	fastDigitalWrite(ADC_CLK_PIN, 0);
	fastDigitalWrite(ADC_CONV_PIN, 1);
	fastDigitalWrite(ADC_CLK_PIN, 1);
	fastDigitalWrite(ADC_CLK_PIN, 0);
	//  uint16_t v = 0;

	//  fastDummy(1);
	//  fastDummy(0);
	//  fastDummy(0);
	// fastDummy(0);

	//  readBitFast16(v,  15);
	//  readBitFast16(v,  14);
	//  readBitFast16(v,  13);
	//  readBitFast16(v,  12);
	readBitFast16(v,  11);
	readBitFast16(v,  10);
	readBitFast16(v,  9);
	readBitFast16(v,  8);
	readBitFast16(v,  7);
	readBitFast16(v,  6);
	readBitFast16(v,  5);
	readBitFast16(v,  4);
	readBitFast16(v,  3);
	readBitFast16(v,  2);
	readBitFast16(v,  1);
	readBitFast16(v,  0);
	return centered ? v ^ 0X8000 : v;
}
#endif  // 0
#endif  // ADS7818_h