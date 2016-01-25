#ifndef shimclient_h
#define shimclient_h

#include "Arduino.h"
#include "Client.h"
#include "IPAddress.h"
#include "Buffer.h"


/** ShimClient class */
class ShimClient : public Client {
private:
    Buffer* responseBuffer;
    Buffer* expectBuffer;
    bool _allowConnect;
    bool _connected;
    bool expectAnything;
    bool _error;
    uint16_t _received;
    IPAddress _expectedIP;
    uint16_t _expectedPort;
    const char* _expectedHost;
    
public:
  ShimClient();
  virtual int connect(IPAddress ip, uint16_t port); //!< connect
  virtual int connect(const char *host, uint16_t port); //!< connect
  virtual size_t write(uint8_t); //!< write
  virtual size_t write(const uint8_t *buf, size_t size); //!< write
  virtual int available(); //!< available
  virtual int read(); //!< read
  virtual int read(uint8_t *buf, size_t size); //!< read
  virtual int peek(); //!< peek
  virtual void flush(); //!< flush
  virtual void stop(); //!< stop
  virtual uint8_t connected(); //!< connected
  virtual operator bool(); //!< bool operator
  
  virtual ShimClient* respond(uint8_t *buf, size_t size); //!< respond
  virtual ShimClient* expect(uint8_t *buf, size_t size); //!< expect
  
  virtual void expectConnect(IPAddress ip, uint16_t port); //!< expectConnect
  virtual void expectConnect(const char *host, uint16_t port); //!< expectConnect
  
  virtual uint16_t received(); //!< received
  virtual bool error(); //!< error
  
  virtual void setAllowConnect(bool b); //!< setAllowConnect
  virtual void setConnected(bool b); //!< setConnected
};

#endif
