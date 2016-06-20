/**
* The MySensors Arduino library handles the wireless radio link and protocol
* between your home built sensors/actuators and HA controller of choice.
* The sensors forms a self healing radio network with optional repeaters. Each
* repeater and gateway builds a routing tables in EEPROM which keeps track of the
* network topology allowing messages to be routed to nodes.
*
* Created by Henrik Ekblad <henrik.ekblad@mysensors.org>
* Copyright (C) 2013-2015 Sensnology AB
* Full contributor list: https://github.com/mysensors/Arduino/graphs/contributors
*
* Documentation: http://www.mysensors.org
* Support Forum: http://forum.mysensors.org
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* version 2 as published by the Free Software Foundation.
*
*******************************
*
* REVISION HISTORY
* Version 1.0 - Henrik Ekblad
*
* DESCRIPTION
* Weather station sensor example using BMP085 and DHT-11 or DHT-22 module
* http://www.mysensors.org/build/pressure
* http://www.mysensors.org/build/humidity
*
* WIRING
* DHT Sensor
* PIN2 D3 Arduino
* 10k resistor between PIN1 and PIN2
*
* BMP Sensor
* SCL A5 Arduino
* SDA A4 Arduino
*/

// Enable debug prints to serial monitor
//#define MY_DEBUG 

// Enable and select radio type attached
#define MY_RADIO_NRF24
//#define MY_RADIO_RFM69

#include <SPI.h>
#include <MySensors.h>

#include <Wire.h>
#include <Adafruit_BMP085.h>

#include <DHT.h>  

// Set this to true if you want to send values altough the values did not change.
// This is only recommended when not running on batteries.
const bool SEND_ALWAYS = true;

// Adapt this constant: set it to the altitude above sealevel at your home location.
const float SEALEVEL = 688; // meters above sealevel

// Constant for the world wide average pressure
const float SEALEVEL_PRESSURE = 1013.25;

// ----------------------------------------------------------------------------
// Child sensor ids
#define CHILD_ID_BARO 0
#define CHILD_ID_TEMP1 1
#define CHILD_ID_HUM 2
#define CHILD_ID_TEMP2 3

// ----------------------------------------------------------------------------
// DHT-11/22 
#define HUMIDITY_SENSOR_DIGITAL_PIN 3

DHT dht;
float lastTemp;
float lastHum;
boolean metric = true;
MyMessage msgHum(CHILD_ID_HUM, V_HUM);
MyMessage msgTemp(CHILD_ID_TEMP2, V_TEMP);

// ----------------------------------------------------------------------------
// BMP085
unsigned long SLEEP_TIME1 = 30000; // 1 minute required for forecast algorithm
unsigned long SLEEP_TIME2 = 30000; // 1 minute required for forecast algorithm

Adafruit_BMP085 bmp = Adafruit_BMP085();    // Digital Pressure Sensor 


/*
DP/Dt explanation
0 = "Stable Weather Pattern"
1 = "Slowly rising Good Weather", "Clear/Sunny "
2 = "Slowly falling L-Pressure ", "Cloudy/Rain "
3 = "Quickly rising H-Press",     "Not Stable"
4 = "Quickly falling L-Press",    "Thunderstorm"
5 = "Unknown (More Time needed)
*/

const char *weatherStrings[] = { "stable", "sunny", "cloudy", "unstable", "thunderstorm", "unknown" };
enum FORECAST
{
	STABLE = 0,			// Stable weather
	SUNNY = 1,			// Slowly rising HP stable good weather
	CLOUDY = 2,			// Slowly falling Low Pressure System, stable rainy weather
	UNSTABLE = 3,		// Quickly rising HP, not stable weather
	THUNDERSTORM = 4,	// Quickly falling LP, Thunderstorm, not stable
	UNKNOWN = 5			// Unknown, more time needed
};

