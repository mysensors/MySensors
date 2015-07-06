/*
 Arduino Tipping Bucket Rain Gauge

 April 26, 2015

 Version 1.4.1 alpha

 Arduino Tipping Bucket Rain Gauge

 Utilizing a tipping bucket sensor, your Vera home automation controller and the MySensors.org
 gateway you can measure and sense local rain.  This sketch will create two devices on your
 Vera controller.  One will display your total precipitation for the last 5 days.  The other, 
 a sensor that changes state if there is recent rain (up to last 120 hours)  above a threshold.  
 Both these settings are user definable.

 There is a build overview video here: https://youtu.be/1eMfKQaLROo

 This sketch features the following:

 * Allows you to set the rain threshold in mm
 * Allows you to determine the tripped indicator window up to 120 hours.
 * Displays the last 5 days of rain in Variable1 through Variable5
   of the Rain Sensor device
 * Configuration changes to Sensor device updated every hour
 * Should run on any Arduino
 * Will retain Tripped/Not Tripped status and data in a power interruption, saving small amount
   of data to EEPROM (Circular Buffer to maximize life of EEPROM)
 * LED status indicator
 * Optional Temp/Humidity (DHT-22 or DHT-11) and Light LUX (BH1750) sensors. To use, uncomment
   #define DHT_ON  and/or #define LUX_ON
 * Optionally send total accumulation of each day's rainfall or send only individual days rainfall totals.
   Comment out #define USE_DAILY to display individual daily rainfall.

 by @BulldogLowell and @PeteWill for free public use

 */
#include <SPI.h>
#include <MySensor.h>
#include <math.h>
#include <Time.h>

#define NODE_ID 24 //or AUTO to let controller assign
#define SKETCH_NAME "Rain Gauge"
#define SKETCH_VERSION "1.4.1a"

#define DWELL_TIME 125  // this allows for radio to come back to power after a transmission, ideally 0 

//#define DEBUG_ON  // comment out this line to disable serial debug
#define DHT_ON // uncomment out this line to enable DHT sensor
#define LUX_ON // uncomment out this line to enable BH1750 sensor
//#define USE_DAILY // displays each time segment as an accumulation of prior periods inclusive.  Comment out to display individual daily rainfall totals in the variables sent to your controller.

#define CALIBRATE_FACTOR 60 // amount of rain per rain bucket tip e.g. 5 is .05mm
#define DHT_LUX_DELAY 300000  //Delay in milliseconds that the DHT and LUX sensors will wait before sending data

#define CHILD_ID_RAIN_LOG 3  // Keeps track of accumulated rainfall
#define CHILD_ID_TRIPPED_INDICATOR 4  // Indicates Tripped when rain detected
#define EEPROM_BUFFER_LOCATION 0  // location of the EEPROM circular buffer
#define E_BUFFER_LENGTH 240
#define RAIN_BUCKET_SIZE 120

#ifdef  DEBUG_ON
  #define DEBUG_PRINT(x)   Serial.print(x)
  #define DEBUG_PRINTLN(x) Serial.println(x)
  #define SERIAL_START(x)  Serial.begin(x)
  #else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
  #define SERIAL_START(x)
#endif
//
MySensor gw;
//
MyMessage msgRainRate(CHILD_ID_RAIN_LOG, V_RAINRATE);
MyMessage msgRain(CHILD_ID_RAIN_LOG, V_RAIN);
//
MyMessage msgRainVAR1(CHILD_ID_RAIN_LOG, V_VAR1);
MyMessage msgRainVAR2(CHILD_ID_RAIN_LOG, V_VAR2);
MyMessage msgRainVAR3(CHILD_ID_RAIN_LOG, V_VAR3);
MyMessage msgRainVAR4(CHILD_ID_RAIN_LOG, V_VAR4);
MyMessage msgRainVAR5(CHILD_ID_RAIN_LOG, V_VAR5);
//
MyMessage msgTripped(CHILD_ID_TRIPPED_INDICATOR, V_TRIPPED);
MyMessage msgTrippedVar1(CHILD_ID_TRIPPED_INDICATOR, V_VAR1);
MyMessage msgTrippedVar2(CHILD_ID_TRIPPED_INDICATOR, V_VAR2);
//
#ifdef DHT_ON
  #include <DHT.h>
  #define CHILD_ID_HUM 0
  #define CHILD_ID_TEMP 1
  #define HUMIDITY_SENSOR_DIGITAL_PIN 8
  DHT dht;
  float lastTemp;
  float lastHum;
  boolean metric = true;
  MyMessage msgHum(CHILD_ID_HUM, V_HUM);
  MyMessage msgTemp(CHILD_ID_TEMP, V_TEMP);
