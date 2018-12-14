#include "Radio.h"

int16_t NRF5_getTxPowerPercent(void)
{
	// NRF5_PA_MAX = 100% NRF5_PA_MIN=0%
	int16_t dbm = NRF5_getTxPowerLevel();
	int16_t dbm_diff = ((int8_t)NRF5_PA_MAX-(int8_t)NRF5_PA_MIN);
	int16_t dbm_min = (int8_t)NRF5_PA_MIN;
	return ((dbm-dbm_min)*100)/dbm_diff;
}

int16_t NRF5_getTxPowerLevel(void)
{
	return (int8_t)NRF_RADIO->TXPOWER;
}

bool NRF5_setTxPowerPercent(const uint8_t powerPercent)
{
	/* Current mapping:
	 * NRF51/NRF52822:
	 *         0.. 2% -> -40dBm (0%)
	 *         3..56% -> -16dBm (54%)
	 *        57..65% -> -12dBm (63%)
	 *        66..72% ->  -8dBm (72%)
	 *        75..84% ->  -4dBm (81%)
	 *        85..95% ->   0dBm (90%)
	 * NRF51  96..100%->   4dBm (100%)
	 * NRF52  96..99% ->   3dBm (97%)
	 * NRF52  100%    ->   4dBm (100%)
	 */

	// Calculate dbm level
	int16_t dbm_diff = ((int8_t)NRF5_PA_MAX-(int8_t)NRF5_PA_MIN);
	int16_t dbm_min = (int8_t)NRF5_PA_MIN;
	int8_t dbm = ((dbm_diff * powerPercent)/100)+dbm_min;
	if (dbm >= (int8_t)NRF5_PA_MAX) {
		NRF_RADIO->TXPOWER = NRF5_PA_MAX;
		return true;
	}
	if (dbm > 1) {
#ifdef RADIO_TXPOWER_TXPOWER_Pos2dBm
		// NRF52840
		NRF_RADIO->TXPOWER = dbm;
#elif defined(NRF51)
		// nRF51x22
		NRF_RADIO->TXPOWER = RADIO_TXPOWER_TXPOWER_Pos4dBm;
#else
		// NRF52822
		if (dbm > 3) {
			NRF_RADIO->TXPOWER = RADIO_TXPOWER_TXPOWER_Pos4dBm;
		} else {
			NRF_RADIO->TXPOWER = RADIO_TXPOWER_TXPOWER_Pos3dBm;
		}
#endif
		return true;
	}
	if (dbm > (int8_t)RADIO_TXPOWER_TXPOWER_Neg4dBm) {
		NRF_RADIO->TXPOWER = RADIO_TXPOWER_TXPOWER_0dBm;
		return true;
	}
	if (dbm > (int8_t)RADIO_TXPOWER_TXPOWER_Neg4dBm) {
		NRF_RADIO->TXPOWER = RADIO_TXPOWER_TXPOWER_0dBm;
		return true;
	}
	if (dbm > (int8_t)RADIO_TXPOWER_TXPOWER_Neg8dBm) {
		NRF_RADIO->TXPOWER = RADIO_TXPOWER_TXPOWER_Neg4dBm;
		return true;
	}
	if (dbm > (int8_t)RADIO_TXPOWER_TXPOWER_Neg12dBm) {
		NRF_RADIO->TXPOWER = RADIO_TXPOWER_TXPOWER_Neg8dBm;
		return true;
	}
	if (dbm > (int8_t)RADIO_TXPOWER_TXPOWER_Neg16dBm) {
		NRF_RADIO->TXPOWER = RADIO_TXPOWER_TXPOWER_Neg12dBm;
		return true;
	}
	if (dbm > (int8_t)RADIO_TXPOWER_TXPOWER_Neg20dBm) {
		NRF_RADIO->TXPOWER = RADIO_TXPOWER_TXPOWER_Neg16dBm;
		return true;
	}
	if (dbm > (int8_t)NRF5_PA_MIN) {
		NRF_RADIO->TXPOWER = RADIO_TXPOWER_TXPOWER_Neg16dBm;
		return true;
	}

	NRF_RADIO->TXPOWER = NRF5_PA_MIN;
	return true;
}
