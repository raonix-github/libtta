#ifndef _TTA_PRIVATE_H_
#define _TTA_PRIVATE_H_

#ifndef _MSC_VER
# include <stdint.h>
# include <sys/time.h>
#else
# include "stdint.h"
# include <time.h>
typedef int ssize_t;
#endif
#include <sys/types.h>

#include "config.h"
#include "tta.h"

TTA_BEGIN_DECLS

/* It's not really the minimal length (the real one is report slave ID
 * in RS485 (4 bytes)) but it's a convenient size to use in RS485 or TCP
 * communications to read many values or write a single one.
 * Maximum between :
 * - HEADER_LENGTH_TCP (7) + function (1) + address (2) + number (2)
 * - HEADER_LENGTH_RS485 (1) + function (1) + address (2) + number (2) + CRC (2)
 */
#define _MIN_REQ_LENGTH 12

#define _REPORT_SLAVE_ID 180

#define _TTA_EXCEPTION_RSP_LENGTH 5

/* Timeouts in microsecond */
// #define _RESPONSE_TIMEOUT    15000
// #define _BYTE_TIMEOUT        5000
// TODO : check !!!!!!!!!!!!!!!!
#define _RESPONSE_TIMEOUT    500000
#define _BYTE_TIMEOUT        50000

/* Function codes */
#define _FC_READ_COILS                0x01
#define _FC_READ_DISCRETE_INPUTS      0x02
#define _FC_READ_HOLDING_REGISTERS    0x03
#define _FC_READ_INPUT_REGISTERS      0x04
#define _FC_WRITE_SINGLE_COIL         0x05
#define _FC_WRITE_SINGLE_REGISTER     0x06
#define _FC_READ_EXCEPTION_STATUS     0x07
#define _FC_WRITE_MULTIPLE_COILS      0x0F
#define _FC_WRITE_MULTIPLE_REGISTERS  0x10
#define _FC_REPORT_SLAVE_ID           0x11
#define _FC_WRITE_AND_READ_REGISTERS  0x17

typedef enum {
    _TTA_BACKEND_TYPE_TCP=0,
    _TTA_BACKEND_TYPE_RS485,
    _TTA_BACKEND_TYPE_RF
} tta_backend_type_t;

/*
 *  ---------- Request     Indication ----------
 *  | Client | ---------------------->| Server |
 *  ---------- Confirmation  Response ----------
 */
typedef enum {
    MSG_INDICATION,
    MSG_CONFIRMATION
} msg_type_t;

typedef struct _tta_backend
{
	unsigned int backend_type;
	unsigned int header_length;
	unsigned int checksum_length;
	unsigned int max_adu_length;
	int (*prepare_response_tid) (const uint8_t *msg, int *msg_length);
	int (*receive) (tta_t *ctx, uint8_t *req, int fgtimeout);
	int (*check_integrity) (tta_t *ctx, uint8_t *msg,
							const int msg_length);
    int (*pre_check_confirmation) (tta_t *ctx, const uint8_t *req,
							const uint8_t *rsp, int rsp_length);
	ttamsg_t *(*conv_pkt2ttamsg) (tta_t *ctx, uint8_t *pkt, int pktlen);
	int (*conv_ttamsg2pkt) (tta_t *ctx, ttamsg_t *ttamsg, uint8_t *pkt);
    int (*connect) (tta_t *ctx);
    void (*close) (tta_t *ctx);
    ssize_t (*send) (tta_t *ctx, const uint8_t *msg, int msg_length);
    ssize_t (*recv) (tta_t *ctx, uint8_t *rsp, int rsp_length);
    int (*flush) (tta_t *ctx);
    int (*select) (tta_t *ctx, fd_set *rset, struct timeval *tv, int msg_length);
} tta_backend_t;

struct _tta {
	int slave;

	tta_proto_t proto;
	int s;		/* Socket or file descriptor */
	int debug;
	unsigned int error_recovery;
	struct timeval response_timeout;
	struct timeval byte_timeout;
	const tta_backend_t *backend;
	void *backend_data;
};

void _tta_init_common(tta_t *ctx);
void _error_print(tta_t *ctx, const char *context);
int _tta_receive_pkt(tta_t *ctx, uint8_t *pkt, int fgtimeout);

#ifndef HAVE_STRLCPY
size_t strlcpy(char *dest, const char *src, size_t dest_size);
#endif

TTA_END_DECLS

#endif  /* _TTA_PRIVATE_H_ */
