/**************************************************************************/
/*!
    Updates a sector that is already formatted for NDEF (using
    mifareclassic_formatndef.pde for example), inserting a new url
*/
/**************************************************************************/

// choose to SPI or I2C or HSU
#if 0
  #include <SPI.h>
  #include <PN532_SPI.h>
  #include "PN532.h"

  PN532SPI pn532spi(SPI, 10);
  PN532 nfc(pn532spi);
#elif 0
  #include <PN532_HSU.h>
  #include <PN532.h>
      
  PN532_HSU pn532hsu(Serial1);
  PN532 nfc(pn532hsu);
#else 
  #include <Wire.h>
  #include <PN532_I2C.h>
  #include <PN532.h>

  PN532_I2C pn532i2c(Wire);
  PN532 nfc(pn532i2c);
#endif


/*  
    We can encode many different kinds of pointers to the card,
    from a URL, to an Email address, to a phone number, and many more
    check the library header .h file to see the large # of supported
    prefixes! 
*/
// For a http://www. url:
const char * url = "elechouse.com";
uint8_t ndefprefix = NDEF_URIPREFIX_HTTP_WWWDOT;

// for an email address
//const char * url = "sevice@elechouse.com";
//uint8_t ndefprefix = NDEF_URIPREFIX_MAILTO;

// for a phone number
//const char * url = "+1 212 555 1212";
//uint8_t ndefprefix = NDEF_URIPREFIX_TEL;


void setup(void) {
  Serial.begin(115200);
  Serial.println("Looking for PN532...");

  nfc.begin();

  uint32_t versiondata = nfc.getFirmwareVersion();
  if (! versiondata) {
    Serial.print("Didn't find PN53x board");
    while (1); // halt
  }
  
  // Got ok data, print it out!
  Serial.print("Found chip PN5"); Serial.println((versiondata>>24) & 0xFF, HEX); 
  Serial.print("Firmware ver. "); Serial.print((versiondata>>16) & 0xFF, DEC); 
  Serial.print('.'); Serial.println((versiondata>>8) & 0xFF, DEC);
  
  // configure board to read RFID tags
  nfc.SAMConfig();
}

void loop(void) {
  uint8_t success;                          // Flag to check if there was an error with the PN532
  uint8_t uid[] = { 0, 0, 0, 0, 0, 0, 0 };  // Buffer to store the returned UID
  uint8_t uidLength;                        // Length of the UID (4 or 7 bytes depending on ISO14443A card type)
  bool authenticated = false;               // Flag to indicate if the sector is authenticated

  // Use the default NDEF keys (these would have have set by mifareclassic_formatndef.pde!)
  uint8_t keya[6] = { 0xA0, 0xA1, 0xA2, 0xA3, 0xA4, 0xA5 };
  uint8_t keyb[6] = { 0xD3, 0xF7, 0xD3, 0xF7, 0xD3, 0xF7 };

  Serial.println("Place your NDEF formatted Mifare Classic card on the reader to update the");
  Serial.println("NDEF record and press any key to continue ...");
  // Wait for user input before proceeding
  while (!Serial.available());
  // a key was pressed1
  while (Serial.available()) Serial.read();
    
  // Wait for an ISO14443A type card (Mifare, etc.).  When one is found
  // 'uid' will be populated with the UID, and uidLength will indicate
  // if the uid is 4 bytes (Mifare Classic) or 7 bytes (Mifare Ultralight)
  success = nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, &uidLength);
  
  if (success) 
  {
    // Display some basic information about the card
    Serial.println("Found an ISO14443A card");
    Serial.print("  UID Length: ");Serial.print(uidLength, DEC);Serial.println(" bytes");
    Serial.print("  UID Value: ");
    nfc.PrintHex(uid, uidLength);
    Serial.println("");
    
    // Make sure this is a Mifare Classic card
    if (uidLength != 4)
    {
      Serial.println("Ooops ... this doesn't seem to be a Mifare Classic card!"); 
      return;
    }
    
    // We probably have a Mifare Classic card ... 
    Serial.println("Seems to be a Mifare Classic card (4 byte UID)");

    // Check if this is an NDEF card (using first block of sector 1 from mifareclassic_formatndef.pde)
    // Must authenticate on the first key using 0xD3 0xF7 0xD3 0xF7 0xD3 0xF7
    success = nfc.mifareclassic_AuthenticateBlock (uid, uidLength, 4, 0, keyb);
    if (!success)
    {
      Serial.println("Unable to authenticate block 4 ... is this card NDEF formatted?");
      return;
    }
    
    Serial.println("Authentication succeeded (seems to be an NDEF/NFC Forum tag) ...");

    // Authenticated seems to have worked
    // Try to write an NDEF record to sector 1
    // Use 0x01 for the URI Identifier Code to prepend "http://www."
    // to the url (and save some space).  For information on URI ID Codes
    // see http://www.ladyada.net/wiki/private/articlestaging/nfc/ndef
    if (strlen(url) > 38)
    {
      // The length is also checked in the WriteNDEFURI function, but lets
      // warn users here just in case they change the value and it's bigger
      // than it should be
      Serial.println("URI is too long ... must be less than 38 characters!");
      return;
    }
    
    Serial.println("Updating sector 1 with URI as NDEF Message");
    
    // URI is within size limits ... write it to the card and report success/failure
    success = nfc.mifareclassic_WriteNDEFURI(1, ndefprefix, url);
    if (success)
    {
      Serial.println("NDEF URI Record written to sector 1");
      Serial.println("");      
    }
    else
    {
      Serial.println("NDEF Record creation failed! :(");
    }
  }
  
  // Wait a bit before trying again
  Serial.println("\n\nDone!");
  delay(1000);
  Serial.flush();
  while(Serial.available()) Serial.read();
}