#endif
//
#ifdef LUX_ON
  //BH1750 is connected to SCL (analog input A5) and SDA (analog input A4)
  #include <BH1750.h>
  #include <Wire.h>
  #define CHILD_ID_LIGHT 2
  BH1750 lightSensor;
  MyMessage msg(CHILD_ID_LIGHT, V_LIGHT_LEVEL);
  unsigned int lastlux;
  byte heartbeat = 10; //Used to send the light lux to gateway as soon as the device is restarted and after the DHT_LUX_DELAY has happened 10 times
#endif
unsigned long sensorPreviousMillis;
int eepromIndex;
int tipSensorPin = 3; // Pin the tipping bucket is connected to. Must be interrupt capable pin
int ledPin = 5; // Pin the LED is connected to.  PWM capable pin required
unsigned long dataMillis;
unsigned long serialInterval = 600000UL;
const unsigned long oneHour = 3600000UL;
unsigned long lastTipTime;
unsigned long lastRainTime; //Used for rainRate calculation
unsigned int rainBucket [RAIN_BUCKET_SIZE] ; /* 24 hours x 5 Days = 120 hours */
unsigned int rainRate = 0;
byte rainWindow = 72;         //default rain window in hours.  Will be overwritten with msgTrippedVar1.
volatile int wasTippedBuffer = 0;
int rainSensorThreshold = 50; //default rain sensor sensitivity in hundredths.  Will be overwritten with msgTrippedVar2.
byte state = 0;
byte oldState = -1;
int lastRainRate = 0;
int lastMeasure = 0;
boolean gotTime = false;
byte lastHour;
byte currentHour;
//
void setup()
{
  SERIAL_START(115200);
  //
  // Set up the IO
  pinMode(tipSensorPin, INPUT_PULLUP);
  attachInterrupt (1, sensorTipped, FALLING);  // depending on location of the hall effect sensor may need CHANGE
  pinMode(ledPin, OUTPUT);
  digitalWrite(ledPin, HIGH);
  //
  //Let's get the controller talking to the Arduino
  gw.begin(getVariables, NODE_ID);
  gw.sendSketchInfo(SKETCH_NAME, SKETCH_VERSION);
  gw.wait(DWELL_TIME);
  gw.present(CHILD_ID_RAIN_LOG, S_RAIN);
  gw.wait(DWELL_TIME);
  gw.present(CHILD_ID_TRIPPED_INDICATOR, S_MOTION);
  gw.wait(DWELL_TIME);
  DEBUG_PRINTLN(F("Sensor Presentation Complete"));
  //
  //Sync time with the server, this will be called hourly in order to keep time from creeping with the crystal
  //
  unsigned long functionTimeout = millis();
  while (timeStatus() == timeNotSet && millis() - functionTimeout < 30000UL)
  {
    gw.process();
    gw.requestTime(receiveTime);
    DEBUG_PRINTLN(F("Getting Time"));
    gw.wait(1000); // call once per second
    DEBUG_PRINTLN(F("."));
  }
  currentHour = hour();
  lastHour = hour();
  //
  //retrieve from EEPROM stored values on a power cycle.
  //
  boolean isDataOnEeprom = false;
  for (int i = 0; i < E_BUFFER_LENGTH; i++)
  {
    byte locator = gw.loadState(EEPROM_BUFFER_LOCATION + i);
    if (locator == 0xFE)  // found the EEPROM circular buffer index
    {
      eepromIndex = EEPROM_BUFFER_LOCATION + i;
      DEBUG_PRINT(F("EEPROM Index "));
      DEBUG_PRINTLN(eepromIndex);
      //Now that we have the buffer index let's populate the rainBucket[] with data from eeprom
      loadRainArray(eepromIndex);
      isDataOnEeprom = true;
      break;
    }
  }
  //
  if (!isDataOnEeprom) // Added for the first time it is run on a new Arduino
  {
    DEBUG_PRINTLN(F("I didn't find valid EEPROM Index, so I'm writing one to location 0"));
    eepromIndex = EEPROM_BUFFER_LOCATION;
    gw.saveState(eepromIndex, 0xFE);
    gw.saveState(eepromIndex + 1, 0xFE);
    //then I will clear out any bad data
    for (int i = 2; i <= E_BUFFER_LENGTH; i++)
    {
      gw.saveState(i, 0x00);
    }
  }
  dataMillis = millis();
  lastTipTime = millis() - oneHour; //why is this -oneHour?? Doesn't millis() start at 0 when first powered on?
  //
  gw.request(CHILD_ID_TRIPPED_INDICATOR, V_VAR1);
  gw.wait(DWELL_TIME);
  gw.request(CHILD_ID_TRIPPED_INDICATOR, V_VAR2);
  gw.wait(DWELL_TIME);
  //
#ifdef DHT_ON
  dht.setup(HUMIDITY_SENSOR_DIGITAL_PIN);
  // Register all sensors to gw (they will be created as child devices)
  gw.present(CHILD_ID_HUM, S_HUM);
  gw.wait(DWELL_TIME);
  gw.present(CHILD_ID_TEMP, S_TEMP);
  gw.wait(DWELL_TIME);
  metric = gw.getConfig().isMetric;
  gw.wait(DWELL_TIME);
#endif
  //
#ifdef LUX_ON
  gw.present(CHILD_ID_LIGHT, S_LIGHT_LEVEL);
  gw.wait(DWELL_TIME);
  lightSensor.begin();
#endif
  //
  DEBUG_PRINTLN(F("Radio Setup Complete!"));
  transmitRainData();
}

