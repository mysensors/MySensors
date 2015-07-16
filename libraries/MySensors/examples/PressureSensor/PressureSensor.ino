#include <SPI.h>
#include <MySensor.h>  
#include <Wire.h>
#include <Adafruit_BMP085.h>

#define BARO_CHILD 0
#define TEMP_CHILD 1

const float ALTITUDE = 688; // <-- adapt this value to your own location's altitude.

// Sleep time between reads (in seconds). Do not change this value as the forecast algorithm needs a sample every minute.
const unsigned long SLEEP_TIME = 60000; 

const char *weather[] = { "stable", "sunny", "cloudy", "unstable", "thunderstorm", "unknown" };
enum FORECAST
{
	STABLE = 0,			// "Stable Weather Pattern"
	SUNNY = 1,			// "Slowly rising Good Weather", "Clear/Sunny "
	CLOUDY = 2,			// "Slowly falling L-Pressure ", "Cloudy/Rain "
	UNSTABLE = 3,		// "Quickly rising H-Press",     "Not Stable"
	THUNDERSTORM = 4,	// "Quickly falling L-Press",    "Thunderstorm"
	UNKNOWN = 5			// "Unknown (More Time needed)
};

Adafruit_BMP085 bmp = Adafruit_BMP085();      // Digital Pressure Sensor 
MySensor gw;

float lastPressure = -1;
float lastTemp = -1;
int lastForecast = -1;

const int LAST_SAMPLES_COUNT = 5;
float lastPressureSamples[LAST_SAMPLES_COUNT];

// this CONVERSION_FACTOR is used to convert from Pa to kPa in forecast algorithm
// get kPa/h be dividing hPa by 10 
#define CONVERSION_FACTOR (1.0/10.0)

int minuteCount = 0;
bool firstRound = true;
// average value is used in forecast algorithm.
float pressureAvg;
// average after 2 hours is used as reference value for the next iteration.
float pressureAvg2;

float dP_dt;
boolean metric;
MyMessage tempMsg(TEMP_CHILD, V_TEMP);
MyMessage pressureMsg(BARO_CHILD, V_PRESSURE);
MyMessage forecastMsg(BARO_CHILD, V_FORECAST);


void setup() 
{
	gw.begin();

	// Send the sketch version information to the gateway and Controller
	gw.sendSketchInfo("Pressure Sensor", "1.1");

	if (!bmp.begin()) 
	{
		Serial.println("Could not find a valid BMP085 sensor, check wiring!");
		while (1) {}
	}

	// Register sensors to gw (they will be created as child devices)
	gw.present(BARO_CHILD, S_BARO);
	gw.present(TEMP_CHILD, S_TEMP);
	metric = gw.getConfig().isMetric;
}

void loop() 
{
	float pressure = bmp.readSealevelPressure(ALTITUDE) / 100.0;
	float temperature = bmp.readTemperature();

	if (!metric) 
	{
		// Convert to fahrenheit
		temperature = temperature * 9.0 / 5.0 + 32.0;
	}

	int forecast = sample(pressure);

	Serial.print("Temperature = ");
	Serial.print(temperature);
	Serial.println(metric ? " *C" : " *F");
	Serial.print("Pressure = ");
	Serial.print(pressure);
	Serial.println(" hPa");
	Serial.print("Forecast = ");
	Serial.println(weather[forecast]);


	if (temperature != lastTemp) 
	{
		gw.send(tempMsg.set(temperature, 1));
		lastTemp = temperature;
	}

	if (pressure != lastPressure) 
	{
		gw.send(pressureMsg.set(pressure, 0));
		lastPressure = pressure;
	}

	if (forecast != lastForecast)
	{
		gw.send(forecastMsg.set(weather[forecast]));
		lastForecast = forecast;
	}

	gw.sleep(SLEEP_TIME);
}

float getLastPressureSamplesAverage()
{
	float lastPressureSamplesAverage = 0;
	for (int i = 0; i < LAST_SAMPLES_COUNT; i++)
	{
		lastPressureSamplesAverage += lastPressureSamples[i];
	}
	lastPressureSamplesAverage /= LAST_SAMPLES_COUNT;

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

	// uncomment when debugging
	//Serial.print(F("Forecast at minute "));
	//Serial.print(minuteCount);
	//Serial.print(F(" dP/dt = "));
	//Serial.print(dP_dt);
	//Serial.print(F("kPa/h --> "));
	//Serial.println(weather[forecast]);

	return forecast;
}
