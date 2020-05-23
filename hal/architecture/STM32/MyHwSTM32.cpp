/*
 * The MySensors Arduino library handles the wireless radio link and protocol
 * between your home built sensors/actuators and HA controller of choice.
 * The sensors forms a self healing radio network with optional repeaters. Each
 * repeater and gateway builds a routing tables in EEPROM which keeps track of the
 * network topology allowing messages to be routed to nodes.
 *
 * Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
 * Copyright (C) 2013-2020 Sensnology AB
 * Full contributor list: https://github.com/mysensors/MySensors/graphs/contributors
 *
 * Documentation: http://www.mysensors.org
 * Support Forum: http://forum.mysensors.org
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 */

#include "MyHwSTM32.h"

/*
* Pinout STM32F103C8 dev board:
* http://wiki.stm32duino.com/images/a/ae/Bluepillpinout.gif
*
* Wiring 
* --------------------------------------------------
RFM69 	CLK		MISO		MOSI	CSN		CE		IRQ
SPI1		PA5		PA6			PA7		PA4		NA		PA3 (default)

RF24 		CLK		MISO		MOSI	CSN		CE		IRQ
SPI1		PA5		PA6			PA7		PA4		PB0		NA

*/

bool hwInit(void)
{
#if !defined(MY_DISABLED_SERIAL)
	MY_SERIALDEVICE.begin(MY_BAUD_RATE);
	#if defined(MY_GATEWAY_SERIAL)
	while (!MY_SERIALDEVICE) {}
#endif
#endif
	//todo EEPROM initialisation????
		Serial1.print("EEPROM.length ");
		Serial1.println(EEPROM.length());

	//if (EEPROM.init() == EEPROM_OK) {
	//	uint16 cnt;
	//	EEPROM.count(&cnt);
	//	if(cnt>=EEPROM.maxcount()) {
	//		// tmp, WIP: format eeprom if full
	//		EEPROM.format();
	//	}
		return true;
	//}
	//return false;
}

void hwReadConfigBlock(void *buf, void *addr, size_t length)
{
	uint8_t *dst = static_cast<uint8_t *>(buf);
	int offs = reinterpret_cast<int>(addr);
	while (length-- > 0) {
		*dst++ = EEPROM.read(offs++);
	}
}

void hwWriteConfigBlock(void *buf, void *addr, size_t length)
{
	uint8_t *src = static_cast<uint8_t *>(buf);
	int offs = reinterpret_cast<int>(addr);
	while (length-- > 0) {
		EEPROM.write(offs++, *src++);
	}
//	EEPROM.commit();
}

uint8_t hwReadConfig(const int addr)
{
	uint8_t value;
	hwReadConfigBlock(&value, reinterpret_cast<void *>(addr), 1);
	return value;
}

void hwWriteConfig(const int addr, uint8_t value)
{
	if (hwReadConfig(addr) != value) {
		hwWriteConfigBlock(&value, reinterpret_cast<void *>(addr), 1);
	}
}

int8_t hwSleep(uint32_t ms)
{
	// TODO: Not supported!
	(void)ms;
	return MY_SLEEP_NOT_POSSIBLE;
}

int8_t hwSleep(const uint8_t interrupt, const uint8_t mode, uint32_t ms)
{
	// TODO: Not supported!
	(void)interrupt;
	(void)mode;
	(void)ms;
	return MY_SLEEP_NOT_POSSIBLE;
}

int8_t hwSleep(const uint8_t interrupt1, const uint8_t mode1, const uint8_t interrupt2,
               const uint8_t mode2,
               uint32_t ms)
{
	// TODO: Not supported!
	(void)interrupt1;
	(void)mode1;
	(void)interrupt2;
	(void)mode2;
	(void)ms;
	return MY_SLEEP_NOT_POSSIBLE;
}


