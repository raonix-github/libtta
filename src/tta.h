#ifndef _TTA_H_
#define _TTA_H_

#include <stdint.h>
#include <sys/time.h>

#include "tta_version.h"

#if defined(_MSC_VER)
# if defined(DLLBUILD)
/* define DLLBUILD when building the DLL */
#  define EXPORT __declspec(dllexport)
# else
#  define EXPORT __declspec(dllimport)
# endif
#else
# define EXPORT
#endif

#ifdef  __cplusplus
# define TTA_BEGIN_DECLS  extern "C" {
# define TTA_END_DECLS    }
#else
# define TTA_BEGIN_DECLS
# define TTA_END_DECLS
#endif

TTA_BEGIN_DECLS

#ifndef FALSE
#define FALSE 0 
#endif          
	            
#ifndef TRUE
#define TRUE 1
#endif

#define TTA_HD_LEN				4

#define	TTA_ALIVE_TIMEOUT			(1000000*60)	// 60 secs
#define	TTA_RSP_TIMEOUT				(15000)			// 15 msecs
#define	TTA_BYTE_TIMEOUT			(5000)			// 5  msecs

typedef struct _tta tta_t;

typedef enum                                                                    
{
	TTA_PROTO_RS485,
	TTA_PROTO_RF,
	TTA_PROTO_TCP,
} tta_proto_t;

typedef struct
{
	uint8_t stx;
	uint8_t devid;
	uint8_t subid;
	uint8_t cmd;
	uint8_t datalen;
} __attribute__((packed)) ttamsg_rs485_hd_t;

typedef struct
{
	uint16_t tid;
	uint16_t len;
	uint8_t devid;
	uint8_t subid;
	uint8_t cmd;
	uint8_t datalen;
} __attribute__((packed)) ttamsg_tcp_hd_t;

typedef struct
{
	uint32_t id;			// rf id
	uint8_t devid;
	uint8_t subid;
	uint8_t cmd;
	uint8_t datalen;
} __attribute__((packed)) ttamsg_rf_hd_t;


typedef struct
{
	uint8_t devid;
	uint8_t subid;
	uint8_t cmd;
	uint8_t datalen;
} __attribute__((packed)) ttamsg_hd_t;

typedef struct _ttamsg ttamsg_t;                                          

struct _ttamsg
{   
	uint8_t *buf;
	ttamsg_hd_t *hd;
	uint8_t *data;                                                              

	// TCP
	int tid;                                                                    
	int len;                                                                    

	// RF
	uint16_t rfsid;

	tta_t *src;
	tta_t *dst;
};

/* Random number to avoid errno conflicts */
#define TTA_ENOBASE 20120516

/* Protocol exceptions */
enum {
	TTA_EXCEPTION_ILLEGAL_FUNCTION = 0x01,
	TTA_EXCEPTION_ILLEGAL_DATA_ADDRESS,
	TTA_EXCEPTION_ILLEGAL_DATA_VALUE,
	TTA_EXCEPTION_SLAVE_OR_SERVER_FAILURE,
	TTA_EXCEPTION_ACKNOWLEDGE,
	TTA_EXCEPTION_SLAVE_OR_SERVER_BUSY,
	TTA_EXCEPTION_NEGATIVE_ACKNOWLEDGE,
	TTA_EXCEPTION_MEMORY_PARITY,
	TTA_EXCEPTION_NOT_DEFINED,
	TTA_EXCEPTION_GATEWAY_PATH,
	TTA_EXCEPTION_GATEWAY_TARGET,
	TTA_EXCEPTION_MAX
};

