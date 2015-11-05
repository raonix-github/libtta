// #define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif
#include <signal.h>
#include <sys/types.h>

#if defined(_WIN32)
# define OS_WIN32
/* ws2_32.dll has getaddrinfo and freeaddrinfo on Windows XP and later.
 * minwg32 headers check WINVER before allowing the use of these */
# ifndef WINVER
# define WINVER 0x0501
# endif
# include <ws2tcpip.h>
# define SHUT_RDWR 2
# define close closesocket
#else
# include <sys/socket.h>
# include <sys/ioctl.h>

#if defined(__OpenBSD__) || (defined(__FreeBSD__) && __FreeBSD__ < 5)
# define OS_BSD
# include <netinet/in_systm.h>
#endif

# include <netinet/in.h>
# include <netinet/ip.h>
# include <netinet/tcp.h>
# include <arpa/inet.h>
# include <netdb.h>
#endif

#if !defined(MSG_NOSIGNAL)
#define MSG_NOSIGNAL 0
#endif

#include "tta_private.h"
#include "tta_tcp.h"
#include "tta_tcp_private.h"

#ifdef OS_WIN32
static int _tta_tcp_init_win32(void)
{
    /* Initialise Windows Socket API */
    WSADATA wsaData;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        fprintf(stderr, "WSAStartup() returned error code %d\n",
                (unsigned int)GetLastError());
        errno = EIO;
        return -1;
    }
    return 0;
}
#endif

static int _tta_tcp_prepare_response_tid(const uint8_t *pkt, int *pktlen)
{
    return (pkt[0] << 8) + pkt[1];
}

static ssize_t _tta_tcp_send(tta_t *ctx, const uint8_t *pkt, int pktlen)
{
    /* MSG_NOSIGNAL
       Requests not to send SIGPIPE on errors on stream oriented
       sockets when the other end breaks the connection.  The EPIPE
       error is still returned. */
    return send(ctx->s, (const char*)pkt, pktlen, MSG_NOSIGNAL);
}

static int _tta_tcp_receive(tta_t *ctx, uint8_t *pkt, int fgtimeout)
{
    return _tta_receive_pkt(ctx, pkt, fgtimeout);
}

static ssize_t _tta_tcp_recv(tta_t *ctx, uint8_t *pkt, int pktlen)
{
    return recv(ctx->s, (char *)pkt, pktlen, 0);
}

static int _tta_tcp_check_integrity(tta_t *ctx, uint8_t *pkt, const int pktlen)
{
    return pktlen;
}

static int _tta_tcp_pre_check_confirmation(tta_t *ctx, const uint8_t *req,
                                       const uint8_t *rsp, int rsp_length)
{
    /* Check TID */
    if (req[0] != rsp[0] || req[1] != rsp[1]) {
        if (ctx->debug) {
            fprintf(stderr, "Invalid TID received 0x%X (not 0x%X)\n",
                    (rsp[0] << 8) + rsp[1], (req[0] << 8) + req[1]);
        }
        errno = EMBBADDATA;
        return -1;
    } else {
        return 0;
    }
}

static ttamsg_t *_tta_tcp_conv_pkt2ttamsg(tta_t *ctx, uint8_t *pkt, int pktlen)
{
	ttamsg_t *ttamsg;
	ttamsg_tcp_hd_t *hd; 
	int len;

	hd = (ttamsg_tcp_hd_t *)pkt;
	len = sizeof(ttamsg_hd_t)+hd->datalen;

	ttamsg = malloc(sizeof(ttamsg_t));
	ttamsg->tid = ntohs(hd->tid);
	ttamsg->len = len;
	ttamsg->buf = malloc(len);
	memcpy(ttamsg->buf, pkt+_TTA_TCP_PREHEADER_LENGTH, len);
	ttamsg->hd = (ttamsg_hd_t *)ttamsg->buf;
	ttamsg->data = ttamsg->buf+sizeof(ttamsg_hd_t);;
	ttamsg->src = ctx;

	return ttamsg;
}

static int _tta_tcp_conv_ttamsg2pkt(tta_t *ctx, ttamsg_t *ttamsg, uint8_t *pkt)
{
    tta_tcp_t *ctx_tcp = ctx->backend_data;
	ttamsg_hd_t *hd;
	int i, datalen;

    /* Increase transaction ID */
    if (ctx_tcp->tid < UINT16_MAX)
        ctx_tcp->tid++;
    else
        ctx_tcp->tid = 0;

	hd = ttamsg->hd;
    datalen = hd->datalen;

    pkt[0] = ctx_tcp->tid >> 8;
    pkt[1] = ctx_tcp->tid & 0x00ff;
	pkt[2] = 0;
	pkt[3] = 0;
    pkt[4] = hd->devid;
    pkt[5] = hd->subid;
    pkt[6] = hd->cmd;
    pkt[7] = datalen;
	for(i=0; i<datalen; i++)
	{
		pkt[ctx->backend->header_length+i] = ttamsg->data[i];
	}

	ttamsg_free(ttamsg);

	return (ctx->backend->header_length+datalen+ctx->backend->checksum_length);
}

