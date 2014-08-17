/* Secret Knock Sensor

   See original instructions here (note: The MySensors adopted code might differ in wiring. The instructions below is correct):
   https://learn.adafruit.com/secret-knock-activated-drawer-lock/ 
   Version 13.10.31  Built with Arduino IDE 1.0.5
   
   By Steve Hoefer http://grathio.com
   Adapted to MySensors by Henrik Ekblad
   
   Licensed under Creative Commons Attribution-Noncommercial-Share Alike 3.0
   http://creativecommons.org/licenses/by-nc-sa/3.0/us/
   (In short: Do what you want, as long as you credit me, don't relicense it, and don't sell it or use it in anything you sell without contacting me.)
   
   ------Wiring------
   Pin 0: Program button used for recording a new Knock (connect Pin0 -> button -> GND) 
   Pin 1: Optional: Connect LED here (remember resisor in series)
   Pin 2: Optional: Piezo element (for beeps). 
   Pin 5: A sound sensor (digital output) for sensing knocks. See MySensors purchase guide. I used this: http://rover.ebay.com/rover/1/711-53200-19255-0/1?icep_ff3=2&pub=5575069610&toolid=10001&campid=5337433187&customid=&icep_item=200941260251&ipn=psmain&icep_vectorid=229466&kwid=902099&mtid=824&kw=lg 
   Pin 4: Connects to either 1. Relay which open door or lock or 
                             2. transistor that opens a solenoid lock when HIGH (see adafruit guide for this option).
                                 
 
   Connect radio according as usual(you can skip IRQ pin) 
   http://www.mysensors.org/build/connect_radio 
*/
 
#include <MySensor.h>  
#include <SPI.h>


#define CHILD_ID 99   // Id of the sensor child

const byte eepromValid = 121;    // If the first byte in eeprom is this then the data is valid.
 
/*Pin definitions*/
const int programButton = 0;   // (Digital 0) Record A New Knock button.
const int ledPin = 1;          // (Digital 1) The LED pin (if any)
const int knockSensor = 5;     // (Digital 5) for using the microphone digital output (tune knob to register knock) 
const int audioOut = 2;        // (Digital 2) for using the peizo as an output device. (Thing that goes beep.)
const int lockPin = 4;         // (Digital 4) The pin that activates the relay/solenoid lock.
 
/*Tuning constants. Changing the values below changes the behavior of the device.*/
int threshold = 3;                 // Minimum signal from the piezo to register as a knock. Higher = less sensitive. Typical values 1 - 10
const int rejectValue = 25;        // If an individual knock is off by this percentage of a knock we don't unlock. Typical values 10-30
const int averageRejectValue = 15; // If the average timing of all the knocks is off by this percent we don't unlock. Typical values 5-20
const int knockFadeTime = 150;     // Milliseconds we allow a knock to fade before we listen for another one. (Debounce timer.)
const int lockOperateTime = 2500;  // Milliseconds that we operate the lock solenoid latch before releasing it.
const int maximumKnocks = 20;      // Maximum number of knocks to listen for.
const int knockComplete = 1200;    // Longest time to wait for a knock before we assume that it's finished. (milliseconds)
 
byte secretCode[maximumKnocks] = {50, 25, 25, 50, 100, 50, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};  // Initial setup: "Shave and a Hair Cut, two bits."
int knockReadings[maximumKnocks];    // When someone knocks this array fills with the delays between knocks.
int knockSensorValue = 0;            // Last reading of the knock sensor.
boolean programModeActive = false;   // True if we're trying to program a new knock.

bool lockStatus;
MySensor gw;
MyMessage lockMsg(CHILD_ID, V_LOCK_STATUS);

 
void setup() {
  
  pinMode(ledPin, OUTPUT); 
  pinMode(knockSensor, INPUT); 
  pinMode(lockPin, OUTPUT);
  pinMode(programButton, INPUT);
  digitalWrite(programButton, HIGH); // Enable internal pull up 

  gw.begin(incomingMessage);
  
  gw.sendSketchInfo("Secret Knock", "1.0");
  gw.present(CHILD_ID, S_LOCK);
  
  readSecretKnock();   // Load the secret knock (if any) from EEPROM.
  
  digitalWrite(lockPin, HIGH); // Unlock the door for a bit when we power up. For system check and to allow a way in if the key is forgotten
  delay(500);                  // Wait a short time
  
  lockStatus = gw.loadState(0);    // Read last lock status from eeprom
  setLockState(lockStatus, true); // Now set the last known state and send it to controller 

  delay(500);          // This delay is here because the solenoid lock returning to place can otherwise trigger and inadvertent knock.
}
 
