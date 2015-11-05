#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif
#include <assert.h>
#include <sys/ioctl.h>

#include "tta_private.h"
#include "tta_rf.h"
#include "tta_rf_private.h"

#define DEBUG
#include "dbg.h"
#include "cc1120.h"

#define DBG_TAG "TTA-RF"

#undef TTARF_PKT_DUMP
#define TTARF_PKT_RSSI_DUMP

static int _tta_rf_flush(tta_t *);

static uint8_t rxfifo[128];
int rxfifo_pos;

static int write_rf (int fd, const uint8_t *buf, int sz)
{
	// TODO : check fifo cap
	uint8_t fifo[128];

	fifo[0] = sz;
	memcpy(fifo+1, buf, sz);
	cc1120_fifo_write (fd, fifo, sz+1);

	return sz;
}

static ssize_t read_rf (int fd, uint8_t *buf, int sz)
{
	memcpy(buf, rxfifo+rxfifo_pos, sz);
	rxfifo_pos += sz;
	return sz;
}

#if 0
static ssize_t read_rf (int fd, uint8_t *buf, int sz)
{
	// TODO 
	uint8_t fifo[128];
	int len;

	len = cc1120_read_single (fd, CC112X_NUM_RXBYTES);

	if (len < sz+3)
		sz = len-3;

	_dbg (DBG_TAG, "len : %d\n", len);
	_dbg (DBG_TAG, "sz : %d\n", sz);

	// rx packet format
	// | len | rx data | stats0 | staus1 |

	cc1120_fifo_read (fd, fifo, sz+3);
	memcpy(buf, fifo+1, sz);

	int i;
	for(i=0; i<sz; i++)
	{
		printf("fifo[%d] : %02x\n", i, fifo[i]);
	}
	for(i=0; i<sz; i++)
	{
		printf("buf[%d] : %02x\n", i, buf[i]);
	}

	_dbg (DBG_TAG, "rx length : %d\n",  fifo[0]);
	_dbg (DBG_TAG, "rx status0 : %d\n", fifo[sz+1]);
	_dbg (DBG_TAG, "rx status1 : 0x%x\n", fifo[sz+2]);

	return sz;
}
#endif

static int close_rf (int fd)
{
	// TODO 
	return 0;
}

static int flush_rf (int fd)
{
	// TODO 
	cc1120_strobe (fd, CC112X_SFTX);
	cc1120_strobe (fd, CC112X_SFRX);
	return 0;
}

enum {
	RF_RX_MODE,
	RF_TX_MODE
};

static void change_rf_mode (int fd, int mode)
{
	switch(mode)
	{
	case RF_RX_MODE:
		break;
	case RF_TX_MODE:
		break;
	}
}

static uint8_t sum_xor (uint8_t *buf, int len)
{
	uint8_t sum=0;
	while(len--)
	{
		sum ^= *buf++;
	}

	return sum;
}

static uint8_t sum_add (uint8_t *buf, int len)
{
	uint8_t sum=0;
	while(len--)
	{
		sum += *buf++;
	}

	return sum & 0xff;
}

#if defined(_WIN32)
#endif

static void _tta_rf_ioctl_rts (int fd, int on)
{
#if HAVE_DECL_TIOCM_RTS
    int flags;

    ioctl(fd, TIOCMGET, &flags);
    if (on) {
        flags |= TIOCM_RTS;
    } else {
        flags &= ~TIOCM_RTS;
    }
    ioctl(fd, TIOCMSET, &flags);
#endif
}

static int _tta_rf_prepare_response_tid(const uint8_t *pkt, int *pktlen)
{
    (*pktlen) -= _TTA_RF_CHECKSUM_LENGTH;
    /* No TID */
    return 0;
}