void loop()
{
  gw.process();
  if (state)
  {
    prettyFade();  // breathe if tripped
  }
  else
  {
    slowFlash();   // blink if not tripped
  }
#ifdef DEBUG_ON  // Serial Debug Block
  if ( (millis() - dataMillis) >= serialInterval)
  {
    for (int i = 24; i <= 120; i = i + 24)
    {
      updateSerialData(i);
    }
    dataMillis = millis();
  }
#endif
  //
  // let's constantly check to see if the rain in the past rainWindow hours is greater than rainSensorThreshold
  //
  int measure = 0; // Check to see if we need to show sensor tripped in this block
  for (int i = 0; i < rainWindow; i++)
  {
    measure += rainBucket [i];
    if (measure != lastMeasure)
    {
      //      DEBUG_PRINT(F("measure value (total rainBucket within rainWindow): "));
      //      DEBUG_PRINTLN(measure);
      lastMeasure = measure;
    }
  }
  //
  state = (measure >= (rainSensorThreshold * 100));
  if (state != oldState)
  {
    gw.send(msgTripped.set(state));
    gw.wait(DWELL_TIME);
    DEBUG_PRINT(F("New Sensor State... Sensor: "));
    DEBUG_PRINTLN(state ? "Tripped" : "Not Tripped");
    oldState = state;
  }
  //
  unsigned long tipDelay = millis() - lastRainTime;
  if (wasTippedBuffer) // if was tipped, then update the 24hour total and transmit to Vera
  {
    DEBUG_PRINTLN(F("Sensor Tipped"));
    DEBUG_PRINT(F("rainBucket [0] value: "));
    DEBUG_PRINTLN(rainBucket [0]);
    gw.send(msgRain.set((float)rainTotal(currentHour) / 100, 1)); //Calculate the total rain for the day
    gw.wait(DWELL_TIME);
    wasTippedBuffer--;
    rainRate = ((oneHour) / tipDelay);
    if (rainRate != lastRainRate)
    {
      gw.send(msgRainRate.set(rainRate, 1));
      gw.wait(DWELL_TIME);
      DEBUG_PRINT(F("RainRate= "));
      DEBUG_PRINTLN(rainRate);
      lastRainRate = rainRate;
    }
    lastRainTime = lastTipTime;
  }
  //
  currentHour = hour();
  if (currentHour != lastHour)
  {
    DEBUG_PRINTLN(F("One hour elapsed."));
    gw.send(msgRain.set((float)rainTotal(currentHour) / 100, 1)); // send today's rainfall
    gw.wait(DWELL_TIME);
    gw.saveState(eepromIndex, highByte(rainBucket[0]));
    gw.saveState(eepromIndex + 1, lowByte(rainBucket[0]));
    DEBUG_PRINT(F("Saving rainBucket[0] to eeprom. rainBucket[0] = "));
    DEBUG_PRINTLN(rainBucket[0]);
    for (int i = RAIN_BUCKET_SIZE - 1; i >= 0; i--)//cascade an hour of values back into the array
    {
      rainBucket [i + 1] = rainBucket [i];
    }
    gw.request(CHILD_ID_TRIPPED_INDICATOR, V_VAR1);
    gw.wait(DWELL_TIME);
    gw.request(CHILD_ID_TRIPPED_INDICATOR, V_VAR2);
    gw.wait(DWELL_TIME);
    rainBucket[0] = 0;
    eepromIndex = eepromIndex + 2;
    if (eepromIndex > EEPROM_BUFFER_LOCATION + E_BUFFER_LENGTH)
    {
      eepromIndex = EEPROM_BUFFER_LOCATION;
    }
    DEBUG_PRINT(F("Writing to EEPROM.  Index: "));
    DEBUG_PRINTLN(eepromIndex);
    gw.saveState(eepromIndex, 0xFE);
    gw.saveState(eepromIndex + 1, 0xFE);
    gw.requestTime(receiveTime); // sync the time every hour
    gw.wait(DWELL_TIME);
    transmitRainData();
    rainRate = 0;
    gw.send(msgRainRate.set(rainRate, 1));
    gw.wait(DWELL_TIME);
    DEBUG_PRINTLN(F("Sending rainRate is 0 to controller"));
    lastHour = currentHour;
  }
  if (millis() - sensorPreviousMillis > DHT_LUX_DELAY)
  {
    #ifdef DHT_ON  //DHT Code
      doDHT();
    #endif
    #ifdef LUX_ON
      doLUX();
    #endif
    sensorPreviousMillis = millis();
  }
}
//
#ifdef DHT_ON
void doDHT(void)
{
  float temperature = dht.getTemperature();
    if (isnan(temperature)) 
    {
      DEBUG_PRINTLN(F("Failed reading temperature from DHT"));
    } else if (temperature != lastTemp) 
    {
      lastTemp = temperature;
      if (!metric) 
      {
        temperature = dht.toFahrenheit(temperature);
      }
      gw.send(msgTemp.set(temperature, 1));
      gw.wait(DWELL_TIME);
      DEBUG_PRINT(F("Temperature is: "));
      DEBUG_PRINTLN(temperature);
    }
    float humidity = dht.getHumidity();;
    if (isnan(humidity)) 
    {
      DEBUG_PRINTLN(F("Failed reading humidity from DHT"));
    } else if (humidity != lastHum) 
    {
      lastHum = humidity;
      gw.send(msgHum.set(humidity, 1));
      gw.wait(DWELL_TIME);
      DEBUG_PRINT(F("Humidity is: "));
      DEBUG_PRINTLN(humidity);
    }
}
#endif
//
#ifdef LUX_ON
void doLUX(void)
{
  unsigned int lux = lightSensor.readLightLevel();// Get Lux value
  DEBUG_PRINT(F("Current LUX Level: "));
  DEBUG_PRINTLN(lux);
  heartbeat++;
  if (lux != lastlux || heartbeat > 10) 
  {
    gw.send(msg.set(lux));
    lastlux = lux;
  }
  if (heartbeat > 10) 
  {
    heartbeat = 0;
  }
}
#endif
//
void sensorTipped()
{
  unsigned long thisTipTime = millis();
  if (thisTipTime - lastTipTime > 100UL)
  {
    rainBucket[0] += CALIBRATE_FACTOR; // adds CALIBRATE_FACTOR hundredths of unit each tip
    wasTippedBuffer++;
  }
  lastTipTime = thisTipTime;
}
//
int rainTotal(int hours)
{
  int total = 0;
  for ( int i = 0; i <= hours; i++)
  {
    total += rainBucket [i];
  }
  return total;
}

