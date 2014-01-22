
#ifndef __PN532_I2C_H__
#define __PN532_I2C_H__

#include <Wire.h>
#include "PN532Interface.h"

class PN532_I2C : public PN532Interface {
public:
    PN532_I2C(TwoWire &wire);
    
    void begin();
    void wakeup();
    virtual int8_t writeCommand(const uint8_t *header, uint8_t hlen, const uint8_t *body = 0, uint8_t blen = 0);
    int16_t readResponse(uint8_t buf[], uint8_t len, uint16_t timeout);
    
private:
    TwoWire* _wire;
    uint8_t command;
    
    int8_t readAckFrame();
    
    inline uint8_t write(uint8_t data) {
        #if ARDUINO >= 100
            return _wire->write(data);
        #else
            return _wire->send(data);
        #endif
    }
    
    inline uint8_t read() {
        #if ARDUINO >= 100
            return _wire->read();
        #else
            return _wire->receive();
        #endif
    }
};

#endif
