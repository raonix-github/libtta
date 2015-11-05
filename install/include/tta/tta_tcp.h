#ifndef _TTA_TCP_H_
#define _TTA_TCP_H_

#include "tta.h"

TTA_BEGIN_DECLS

#if defined(_WIN32) && !defined(__CYGWIN__)
/* Win32 with MinGW, supplement to <errno.h> */
#include <winsock2.h>
#define ECONNRESET   WSAECONNRESET
#define ECONNREFUSED WSAECONNREFUSED
#define ETIMEDOUT    WSAETIMEDOUT
#define ENOPROTOOPT  WSAENOPROTOOPT
#endif

#define TTA_TCP_DEFAULT_PORT		502
#define TTA_TCP_MAX_ADU_LENGTH	260


tta_t* tta_new_tcp(const char *ip_address, int port);
int tta_tcp_listen(tta_t *ctx, int nb_connection);
int tta_tcp_accept(tta_t *ctx, int *socket);
int tta_tcp_set_ip(tta_t *ctx, const char *ip, int port);

TTA_END_DECLS

#endif /* _TTA_TCP_H_ */