void updateSerialData(int x)
{
  DEBUG_PRINT(F("Rain last "));
  DEBUG_PRINT(x);
  DEBUG_PRINTLN(F(" hours: "));
  float tipCount = 0;
  for (int i = 0; i < x; i++)
  {
    tipCount = tipCount + rainBucket [i];
  }
  tipCount = tipCount / 100;
  DEBUG_PRINTLN(tipCount);
}

void loadRainArray(int value) // retrieve stored rain array from EEPROM on powerup
{
  for (int i = 0; i < RAIN_BUCKET_SIZE; i++)
  {
    value = value - 2;
    if (value < EEPROM_BUFFER_LOCATION)
    {
      value = EEPROM_BUFFER_LOCATION + E_BUFFER_LENGTH;
    }
    DEBUG_PRINT(F("EEPROM location: "));
    DEBUG_PRINTLN(value);
    byte rainValueHigh = gw.loadState(value);
    byte rainValueLow = gw.loadState(value + 1);
    unsigned int rainValue = rainValueHigh << 8;
    rainValue |= rainValueLow;
    rainBucket[i + 1] = rainValue;
    //
    DEBUG_PRINT(F("rainBucket[ value: "));
    DEBUG_PRINT(i + 1);
    DEBUG_PRINT(F("] value: "));
    DEBUG_PRINTLN(rainBucket[i + 1]);
  }
}

