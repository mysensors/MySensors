#ifndef UIPETHERNET_CONF_H
#define UIPETHERNET_CONF_H

/* for TCP */
#define UIP_SOCKET_NUMPACKETS    5
#define UIP_CONF_MAX_CONNECTIONS 4

/* for UDP */
#define UIP_CONF_UDP             0
#define UIP_CONF_BROADCAST       1
#define UIP_CONF_UDP_CONNS       4

/* number of attempts on write before returning number of bytes sent so far
 * set to -1 to block until connection is closed by timeout */
#define UIP_ATTEMPTS_ON_WRITE    -1

/* timeout after which UIPClient::connect gives up. The timeout is specified in seconds.
 * if set to a number <= 0 connect will timeout when uIP does (which might be longer than you expect...) */
#define UIP_CONNECT_TIMEOUT      -1

#endif
