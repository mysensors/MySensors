#ifndef client_h
#define client_h
#include "IPAddress.h"

/** Client class */
class Client {
public:
  virtual int connect(IPAddress ip, uint16_t port) =0; //!< connect
  virtual int connect(const char *host, uint16_t port) =0; //!< connect
  virtual size_t write(uint8_t) =0; //!< write
  virtual size_t write(const uint8_t *buf, size_t size) =0; //!< write
  virtual int available() = 0; //!< available
  virtual int read() = 0; //!< read
  virtual int read(uint8_t *buf, size_t size) = 0; //!< read
  virtual int peek() = 0; //!< peek
  virtual void flush() = 0; //!< flush
  virtual void stop() = 0; //!< stop
  virtual uint8_t connected() = 0; //!< connected
  virtual operator bool() = 0; //!< bool operator
};

#endif
