// Default sensor sketch for Sensebender Micro module
// Act as a temperature / humidity sensor by default.
//
// If A0 is held low while powering on, it will enter testmode, which verifies all on-board peripherals
// 
// Battery voltage is as battery percentage (Internal message), and optionally as a sensor value (See defines below)


#include <MySensor.h>
#include <Wire.h>
#include <SI7021.h>
#include <SPI.h>
#include <SPIFlash.h>
#include <EEPROM.h>  
#include <sha204_lib_return_codes.h>
#include <sha204_library.h>
#include <RunningAverage.h>
#include <avr/power.h>

// Define a static node address, remove if you want auto address assignment
//#define NODE_ADDRESS   3

// Uncomment the line below, to transmit battery voltage as a normal sensor value
//#define BATT_SENSOR    199

#define RELEASE "1.2"

#define AVERAGES 2

// Child sensor ID's
#define CHILD_ID_TEMP  1
#define CHILD_ID_HUM   2

// How many milli seconds between each measurement
#define MEASURE_INTERVAL 60000

// FORCE_TRANSMIT_INTERVAL, this number of times of wakeup, the sensor is forced to report all values to the controller
#define FORCE_TRANSMIT_INTERVAL 30 

// When MEASURE_INTERVAL is 60000 and FORCE_TRANSMIT_INTERVAL is 30, we force a transmission every 30 minutes.
// Between the forced transmissions a tranmission will only occur if the measured value differs from the previous measurement

// HUMI_TRANSMIT_THRESHOLD tells how much the humidity should have changed since last time it was transmitted. Likewise with
// TEMP_TRANSMIT_THRESHOLD for temperature threshold.
#define HUMI_TRANSMIT_THRESHOLD 0.5
#define TEMP_TRANSMIT_THRESHOLD 0.5

// Pin definitions
#define TEST_PIN       A0
#define LED_PIN        A2
#define ATSHA204_PIN   17 // A3

const int sha204Pin = ATSHA204_PIN;
atsha204Class sha204(sha204Pin);

SI7021 humiditySensor;
SPIFlash flash(8, 0x1F65);

MySensor gw;

// Sensor messages
MyMessage msgHum(CHILD_ID_HUM, V_HUM);
MyMessage msgTemp(CHILD_ID_TEMP, V_TEMP);

#ifdef BATT_SENSOR
MyMessage msgBatt(BATT_SENSOR, V_VOLTAGE);
#endif

// Global settings
int measureCount = 0;
int sendBattery = 0;
boolean isMetric = true;
boolean highfreq = true;

// Storage of old measurements
float lastTemperature = -100;
int lastHumidity = -100;
long lastBattery = -100;

RunningAverage raHum(AVERAGES);

/****************************************************
 *
 * Setup code 
 *
 ****************************************************/
void setup() {
  
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  Serial.begin(115200);
  Serial.print(F("Sensebender Micro FW "));
  Serial.print(RELEASE);
  Serial.flush();

  // First check if we should boot into test mode

  pinMode(TEST_PIN,INPUT);
  digitalWrite(TEST_PIN, HIGH); // Enable pullup
  if (!digitalRead(TEST_PIN)) testMode();

  // Make sure that ATSHA204 is not floating
  pinMode(ATSHA204_PIN, INPUT);
  digitalWrite(ATSHA204_PIN, HIGH);
  
  digitalWrite(TEST_PIN,LOW);
  digitalWrite(LED_PIN, HIGH); 

#ifdef NODE_ADDRESS
  gw.begin(NULL, NODE_ADDRESS, false);
#else
  gw.begin(NULL,AUTO,false);
#endif

  humiditySensor.begin();

  digitalWrite(LED_PIN, LOW);

  Serial.flush();
  Serial.println(F(" - Online!"));
  gw.sendSketchInfo("Sensebender Micro", RELEASE);
  
  gw.present(CHILD_ID_TEMP,S_TEMP);
  gw.present(CHILD_ID_HUM,S_HUM);
  
#ifdef BATT_SENSOR
  gw.present(BATT_SENSOR, S_POWER);
#endif

  
  isMetric = gw.getConfig().isMetric;
  Serial.print(F("isMetric: ")); Serial.println(isMetric);
  raHum.clear();
  sendTempHumidityMeasurements(false);
  sendBattLevel(false);
}


/***********************************************
 *
 *  Main loop function
 *
 ***********************************************/
void loop() {
  measureCount ++;
  sendBattery ++;
  bool forceTransmit = false;
  
  if ((measureCount == 5) && highfreq) 
  {
    clock_prescale_set(clock_div_8); // Switch to 1Mhz for the reminder of the sketch, save power.
    highfreq = false;
  } 
  
  if (measureCount > FORCE_TRANSMIT_INTERVAL) { // force a transmission
    forceTransmit = true; 
    measureCount = 0;
  }
    
  gw.process();

  sendTempHumidityMeasurements(forceTransmit);
  if (sendBattery > 60) 
  {
     sendBattLevel(forceTransmit); // Not needed to send battery info that often
     sendBattery = 0;
  }
  
  gw.sleep(MEASURE_INTERVAL);  
}


/*********************************************
 *
 * Sends temperature and humidity from Si7021 sensor
 *
 * Parameters
 * - force : Forces transmission of a value (even if it's the same as previous measurement)
 *
 *********************************************/
