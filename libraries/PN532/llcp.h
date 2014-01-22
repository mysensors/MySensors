
#ifndef __LLCP_H__
#define __LLCP_H__

#include "mac_link.h"

#define LLCP_DEFAULT_TIMEOUT  20000
#define LLCP_DEFAULT_DSAP     0x04
#define LLCP_DEFAULT_SSAP     0x20

class LLCP {
public:
	LLCP(PN532Interface &interface) : link(interface) {
        headerBuf = link.getHeaderBuffer(&headerBufLen);
        sequence = 0;
	};

	/**
    * @brief    Actiave PN532 as a target
    * @param    timeout max time to wait, 0 means no timeout
    * @return   > 0     success
    *           = 0     timeout
    *           < 0     failed
    */
	int8_t activate(uint16_t timeout = 0);

    int8_t waitForConnection(uint16_t timeout = LLCP_DEFAULT_TIMEOUT);

    int8_t waitForDisconnection(uint16_t timeout = LLCP_DEFAULT_TIMEOUT);

    int8_t connect(uint16_t timeout = LLCP_DEFAULT_TIMEOUT);

    int8_t disconnect(uint16_t timeout = LLCP_DEFAULT_TIMEOUT);

	/**
    * @brief    write a packet, the packet should be less than (255 - 2) bytes
    * @param    header  packet header
    * @param    hlen    length of header
    * @param    body    packet body
    * @param    blen    length of body
    * @return   true    success
    *           false   failed
    */
    bool write(const uint8_t *header, uint8_t hlen, const uint8_t *body = 0, uint8_t blen = 0);

    /**
    * @brief    read a  packet, the packet will be less than (255 - 2) bytes
    * @param    buf     the buffer to contain the packet
    * @param    len     lenght of the buffer
    * @return   >=0     length of the packet 
    *           <0      failed
    */
    int16_t read(uint8_t *buf, uint8_t len);

    uint8_t *getHeaderBuffer(uint8_t *len) {
        uint8_t *buf = link.getHeaderBuffer(len);
        len -= 3;       // I PDU header has 3 bytes
        return buf;
    };

private:
	MACLink link;
	uint8_t ssap;
	uint8_t dsap;
    uint8_t *headerBuf;
    uint8_t headerBufLen;
    uint8_t sequence;

	static uint8_t SYMM_PDU[2];
};

#endif // __LLCP_H__
