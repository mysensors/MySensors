

#ifndef __MAC_LINK_H__
#define __MAC_LINK_H__

#include "PN532.h"

class MACLink {
public:
    MACLink(PN532Interface &interface) : pn532(interface) {

    };
    
    /**
    * @brief    Activate PN532 as a target
    * @param    timeout max time to wait, 0 means no timeout
    * @return   > 0     success
    *           = 0     timeout
    *           < 0     failed
    */
    int8_t activateAsTarget(uint16_t timeout = 0);

    /**
    * @brief    write a PDU packet, the packet should be less than (255 - 2) bytes
    * @param    header  packet header
    * @param    hlen    length of header
    * @param 	body	packet body
    * @param 	blen	length of body
    * @return   true    success
    *           false   failed
    */
    bool write(const uint8_t *header, uint8_t hlen, const uint8_t *body = 0, uint8_t blen = 0);

    /**
    * @brief    read a PDU packet, the packet will be less than (255 - 2) bytes
    * @param    buf     the buffer to contain the PDU packet
    * @param    len     lenght of the buffer
    * @return   >=0     length of the PDU packet 
    *           <0      failed
    */
    int16_t read(uint8_t *buf, uint8_t len);

    uint8_t *getHeaderBuffer(uint8_t *len) {
        return pn532.getBuffer(len);
    };
    
private:
    PN532 pn532;
};

#endif // __MAC_LINK_H__
