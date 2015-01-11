#include <SPI.h>
#include <MySensor.h>  
#include <Wire.h>
#include <Adafruit_BMP085.h>

#define BARO_CHILD 0
#define TEMP_CHILD 1

unsigned long SLEEP_TIME = 60000; // Sleep time between reads (in seconds)

Adafruit_BMP085 bmp = Adafruit_BMP085();      // Digital Pressure Sensor 
MySensor gw;

float lastPressure = -1;
float lastTemp = -1;
int lastForecast = -1;
const char *weather[] = {"stable","sunny","cloudy","unstable","thunderstorm","unknown"};
int minutes;
float pressureSamples[7][6];
int minuteCount = 0;
bool firstRound = true;
float pressureAvg[7];
float dP_dt;
boolean metric; 
MyMessage tempMsg(TEMP_CHILD, V_TEMP);
MyMessage pressureMsg(BARO_CHILD, V_PRESSURE);
MyMessage forecastMsg(BARO_CHILD, V_FORECAST);

void setup() {
  gw.begin();

  // Send the sketch version information to the gateway and Controller
  gw.sendSketchInfo("Pressure Sensor", "1.0");

  if (!bmp.begin()) {
    Serial.println("Could not find a valid BMP085 sensor, check wiring!");
    while (1) { }
  }

  // Register sensors to gw (they will be created as child devices)
  gw.present(BARO_CHILD, S_BARO);
  gw.present(TEMP_CHILD, S_TEMP);
  metric =  gw.getConfig().isMetric;
}

void loop() {
  float pressure = bmp.readSealevelPressure(205)/100; // 205 meters above sealevel
  float temperature = bmp.readTemperature();
  
  if (!metric) {
    // Convert to fahrenheit
    temperature = temperature * 9.0 / 5.0 + 32.0;
  }
  
  int forecast = sample(pressure);

  Serial.print("Temperature = ");
  Serial.print(temperature);
  Serial.println(metric?" *C":" *F");
  Serial.print("Pressure = ");
  Serial.print(pressure);
  Serial.println(" Pa");
  Serial.println(weather[forecast]);


  if (temperature != lastTemp) {
    gw.send(tempMsg.set(temperature,1));
    lastTemp = temperature;
  }

  if (pressure != lastPressure) {
    gw.send(pressureMsg.set(pressure, 0));
    lastPressure = pressure;
  }

  if (forecast != lastForecast) {
    gw.send(forecastMsg.set(weather[forecast]));
    lastForecast = forecast;
  }
  
  /*
   DP/Dt explanation

   0 = "Stable Weather Pattern"
   1 = "Slowly rising Good Weather", "Clear/Sunny "
   2 = "Slowly falling L-Pressure ", "Cloudy/Rain "
   3 = "Quickly rising H-Press",     "Not Stable"
   4 = "Quickly falling L-Press",    "Thunderstorm"
   5 = "Unknown (More Time needed) 
  */

  gw.sleep(SLEEP_TIME);
}

