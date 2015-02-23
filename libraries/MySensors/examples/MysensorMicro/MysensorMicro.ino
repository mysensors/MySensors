// Default sensor sketch for MySensor Micro module
// Act as a temperature / humidity sensor by default.
//
// If A1 is held low while powering on, it will clear eeprom (and thereby the stored nodeid)
// 
// Battery voltage is repported as child sensorId 199, as well as battery percentage


#include <MySensor.h>
#include <Wire.h>
#include <SI7021.h>
#include <SPI.h>
#include <EEPROM.h>  

// Define a static node address, remove if you want auto address assignment
//#define NODE_ADDRESS   3

// Child sensor ID's
#define CHILD_ID_TEMP  1
#define CHILD_ID_HUM   2
#define CHILD_ID_BATT  199

//Pin definitions
#define RESET_CFG_PIN  A1
#define LED_PIN        A2

#define MEASURE_INTERVAL 60000

// FORCE_TRANSMIT_INTERVAL, this number of times of wakeup, the sensor is forced to report all values to the controller
#define FORCE_TRANSMIT_INTERVAL 30 

SI7021 humiditySensor;
MySensor gw;

// Sensor messages
MyMessage msgHum(CHILD_ID_HUM, V_HUM);
MyMessage msgTemp(CHILD_ID_TEMP, V_TEMP);
MyMessage msgBattery(CHILD_ID_BATT, V_VOLTAGE);

// Global settings
int measureCount = 0;

// Storage of old measurements
float lastTemperature = -100;
int lastHumidity = -100;
long lastBattery = -100;


void setup() {

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  // First check if we should boot into clear eeprom mode
  pinMode(RESET_CFG_PIN, INPUT);
  digitalWrite(RESET_CFG_PIN, HIGH); // Enable pullup
  if (!digitalRead(RESET_CFG_PIN)) resetEEP();
  digitalWrite(RESET_CFG_PIN, LOW);
  
  digitalWrite(LED_PIN, HIGH); 
  
#ifdef NODE_ADDRESS
  gw.begin(NULL, NODE_ADDRESS, false);
#else
  gw.begin(NULL,AUTO,false);
#endif

  digitalWrite(LED_PIN, LOW);

  humiditySensor.begin();
  
  gw.sendSketchInfo("EnvironmentSensor", "1.0");
  
  gw.present(CHILD_ID_TEMP,S_TEMP);
  gw.present(CHILD_ID_HUM,S_HUM);
  
  gw.present(CHILD_ID_BATT, S_POWER);
  switchClock(1<<CLKPS2);
}


// Main loop function
void loop() {
  measureCount ++;
  bool forceTransmit = false;
  
  if (measureCount > FORCE_TRANSMIT_INTERVAL
  ) {// Every 60th time we wake up, force a transmission on all sensors.
    forceTransmit = true; 
    measureCount = 0;
  }
  
  gw.process();
  sendBattLevel(forceTransmit);
  sendTempHumidityMeasurements(forceTransmit);
  
  gw.sleep(MEASURE_INTERVAL);  
}

/*
 * Sends temperature and humidity from Si7021 sensor
 *
 * Parameters
 * - force : Forces transmission of a value (even if it's the same as previous measurement)
 */
void sendTempHumidityMeasurements(bool force)
{
  if (force) {
    lastHumidity = -100;
    lastTemperature = -100;
  }
  
  si7021_env data = humiditySensor.getHumidityAndTemperature();
  
  float temperature = data.celsiusHundredths/100;
    
  int humidity = data.humidityPercent;

  if (lastTemperature != temperature) {
    gw.send(msgTemp.set(temperature,1));
    lastTemperature = temperature;
  }
  if (lastHumidity != humidity) {    
    gw.send(msgHum.set(humidity));
    lastHumidity = humidity;
  }
}

/*
 * Sends battery information (both voltage, and battery percentage)
 *
 * Parameters
 * - force : Forces transmission of a value
 */
void sendBattLevel(bool force)
{
  if (force) lastBattery = -1;
  long vcc = readVcc();
  if (vcc != lastBattery) {
    lastBattery = vcc;
    // Calculate percentage
    gw.send(msgBattery.set(vcc));
    vcc = vcc - 1900; // subtract 1.9V from vcc, as this is the lowest voltage we will operate at
    
    long percent = vcc / 14;
    gw.sendBatteryLevel(percent);
  }
}

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


/*
 * Resets eeprom to default value (while blinking LED_PIN)
 */
void resetEEP()
{
  for (int i=0;i<512;i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);
    delay(100);
    EEPROM.write(i, 0xff);
  }
}

void switchClock(unsigned char clk)
{
  cli();
  
  CLKPR = 1<<CLKPCE; // Set CLKPCE to enable clk switching
  CLKPR = clk;  
  sei();
}