const char *situationStrings[] = { "very low", "low", "normal", "high", "very high" };
enum WEATHER_SITUATION
{
	VERY_LOW_PRESSURE = 0,		// p > -7.5hPa
	LOW_PRESSURE = 1,			// p > -2.5hPa
	NORMAL_PRESSURE = 2,		// p < +/-2.5hPa	
	HIGH_PRESSURE = 3,			// p > +2.5hPa
	VERY_HIGH_PRESSURE = 4,		// p > +7.5hPa
};

float lastPressure = -1;
float lastPressureTemp = -1;
int lastForecast = -1;
int lastSituation = NORMAL_PRESSURE;

const int LAST_SAMPLES_COUNT = 5;
float lastPressureSamples[LAST_SAMPLES_COUNT];

// get kPa/h be dividing hPa by 10 
#define CONVERSION_FACTOR (1.0/10.0)

int minuteCount = 0;
bool firstRound = true;

// average value is used in forecast algorithm.
float pressureAvg;
// average after 2 hours is used as reference value for the next iteration.
float pressureAvg2;
float dP_dt;

MyMessage tempMsg(CHILD_ID_TEMP1, V_TEMP);
MyMessage pressureMsg(CHILD_ID_BARO, V_PRESSURE);
MyMessage forecastMsg(CHILD_ID_BARO, V_FORECAST);
MyMessage situationMsg(CHILD_ID_BARO, V_VAR1);
MyMessage forecastMsg2(CHILD_ID_BARO, V_VAR2);


void initPressureSensor()
{
	if (!bmp.begin())
	{
		Serial.println(F("Could not find a valid BMP085 sensor, check wiring!"));
		while (1) {}
	}
}

void initHumiditySensor()
{
	dht.setup(HUMIDITY_SENSOR_DIGITAL_PIN);
}

void setup()
{
	initPressureSensor();
	initHumiditySensor();
	metric = getConfig().isMetric;
}

void presentation()
{
	sendSketchInfo("Weather Station Sensor", "1.0");

	present(CHILD_ID_BARO, S_BARO);
	present(CHILD_ID_TEMP1, S_TEMP);
	present(CHILD_ID_HUM, S_HUM);
	present(CHILD_ID_TEMP2, S_TEMP);
}

int getWeatherSituation(float pressure)
{
	int situation = NORMAL_PRESSURE;

	float delta = pressure - SEALEVEL_PRESSURE;
	if (delta > 7.5)
	{
		situation = VERY_HIGH_PRESSURE;
	}
	else if (delta > 2.5)
	{
		situation = HIGH_PRESSURE;
	}
	else if (delta < -7.5)
	{
		situation = VERY_LOW_PRESSURE;
	}
	else if (delta < -2.5)
	{
		situation = LOW_PRESSURE;
	}
	else
	{
		situation = NORMAL_PRESSURE;
	}

	return situation;
}

