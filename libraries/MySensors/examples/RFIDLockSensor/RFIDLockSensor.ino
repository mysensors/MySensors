/* 
  RFID Lock sensor/actuator
  
  Henrik Ekblad <henrik.ekblad@mysensors.org>

  Use RFID tag to lock/unlock a door or trigger a scene on your controller.
  This example sketch allows you to add an optional relay or solenoid 
  which can be activated/opened by RFID or controller.  

  Use the I2C wiring option for your RFID module and connect to the following Arduino pins.
  
  RFID       Arduino
  -----      -------
  GND   ->   GND
  VCC   ->   +5V
  SCL   ->   A5
  SDA   ->   A4
  
  Use normal wiring for NRF24L01 radio
  
  Attach a optional relay or solonoid lock to pin 4
  
*/
 
#include <MySensor.h>  
#include <SPI.h>
#include <Wire.h>
#include <PN532_I2C.h>
#include <PN532.h>


// Add your valid rfid keys here. To find you your key just run sketch; hold your new RFID tag in fron ot the reader; 
// and copy the key from serial output of this sketch.
const uint8_t maxKeyLength = 7;
uint8_t validKeys[][maxKeyLength] = {
                    { 0xB3, 0xC6, 0xD9, 0x80, 0x00, 0x00, 0x00 },
                    { 0, 0, 0, 0, 0, 0, 0 },    // ADD YOUR KEYS HERE!
                    { 0, 0, 0, 0, 0, 0, 0 }};
int keyCount = sizeof validKeys / maxKeyLength; 


#define CHILD_ID 99   // Id of the sensor child
 
/*Pin definitions*/
const int lockPin = 4;         // (Digital 4) The pin that activates the relay/solenoid lock.

bool lockStatus;
MySensor gw;
MyMessage lockMsg(CHILD_ID, V_LOCK_STATUS);
PN532_I2C pn532i2c(Wire);
PN532 nfc(pn532i2c);
 
void setup() {
  
  pinMode(lockPin, OUTPUT);

  nfc.begin();
  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
    Serial.print("Couldn't find PN53x board");
    while (1); // halt
  }
  Serial.print("Found NFC chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX); 
  Serial.print("Firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC); 
  Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);
  // Set the max number of retry attempts to read from a card
  // This prevents us from waiting forever for a card, which is
  // the default behaviour of the PN532.
  nfc.setPassiveActivationRetries(0x3);
  
  // configure board to read RFID tags
  nfc.SAMConfig();

  // Init mysensors library
  gw.begin(incomingMessage);
  
  gw.sendSketchInfo("RFID Lock", "1.0");
  gw.present(CHILD_ID, S_LOCK);
  
  lockStatus = gw.loadState(0);    // Read last lock status from eeprom
  setLockState(lockStatus, true); // Now set the last known state and send it to controller 
}
 
void loop() {
  gw.process(); // Process incomming messages

  boolean success;
  uint8_t key[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t currentKeyLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)

  
  // Wait for an ISO14443A type cards (Mifare, etc.).  When one is found
  // 'uid' will be populated with the UID, and uidLength will indicate
  // if the uid is 4 bytes (Mifare Classic) or 7 bytes (Mifare Ultralight)
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &key[0], &currentKeyLength);
  
  if (success) {
    Serial.print("Found tag id: ");
    for (uint8_t i=0; i < currentKeyLength; i++) 
    {
      if (i>0) Serial.print(",");
      Serial.print("0x");Serial.print(key[i], HEX); 
    }
    for (uint8_t i=currentKeyLength; i < maxKeyLength; i++) 
    {
      Serial.print(",0x00"); 
    }


    Serial.println("");

    boolean valid = false;
    // Compare this key to the valid once registered here in sketch 
    for (int i=0;i<keyCount && !valid;i++) {
      for (int j=0;i<currentKeyLength && !valid;j++) {
        if (key[j] != validKeys[i][j]) {
          break;
        }
        if (j==currentKeyLength-1) {
          valid = true;
        }
      }
    }
    if (valid) {
      // Switch lock status
      setLockState(!lockStatus, true);       
    }
    
    // Wait for card/tag to leave reader    
    while(nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, &key[0], &currentKeyLength));
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
  digitalWrite(lockPin, state);
  gw.saveState(0,state);
  lockStatus = state;
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