static ssize_t _tta_rf_send(tta_t *ctx, const uint8_t *pkt, int pktlen)
{
	int status;
	int rc;
	int fgstart = 0;

	cc1120_strobe (ctx->s, CC112X_SIDLE);
	cc1120_strobe (ctx->s, CC112X_SFTX);

	rc = write_rf(ctx->s, pkt, pktlen);

	// 2014.08.01 bygomma : LBT
	uint8_t rssi0;
	int retry = 0;
	cc1120_strobe (ctx->s, CC112X_SRX);
	do 
	{
		usleep (1000);
		if (retry++ > 10)
		{
			rc = -1;
			goto out;
		}

		rssi0 = cc1120_read_single (ctx->s, CC112X_RSSI0);
//		_dbg (DBG_TAG, "rssi0 : 0x%02x\n", rssi0);
	} while (!(rssi0 & 0x01));
	// 2014.08.01 bygomma : end

	cc1120_strobe (ctx->s, CC112X_STX);

	//
	// wait tx complete
	//
#if 1
    struct timeval now, timeout;
	gettimeofday(&timeout, NULL);
	timeout.tv_sec += 1;
	timeout.tv_usec += 0;
	while (1)
	{
		/////////////////////////////////////////////////////
#if 0
		uint8_t rssi1;
		rssi0 = cc1120_read_single (ctx->s, CC112X_RSSI0);
		rssi1 = cc1120_read_single (ctx->s, CC112X_RSSI1);
		_dbg (DBG_TAG, "rssi0 : 0x%02x\n", rssi0);
		_dbg (DBG_TAG, "rssi1 : %d(%02x)\n", (signed char)rssi1, rssi1);
#endif
		/////////////////////////////////////////////////////

		// check GPIO3 (PKT_SYNC_RXTX)
		status = cc1120_read_single (ctx->s, CC112X_GPIO_STATUS);

		if (status & 0x08)
			fgstart = 1;
		else if(fgstart == 1)
			break;

		// TODO : Check !!!!!!!!!!!
		gettimeofday(&now, NULL);
		if (now.tv_sec > timeout.tv_sec)
		{
	        if (ctx->debug)
			{
				printf ("Tx Timeout\n");
			}
			rc = -1;
			goto out;
		}
		else if (now.tv_sec == timeout.tv_sec
				&& now.tv_usec > timeout.tv_usec)
		{
	        if (ctx->debug)
			{
				printf ("Tx Timeout\n");
			}
			rc = -1;
			goto out;
		}

		usleep (1000);
	}
#endif

out:
	cc1120_strobe (ctx->s, CC112X_SIDLE);
	cc1120_strobe (ctx->s, CC112X_SFTX);

	return rc;
}

static int _tta_rf_receive(tta_t *ctx, uint8_t *pkt, int fgtimeout)
{
    int rc;
    tta_rf_t *ctx_rf = ctx->backend_data;

    struct timeval now, timeout;
	uint8_t fifo[128];
	uint8_t numrx ;
	uint8_t marcstate;
	
	///////////////////////////////////////////////////////////////
	// TODO : check timeout
	// TODO : this part must be checked soon !!!!!!!!!!!!!!!!!!!!!!
	///////////////////////////////////////////////////////////////
	gettimeofday(&timeout, NULL);
	timeout.tv_sec += ctx->response_timeout.tv_sec;
	timeout.tv_usec += ctx->response_timeout.tv_usec;
	///////////////////////////////////////////////////////////////

	// TODO : move to cc1120.c
	cc1120_strobe (ctx->s, CC112X_SIDLE);
	cc1120_strobe (ctx->s, CC112X_SFRX);
	cc1120_strobe (ctx->s, CC112X_SRX);

	while(1)
	{
		numrx = cc1120_read_single (ctx->s, CC112X_NUM_RXBYTES);
		marcstate = cc1120_read_single (ctx->s, CC112X_MARCSTATE);

		if (numrx > 0 && marcstate == 0x41)
		{
			cc1120_fifo_read (ctx->s, fifo, numrx);
#ifdef TTARF_PKT_DUMP
			int i;
			for(i=0; i<numrx; i++)
			{
				printf("{%02x}", fifo[i]);
			}
			printf("\n");
#endif
#ifdef TTARF_PKT_RSSI_DUMP
			printf ("RSSI: %d(%02x), LQI: %d\n", (signed char)fifo[numrx-2],
					fifo[numrx-2], fifo[numrx-1]&0x7f);
#endif
			// TODO : check CRC
			if (fifo[numrx-1] & 0x80)
			{
				memcpy(rxfifo, fifo+1, fifo[0]);
				rxfifo_pos = 0;
				break;
			}
			else
			{
				rc = -1;
				goto out;
			}
		}

		gettimeofday(&now, NULL);

		if (now.tv_sec > timeout.tv_sec)
		{
	        if (ctx->debug)
			{
				printf ("Rx Timeout\n");
			}
			rc = -1;
			goto out;
		}
		else if (now.tv_sec == timeout.tv_sec
				&& now.tv_usec > timeout.tv_usec)
		{
	        if (ctx->debug)
			{
				printf ("Rx Timeout\n");
			}
			rc = -1;
			goto out;
		}

		// TODO
		usleep(100000);
	}

	////////////////////////////////////////////////////////////////


    if (ctx_rf->confirmation_to_ignore) {
        _tta_receive_pkt(ctx, pkt, fgtimeout);
        /* Ignore errors and reset the flag */
        ctx_rf->confirmation_to_ignore = FALSE;
        rc = 0;
        if (ctx->debug)
		{
            printf("Confirmation to ignore\n");
        }
    } else {
        rc = _tta_receive_pkt(ctx, pkt, fgtimeout);
        if (rc == 0)
		{
            /* The next expected message is a confirmation to ignore */
            ctx_rf->confirmation_to_ignore = TRUE;
        }
    }

out:
	cc1120_strobe (ctx->s, CC112X_SIDLE);
	cc1120_strobe (ctx->s, CC112X_SFRX);

    return rc;
}