void transmitRainData(void)
{
  DEBUG_PRINT(F("In transmitRainData. currentHour = "));
  DEBUG_PRINTLN(currentHour);
  int rainUpdateTotal = 0;
  for (int i = currentHour; i >= 0; i--)
  {
    rainUpdateTotal += rainBucket[i];
    DEBUG_PRINT(F("Adding rainBucket["));
    DEBUG_PRINT(i);
    DEBUG_PRINTLN(F("] to rainUpdateTotal."));
  }
  DEBUG_PRINT(F("TX Day 1: rainUpdateTotal = "));
  DEBUG_PRINTLN((float)rainUpdateTotal / 100.0);
  gw.send(msgRainVAR1.set((float)rainUpdateTotal / 100.0, 1)); //Send current day rain totals (resets at midnight)
  gw.wait(DWELL_TIME);
#ifdef USE_DAILY
  rainUpdateTotal = 0;
#endif
  for (int i = currentHour + 24; i > currentHour; i--)
  {
    rainUpdateTotal += rainBucket[i];
    DEBUG_PRINT(F("Adding rainBucket["));
    DEBUG_PRINT(i);
    DEBUG_PRINTLN(F("] to rainUpdateTotal."));
  }
  DEBUG_PRINT(F("TX Day 2: rainUpdateTotal = "));
  DEBUG_PRINTLN((float)rainUpdateTotal / 100.0);
  gw.send(msgRainVAR2.set((float)rainUpdateTotal / 100.0, 1));
  gw.wait(DWELL_TIME);
#ifdef USE_DAILY
  rainUpdateTotal = 0;
#endif
  for (int i = currentHour + 48; i > currentHour + 24; i--)
  {
    rainUpdateTotal += rainBucket[i];
    DEBUG_PRINT(F("Adding rainBucket["));
    DEBUG_PRINT(i);
    DEBUG_PRINTLN(F("] to rainUpdateTotal."));
  }
  DEBUG_PRINT(F("TX Day 3: rainUpdateTotal = "));
  DEBUG_PRINTLN((float)rainUpdateTotal / 100.0);
  gw.send(msgRainVAR3.set((float)rainUpdateTotal / 100.0, 1));
  gw.wait(DWELL_TIME);
#ifdef USE_DAILY
  rainUpdateTotal = 0;
#endif
  for (int i = currentHour + 72; i > currentHour + 48; i--)
  {
    rainUpdateTotal += rainBucket[i];
    DEBUG_PRINT(F("Adding rainBucket["));
    DEBUG_PRINT(i);
    DEBUG_PRINTLN(F("] to rainUpdateTotal."));
  }
  DEBUG_PRINT(F("TX Day 4: rainUpdateTotal = "));
  DEBUG_PRINTLN((float)rainUpdateTotal / 100.0);
  gw.send(msgRainVAR4.set((float)rainUpdateTotal / 100.0, 1));
  gw.wait(DWELL_TIME);
#ifdef USE_DAILY
  rainUpdateTotal = 0;
#endif
  for (int i = currentHour + 96; i > currentHour + 72; i--)
  {
    rainUpdateTotal += rainBucket[i];
    DEBUG_PRINT(F("Adding rainBucket["));
    DEBUG_PRINT(i);
    DEBUG_PRINTLN(F("] to rainUpdateTotal."));
  }
  DEBUG_PRINT(F("TX Day 5: rainUpdateTotal = "));
  DEBUG_PRINTLN((float)rainUpdateTotal / 100.0);
  gw.send(msgRainVAR5.set((float)rainUpdateTotal / 100.0, 1));
  gw.wait(DWELL_TIME);
}