void loop() {
  gw.process(); // Process incomming messages
  // Listen for any knock at all.
  knockSensorValue = digitalRead(knockSensor);
  if (digitalRead(programButton) == LOW){  // is the program button pressed?
    delay(100);   // Cheap debounce.
    if (digitalRead(programButton) == LOW){ 
      if (programModeActive == false){     // If we're not in programming mode, turn it on.
        programModeActive = true;          // Remember we're in programming mode.
        digitalWrite(ledPin, HIGH);        // Turn on the red light too so the user knows we're programming.
        chirp(500, 1500);                  // And play a tone in case the user can't see the LED.
        chirp(500, 1000);
      } else {                             // If we are in programing mode, turn it off.
        programModeActive = false;
        digitalWrite(ledPin, LOW);
        chirp(500, 1000);                  // Turn off the programming LED and play a sad note.
        chirp(500, 1500);
        delay(500);
      }
      while (digitalRead(programButton) == LOW){
        delay(10);                         // Hang around until the button is released.
      } 
    }
    delay(250);   // Another cheap debounce. Longer because releasing the button can sometimes be sensed as a knock.
  }
  
  
  if (knockSensorValue == 0) {
     if (programModeActive == true){  // Blink the LED when we sense a knock.
       digitalWrite(ledPin, LOW);
     } else {
       digitalWrite(ledPin, HIGH);
     }
     knockDelay();
     if (programModeActive == true){  // Un-blink the LED.
       digitalWrite(ledPin, HIGH);
     } else {
       digitalWrite(ledPin, LOW);
     }
     listenToSecretKnock();           // We have our first knock. Go and see what other knocks are in store...
  }
 
} 
 
// Records the timing of knocks.
void listenToSecretKnock(){
  int i = 0;
  // First reset the listening array.
  for (i=0; i < maximumKnocks; i++){
    knockReadings[i] = 0;
  }
  
  int currentKnockNumber = 0;               // Position counter for the array.
  int startTime = millis();                 // Reference for when this knock started.
  int now = millis();   
 
  do {                                      // Listen for the next knock or wait for it to timeout. 
    knockSensorValue = digitalRead(knockSensor);

    if (knockSensorValue == 0){                   // Here's another knock. Save the time between knocks.
          Serial.println("knock");

      now=millis();
      knockReadings[currentKnockNumber] = now - startTime;
      currentKnockNumber ++;                             
      startTime = now;          
 
       if (programModeActive==true){     // Blink the LED when we sense a knock.
         digitalWrite(ledPin, LOW);
       } else {
         digitalWrite(ledPin, HIGH);
       } 
       knockDelay();
       if (programModeActive == true){  // Un-blink the LED.
         digitalWrite(ledPin, HIGH);
       } else {
         digitalWrite(ledPin, LOW);
       }
    }
 
    now = millis();
  
    // Stop listening if there are too many knocks or there is too much time between knocks.
  } while ((now-startTime < knockComplete) && (currentKnockNumber < maximumKnocks));
  Serial.println("end");

  //we've got our knock recorded, lets see if it's valid
  if (programModeActive == false){           // Only do this if we're not recording a new knock.
    if (validateKnock() == true){
      // Lock/unlock door
      chirp(500, 1500);                  // And play a tone in case the user can't see the LED.
      chirp(500, 1000);
      setLockState(!lockStatus, true); 
    } else {
      Serial.println("fail unlock");

      // knock is invalid. Blink the LED as a warning to others.
      for (i=0; i < 4; i++){          
        digitalWrite(ledPin, HIGH);
        delay(50);
        digitalWrite(ledPin, LOW);
        delay(50);
      }
    }
  } else { // If we're in programming mode we still validate the lock because it makes some numbers we need, we just don't do anything with the return.
    validateKnock();
  }
}
 
 
// Unlocks the door.
void setLockState(bool state, bool send){
  if (state) 
     Serial.println("open lock");
  else
     Serial.println("close lock");
  if (send)
    gw.send(lockMsg.set(state));
  
  digitalWrite(ledPin, state);
  digitalWrite(lockPin, state);
  gw.saveState(0,state);
  lockStatus = state;
  delay(500);   // This delay is here because releasing the latch can cause a vibration that will be sensed as a knock.
}
 