#define EMBXILFUN  (TTA_ENOBASE + TTA_EXCEPTION_ILLEGAL_FUNCTION)
#define EMBXILADD  (TTA_ENOBASE + TTA_EXCEPTION_ILLEGAL_DATA_ADDRESS)
#define EMBXILVAL  (TTA_ENOBASE + TTA_EXCEPTION_ILLEGAL_DATA_VALUE)
#define EMBXSFAIL  (TTA_ENOBASE + TTA_EXCEPTION_SLAVE_OR_SERVER_FAILURE)
#define EMBXACK    (TTA_ENOBASE + TTA_EXCEPTION_ACKNOWLEDGE)
#define EMBXSBUSY  (TTA_ENOBASE + TTA_EXCEPTION_SLAVE_OR_SERVER_BUSY)
#define EMBXNACK   (TTA_ENOBASE + TTA_EXCEPTION_NEGATIVE_ACKNOWLEDGE)
#define EMBXMEMPAR (TTA_ENOBASE + TTA_EXCEPTION_MEMORY_PARITY)
#define EMBXGPATH  (TTA_ENOBASE + TTA_EXCEPTION_GATEWAY_PATH)
#define EMBXGTAR   (TTA_ENOBASE + TTA_EXCEPTION_GATEWAY_TARGET)

/* Native libtta error codes */
#define EMBBADCRC	(EMBXGTAR + 1)
#define EMBBADDATA	(EMBXGTAR + 2)
#define EMBBADEXC	(EMBXGTAR + 3)
#define EMBUNKEXC	(EMBXGTAR + 4)
#define EMBMDATA	(EMBXGTAR + 5)
#define EMBBADSLAVE	(EMBXGTAR + 6)

#define EMBBADRFID	(EMBXGTAR + 7)

extern const unsigned int libtta_version_major;
extern const unsigned int libtta_version_minor;
extern const unsigned int libtta_version_micro;


typedef enum
{   
	TTA_ERROR_RECOVERY_NONE          = 0,
	TTA_ERROR_RECOVERY_LINK          = (1<<1),
	TTA_ERROR_RECOVERY_PROTOCOL      = (1<<2),
} tta_error_recovery_mode;


EXPORT ttamsg_t *ttamsg_new(int len);
EXPORT void ttamsg_free(ttamsg_t *ttamsg);
EXPORT ttamsg_t *ttamsg_make(uint8_t devid, uint8_t subid, uint8_t cmd,
		uint8_t *data, uint8_t datalen);
EXPORT ttamsg_t *ttamsg_duplicate(ttamsg_t *org);
EXPORT void ttamsg_print(ttamsg_t *ttamsg);

EXPORT const char *tta_strerror(int errnum);

EXPORT int tta_set_error_recovery(tta_t *ctx, unsigned int error_recovery);
EXPORT void tta_set_socket(tta_t *ctx, int socket);
EXPORT int tta_get_socket(tta_t *ctx);
EXPORT void tta_get_response_timeout(tta_t *ctx, struct timeval *timeout);
EXPORT void tta_set_response_timeout(tta_t *ctx, const struct timeval *timeout);
EXPORT void tta_get_byte_timeout(tta_t *ctx, struct timeval *timeout);
EXPORT void tta_set_byte_timeout(tta_t *ctx, const struct timeval *timeout);
EXPORT void tta_set_debug(tta_t *ctx, int boolean);
EXPORT int tta_get_header_length(tta_t *ctx);

EXPORT void tta_free(tta_t *ctx);
EXPORT int tta_connect(tta_t *ctx);
EXPORT void tta_close(tta_t *ctx);
EXPORT int tta_flush(tta_t *ctx);
EXPORT int tta_send(tta_t *ctx, ttamsg_t *ttamsg );
EXPORT int tta_receive(tta_t *ctx, ttamsg_t **ttamsg, int fgtm);
EXPORT int tta_reply(tta_t *ctx, ttamsg_t *hareq, uint8_t *data, int datalen);
EXPORT int tta_request(tta_t *ctx, ttamsg_t *hareq, ttamsg_t **harsp);
EXPORT int tta_flush(tta_t *ctx);

EXPORT ttamsg_t *tta_conv_pkt2ttamsg(tta_t *ctx, uint8_t *pkt, int pktlen);
EXPORT int tta_conv_ttamsg2pkt(tta_t *ctx, ttamsg_t *ttamsg, uint8_t *pkt);

#include "tta_rf.h"
#include "tta_tcp.h"
#include "tta_rs485.h"

TTA_END_DECLS

#endif