void getVariables(const MyMessage &message)
{
  if (message.sensor == CHILD_ID_RAIN_LOG)
  {
    // nothing to do here
  }
  else if (message.sensor == CHILD_ID_TRIPPED_INDICATOR)
  {
    if (message.type == V_VAR1)
    {
      rainWindow = atoi(message.data);
      if (rainWindow > 120)
      {
        rainWindow = 120;
      }
      else if (rainWindow < 1)
      {
        rainWindow = 1;
      }
      if (rainWindow != atoi(message.data))   // if I changed the value back inside the boundries, push that number back to Vera
      {
        gw.send(msgTrippedVar1.set(rainWindow));
      }
    }
    else if (message.type == V_VAR2)
    {
      rainSensorThreshold = atoi(message.data);
      if (rainSensorThreshold > 10000)
      {
        rainSensorThreshold = 10000;
      }
      else if (rainSensorThreshold < 1)
      {
        rainSensorThreshold = 1;
      }
      if (rainSensorThreshold != atoi(message.data))  // if I changed the value back inside the boundries, push that number back to Vera
      {
        gw.send(msgTrippedVar2.set(rainSensorThreshold));
      }
    }
  }
}

void prettyFade(void)
{
  float val = (exp(sin(millis() / 2000.0 * PI)) - 0.36787944) * 108.0;
  analogWrite(ledPin, val);
}

void slowFlash(void)
{
  static boolean ledState = true;
  static unsigned long pulseStart = millis();
  if (millis() - pulseStart < 100UL)
  {
    digitalWrite(ledPin, !ledState);
    pulseStart = millis();
  }
}

void receiveTime(unsigned long time)
{
  DEBUG_PRINTLN(F("Time received..."));
  setTime(time);
  char theTime[6];
  sprintf(theTime, "%d:%2d", hour(), minute());
  DEBUG_PRINTLN(theTime);
}