static int _tta_tcp_set_ipv4_options(int s)
{
    int rc;
    int option;

    /* Set the TCP no delay flag */
    /* SOL_TCP = IPPROTO_TCP */
    option = 1;
    rc = setsockopt(s, IPPROTO_TCP, TCP_NODELAY,
                    (const void *)&option, sizeof(int));
    if (rc == -1) {
        return -1;
    }

#ifndef OS_WIN32
    /**
     * Cygwin defines IPTOS_LOWDELAY but can't handle that flag so it's
     * necessary to workaround that problem.
     **/
    /* Set the IP low delay option */
    option = IPTOS_LOWDELAY;
    rc = setsockopt(s, IPPROTO_IP, IP_TOS,
                    (const void *)&option, sizeof(int));
    if (rc == -1) {
        return -1;
    }
#endif

    return 0;
}

static int _connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen,
                    struct timeval *tv)
{
    int rc;

    rc = connect(sockfd, addr, addrlen);
    if (rc == -1 && errno == EINPROGRESS) {
        fd_set wset;
        int optval;
        socklen_t optlen = sizeof(optval);

        /* Wait to be available in writing */
        FD_ZERO(&wset);
        FD_SET(sockfd, &wset);
        rc = select(sockfd + 1, NULL, &wset, NULL, tv);
        if (rc <= 0) {
            /* Timeout or fail */
            return -1;
        }

        /* The connection is established if SO_ERROR and optval are set to 0 */
        rc = getsockopt(sockfd, SOL_SOCKET, SO_ERROR, (void *)&optval, &optlen);
        if (rc == 0 && optval == 0) {
            return 0;
        } else {
            errno = ECONNREFUSED;
            return -1;
        }
    }
    return rc;
}

/* Establishes a tta TCP connection with a TTA server. */
static int _tta_tcp_connect(tta_t *ctx)
{
    int rc;
    /* Specialized version of sockaddr for Internet socket address (same size) */
    struct sockaddr_in addr;
    tta_tcp_t *ctx_tcp = ctx->backend_data;
    int flags = SOCK_STREAM;

#ifdef OS_WIN32
    if (_tta_tcp_init_win32() == -1) {
        return -1;
    }
#endif

#ifdef SOCK_CLOEXEC
    flags |= SOCK_CLOEXEC;
#endif

#ifdef SOCK_NONBLOCK
    flags |= SOCK_NONBLOCK;
#endif

    ctx->s = socket(PF_INET, flags, 0);
    if (ctx->s == -1) {
        return -1;
    }

    rc = _tta_tcp_set_ipv4_options(ctx->s);
    if (rc == -1) {
        close(ctx->s);
        return -1;
    }

    if (ctx->debug) {
        printf("Connecting to %s:%d\n", ctx_tcp->ip, ctx_tcp->port);
    }

    addr.sin_family = AF_INET;
    addr.sin_port = htons(ctx_tcp->port);
    addr.sin_addr.s_addr = inet_addr(ctx_tcp->ip);
    rc = _connect(ctx->s, (struct sockaddr *)&addr, sizeof(addr), &ctx->response_timeout);
    if (rc == -1) {
        close(ctx->s);
        return -1;
    }

    return 0;
}

/* Closes the network connection and socket in TCP mode */
static void _tta_tcp_close(tta_t *ctx)
{
    shutdown(ctx->s, SHUT_RDWR);
    close(ctx->s);
}

static int _tta_tcp_flush(tta_t *ctx)
{
    int rc;
    int rc_sum = 0;

    do {
        /* Extract the garbage from the socket */
        char devnull[TTA_TCP_MAX_ADU_LENGTH];
#ifndef OS_WIN32
        rc = recv(ctx->s, devnull, TTA_TCP_MAX_ADU_LENGTH, MSG_DONTWAIT);
#else
        /* On Win32, it's a bit more complicated to not wait */
        fd_set rset;
        struct timeval tv;

        tv.tv_sec = 0;
        tv.tv_usec = 0;
        FD_ZERO(&rset);
        FD_SET(ctx->s, &rset);
        rc = select(ctx->s+1, &rset, NULL, NULL, &tv);
        if (rc == -1) {
            return -1;
        }

        if (rc == 1) {
            /* There is data to flush */
            rc = recv(ctx->s, devnull, TTA_TCP_MAX_ADU_LENGTH, 0);
        }
#endif
        if (rc > 0) {
            rc_sum += rc;
        }
    } while (rc == TTA_TCP_MAX_ADU_LENGTH);

    return rc_sum;
}