static ssize_t _tta_rf_recv(tta_t *ctx, uint8_t *pkt, int pktlen)
{
	int rc;
	rc = read_rf(ctx->s, pkt, pktlen);
	return rc;
}

/* The check_integrity function shall return 0 is the message is ignored and the
   message length if the SUM is valid. Otherwise it shall return -1 and set
   errno to EMBADCRC. */
static int _tta_rf_check_integrity(tta_t *ctx, uint8_t *pkt,
                                const int pktlen)
{
    uint16_t xor_calculated, add_calculated;
    uint16_t xor_received, add_received;

	xor_calculated = sum_xor(pkt, pktlen-2);
	add_calculated = sum_add(pkt, pktlen-1);
	xor_received = pkt[pktlen-2];
	add_received = pkt[pktlen-1];

    /* Check CRC of pkt */
    if (xor_calculated == xor_received && add_calculated == add_received)
	{
        return pktlen;
    }
	else
	{
        if (ctx->debug)
		{
            printf("ERROR SUM received %0X:%0X != SUM calculated %0X:%0X\n",
                    xor_received, add_received, xor_calculated, add_calculated);
        }

        if (ctx->error_recovery & TTA_ERROR_RECOVERY_PROTOCOL)
		{
            _tta_rf_flush(ctx);
        }
        errno = EMBBADCRC;
        return -1;
    }
}

static int _tta_rf_pre_check_confirmation(tta_t *ctx, const uint8_t *req,
                                       const uint8_t *rsp, int rsp_length)
{
    tta_rf_t *ctx_rf = ctx->backend_data;

	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// TODO : for test
#if 1
	// Check RF ID 
	if (memcmp (&rsp[0], &ctx_rf->id[0], sizeof(ctx_rf->id)))
	{
        if (ctx->debug)
		{
            printf("ERROR The responding ACK is invalid\n");
        }
        errno = EMBBADRFID;
        return -1;
    }

	return 0;
#endif

	// Check Response 
    if ((req[1] != rsp[1]) || (req[3] != (rsp[3] & 0x7F)))
	{
        if (ctx->debug)
		{
            printf("ERROR The responding ACK is invalid\n");
        }
        errno = EMBBADSLAVE;
        return -1;
    }

	return 0;
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
}

static ttamsg_t *_tta_rf_conv_pkt2ttamsg(tta_t *ctx, uint8_t *pkt, int pktlen)
{
	ttamsg_t *ttamsg;
	ttamsg_rf_hd_t *hd; 
	int len;

	hd = (ttamsg_rf_hd_t *)pkt;
	len = sizeof(ttamsg_hd_t)+hd->datalen;

	ttamsg = malloc(sizeof(ttamsg_t));
	ttamsg->tid = 0;

	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// TODO : check
	// ttamsg->rfsid = ntohs(hd->rfsid);;
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

	ttamsg->len = len;
	ttamsg->buf = malloc(len);
	memcpy(ttamsg->buf, pkt+_TTA_RF_PREHEADER_LENGTH, len);
	ttamsg->hd = (ttamsg_hd_t *)ttamsg->buf;
	ttamsg->data = ttamsg->buf+sizeof(ttamsg_hd_t);;
	ttamsg->src = ctx;

	return ttamsg;
}

