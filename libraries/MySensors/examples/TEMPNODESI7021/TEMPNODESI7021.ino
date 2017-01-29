/*
  Tempnode based on SI7021

  This example code is in the public domain.

  modified 14 oktober 2015 by Gert Sanders:
    original version based on examples from 1.5 library
  modified 16 oktober 2015 by Gert Sanders:
    updated to library 1.6 format
    calibrated the voltage measurement for a node (nr "6"), needs to be done for your own nodes as well.
  modified 30 november 2015 by Gert Sanders:
    added code to send only when change is large enough and with forced sending after several measurement loops

From the library info:
 * The ArduinoGateway prints data received from sensors on the serial link.
 * The gateway accepts input on serial which will be sent out on radio network.
 *
 * The GW code is designed for Arduino Nano 328p / 16MHz
 *
 * Wire connections (OPTIONAL):
 * - Inclusion button should be connected between digital pin 3 and GND
 * - RX/TX/ERR leds need to be connected between +5V (anode) and digital pin 6/5/4 with resistor 270-330R in a series
 *
 * LEDs (OPTIONAL):
 * - To use the feature, uncomment MY_LEDS_BLINKING_FEATURE in MyConfig.h
 * - RX (green) - blink fast on radio message recieved. In inclusion mode will blink fast only on presentation recieved
 * - TX (yellow) - blink fast on radio message transmitted. In inclusion mode will blink slowly
 * - ERR (red) - fast blink on error during transmission error or recieve crc error
 *


 */

//#define MYDEBUG
//#define MY_DEBUG
//#define MY_DEBUG_VERBOSE

//#define WITH_LEDS_BLINKING 0

//#define MY_NODE_ID 19
//#define MY_PARENT_ID 0

#define MY_BAUD_RATE 57600

#define MY_RADIO_NRF24
//#define MY_RADIO_RFM69

// Enable serial gateway
//#define MY_GATEWAY_SERIAL

// Enabled repeater feature for this node
//#define MY_REPEATER_FEATURE

// Flash leds on rx/tx/err
//#define MY_LEDS_BLINKING_FEATURE
// Set blinking period
//#define MY_DEFAULT_LED_BLINK_PERIOD 300

// Enable inclusion mode
//#define MY_INCLUSION_MODE_FEATURE
// Enable Inclusion mode button on gateway
//#define MY_INCLUSION_BUTTON_FEATURE
// Set inclusion mode duration (in seconds)
//#define MY_INCLUSION_MODE_DURATION 60
// Digital pin used for inclusion mode button
//#define MY_INCLUSION_MODE_BUTTON_PIN  3

//#define MY_DEFAULT_ERR_LED_PIN 5  // Error led pin
//#define MY_DEFAULT_RX_LED_PIN  6  // Receive led pin
//#define MY_DEFAULT_TX_LED_PIN  7  // the PCB, on board LED

// possible values: RF24_PA_LOW (is default on gateway), RF24_PA_MED, RF24_PA_HIGH, RF24_PA_MAX (is default on nodes)
#define MY_RF24_PA_LEVEL RF24_PA_HIGH

// RF channel for the sensor net, 0-127
#define MY_RF24_CHANNEL     76

//RF24_250KBPS for 250kbs, RF24_1MBPS for 1Mbps, or RF24_2MBPS for 2Mbps
#define MY_RF24_DATARATE      RF24_250KBPS



#include <Vcc.h>
#include <SPI.h>
#include <MySensor.h>
#include <Wire.h>
#include <SI7021.h>

// define childID's of sensors

#define BATTERYVOLTAGE  0
#define TEMPERATURE     1
#define HUMIDITY        2

// define pins
#define ledpin 8   // define here the built in LED pin number, usually this is 13

// blipled timings
#define NormalOnTime 5
#define NormalOffTime 75
#define PRESENTATIONWAITTIME 500
#define MESSAGEWAITTIME 50

//unsigned long SLEEPTIME0 =    10000; // 10 seconds (used during testing)
#define SLEEPTIME0     300000 // 5 minutes

//unsigned long SLEEPTIME1 =      5000; // 5 seconds
#define SLEEPTIME1     30000 // 30 seconds

//unsigned long SLEEPTIME2 = 86400000; // 24 hours

#define REPEATWITHOUTSENDING 10   // if you would send every 5 minutes, this counter could give you a 55 minute interval

int SendInterval = REPEATWITHOUTSENDING;  // gives a trigger when expires (when becoming 0)

const float VccMin   = 1.9;           // Minimum expected Vcc level, in Volts.
const float VccMax   = 3.3;           // Maximum expected Vcc level, in Volts.
//const float VccCorrection = 3.127 / 3.19 ; // Measured Vcc by multimeter divided by reported Vcc
const float VccCorrection = 3.238 / 3.238 ; // Measured Vcc by multimeter divided by reported Vcc, use this to calibrate

float BatteryVoltage = 0.0;
float PreviousBatteryVoltage = 0.0;
//int BatteryPcnt = 0;

float Temperature = 0.0;
float PreviousTemperature = 0.0;

int HumidityPercent = 0;
int PreviousHumidityPercent = 0;

const float tempThreshold = 0.5 ;                  // send only if change > treshold (Celcius)
const int humThreshold = 1 ;                        // send only if change > treshold (% RG)
const float voltageThreshold = 0.05 ;               // send only if change > treshold (Volt)

boolean BatOK = true;
boolean HumOK = true;
boolean TempOK = true;

// instantiate classes
Vcc vcc(VccCorrection);
MyMessage BatMsg(BATTERYVOLTAGE, V_VOLTAGE);
MyMessage HumMsg(HUMIDITY, V_HUM);
MyMessage TempMsg(TEMPERATURE, V_TEMP);
SI7021 TempHumSensor;