// The BMP provides a temperature value, too!
bool updatePressureSensor()
{
	bool changed = false;

	//sealevel pressure p0 from absolute pressure.
	float pressure = bmp.readSealevelPressure(SEALEVEL) / 100.0;
	float temperature = bmp.readTemperature();
	if (!metric) 
	{
		// Convert to fahrenheit
		temperature = temperature * 9.0 / 5.0 + 32.0;
	}

	int forecast = sample(pressure);
	int situation = getWeatherSituation(pressure);


	if (SEND_ALWAYS || (temperature != lastPressureTemp))
	{
		changed = true;
		lastPressureTemp = temperature;
		#ifdef MY_DEBUG
		Serial.print(F("Temperature = "));
		Serial.print(temperature);
		Serial.println(metric ? F(" *C") : F(" *F"));
		#endif
		if (!send(tempMsg.set(lastPressureTemp, 1)))
		{
			lastPressureTemp = -1.0;
		}
	}

	if (SEND_ALWAYS || (pressure != lastPressure))
	{
		changed = true;
		lastPressure = pressure;
		#ifdef MY_DEBUG
		Serial.print(F("sealevel Pressure = "));
		Serial.print(pressure);
		Serial.println(F(" hPa"));
		#endif
		if (!send(pressureMsg.set(lastPressure, 1)))
		{
			lastPressure = -1.0;
		}
	}

	if (SEND_ALWAYS || (forecast != lastForecast))
	{
		changed = true;
		lastForecast = forecast;
		#ifdef MY_DEBUG
		Serial.print(F("Forecast = "));
		Serial.println(weatherStrings[forecast]);
		#endif
		if (send(forecastMsg.set(weatherStrings[lastForecast])))
		{
			if (!send(forecastMsg2.set(lastForecast)))
			{
			}
		}
		else
		{
			lastForecast = -1.0;
		}
	}

	if (SEND_ALWAYS || (situation != lastSituation))
	{
		changed = true;
		lastSituation = situation;
		#ifdef MY_DEBUG
		Serial.print(F("Situation = "));
		Serial.println(situationStrings[situation]);
		#endif
		if (!send(situationMsg.set(lastSituation, 0)))
		{
			lastSituation = -1.0;
		}
	}


	return changed;
}

// The dht provides a temperature, too!
bool updateHumiditySensor()
{
	bool changed = false;

	float temperature = dht.getTemperature();
	float humidity = dht.getHumidity();

	if (!isnan(temperature))
	{
		if (SEND_ALWAYS || (temperature != lastTemp))
		{
			lastTemp = temperature;
			if (!metric)
			{
				temperature = dht.toFahrenheit(temperature);
			}

			changed = true;
			#ifdef MY_DEBUG
			Serial.print(F("T: "));
			Serial.println(temperature);
			#endif
			if (!send(msgTemp.set(temperature, 1)))
			{
				lastTemp = -1.0;
			}
		}
	}
	else
	{
		Serial.println(F("Failed reading temperature from DHT"));
	}
	
	
	if (!isnan(humidity))
	{
		if (SEND_ALWAYS || (humidity != lastHum))
		{
			lastHum = humidity;
			changed = true;
			#ifdef MY_DEBUG
			Serial.print(F("H: "));
			Serial.println(humidity);
			#endif
			if (!send(msgHum.set(lastHum, 1)))
			{
				lastHum = -1.0;
			}
		}
	}
	else
	{
		Serial.println(F("Failed reading humidity from DHT"));
	}
	
	return changed;
}

void loop() 
{
	updatePressureSensor();
	sleep(SLEEP_TIME1);
	updateHumiditySensor();
	sleep(SLEEP_TIME2);
}


float getLastPressureSamplesAverage()
{
	float lastPressureSamplesAverage = 0;
	for (int i = 0; i < LAST_SAMPLES_COUNT; i++)
	{
		lastPressureSamplesAverage += lastPressureSamples[i];
	}

	lastPressureSamplesAverage /= LAST_SAMPLES_COUNT;

	// Uncomment when dubugging
	//Serial.print(F("### 5min-Average:"));
	//Serial.print(lastPressureSamplesAverage);
	//Serial.println(F(" hPa"));

	return lastPressureSamplesAverage;
}



