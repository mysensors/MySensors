#include <OneWire.h>
#include <DallasTemperature.h>

// Data wire is plugged into port 2 on the Arduino
#define ONE_WIRE_BUS 3
#define TEMPERATURE_PRECISION 9

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature. 
DallasTemperature sensors(&oneWire);

// arrays to hold device addresses
DeviceAddress insideThermometer, outsideThermometer;

// function that will be called when an alarm condition exists during DallasTemperatures::processAlarms();
void newAlarmHandler(uint8_t* deviceAddress)
{
  Serial.println("Alarm Handler Start"); 
  printAlarmInfo(deviceAddress);
  printTemp(deviceAddress);
  Serial.println();
  Serial.println("Alarm Handler Finish");
}

void printCurrentTemp(DeviceAddress deviceAddress)
{
  printAddress(deviceAddress);
  printTemp(deviceAddress);
  Serial.println();
}

void printAddress(DeviceAddress deviceAddress)
{
  Serial.print("Address: ");
  for (uint8_t i = 0; i < 8; i++)
  {
    if (deviceAddress[i] < 16) Serial.print("0");
    Serial.print(deviceAddress[i], HEX);
  }
  Serial.print(" ");
}

void printTemp(DeviceAddress deviceAddress)
{
  float tempC = sensors.getTempC(deviceAddress);
  if (tempC != DEVICE_DISCONNECTED)
  {
    Serial.print("Current Temp C: ");
    Serial.print(tempC);
  }
  else Serial.print("DEVICE DISCONNECTED");
  Serial.print(" ");
}

void printAlarmInfo(DeviceAddress deviceAddress)
{
  char temp;
  printAddress(deviceAddress);
  temp = sensors.getHighAlarmTemp(deviceAddress);
  Serial.print("High Alarm: ");
  Serial.print(temp, DEC);
  Serial.print("C");
  Serial.print(" Low Alarm: ");
  temp = sensors.getLowAlarmTemp(deviceAddress);
  Serial.print(temp, DEC);
  Serial.print("C");
  Serial.print(" ");
}

void setup(void)
{
  // start serial port
  Serial.begin(9600);
  Serial.println("Dallas Temperature IC Control Library Demo");

  // Start up the library
  sensors.begin();
  
  // locate devices on the bus
  Serial.print("Found ");
  Serial.print(sensors.getDeviceCount(), DEC);
  Serial.println(" devices.");

  // search for devices on the bus and assign based on an index
  if (!sensors.getAddress(insideThermometer, 0)) Serial.println("Unable to find address for Device 0"); 
  if (!sensors.getAddress(outsideThermometer, 1)) Serial.println("Unable to find address for Device 1"); 

  Serial.print("Device insideThermometer ");
  printAlarmInfo(insideThermometer);
  Serial.println();
  
  Serial.print("Device outsideThermometer ");
  printAlarmInfo(outsideThermometer);
  Serial.println();
  
  // set alarm ranges
  Serial.println("Setting alarm temps...");
  sensors.setHighAlarmTemp(insideThermometer, 26);
  sensors.setLowAlarmTemp(insideThermometer, 22);
  sensors.setHighAlarmTemp(outsideThermometer, 25);
  sensors.setLowAlarmTemp(outsideThermometer, 21);
  
  Serial.print("New insideThermometer ");
  printAlarmInfo(insideThermometer);
  Serial.println();
  
  Serial.print("New outsideThermometer ");
  printAlarmInfo(outsideThermometer);
  Serial.println();

  // attach alarm handler
  sensors.setAlarmHandler(&newAlarmHandler);

}

void loop(void)
{ 
  // ask the devices to measure the temperature
  sensors.requestTemperatures();
  
  // if an alarm condition exists as a result of the most recent 
  // requestTemperatures() request, it exists until the next time 
  // requestTemperatures() is called AND there isn't an alarm condition
  // on the device
  if (sensors.hasAlarm())
  {
    Serial.println("Oh noes!  There is at least one alarm on the bus.");
  }

  // call alarm handler function defined by sensors.setAlarmHandler
  // for each device reporting an alarm
  sensors.processAlarms();

  if (!sensors.hasAlarm())
  {
    // just print out the current temperature
    printCurrentTemp(insideThermometer);
    printCurrentTemp(outsideThermometer);
  }
  
  delay(1000);
}