///////////////////////////////////////////////////////////////////////////
//////////// PRESENTATION FUNCTION ////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
void presentation()
{
  sendSketchInfo("TempHum sensor", "1.3");
  sleep(PRESENTATIONWAITTIME);
  present(BATTERYVOLTAGE, S_MULTIMETER);
  sleep(PRESENTATIONWAITTIME);
  present(TEMPERATURE, S_TEMP);
  sleep(PRESENTATIONWAITTIME);
  present(HUMIDITY, S_HUM);
  sleep(PRESENTATIONWAITTIME);
}


///////////////////////////////////////////////////////////////////////////////
//////////// RECEIVE FUNCTION /////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// the receive function handles all incoming messages, keep treatment short  //
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//void receive(const MyMessage &message)
//{
//}





///////////////////////////////////////////////////////////////////////////
//////////// SETUP FUNCTION ///////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
// the setup function runs once when you press reset or power the board  //
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
void setup()
{

#ifdef MYDEBUG
  //  Serial.begin(MY_BAUD_RATE);
#endif
  // initialize digital LEDpin as an output.
  pinMode(ledpin, OUTPUT);
  digitalWrite(ledpin, LOW);

  TempHumSensor.begin();

#ifdef MY_DEBUG
  Serial.print("DeviceID = ");
  int deviceid = TempHumSensor.getDeviceId();
  Serial.println(deviceid);
#endif

  SendInterval = REPEATWITHOUTSENDING;  // every 5 minutes decremented, gives half hour trigger

  BatOK = false;
  HumOK = false;
  TempOK = false;

  PreviousBatteryVoltage = 20.0;
  PreviousHumidityPercent = 101;
  PreviousTemperature = 100.0;

} ////////////////////////////////////////// END SETUP /////////////////




/////////////////////////////////////////////////////////
//////////// LOOP FUNCTION //////////////////////////////
/////////////////////////////////////////////////////////
// the loop function runs over and over again forever  //
/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////

void loop()
{

  if (!BatOK)
  {
    BatteryVoltage = vcc.Read_Volts();
//    BatteryPcnt = int(vcc.Read_Perc(VccMin, VccMax));
  }

  if (!(TempOK || HumOK)) // only measure again if needed, otherwise try to resend
  {
    si7021_env data = TempHumSensor.getHumidityAndTemperature();
    Temperature =  data.celsiusHundredths / 100.0;
    HumidityPercent = data.humidityPercent;
  }

#ifdef MY_DEBUG
  Serial.print("Battery:");
  Serial.print(BatteryVoltage, 3);
  Serial.print(" Batt%:");
  Serial.print(BatteryPcnt, 1);
  Serial.print(" Temperature:");
  Serial.print(Temperature, 2);
  Serial.print("C");
  Serial.print(" Humidity:");
  Serial.print(HumidityPercent, 1);
  Serial.println("%");
#endif

    if ((!BatOK)&&((abs(BatteryVoltage-PreviousBatteryVoltage) >= voltageThreshold)||(SendInterval <= 0)))
  //if (!BatOK)
  {
    BatOK = send(BatMsg.set(BatteryVoltage, 3));
    if (BatOK)
    {
      PreviousBatteryVoltage = BatteryVoltage;
    }
    sleep(MESSAGEWAITTIME);
  }

    if ((!HumOK)&&((abs(HumidityPercent-PreviousHumidityPercent) >= humThreshold)||(SendInterval <= 0)))
  //if (!HumOK)
  {
    HumOK = send(HumMsg.set(HumidityPercent, 1));
    if (HumOK)
    {
      PreviousHumidityPercent = HumidityPercent;
    }
    sleep(MESSAGEWAITTIME);
  }

    if ((!TempOK)&&((abs(Temperature-PreviousTemperature) >= tempThreshold)||(SendInterval <= 0)))
  //if (!TempOK)
  {
    TempOK = send(TempMsg.set(Temperature, 2));
    if (TempOK)
    {
      PreviousTemperature = Temperature;
    }
    //sleep(MESSAGEWAITTIME);
  }






#ifdef MYDEBUG
  delay(1000);
#else

  if (SendInterval <= 0)
  {
    SendInterval = REPEATWITHOUTSENDING;  // every sleepcycle decremented, gives half hour trigger
  }


  if (BatOK && HumOK && TempOK)
  { // 5 mintes , all values sent so time to sleep longer
    BlinkLed();
    BatOK = false;
    HumOK = false;
    TempOK = false;
    sleep(SLEEPTIME0);
    SendInterval--;
  }
  else
  { // 30 seconds ! need to try sending something again
    BlipLed();
    sleep(SLEEPTIME1);
  }

#endif



} /////////////////////// END LOOP ///////////////////






////////////////////////////////////////////////////////
//////////// SUPPORTING FUNCTIONS //////////////////////
////////////////////////////////////////////////////////


////////////////////////////////////////////////////////
void BlinkLed()
{
  digitalWrite(ledpin, HIGH);
  wait(NormalOnTime);
  digitalWrite(ledpin, LOW);
  wait(NormalOffTime);
  digitalWrite(ledpin, HIGH);
  wait(NormalOnTime);
  digitalWrite(ledpin, LOW);
  wait(NormalOffTime);
  digitalWrite(ledpin, HIGH);
  wait(NormalOnTime);
  digitalWrite(ledpin, LOW);
}


////////////////////////////////////////////////////////
void BlipLed()
{
  digitalWrite(ledpin, HIGH);
  wait(NormalOnTime);
  digitalWrite(ledpin, LOW);
}