int sample(float pressure) {
	// Algorithm found here
	// http://www.freescale.com/files/sensors/doc/app_note/AN3914.pdf
	if (minuteCount == 180)
		minuteCount = 5;
        
        //From 0 to 5 min.
        if (minuteCount <= 5){
          pressureSamples[0][minuteCount] = pressure;
        }
        //From 30 to 35 min.
        if ((minuteCount <= 30) && (minuteCount >= 35)){
          pressureSamples[1][minuteCount - 30] = pressure;  
        }
        //From 55 to 60 min.
        if ((minuteCount <= 55) && (minuteCount >= 60)){
          pressureSamples[2][minuteCount - 55] = pressure;  
        }
        //From 90 to 95 min.
        if ((minuteCount <= 90) && (minuteCount >= 95)){
          pressureSamples[3][minuteCount - 90] = pressure;  
        }
        //From 115 to 119 min.
        if ((minuteCount <= 115) && (minuteCount >= 120)){
          pressureSamples[4][minuteCount - 115] = pressure;  
        }
        //From 150 to 155 min.
        if ((minuteCount <= 150) && (minuteCount >= 155)){
          pressureSamples[5][minuteCount - 150] = pressure;  
        }
        //From 175 to 180 min.
        if ((minuteCount <= 175) && (minuteCount >= 180)){
          pressureSamples[6][minuteCount - 175] = pressure;  
        }
        

	minuteCount++;

	if (minuteCount == 5) {
		// Avg pressure in first 5 min, value averaged from 0 to 5 min.
		pressureAvg[0] = ((pressureSamples[0][0] + pressureSamples[0][1] 
                                  + pressureSamples[0][2] + pressureSamples[0][3]
                                  + pressureSamples[0][4] + pressureSamples[0][5]) / 6);
	} else if (minuteCount == 35) {
		// Avg pressure in 30 min, value averaged from 0 to 5 min.
		pressureAvg[1] = ((pressureSamples[1][0] + pressureSamples[1][1] 
                                  + pressureSamples[1][2] + pressureSamples[1][3]
                                  + pressureSamples[1][4] + pressureSamples[1][5]) / 6);
		float change = (pressureAvg[1] - pressureAvg[0]);
		if (firstRound) // first time initial 3 hour
			dP_dt = ((65.0 / 1023.0) * 2 * change); // note this is for t = 0.5hour
		else
			dP_dt = (((65.0 / 1023.0) * change) / 1.5); // divide by 1.5 as this is the difference in time from 0 value.
	} else if (minuteCount == 60) {
		// Avg pressure at end of the hour, value averaged from 0 to 5 min.
		pressureAvg[2] = ((pressureSamples[2][0] + pressureSamples[2][1] 
                                  + pressureSamples[2][2] + pressureSamples[2][3]
                                  + pressureSamples[2][4] + pressureSamples[2][5]) / 6);
		float change = (pressureAvg[2] - pressureAvg[0]);
		if (firstRound) //first time initial 3 hour
			dP_dt = ((65.0 / 1023.0) * change); //note this is for t = 1 hour
		else
			dP_dt = (((65.0 / 1023.0) * change) / 2); //divide by 2 as this is the difference in time from 0 value
	} else if (minuteCount == 95) {
		// Avg pressure at end of the hour, value averaged from 0 to 5 min.
		pressureAvg[3] = ((pressureSamples[3][0] + pressureSamples[3][1] 
                                  + pressureSamples[3][2] + pressureSamples[3][3]
                                  + pressureSamples[3][4] + pressureSamples[3][5]) / 6);
		float change = (pressureAvg[3] - pressureAvg[0]);
		if (firstRound) // first time initial 3 hour
			dP_dt = (((65.0 / 1023.0) * change) / 1.5); // note this is for t = 1.5 hour
		else
			dP_dt = (((65.0 / 1023.0) * change) / 2.5); // divide by 2.5 as this is the difference in time from 0 value
	} else if (minuteCount == 120) {
		// Avg pressure at end of the hour, value averaged from 0 to 5 min.
		pressureAvg[4] = ((pressureSamples[4][0] + pressureSamples[4][1] 
                                  + pressureSamples[4][2] + pressureSamples[4][3]
                                  + pressureSamples[4][4] + pressureSamples[4][5]) / 6);
		float change = (pressureAvg[4] - pressureAvg[0]);
		if (firstRound) // first time initial 3 hour
			dP_dt = (((65.0 / 1023.0) * change) / 2); // note this is for t = 2 hour
		else
			dP_dt = (((65.0 / 1023.0) * change) / 3); // divide by 3 as this is the difference in time from 0 value
	} else if (minuteCount == 155) {
		// Avg pressure at end of the hour, value averaged from 0 to 5 min.
		pressureAvg[5] = ((pressureSamples[5][0] + pressureSamples[5][1] 
                                  + pressureSamples[5][2] + pressureSamples[5][3]
                                  + pressureSamples[5][4] + pressureSamples[5][5]) / 6);
		float change = (pressureAvg[5] - pressureAvg[0]);
		if (firstRound) // first time initial 3 hour
			dP_dt = (((65.0 / 1023.0) * change) / 2.5); // note this is for t = 2.5 hour
		else
			dP_dt = (((65.0 / 1023.0) * change) / 3.5); // divide by 3.5 as this is the difference in time from 0 value
	} else if (minuteCount == 180) {
		// Avg pressure at end of the hour, value averaged from 0 to 5 min.
		pressureAvg[6] = ((pressureSamples[6][0] + pressureSamples[6][1] 
                                  + pressureSamples[6][2] + pressureSamples[6][3]
                                  + pressureSamples[6][4] + pressureSamples[6][5]) / 6);
		float change = (pressureAvg[6] - pressureAvg[0]);
		if (firstRound) // first time initial 3 hour
			dP_dt = (((65.0 / 1023.0) * change) / 3); // note this is for t = 3 hour
		else
			dP_dt = (((65.0 / 1023.0) * change) / 4); // divide by 4 as this is the difference in time from 0 value
		pressureAvg[0] = pressureAvg[5]; // Equating the pressure at 0 to the pressure at 2 hour after 3 hours have past.
		firstRound = false; // flag to let you know that this is on the past 3 hour mark. Initialized to 0 outside main loop.
	}

	if (minuteCount < 35 && firstRound) //if time is less than 35 min on the first 3 hour interval.
		return 5; // Unknown, more time needed
	else if (dP_dt < (-0.25))
		return 4; // Quickly falling LP, Thunderstorm, not stable
	else if (dP_dt > 0.25)
		return 3; // Quickly rising HP, not stable weather
	else if ((dP_dt > (-0.25)) && (dP_dt < (-0.05)))
		return 2; // Slowly falling Low Pressure System, stable rainy weather
	else if ((dP_dt > 0.05) && (dP_dt < 0.25))
		return 1; // Slowly rising HP stable good weather
	else if ((dP_dt > (-0.05)) && (dP_dt < 0.05))
		return 0; // Stable weather
	else
		return 5; // Unknown
}



