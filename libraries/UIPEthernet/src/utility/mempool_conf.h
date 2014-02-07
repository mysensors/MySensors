#ifndef MEMPOOLCONF_H
#define MEMPOOLCONF_H
#include "uipethernet-conf.h"
extern "C" {
  #include "uipopt.h"
}
#include <inttypes.h>

typedef uint16_t memaddress;
typedef uint8_t memhandle;

#if UIP_SOCKET_NUMPACKETS and UIP_CONNS
#define NUM_TCP_MEMBLOCKS (UIP_SOCKET_NUMPACKETS*2)*UIP_CONNS
#else
#define NUM_TCP_MEMBLOCKS 0
#endif

#if UIP_UDP and UIP_UDP_NUMPACKETS and UIP_UDP_CONNS
#define NUM_UDP_MEMBLOCKS (UIP_UDP_NUMPACKETS+1)*UIP_UDP_CONNS
#else
#define NUM_UDP_MEMBLOCKS 0
#endif

#define NUM_MEMBLOCKS (NUM_TCP_MEMBLOCKS+NUM_UDP_MEMBLOCKS)

#define MEMBLOCK_MV

#endif
