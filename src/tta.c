#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <limits.h>
#include <time.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif

#define DEBUG
#include "dbg.h"
#include "config.h"
#include "tta.h"
#include "tta_private.h"

#define DUMP_PKT

#define min(x,y)		((x) < (y) ? (x) : (y))

/* Internal use */
#define MSG_LENGTH_UNDEFINED -1

/* Exported version */
const unsigned int libtta_version_major = LIBTTA_VERSION_MAJOR;
const unsigned int libtta_version_minor = LIBTTA_VERSION_MINOR;
const unsigned int libtta_version_micro = LIBTTA_VERSION_MICRO;

/* Max between RS485 and TCP max adu length (so TCP) */
#define MAX_MESSAGE_LENGTH 260

/* 3 steps are used to parse the query */
typedef enum
{
	_STEP_HD,
	_STEP_DATA,
	_STEP_TAIL
} _step_t;

const char *tta_strerror(int errnum)
{
	switch (errnum) {
	case EMBXILFUN:
		return "Illegal function";
	case EMBXILADD:
		return "Illegal data address";
	case EMBXILVAL:
		return "Illegal data value";
	case EMBXSFAIL:
		return "Slave device or server failure";
	case EMBXACK:
		return "Acknowledge";
	case EMBXSBUSY:
		return "Slave device or server is busy";
	case EMBXNACK:
		return "Negative acknowledge";
	case EMBXMEMPAR:
		return "Memory parity error";
	case EMBXGPATH:
		return "Gateway path unavailable";
	case EMBXGTAR:
		return "Target device failed to respond";
	case EMBBADCRC:
		return "Invalid CRC";
	case EMBBADDATA:
		return "Invalid data";
	case EMBBADEXC:
		return "Invalid exception code";
	case EMBMDATA:
		return "Too many data";
	case EMBBADSLAVE:
		return "Response not from requested slave";
	default:
		return strerror(errnum);
	}
}

void _error_print(tta_t *ctx, const char *context)
{
	if (ctx->debug) {
		// fprintf(stderr, "ERROR %s", tta_strerror(errno));
		printf("ERROR %s", tta_strerror(errno));
		if (context != NULL) {
			// fprintf(stderr, ": %s\n", context);
			printf(": %s\n", context);
		} else {
			// fprintf(stderr, "\n");
			printf("\n");
		}
	}
}

static int _sleep_and_flush(tta_t *ctx)
{
#ifdef _WIN32
	/* usleep doesn't exist on Windows */
	Sleep((ctx->response_timeout.tv_sec * 1000) +
			(ctx->response_timeout.tv_usec / 1000));
#else
	/* usleep source code */
	struct timespec request, remaining;
	request.tv_sec = ctx->response_timeout.tv_sec;
	request.tv_nsec = ((long int)ctx->response_timeout.tv_usec % 1000000)
		* 1000;
	while (nanosleep(&request, &remaining) == -1 && errno == EINTR)
		request = remaining;
#endif
	return tta_flush(ctx);
}

/*
 *  ---------- Request     Indication ----------
 *  | Client | ---------------------->| Server |
 *  ---------- Confirmation  Response ----------
 */

/* Computes the date length to read */
static int compute_data_length_after_hd(tta_t *ctx, uint8_t *pkt)
{
	const int hdlen = ctx->backend->header_length;
	int len;

	len = pkt[hdlen-1];
	len += ctx->backend->checksum_length;

	return len;
}