static int _tta_rf_conv_ttamsg2pkt(tta_t *ctx, ttamsg_t *ttamsg, uint8_t *pkt)
{
	tta_rf_t *ctx_rf = ctx->backend_data;
	ttamsg_hd_t *hd;
	int i, hdlen, datalen;

	hd = ttamsg->hd;
	datalen = hd->datalen;

	// TODO : for test
#if 1
	pkt[0] = ctx_rf->id[0];
	pkt[1] = ctx_rf->id[1];
	pkt[2] = ctx_rf->id[2];
	pkt[3] = ctx_rf->id[3];
	pkt[4] = hd->devid;
	pkt[5] = hd->subid;
	pkt[6] = hd->cmd;
	pkt[7] = datalen;

	hdlen = ctx->backend->header_length;

	for(i=0; i<datalen; i++)
		pkt[hdlen+i] = ttamsg->data[i];

	pkt[hdlen+datalen] = sum_xor(pkt, hdlen+datalen);
	pkt[hdlen+datalen+1] = sum_add(pkt, hdlen+datalen+1);

	ttamsg_free(ttamsg);

	return (hdlen+datalen+ctx->backend->checksum_length);
#endif
#if 0
	pkt[0] = STX;
	pkt[1] = hd->devid;
	pkt[2] = hd->subid;
	pkt[3] = hd->cmd;
	pkt[4] = datalen;

	hdlen = ctx->backend->header_length;

	for(i=0; i<datalen; i++)
	{
		pkt[hdlen+i] = ttamsg->data[i];
	}
	pkt[hdlen+datalen] = sum_xor(pkt, hdlen+datalen);
	pkt[hdlen+datalen+1] = sum_add(pkt, hdlen+datalen+1);

	ttamsg_free(ttamsg);

	return (ctx->backend->header_length+datalen+ctx->backend->checksum_length);
#endif
}

/* Sets up a radio communications */
static int _tta_rf_connect(tta_t *ctx)
{
#if defined(_WIN32)
#else
    int flags;
#endif

    tta_rf_t *ctx_rf = ctx->backend_data;

    if (ctx->debug) {
        printf("Opening %s\n", ctx_rf->device);
    }

    flags = O_RDWR | O_NDELAY;
    ctx->s = open(ctx_rf->device, flags);
    if (ctx->s == -1) {
        printf("ERROR Can't open the device %s (%s)\n",
                ctx_rf->device, strerror(errno));
        return -1;
    }

//	cc1120_hw_reset (ctx->s);
	cc1120_strobe (ctx->s, CC112X_SRES);
	usleep (500000);

	cc1120_writesetting (ctx->s);
	cc1120_manualcalibration (ctx->s);

	cc1120_strobe (ctx->s, CC112X_SIDLE);

    return 0;
}

static void _tta_rf_close(tta_t *ctx)
{
    /* Closes the file descriptor in RF mode */
    tta_rf_t *ctx_rf = ctx->backend_data;

#if defined(_WIN32)
#else
    close_rf(ctx->s);
#endif
}

static int _tta_rf_flush(tta_t *ctx)
{
#if defined(_WIN32)
#else
    return flush_rf(ctx->s);
#endif
}

static int _tta_rf_select(tta_t *ctx, fd_set *rset,
                       struct timeval *tv, int length_to_read)
{
	return  1;

#if 0
    int rc;
    while ((rc = select(ctx->s+1, rset, NULL, NULL, tv)) == -1) {
        if (errno == EINTR) {
            if (ctx->debug) {
                printf("A non blocked signal was caught\n");
            }
            /* Necessary after an error */
            FD_ZERO(rset);
            FD_SET(ctx->s, rset);
        } else {
            return -1;
        }
    }

    if (rc == 0) {
        /* Timeout */
        errno = ETIMEDOUT;
        return -1;
    }

    return rc;
#endif

#if 0
    struct timeval now, timeout;
	uint8_t status;

	gettimeofday(&timeout, NULL);
	timeout.tv_sec += tv->tv_sec;
	timeout.tv_usec += tv->tv_usec;

	cc1120_strobe (ctx->s, CC112X_SIDLE);
	cc1120_strobe (ctx->s, CC112X_SFRX);
	cc1120_strobe (ctx->s, CC112X_SRX);

	uint8_t fifo[128];
	uint8_t numrx, pllen;

	while(1)
	{
		numrx = cc1120_read_single (ctx->s, CC112X_NUM_RXBYTES);
		if (numrx > 0)
		{
			cc1120_fifo_dread (ctx->s, 0x80, &pllen, 1);
			_dbg (DBG_TAG, "NUM_RXBYTES : %d\n", numrx);
			_dbg (DBG_TAG, "pllen : %d\n", pllen);

			if (numrx-3 > pllen)
			{
				// TODO : check CRC
				// if (fifo[numrx-1] & 0x80)
				if (1)
				{
					cc1120_fifo_read (ctx->s, fifo, numrx);
					memcpy(rxfifo, fifo+1, pllen);
					rxfifo_pos = 0;
					return 1;
				}
			}
		}
#endif

#if 0
		numrx = cc1120_read_single (ctx->s, CC112X_NUM_TXBYTES);
		if (numrx > 0)
		{
			_dbg (DBG_TAG, "POLLOUT >> NUM_TXBYTES : %d\n", numrx);
			return 1;
		}
#endif

#if 0
		gettimeofday(&now, NULL);

		if (now.tv_sec > timeout.tv_sec)
			return -1;
		else if (now.tv_sec == timeout.tv_sec
				&& now.tv_usec > timeout.tv_usec)
			return -1;

		// TODO
		usleep(100000);
	}
#endif

}