//void hwRandomNumberInit(void)
//{
//	// This function initializes the random number generator with a seed
//	// of 32 bits.  This method is good enough to earn FIPS 140-2 conform
//	// random data. This should reach to generate 32 Bit for randomSeed().
//	uint32_t seed = 0;
//	uint32_t timeout = millis() + 20;
//
//	// Trigger floating effect of an unconnected pin
//	pinMode(MY_SIGNING_SOFT_RANDOMSEED_PIN, INPUT_PULLUP);
//	pinMode(MY_SIGNING_SOFT_RANDOMSEED_PIN, INPUT);
//	delay(10);
//
//	// Generate 32 bits of datas
//	for (uint8_t i=0; i<32; i++) {
//		const int pinValue = analogRead(MY_SIGNING_SOFT_RANDOMSEED_PIN);
//		// Wait until the analog value has changed
//		while ((pinValue == analogRead(MY_SIGNING_SOFT_RANDOMSEED_PIN)) && (timeout>=millis())) {
//			seed ^= (millis() << i);
//			// Check of data generation is slow
//			if (timeout<=millis()) {
//				// Trigger pin again
//				pinMode(MY_SIGNING_SOFT_RANDOMSEED_PIN, INPUT_PULLUP);
//				pinMode(MY_SIGNING_SOFT_RANDOMSEED_PIN, INPUT);
//				// Pause a short while
//				delay(seed % 10);
//				timeout = millis() + 20;
//			}
//		}
//	}
//	randomSeed(seed);
//
////	// use internal temperature sensor as noise source
////	adc_reg_map *regs = ADC1->regs;
////	regs->CR2 |= ADC_CR2_TSVREFE;
////	regs->SMPR1 |= ADC_SMPR1_SMP16;
////
////	uint32_t seed = 0;
////	uint16_t currentValue = 0;
////	uint16_t newValue = 0;
////
////	for (uint8_t i = 0; i < 32; i++) {
////		const uint32_t timeout = hwMillis() + 20;
////		while (timeout >= hwMillis()) {
////			newValue = adc_read(ADC1, 16);
////			if (newValue != currentValue) {
////				currentValue = newValue;
////				break;
////			}
////		}
////		seed ^= ( (newValue + hwMillis()) & 7) << i;
////	}
////	randomSeed(seed);
////	regs->CR2 &= ~ADC_CR2_TSVREFE; // disable VREFINT and temp sensor
//}

bool hwUniqueID(unique_id_t *uniqueID)
{
	(void)memcpy((uint8_t *)uniqueID, (uint32_t *)0x1FFFF7E0, 16); // FlashID + ChipID
	return true;
}

uint16_t hwCPUVoltage(void)
{
	//Not yet implemented
	return FUNCTION_NOT_SUPPORTED;

	//adc_reg_map *regs = ADC1->regs;
	//regs->CR2 |= ADC_CR2_TSVREFE; // enable VREFINT and temp sensor
	//regs->SMPR1 =  ADC_SMPR1_SMP17; // sample rate for VREFINT ADC channel
	//adc_calibrate(ADC1);
//
	//const uint16_t vdd = adc_read(ADC1, 17);
	//regs->CR2 &= ~ADC_CR2_TSVREFE; // disable VREFINT and temp sensor
	//return (uint16_t)(1200u * 4096u / vdd);
}

uint16_t hwCPUFrequency(void)
{
	return F_CPU/100000UL;
}
//https://github.com/stm32duino/Arduino_Core_STM32/issues/5#issuecomment-411868042
int8_t hwCPUTemperature(void)
{
		//STM32ADC adc(ADC1);

		//adc.begin(PB1);

	//  adc.startConversion();
		
    //uint32_t v = adc.read();
    //uint32_t vcc = adc.measureVcc();
    //int16_t temp = adc.measureTemp();
    //printf("ADC: %lu, vcc: %lumV temp: %dC\n", v, vcc, temp);
    //delay(1000);

//		return adc.measureTemp();

	//adc_reg_map *regs = ADC1->regs;
	//regs->CR2 |= ADC_CR2_TSVREFE; // enable VREFINT and Temperature sensor
	//regs->SMPR1 |= ADC_SMPR1_SMP16 | ADC_SMPR1_SMP17;
	//adc_calibrate(ADC1);
//
	////const uint16_t adc_temp = adc_read(ADC1, 16);
	////const uint16_t vref = 1200 * 4096 / adc_read(ADC1, 17);
	//// calibrated at 25°C, ADC output = 1430mV, avg slope = 4.3mV / °C, increasing temp ~ lower voltage
	//const int8_t temp = static_cast<int8_t>((1430.0 - (adc_read(ADC1, 16) * 1200 / adc_read(ADC1,
	//                                        17))) / 4.3 + 25.0);
	//regs->CR2 &= ~ADC_CR2_TSVREFE; // disable VREFINT and temp sensor
	//return (temp - MY_STM32F1_TEMPERATURE_OFFSET) / MY_STM32F1_TEMPERATURE_GAIN;
	return FUNCTION_NOT_SUPPORTED;

}

uint16_t hwFreeMem(void)
{
	//Not yet implemented
	return FUNCTION_NOT_SUPPORTED;
}


void hwWatchdogReset(void)
{
	IWatchdog.reload();
	//NRF_WDT->RR[0] = WDT_RR_RR_Reload;
}

void hwReboot(void)
{
	NVIC_SystemReset();
	while (true)
		;
}