static int _send_pkt(tta_t *ctx, uint8_t *pkt, int pktlen)
{
	int rc;

#ifdef DUMP_PKT
	int i;
	if (ctx->debug)
	{
		for (i = 0; i < pktlen; i++)
		{
			printf("<%.2X>", pkt[i]); fflush(stdout);
		}
		printf("\n");
	}
#endif

	/* In recovery mode, the write command will be issued until to be
	   successful! Disabled by default. */
	do {
		rc = ctx->backend->send(ctx, pkt, pktlen);
		if (rc == -1) {
			_error_print(ctx, NULL);
			if (ctx->error_recovery & TTA_ERROR_RECOVERY_LINK) {
				int saved_errno = errno;

				if ((errno == EBADF || errno == ECONNRESET || errno == EPIPE)) {
					tta_close(ctx);
					tta_connect(ctx);
				} else {
					_sleep_and_flush(ctx);
				}
				errno = saved_errno;
			}
		}
	} while ((ctx->error_recovery & TTA_ERROR_RECOVERY_LINK) &&
			rc == -1);

	if (rc > 0 && rc != pktlen)
	{
		errno = EMBBADDATA;
		return -1;
	}

	return rc;
}


/* Waits a response from a tta server or a request from a tta client.
   This function blocks if there is no replies (3 timeouts).

   The function shall return the number of received characters and the received
   message in an array of uint8_t if successful. Otherwise it shall return -1
   and errno is set to one of the values defined below:
   - ECONNRESET
   - EMBBADDATA
   - EMBUNKEXC
   - ETIMEDOUT
   - read() or recv() error codes
   */
int _tta_receive_pkt(tta_t *ctx, uint8_t *pkt, int fgtimeout)
{
	fd_set rfds;
	struct timeval tv;
	struct timeval *p_tv;
	int length_to_read;
	int pktlen = 0;
	_step_t step;
	int rc;

	if (ctx->debug) {
		if (fgtimeout) {
			printf("Waiting until timeout...\n");
		} else {
			printf("Waiting without timeout...\n");
		}
	}

	/* Add a file descriptor to the set */
	FD_ZERO(&rfds);
	FD_SET(ctx->s, &rfds);

	step = _STEP_HD;
	length_to_read = ctx->backend->header_length;

	if (fgtimeout) {
		tv.tv_sec = ctx->response_timeout.tv_sec;
		tv.tv_usec = ctx->response_timeout.tv_usec;
		p_tv = &tv;
	} else {
		/* Wait for a message, we don't know when the message will be
		 * received */
		p_tv = NULL;
	}

	while (length_to_read != 0)
	{
		rc = ctx->backend->select(ctx, &rfds, p_tv, length_to_read);
		if (rc == -1)
		{
			_error_print(ctx, "select");
			if (ctx->error_recovery & TTA_ERROR_RECOVERY_LINK)
			{
				int saved_errno = errno;

				if (errno == ETIMEDOUT)
				{
					_sleep_and_flush(ctx);
				}
				else if (errno == EBADF)
				{
					tta_close(ctx);
					tta_connect(ctx);
				}
				errno = saved_errno;
			}
			return -1;
		}

		rc = ctx->backend->recv(ctx, pkt+pktlen, length_to_read);
		if (rc == 0)
		{
			errno = ECONNRESET;
			rc = -1;
		}

		if (rc == -1)
		{
			_error_print(ctx, "read");
			if ((ctx->error_recovery & TTA_ERROR_RECOVERY_LINK) &&
					(errno == ECONNRESET || errno == ECONNREFUSED ||
					 errno == EBADF))
			{
				int saved_errno = errno;

				tta_close(ctx);
				tta_connect(ctx);
				/* Could be removed by previous calls */
				errno = saved_errno;
			}
			return -1;
		}

		/* Display the hex code of each character received */
#ifdef DUMP_PKT
		if (ctx->debug)
		{
			int i;
			for (i=0; i < rc; i++)
			{
				printf("[%.2X]", pkt[pktlen + i]); fflush(stdout);
			}

		}
#endif
		/* Sums bytes received */
		pktlen += rc;
		/* Computes remaining bytes */
		length_to_read -= rc;

		if (length_to_read == 0)
		{
			switch (step)
			{
				case _STEP_HD:
					/* Function code position */
					length_to_read = compute_data_length_after_hd(ctx, pkt);
					if((pktlen+length_to_read) > (int)ctx->backend->max_adu_length)
					{
						errno = EMBBADDATA;
						_error_print(ctx, "too many data");
						return -1;
					}
					step = _STEP_DATA;
					break;
				case _STEP_DATA:
				case _STEP_TAIL:
				default:
					break;
			}
		}

		if (length_to_read > 0)
		{
			/* If there is no character in the buffer, the allowed timeout
			   interval between two consecutive bytes is defined by
			   byte_timeout */
			tv.tv_sec = ctx->byte_timeout.tv_sec;
			tv.tv_usec = ctx->byte_timeout.tv_usec;
			p_tv = &tv;
		}
	}

#ifdef DUMP_PKT
	/* Display the hex code of each character received */
	if (ctx->debug)
		printf("\n");
#endif

	return ctx->backend->check_integrity(ctx, pkt, pktlen);
}