void sendTempHumidityMeasurements(bool force)
{
  bool tx = force;
  
  si7021_env data = humiditySensor.getHumidityAndTemperature();
  float oldAvgHum = raHum.getAverage();
  
  raHum.addValue(data.humidityPercent);
  
  float diffTemp = abs(lastTemperature - (isMetric ? data.celsiusHundredths : data.fahrenheitHundredths)/100);
  float diffHum = abs(oldAvgHum - raHum.getAverage());

  Serial.print(F("TempDiff :"));Serial.println(diffTemp);
  Serial.print(F("HumDiff  :"));Serial.println(diffHum); 

  if (isnan(diffHum)) tx = true; 
  if (diffTemp > TEMP_TRANSMIT_THRESHOLD) tx = true;
  if (diffHum >= HUMI_TRANSMIT_THRESHOLD) tx = true;

  if (tx) {
    measureCount = 0;
    float temperature = (isMetric ? data.celsiusHundredths : data.fahrenheitHundredths) / 100.0;
     
    int humidity = data.humidityPercent;
    Serial.print("T: ");Serial.println(temperature);
    Serial.print("H: ");Serial.println(humidity);
    
    gw.send(msgTemp.set(temperature,1));
    gw.send(msgHum.set(humidity));
    lastTemperature = temperature;
    lastHumidity = humidity;
  }
}

/********************************************
 *
 * Sends battery information (battery percentage)
 *
 * Parameters
 * - force : Forces transmission of a value
 *
 *******************************************/
void sendBattLevel(bool force)
{
  if (force) lastBattery = -1;
  long vcc = readVcc();
  if (vcc != lastBattery) {
    lastBattery = vcc;

#ifdef BATT_SENSOR
    gw.send(msgBatt.set(vcc));
#endif

    // Calculate percentage

    vcc = vcc - 1900; // subtract 1.9V from vcc, as this is the lowest voltage we will operate at
    
    long percent = vcc / 14.0;
    gw.sendBatteryLevel(percent);
  }
}

/*******************************************
 *
 * Internal battery ADC measuring 
 *
 *******************************************/
long readVcc() {
  // Read 1.1V reference against AVcc
  // set the reference to Vcc and the measurement to the internal 1.1V reference
  #if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
    ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  #elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
    ADMUX = _BV(MUX5) | _BV(MUX0);
  #elif defined (__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
    ADcdMUX = _BV(MUX3) | _BV(MUX2);
  #else
    ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  #endif  
 
  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Start conversion
  while (bit_is_set(ADCSRA,ADSC)); // measuring
 
  uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH  
  uint8_t high = ADCH; // unlocks both
 
  long result = (high<<8) | low;
 
  result = 1125300L / result; // Calculate Vcc (in mV); 1125300 = 1.1*1023*1000
  return result; // Vcc in millivolts
}

/****************************************************
 *
 * Verify all peripherals, and signal via the LED if any problems.
 *
 ****************************************************/
void testMode()
{
  uint8_t rx_buffer[SHA204_RSP_SIZE_MAX];
  uint8_t ret_code;
  byte tests = 0;
  
  digitalWrite(LED_PIN, HIGH); // Turn on LED.
  Serial.println(F(" - TestMode"));
  Serial.println(F("Testing peripherals!"));
  Serial.flush();
  Serial.print(F("-> SI7021 : ")); 
  Serial.flush();
  
  if (humiditySensor.begin()) 
  {
    Serial.println(F("ok!"));
    tests ++;
  }
  else
  {
    Serial.println(F("failed!"));
  }
  Serial.flush();

  Serial.print(F("-> Flash : "));
  Serial.flush();
  if (flash.initialize())
  {
    Serial.println(F("ok!"));
    tests ++;
  }
  else
  {
    Serial.println(F("failed!"));
  }
  Serial.flush();

  
  Serial.print(F("-> SHA204 : "));
  ret_code = sha204.sha204c_wakeup(rx_buffer);
  Serial.flush();
  if (ret_code != SHA204_SUCCESS)
  {
    Serial.print(F("Failed to wake device. Response: ")); Serial.println(ret_code, HEX);
  }
  Serial.flush();
  if (ret_code == SHA204_SUCCESS)
  {
    ret_code = sha204.getSerialNumber(rx_buffer);
    if (ret_code != SHA204_SUCCESS)
    {
      Serial.print(F("Failed to obtain device serial number. Response: ")); Serial.println(ret_code, HEX);
    }
    else
    {
      Serial.print(F("Ok (serial : "));
      for (int i=0; i<9; i++)
      {
        if (rx_buffer[i] < 0x10)
        {
          Serial.print('0'); // Because Serial.print does not 0-pad HEX
        }
        Serial.print(rx_buffer[i], HEX);
      }
      Serial.println(")");
      tests ++;
    }

  }
  Serial.flush();

  Serial.println(F("Test finished"));
  
  if (tests == 3) 
  {
    Serial.println(F("Selftest ok!"));
    while (1) // Blink OK pattern!
    {
      digitalWrite(LED_PIN, HIGH);
      delay(200);
      digitalWrite(LED_PIN, LOW);
      delay(200);
    }
  }
  else 
  {
    Serial.println(F("----> Selftest failed!"));
    while (1) // Blink FAILED pattern! Rappidly blinking..
    {
    }
  }  
}

