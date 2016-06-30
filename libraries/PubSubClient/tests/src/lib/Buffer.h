#ifndef buffer_h
#define buffer_h

#include "Arduino.h"

/** Buffer class */
class Buffer {
private:
    uint8_t buffer[1024];
    uint16_t pos;
    uint16_t length;
    
public:
    Buffer(); //!< Buffer
    Buffer(uint8_t* buf, size_t size); //!< Buffer
    
    virtual bool available(); //!< available
    virtual uint8_t next(); //!< next
    virtual void reset(); //!< reset
    
    virtual void add(uint8_t* buf, size_t size); //!< add
};

#endif