static int check_confirmation(tta_t *ctx, uint8_t *req,
		uint8_t *buf, int rsp_length)
{
	int rc = 0;

	if (ctx->backend->pre_check_confirmation) {
		rc = ctx->backend->pre_check_confirmation(ctx, req, buf, rsp_length);
		if (rc == -1) {
			if (ctx->error_recovery & TTA_ERROR_RECOVERY_PROTOCOL) {
				_sleep_and_flush(ctx);
			}
			return -1;
		}
	}

#if 0
	/* Exception code */
	if (function >= 0x80) {
		if (rsp_length == (offset + 2 + ctx->backend->checksum_length) &&
				req[offset] == (buf[offset] - 0x80)) {
			/* Valid exception code received */

			int exception_code = buf[offset + 1];
			if (exception_code < TTA_EXCEPTION_MAX) {
				errno = TTA_ENOBASE + exception_code;
			} else {
				errno = EMBBADEXC;
			}
			_error_print(ctx, NULL);
			return -1;
		} else {
			errno = EMBBADEXC;
			_error_print(ctx, NULL);
			return -1;
		}
	}
#endif

	return rc;
}


#if 0
int tta_reply_exception(tta_t *ctx, const uint8_t *req,
		unsigned int exception_code)
{
	int hdlen = ctx->backend->header_length;
	uint8_t buf[MAX_MESSAGE_LENGTH];
	int rsp_length;
	int dummy_length = 99;
	msg_info_t minfo;

	minfo.devid = req[hdlen-4];
	minfo.subid = req[hdlen-3];
	minfo.cmd = req[hdlen-2];
	minfo.tid = ctx->backend->prepare_response_tid(req, &dummy_length);
	rsp_length = ctx->backend->build_response_basis(&minfo, (uint8_t *)&exception_code,
			sizeof(exception_code), buf);

	/* Positive exception code */
	if (exception_code < TTA_EXCEPTION_MAX) {
		return _send_pkt(ctx, buf, rsp_length);
	} else {
		errno = EINVAL;
		return -1;
	}
}
#endif

void _tta_init_common(tta_t *ctx)
{
	/* Slave and socket are initialized to -1 */
	ctx->slave = -1;
	ctx->s = -1;

	ctx->debug = FALSE;
	ctx->error_recovery = TTA_ERROR_RECOVERY_NONE;

	ctx->response_timeout.tv_sec = 0;
	ctx->response_timeout.tv_usec = _RESPONSE_TIMEOUT;

	ctx->byte_timeout.tv_sec = 0;
	ctx->byte_timeout.tv_usec = _BYTE_TIMEOUT;
}

/* -----------------------------------------------------------------------------
 *
 * TTA Message API
 *
 * ---------------------------------------------------------------------------*/
ttamsg_t *ttamsg_new(int len)
{
	ttamsg_t *ttamsg;

	ttamsg = malloc(sizeof(ttamsg_t));
	ttamsg->tid = 0;
	ttamsg->len = len;
	ttamsg->buf = malloc(len);
	ttamsg->hd = (ttamsg_hd_t *)ttamsg->buf;
	ttamsg->data = ttamsg->buf+sizeof(ttamsg_hd_t);;

	return ttamsg;
}

