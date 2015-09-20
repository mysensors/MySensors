// snep_test.ino
// send a SNEP message to adnroid and get a message from android

#include "SPI.h"
#include "PN532_SPI.h"
#include "llcp.h"
#include "snep.h"

PN532_SPI pn532spi(SPI, 10);
SNEP nfc(pn532spi);

void setup()
{
    Serial.begin(115200);
    Serial.println("-------Peer to Peer--------");
}

uint8_t message[] = {
0xD2, 0xA, 0xB, 0x74,0x65, 0x78, 0x74, 0x2F, 0x70, 0x6C, 
0x61, 0x69, 0x6E, 0x68, 0x65, 0x6C, 0x6C, 0x6F, 0x20, 0x77, 
0x6F, 0x72, 0x6C, 0x64};

uint8_t buf[128];

void loop()
{

    nfc.write(message, sizeof(message));
    delay(3000);

    int16_t len = nfc.read(buf, sizeof(buf));
    if (len > 0) {
      Serial.println("get a SNEP message:");
      for (uint8_t i = 0; i < len; i++) {
        Serial.print(buf[i], HEX);
        Serial.print(' ');
      }
      Serial.print('\n');
      for (uint8_t i = 0; i < len; i++) {
        char c = buf[i];
        if (c <= 0x1f || c > 0x7f) {
          Serial.print('.');
        } else {
          Serial.print(c);
        }
      }
      Serial.print('\n');
    }
    delay(3000);
}

