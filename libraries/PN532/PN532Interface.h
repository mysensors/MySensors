

#ifndef __PN532_INTERFACE_H__
#define __PN532_INTERFACE_H__

#if ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#define PN532_PREAMBLE                      (0x00)
#define PN532_STARTCODE1                    (0x00)
#define PN532_STARTCODE2                    (0xFF)
#define PN532_POSTAMBLE                     (0x00)

#define PN532_HOSTTOPN532                   (0xD4)
#define PN532_PN532TOHOST                   (0xD5)

#define PN532_ACK_WAIT_TIME                 (10)  // ms, timeout of waiting for ACK

#define PN532_INVALID_ACK             (-1)
#define PN532_TIMEOUT                 (-2)
#define PN532_INVALID_FRAME           (-3)
#define PN532_NO_SPACE                (-4)

class PN532Interface
{
public:
    virtual void begin() = 0;
    virtual void wakeup() = 0;

    /**
    * @brief    write a command and check ack
    * @param    header  packet header
    * @param    hlen    length of header
    * @param    body    packet body
    * @param    blen    length of body
    * @return   0       success
    *           not 0   failed
    */
    virtual int8_t writeCommand(const uint8_t *header, uint8_t hlen, const uint8_t *body = 0, uint8_t blen = 0) = 0;

    /**
    * @brief    read the response of a command, strip prefix and suffix
    * @param    buf     to contain the response data
    * @param    len     lenght to read
    * @param    timeout max time to wait, 0 means no timeout
    * @return   >=0     length of response without prefix and suffix
    *           <0      failed to read response
    */
    virtual int16_t readResponse(uint8_t buf[], uint8_t len, uint16_t timeout = 1000) = 0;
};

#endif