// Checks to see if our knock matches the secret.
// Returns true if it's a good knock, false if it's not.
boolean validateKnock(){
  int i = 0;
 
  int currentKnockCount = 0;
  int secretKnockCount = 0;
  int maxKnockInterval = 0;               // We use this later to normalize the times.
  
  for (i=0;i<maximumKnocks;i++){
    if (knockReadings[i] > 0){
      currentKnockCount++;
    }
    if (secretCode[i] > 0){         
      secretKnockCount++;
    }
    
    if (knockReadings[i] > maxKnockInterval){   // Collect normalization data while we're looping.
      maxKnockInterval = knockReadings[i];
    }
  }
  
  // If we're recording a new knock, save the info and get out of here.
  if (programModeActive == true){
      for (i=0; i < maximumKnocks; i++){ // Normalize the time between knocks. (the longest time = 100)
        secretCode[i] = map(knockReadings[i], 0, maxKnockInterval, 0, 100); 
      }
      saveSecretKnock();                // save the result to EEPROM
      programModeActive = false;
      playbackKnock(maxKnockInterval);
      return false;
  }
  
  if (currentKnockCount != secretKnockCount){  // Easiest check first. If the number of knocks is wrong, don't unlock.
    return false;
  }
  
  /*  Now we compare the relative intervals of our knocks, not the absolute time between them.
      (ie: if you do the same pattern slow or fast it should still open the door.)
      This makes it less picky, which while making it less secure can also make it
      less of a pain to use if you're tempo is a little slow or fast. 
  */
  int totaltimeDifferences = 0;
  int timeDiff = 0;
  for (i=0; i < maximumKnocks; i++){    // Normalize the times
    knockReadings[i]= map(knockReadings[i], 0, maxKnockInterval, 0, 100);      
    timeDiff = abs(knockReadings[i] - secretCode[i]);
    if (timeDiff > rejectValue){        // Individual value too far out of whack. No access for this knock!
      return false;
    }
    totaltimeDifferences += timeDiff;
  }
  // It can also fail if the whole thing is too inaccurate.
  if (totaltimeDifferences / secretKnockCount > averageRejectValue){
    return false; 
  }
  
  return true;
}
 
 
// reads the secret knock from EEPROM. (if any.)
void readSecretKnock(){
  byte reading;
  int i;
  reading = gw.loadState(1);
  if (reading == eepromValid){    // only read EEPROM if the signature byte is correct.
    for (int i=0; i < maximumKnocks ;i++){
      secretCode[i] =  gw.loadState(i+2);
    }
  }
}
 
 
//saves a new pattern too eeprom
void saveSecretKnock(){
  gw.saveState(1, 0); // clear out the signature. That way we know if we didn't finish the write successfully.
  for (int i=0; i < maximumKnocks; i++){
    gw.saveState(i+2, secretCode[i]);
  }
  gw.saveState(1, eepromValid);  // all good. Write the signature so we'll know it's all good.
}
 
// Plays back the pattern of the knock in blinks and beeps
void playbackKnock(int maxKnockInterval){
      digitalWrite(ledPin, LOW);
      delay(1000);
      digitalWrite(ledPin, HIGH);
      chirp(200, 1800);
      for (int i = 0; i < maximumKnocks ; i++){
        digitalWrite(ledPin, LOW);
        // only turn it on if there's a delay
        if (secretCode[i] > 0){                                   
          delay(map(secretCode[i], 0, 100, 0, maxKnockInterval)); // Expand the time back out to what it was. Roughly. 
          digitalWrite(ledPin, HIGH);
          chirp(200, 1800);
        }
      }
      digitalWrite(ledPin, LOW);
}
 
// Deals with the knock delay thingy.
void knockDelay(){
  int itterations = (knockFadeTime / 20);      // Wait for the peak to dissipate before listening to next one.
  for (int i=0; i < itterations; i++){
    delay(10);
    analogRead(knockSensor);                  // This is done in an attempt to defuse the analog sensor's capacitor that will give false readings on high impedance sensors.
    delay(10);
  } 
}
 
// Plays a non-musical tone on the piezo.
// playTime = milliseconds to play the tone
// delayTime = time in microseconds between ticks. (smaller=higher pitch tone.)
void chirp(int playTime, int delayTime){
  long loopTime = (playTime * 1000L) / delayTime;
  pinMode(audioOut, OUTPUT);
  for(int i=0; i < loopTime; i++){
    digitalWrite(audioOut, HIGH);
    delayMicroseconds(delayTime);
    digitalWrite(audioOut, LOW);
  }
  pinMode(audioOut, INPUT);
}



void incomingMessage(const MyMessage &message) {
  // We only expect one type of message from controller. But we better check anyway.
  if (message.type==V_LOCK_STATUS) {
     // Change relay state
     setLockState(message.getBool(), false); 
  
     // Write some debug info
     Serial.print("Incoming lock status:");
     Serial.println(message.getBool());
   } 
}

