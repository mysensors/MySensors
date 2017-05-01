#include <MySensor.h>
#include <SPI.h>
#include <Wire.h>               // Requirement for LCD to work. LCD is wired through i2C bus (A4 and A5)
#include <LCD.h>                // Standard LCD library
#include <LiquidCrystal_I2C.h>  // LCD to i2C library
LiquidCrystal_I2C lcd(0x27,2,1,0,4,5,6,7,3,POSITIVE);//, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);
 
#define NODE_ID 10              // Or set to AUTO if you want your home automation controller to set this for you. 
#define washStatus_CHILD_ID 0   // Custom status of washer based on light conditions.

unsigned long reportDelay = 15000;         // Timeout between reports to the gateway
unsigned long previousMillis = 0;
 
MySensor gw;
MyMessage washStatusMsg(washStatus_CHILD_ID, V_VAR1); 

void setup() 
{ 
  lcd.begin(16,2);
  lcd.backlight();
  gw.begin(NULL, NODE_ID);
  // Send the sketch version information to the gateway and Controller
  gw.sendSketchInfo("Washer Monitor", "1.0");
  lcd.setCursor(3, 0);
  lcd.print("*MySensors");
  lcd.setCursor(1, 1);
  lcd.print("WASHER MONITOR");
  delay(2500);
  lcd.clear();
  lcd.setCursor(2, 0);
  lcd.print("Version 1.0A");
  lcd.setCursor(2, 1);
  lcd.print("By drock1985");
  gw.present(washStatus_CHILD_ID, S_CUSTOM);
//  delay(5000);
}
 
void loop()
{
     gw.process();
     unsigned long currentMillis = millis();   // Start counting the milliseconds for next variable
  // Set up Photoresistors   
     int lidLockSensorP = analogRead(A6);
     int soakSensorP = analogRead(A0);
     int washSensorP = analogRead(A1);
     int rinseSensorP = analogRead(A2);
     int spinSensorP = analogRead(A3);
     
  // Convert the analog reading (which goes from 0 - 1023) to a more reasonable number
     float lidLockSensor = lidLockSensorP * (5.0 / 256.0);
     float soakSensor = soakSensorP * (5.0 / 256.0);
     float washSensor = washSensorP * (5.0 / 256.0);
     float rinseSensor = rinseSensorP * (5.0 / 256.0);
     float spinSensor = spinSensorP * (5.0 / 256.0);

if ( ((unsigned long) (currentMillis - previousMillis) >= reportDelay) && washSensor >= 10.00 && rinseSensor >= 10.00 && spinSensor >= 10.00 ) {
  gw.send(washStatusMsg.set("Washer On/Operating"));
  lcd.clear();
  lcd.setCursor(1,0);
  lcd.print("WASHER STATUS:");
  lcd.setCursor(4,1);
  lcd.print("RUNNING");
  previousMillis = currentMillis;
}

if ( ((unsigned long) (currentMillis - previousMillis) >= reportDelay) && lidLockSensor >= 10.00 && rinseSensor >= 10.00 && spinSensor >= 10.00 ) {
  gw.send(washStatusMsg.set("Rinse Cycle"));
  lcd.clear();
  lcd.setCursor(1,0);
  lcd.print("WASHER STATUS:");
  lcd.setCursor(3,1);
  lcd.print("RINSE CYCLE");
  previousMillis = currentMillis;
}

if ( ((unsigned long) (currentMillis - previousMillis) >= reportDelay) && lidLockSensor >= 10.00 && spinSensor >= 10.00 ) {
  gw.send(washStatusMsg.set("Spin Cycle"));
  lcd.clear();
  lcd.setCursor(1,0);
  lcd.print("WASHER STATUS:");
  lcd.setCursor(3,1);
  lcd.print("SPIN CYCLE");
  previousMillis = currentMillis;
}

if ( ((unsigned long) (currentMillis - previousMillis) >= reportDelay) && lidLockSensor <= 8.00 && soakSensor <= 8.00 && washSensor <= 8.00 && rinseSensor <= 8.00 && spinSensor <= 8.00) {
  gw.send(washStatusMsg.set("Washer Off"));
  lcd.clear();
  lcd.setCursor(1,0);
  lcd.print("WASHER STATUS:");
  lcd.setCursor(3,1);
  lcd.print("IDLE/OFF");
  previousMillis = currentMillis;
}

if ( ((unsigned long) (currentMillis - previousMillis) >= reportDelay) && lidLockSensor >= 8.00 && soakSensor <= 8.00 && washSensor <= 8.00 && rinseSensor <= 8.00 && spinSensor <= 8.00) {
  gw.send(washStatusMsg.set("Lid Locked/Wash IDLE"));
  lcd.clear();
  lcd.setCursor(1,0);
  lcd.print("WASHER STATUS:");
  lcd.setCursor(4,1);
  lcd.print("LOCKED");
  previousMillis = currentMillis;
}

}