void ttamsg_free(ttamsg_t *ttamsg)
{
	if(ttamsg->buf)
		free(ttamsg->buf);

	free(ttamsg);
}

ttamsg_t *ttamsg_make(uint8_t devid, uint8_t subid, uint8_t cmd,
		uint8_t *data, uint8_t datalen)
{
	ttamsg_t *ttamsg;
	int len;

	len = sizeof(ttamsg_hd_t)+datalen;
	ttamsg = ttamsg_new(len);
	ttamsg->len = len;
	ttamsg->hd->devid = devid;
	ttamsg->hd->subid = subid;
	ttamsg->hd->cmd = cmd;
	ttamsg->hd->datalen = datalen;
	if(datalen)
		memcpy(ttamsg->data, data, datalen);

	return ttamsg;
}

ttamsg_t *ttamsg_duplicate(ttamsg_t *org)
{
	ttamsg_t *ttamsg;

	ttamsg = ttamsg_new(org->len);
	memcpy(ttamsg->buf, org->buf, org->len);

	return ttamsg;
}

void ttamsg_print(ttamsg_t *ttamsg)
{
	int i;

	printf("TTA Message\n");
	printf("  -----------------------------------\n");
	printf("  len:      %d\n", ttamsg->len);
	printf("  tid:      %d\n", ttamsg->tid);
	printf("  -----------------------------------\n");
	printf("  devid:    %.2x\n",   ttamsg->hd->devid);
	printf("  subid:    %.2x\n",   ttamsg->hd->subid);
	printf("  cmd:      %.2x\n",   ttamsg->hd->cmd);
	printf("  datalen:  %.2x\n",   ttamsg->hd->datalen);
	if(ttamsg->hd->datalen)
	{
		printf("  data:\n");
		for (i=0; i < ttamsg->hd->datalen; i++)
		{
			printf("[%.2X]", ttamsg->data[i]);
			if(i%8 == 7)
			{
				printf("\n");
			}
		}
		if(i%8 != 0) printf("\n");
	}
	printf("  -----------------------------------\n");
}

/* -----------------------------------------------------------------------------
 *
 * TTA API
 * 
 * ---------------------------------------------------------------------------*/
int tta_set_error_recovery(tta_t *ctx, unsigned int error_recovery)
{
	/* The type of tta_error_recovery_mode is unsigned enum */
	ctx->error_recovery = error_recovery;
	return 0;
}

void tta_set_socket(tta_t *ctx, int socket)
{
	ctx->s = socket;
}

int tta_get_socket(tta_t *ctx)
{
	return ctx->s;
}

/* Get the timeout interval used to wait for a response */
void tta_get_response_timeout(tta_t *ctx, struct timeval *timeout)
{
	*timeout = ctx->response_timeout;
}

void tta_set_response_timeout(tta_t *ctx, const struct timeval *timeout)
{
	ctx->response_timeout = *timeout;
}

/* Get the timeout interval between two consecutive bytes of a message */
void tta_get_byte_timeout(tta_t *ctx, struct timeval *timeout)
{
	*timeout = ctx->byte_timeout;
}

void tta_set_byte_timeout(tta_t *ctx, const struct timeval *timeout)
{
	ctx->byte_timeout = *timeout;
}

int tta_get_header_length(tta_t *ctx)
{
	return ctx->backend->header_length;
}

int tta_connect(tta_t *ctx)
{
	return ctx->backend->connect(ctx);
}

void tta_close(tta_t *ctx)
{
	if (ctx == NULL)
		return;

	ctx->backend->close(ctx);
}

void tta_free(tta_t *ctx)
{
	if (ctx == NULL)
		return;

	free(ctx->backend_data);
	free(ctx);
}

void tta_set_debug(tta_t *ctx, int boolean)
{
	ctx->debug = boolean;
}

ttamsg_t *tta_conv_pkt2ttamsg(tta_t *ctx, uint8_t *pkt, int pktlen)
{
	return ctx->backend->conv_pkt2ttamsg(ctx, pkt, pktlen);
}