// Algorithm found here
// http://www.freescale.com/files/sensors/doc/app_note/AN3914.pdf
// Pressure in hPa -->  forecast done by calculating kPa/h
int sample(float pressure)
{
	// Calculate the average of the last n minutes.
	int index = minuteCount % LAST_SAMPLES_COUNT;
	lastPressureSamples[index] = pressure;

	minuteCount++;
	if (minuteCount > 185)
	{
		minuteCount = 6;
	}

	if (minuteCount == 5)
	{
		pressureAvg = getLastPressureSamplesAverage();
	}
	else if (minuteCount == 35)
	{
		float lastPressureAvg = getLastPressureSamplesAverage();
		float change = (lastPressureAvg - pressureAvg) * CONVERSION_FACTOR;
		if (firstRound) // first time initial 3 hour
		{
			dP_dt = change * 2; // note this is for t = 0.5hour
		}
		else
		{
			dP_dt = change / 1.5; // divide by 1.5 as this is the difference in time from 0 value.
		}
	}
	else if (minuteCount == 65)
	{
		float lastPressureAvg = getLastPressureSamplesAverage();
		float change = (lastPressureAvg - pressureAvg) * CONVERSION_FACTOR;
		if (firstRound) //first time initial 3 hour
		{
			dP_dt = change; //note this is for t = 1 hour
		}
		else
		{
			dP_dt = change / 2; //divide by 2 as this is the difference in time from 0 value
		}
	}
	else if (minuteCount == 95)
	{
		float lastPressureAvg = getLastPressureSamplesAverage();
		float change = (lastPressureAvg - pressureAvg) * CONVERSION_FACTOR;
		if (firstRound) // first time initial 3 hour
		{
			dP_dt = change / 1.5; // note this is for t = 1.5 hour
		}
		else
		{
			dP_dt = change / 2.5; // divide by 2.5 as this is the difference in time from 0 value
		}
	}
	else if (minuteCount == 125)
	{
		float lastPressureAvg = getLastPressureSamplesAverage();
		pressureAvg2 = lastPressureAvg; // store for later use.
		float change = (lastPressureAvg - pressureAvg) * CONVERSION_FACTOR;
		if (firstRound) // first time initial 3 hour
		{
			dP_dt = change / 2; // note this is for t = 2 hour
		}
		else
		{
			dP_dt = change / 3; // divide by 3 as this is the difference in time from 0 value
		}
	}
	else if (minuteCount == 155)
	{
		float lastPressureAvg = getLastPressureSamplesAverage();
		float change = (lastPressureAvg - pressureAvg) * CONVERSION_FACTOR;
		if (firstRound) // first time initial 3 hour
		{
			dP_dt = change / 2.5; // note this is for t = 2.5 hour
		}
		else
		{
			dP_dt = change / 3.5; // divide by 3.5 as this is the difference in time from 0 value
		}
	}
	else if (minuteCount == 185)
	{
		float lastPressureAvg = getLastPressureSamplesAverage();
		float change = (lastPressureAvg - pressureAvg) * CONVERSION_FACTOR;
		if (firstRound) // first time initial 3 hour
		{
			dP_dt = change / 3; // note this is for t = 3 hour
		}
		else
		{
			dP_dt = change / 4; // divide by 4 as this is the difference in time from 0 value
		}
		pressureAvg = pressureAvg2; // Equating the pressure at 0 to the pressure at 2 hour after 3 hours have past.
		firstRound = false; // flag to let you know that this is on the past 3 hour mark. Initialized to 0 outside main loop.
	}

	int forecast = UNKNOWN;
	if (minuteCount < 35 && firstRound) //if time is less than 35 min on the first 3 hour interval.
	{
		forecast = UNKNOWN;
	}
	else if (dP_dt < (-0.25))
	{
		forecast = THUNDERSTORM;
	}
	else if (dP_dt > 0.25)
	{
		forecast = UNSTABLE;
	}
	else if ((dP_dt > (-0.25)) && (dP_dt < (-0.05)))
	{
		forecast = CLOUDY;
	}
	else if ((dP_dt > 0.05) && (dP_dt < 0.25))
	{
		forecast = SUNNY;
	}
	else if ((dP_dt >(-0.05)) && (dP_dt < 0.05))
	{
		forecast = STABLE;
	}
	else
	{
		forecast = UNKNOWN;
	}

	// Uncomment when dubugging
	// Serial.print(F("Forecast at minute "));
	// Serial.print(minuteCount);
	// Serial.print(F(" dP/dt = "));
	// Serial.print(dP_dt);
	// Serial.print(F("kPa/h --> "));	
	// Serial.println(weatherStrings[forecast]);

	return forecast;
}