/* Listens for any request from one or many tta masters in TCP */
int tta_tcp_listen(tta_t *ctx, int nb_connection)
{
    int new_socket;
    int yes;
    struct sockaddr_in addr;
    tta_tcp_t *ctx_tcp = ctx->backend_data;

#ifdef OS_WIN32
    if (_tta_tcp_init_win32() == -1) {
        return -1;
    }
#endif

    new_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (new_socket == -1) {
        return -1;
    }

    yes = 1;
    if (setsockopt(new_socket, SOL_SOCKET, SO_REUSEADDR,
                   (char *) &yes, sizeof(yes)) == -1) {
        close(new_socket);
        return -1;
    }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    /* If the tta port is < to 1024, we need the setuid root. */
    addr.sin_port = htons(ctx_tcp->port);
    addr.sin_addr.s_addr = INADDR_ANY;
    if (bind(new_socket, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        close(new_socket);
        return -1;
    }

    if (listen(new_socket, nb_connection) == -1) {
        close(new_socket);
        return -1;
    }

    return new_socket;
}

/* On success, the function return a non-negative integer that is a descriptor
   for the accepted socket. On error, -1 is returned, and errno is set
   appropriately. */
int tta_tcp_accept(tta_t *ctx, int *s)
{
    struct sockaddr_in addr;
    socklen_t addrlen;

    addrlen = sizeof(addr);
#ifdef HAVE_ACCEPT4
    /* Inherit socket flags and use accept4 call */
    ctx->s = accept4(*s, (struct sockaddr *)&addr, &addrlen, SOCK_CLOEXEC);
#else
    ctx->s = accept(*s, (struct sockaddr *)&addr, &addrlen);
#endif

    if (ctx->s == -1) {
        close(*s);
        *s = 0;
        return -1;
    }

    if (ctx->debug) {
        printf("The client connection from %s is accepted\n",
               inet_ntoa(addr.sin_addr));
    }

    return ctx->s;
}

static int _tta_tcp_select(tta_t *ctx, fd_set *rset, struct timeval *tv, int length_to_read)
{
    int s_rc;
    while ((s_rc = select(ctx->s+1, rset, NULL, NULL, tv)) == -1) {
        if (errno == EINTR) {
            if (ctx->debug) {
                fprintf(stderr, "A non blocked signal was caught\n");
            }
            /* Necessary after an error */
            FD_ZERO(rset);
            FD_SET(ctx->s, rset);
        } else {
            return -1;
        }
    }

    if (s_rc == 0) {
        errno = ETIMEDOUT;
        return -1;
    }

    return s_rc;
}

const tta_backend_t _tta_tcp_backend = {
    _TTA_BACKEND_TYPE_TCP,
    _TTA_TCP_HEADER_LENGTH,
    _TTA_TCP_CHECKSUM_LENGTH,
    TTA_TCP_MAX_ADU_LENGTH,

    _tta_tcp_prepare_response_tid,

    _tta_tcp_receive,
    _tta_tcp_check_integrity,
    _tta_tcp_pre_check_confirmation,
	_tta_tcp_conv_pkt2ttamsg,
	_tta_tcp_conv_ttamsg2pkt,

    _tta_tcp_connect,
    _tta_tcp_close,
    _tta_tcp_send,
    _tta_tcp_recv,
    _tta_tcp_flush,
    _tta_tcp_select
};

tta_t* tta_new_tcp(const char *ip, int port)
{
    tta_t *ctx;
    tta_tcp_t *ctx_tcp;
    size_t dest_size;
    size_t ret_size;

#if defined(OS_BSD)
    /* MSG_NOSIGNAL is unsupported on *BSD so we install an ignore
       handler for SIGPIPE. */
    struct sigaction sa;

    sa.sa_handler = SIG_IGN;
    if (sigaction(SIGPIPE, &sa, NULL) < 0) {
        /* The debug flag can't be set here... */
        fprintf(stderr, "Coud not install SIGPIPE handler.\n");
        return NULL;
    }
#endif

    ctx = (tta_t *) malloc(sizeof(tta_t));
    _tta_init_common(ctx);

	ctx->proto = TTA_PROTO_RS485;
    ctx->backend = &(_tta_tcp_backend);
    ctx->backend_data = (tta_tcp_t *) malloc(sizeof(tta_tcp_t));
    ctx_tcp = (tta_tcp_t *)ctx->backend_data;

	if(ip)
	{
		dest_size = sizeof(char) * 16;
		ret_size = strlcpy(ctx_tcp->ip, ip, dest_size);
		if (ret_size == 0) {
			fprintf(stderr, "The IP string is empty\n");
			tta_free(ctx);
			errno = EINVAL;
			return NULL;
		}

		if (ret_size >= dest_size) {
			fprintf(stderr, "The IP string has been truncated\n");
			tta_free(ctx);
			errno = EINVAL;
			return NULL;
		}
		ctx_tcp->port = port;
	}

    ctx_tcp->tid = 0;

    return ctx;
}

int tta_tcp_set_ip(tta_t *ctx, const char *ip, int port)
{
    tta_tcp_t *ctx_tcp;
    size_t dest_size;
    size_t ret_size;

    ctx_tcp = (tta_tcp_t *)ctx->backend_data;

	dest_size = sizeof(char) * 16;
	ret_size = strlcpy(ctx_tcp->ip, ip, dest_size);
	if (ret_size == 0) {
		fprintf(stderr, "The IP string is empty\n");
		errno = EINVAL;
		return -1;
	}

	if (ret_size >= dest_size) {
		fprintf(stderr, "The IP string has been truncated\n");
		errno = EINVAL;
		return -1;
	}
	ctx_tcp->port = port;

	return 0;
}