int tta_conv_ttamsg2pkt(tta_t *ctx, ttamsg_t *ttamsg, uint8_t *pkt)
{
	return ctx->backend->conv_ttamsg2pkt(ctx, ttamsg, pkt);
}

int tta_flush(tta_t *ctx)
{
	int rc = ctx->backend->flush(ctx);
	if (rc != -1 && ctx->debug)
	{
		/* Not all backends are able to return the number of bytes flushed */
		printf("Bytes flushed (%d)\n", rc);
	}
	return rc;
}

int tta_send(tta_t *ctx, ttamsg_t *ttamsg )
{
	uint8_t buf[MAX_MESSAGE_LENGTH];
	int buflen;
	int rc;

	buflen = ctx->backend->conv_ttamsg2pkt(ctx, ttamsg, buf);

	rc = _send_pkt(ctx, buf, buflen);
	if (rc == -1)
		return -1;

	return rc;
}

int tta_receive(tta_t *ctx, ttamsg_t **ttamsg, int fgtm)
{
	uint8_t buf[MAX_MESSAGE_LENGTH];
	int rc;

	rc = ctx->backend->receive(ctx, buf, fgtm);
	if (rc == -1)
		return -1;

	*ttamsg = ctx->backend->conv_pkt2ttamsg(ctx, buf, rc);

	return rc;
}

int tta_reply(tta_t *ctx, ttamsg_t *ttareq, uint8_t *data, int datalen)
{
	ttamsg_t *ttarsp;
	ttamsg_hd_t *hd;
	int rc;

	hd = ttareq->hd;
	ttarsp = ttamsg_make(hd->devid, hd->subid, hd->cmd | 0x80, data, datalen);
	rc = tta_send(ctx, ttarsp);

	return rc;
}

// Send request pakcet & recevie reply packet
int tta_request(tta_t *ctx, ttamsg_t *ttareq, ttamsg_t **ttarsp)
{
	uint8_t reqbuf[MAX_MESSAGE_LENGTH];
	uint8_t rspbuf[MAX_MESSAGE_LENGTH];
	int reqlen, rsplen;
	int rc;

	reqlen = ctx->backend->conv_ttamsg2pkt(ctx, ttareq, reqbuf);

	rc = _send_pkt(ctx, reqbuf, reqlen);
	if (rc == -1)
		return -1;

	rsplen = ctx->backend->receive(ctx, rspbuf, 1);
	if (rsplen == -1)
		return -1;

	rc = check_confirmation(ctx, reqbuf, rspbuf, rsplen);
	if (rc == -1)
		return -1;

	*ttarsp = ctx->backend->conv_pkt2ttamsg(ctx, rspbuf, rsplen);

	return rc;
}

/* -------------------------------------------------------------------------- */

#ifndef HAVE_STRLCPY
/*
 * Function strlcpy was originally developed by
 * Todd C. Miller <Todd.Miller@courtesan.com> to simplify writing secure code.
 * See ftp://ftp.openbsd.org/pub/OpenBSD/src/lib/libc/string/strlcpy.3
 * for more information.
 *
 * Thank you Ulrich Drepper... not!
 *
 * Copy src to string dest of size dest_size.  At most dest_size-1 characters
 * will be copied.  Always NUL terminates (unless dest_size == 0).  Returns
 * strlen(src); if retval >= dest_size, truncation occurred.
 */
size_t strlcpy(char *dest, const char *src, size_t dest_size)
{
	register char *d = dest;
	register const char *s = src;
	register size_t n = dest_size;

	/* Copy as many bytes as will fit */
	if (n != 0 && --n != 0) {
		do {
			if ((*d++ = *s++) == 0)
				break;
		} while (--n != 0);
	}

	/* Not enough room in dest, add NUL and traverse rest of src */
	if (n == 0) {
		if (dest_size != 0)
			*d = '\0'; /* NUL-terminate dest */
		while (*s++)
			;
	}

	return (s - src - 1); /* count does not include NUL */
}
#endif
