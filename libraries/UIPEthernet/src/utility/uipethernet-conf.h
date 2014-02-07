#ifndef UIPETHERNET_CONF_H
#define UIPETHERNET_CONF_H

/* for TCP */
#define UIP_SOCKET_NUMPACKETS    5
#define UIP_CONF_MAX_CONNECTIONS 4

/* for UDP */
#define UIP_CONF_UDP             0//1
#define UIP_CONF_BROADCAST       0//1
#define UIP_CONF_UDP_CONNS       0//4
#define UIP_UDP_NUMPACKETS       0//5

/* number of attempts on write before returning number of bytes sent so far
 * set to -1 to block until connection is closed by timeout */
#define UIP_ATTEMPTS_ON_WRITE    -1

#endif
