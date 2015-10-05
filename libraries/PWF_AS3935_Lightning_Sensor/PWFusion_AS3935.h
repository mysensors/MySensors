/***************************************************************************
* File Name: PWFusion_AS3935.h
* Processor/Platform: Arduino Uno R3 (tested)
* Development Environment: Arduino 1.0.5
*
* Designed for use with with Playing With Fusion AS3935 Lightning Sensor
* Breakout: SEN-39001. 
*
*   SEN-39001 (universal applications)
*   ---> http://www.playingwithfusion.com/productview.php?pdid=22
*
* Copyright � 2014 Playing With Fusion, Inc.
* SOFTWARE LICENSE AGREEMENT: This code is released under the MIT License.
*
* Permission is hereby granted, free of charge, to any person obtaining a
* copy of this software and associated documentation files (the "Software"),
* to deal in the Software without restriction, including without limitation
* the rights to use, copy, modify, merge, publish, distribute, sublicense,
* and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
* FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
* DEALINGS IN THE SOFTWARE.
* **************************************************************************
* REVISION HISTORY:
* Author		Date		Comments
* J. Steinlage	2014Aug20	Original version
* 
* Playing With Fusion, Inc. invests time and resources developing open-source
* code. Please support Playing With Fusion and continued open-source 
* development by buying products from Playing With Fusion!
*
* **************************************************************************
* ADDITIONAL NOTES:
* This file contains functions to interface with the AS3935 Franklin 
* Lightning Sensor manufactured by AMS. Originally designed for application
* on the Arduino Uno platform.
**************************************************************************/
#ifndef PWF_AS3935_h
#define PWF_AS3935_h

#include "Arduino.h"
#include "avr/pgmspace.h"
#include "util/delay.h"
#include "stdlib.h"
#include <SPI.h>

class PWF_AS3935
{
 public:
	PWF_AS3935(int8_t CSx, int8_t IRQx, int8_t SIx);
	void AS3935_ManualCal(uint8_t capacitance, uint8_t location, uint8_t disturber);
	void AS3935_DefInit(void);
	void AS3935_PowerUp(void);
	void AS3935_PowerDown(void);
	void AS3935_DisturberEn(void);
	void AS3935_DisturberDis(void);
	void AS3935_SetIRQ_Output_Source(uint8_t irq_select);
	void AS3935_SetTuningCaps(uint8_t cap_val);
	uint8_t AS3935_GetInterruptSrc(void);
	uint8_t AS3935_GetLightningDistKm(void);
	uint32_t AS3935_GetStrikeEnergyRaw(void);
	uint8_t AS3935_SetMinStrikes(uint8_t min_strk);
	void AS3935_ClearStatistics(void);
	void AS3935_SetIndoors(void);
	void AS3935_SetOutdoors(void);
	uint8_t AS3935_GetNoiseFloorLvl(void);
	void AS3935_SetNoiseFloorLvl(uint8_t nf_sel);
	uint8_t AS3935_GetWatchdogThreshold(void);
	void AS3935_SetWatchdogThreshold(uint8_t wdth);
	uint8_t AS3935_GetSpikeRejection(void);
	void AS3935_SetSpikeRejection(uint8_t srej);
	void AS3935_SetLCO_FDIV(uint8_t fdiv);
	void AS3935_PrintAllRegs(void);
	
 private:
	int8_t _cs, _irq, _si;
	uint8_t _sing_reg_read(uint8_t RegAdd);
	void _sing_reg_write(uint8_t RegAdd, uint8_t DataMask, uint8_t RegData);
	void _AS3935_Reset(void);
	void _CalRCO(void);
};

#endif