const tta_backend_t _tta_rf_backend = {
    _TTA_BACKEND_TYPE_RF,
    _TTA_RF_HEADER_LENGTH,
    _TTA_RF_CHECKSUM_LENGTH,
    TTA_RF_MAX_ADU_LENGTH,

    _tta_rf_prepare_response_tid,

    _tta_rf_receive,
    _tta_rf_check_integrity,
    _tta_rf_pre_check_confirmation,
	_tta_rf_conv_pkt2ttamsg,
	_tta_rf_conv_ttamsg2pkt,

    _tta_rf_connect,
    _tta_rf_close,
    _tta_rf_send,
    _tta_rf_recv,
    _tta_rf_flush,
    _tta_rf_select
};

tta_t* tta_new_rf (const char *device)
{
    tta_t *ctx;
    tta_rf_t *ctx_rf;
    size_t dest_size;
    size_t ret_size;

    ctx = (tta_t *) malloc(sizeof(tta_t));
    _tta_init_common(ctx);

	ctx->proto = TTA_PROTO_RF;
    ctx->backend = &_tta_rf_backend;
    ctx->backend_data = (tta_rf_t *) malloc(sizeof(tta_rf_t));
    ctx_rf = (tta_rf_t *)ctx->backend_data;

    dest_size = sizeof(ctx_rf->device);
    ret_size = strlcpy(ctx_rf->device, device, dest_size);
    if (ret_size == 0) {
        printf("The device string is empty\n");
        tta_free(ctx);
        errno = EINVAL;
        return NULL;
    }

    if (ret_size >= dest_size) {
        printf("The device string has been truncated\n");
        tta_free(ctx);
        errno = EINVAL;
        return NULL;
    }

#if 0
    ctx_rf->ch = ch;
    if (ch > 0 || ch <= _TTA_RF_MAX_CH) {
        ctx_rf->ch = ch;
    } else {
        tta_free(ctx);
        errno = EINVAL;
        return NULL;
    }
#endif

    ctx_rf->confirmation_to_ignore = FALSE;

    return ctx;
}

int tta_rf_set_id (tta_t *ctx, uint8_t *id)
{
	tta_rf_t *ctx_rf;

	ctx_rf = (tta_rf_t *)ctx->backend_data;
	memcpy (ctx_rf->id, id, sizeof(ctx_rf->id));
	return 0;
}

int tta_rf_get_id (tta_t *ctx, uint8_t *id)
{
	tta_rf_t *ctx_rf;

	ctx_rf = (tta_rf_t *)ctx->backend_data;
	memcpy (id, ctx_rf->id, sizeof(ctx_rf->id));
	return 0;
}

int tta_rf_set_ch (tta_t *ctx, int ch)
{
	tta_rf_t *ctx_rf;

	ctx_rf = (tta_rf_t *)ctx->backend_data;
	ctx_rf->ch = ch;

	cc1120_set_ch (ctx->s, ch);

	return 0;
}

int tta_rf_set_power (tta_t *ctx, int power)
{
	tta_rf_t *ctx_rf;

	ctx_rf = (tta_rf_t *)ctx->backend_data;
	ctx_rf->power = power;

	cc1120_set_power (ctx->s, power);

	return 0;
}

int tta_rf_set_agc_cs_thr (tta_t *ctx, int thr)
{
	signed char sval;

	sval = (signed char)thr;
	_dbg (DBG_TAG, "sval : %d(%02x)\n", sval, (uint8_t)sval);
	cc1120_write_single (ctx->s, CC112X_AGC_CS_THR, (uint8_t)sval);
	return 0;
}

int tta_rf_set_agc_gain_adjust (tta_t *ctx, int val)
{
	signed char sval;

	sval = (signed char)val;
	_dbg (DBG_TAG, "sval : %d(%02x)\n", sval, (uint8_t)sval);
	cc1120_write_single (ctx->s, CC112X_AGC_GAIN_ADJUST,(uint8_t)sval);
	return 0;
}

// rssi -4096 is invalid
int tta_rf_get_rssi (tta_t *ctx, int *rssi)
{
	return cc1120_get_rssi (ctx->s, rssi);
}
