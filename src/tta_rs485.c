#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#ifndef _MSC_VER
#include <unistd.h>
#endif
#include <assert.h>

#include "tta_private.h"
#include "tta_rs485.h"
#include "tta_rs485_private.h"

#if HAVE_DECL_TIOCSRS485 || HAVE_DECL_TIOCM_RTS
#include <sys/ioctl.h>
#endif

#if HAVE_DECL_TIOCSRS485
#include <linux/serial.h>
#endif

static int _tta_rs485_flush(tta_t *);

// 2012.09.07 bygomma : RS485
#include <sys/ioctl.h>
static int write_uart(int fd, const uint8_t *buf, int size)
{
	int rc, rtsctl, txempty;
	int cnt=0;

	rtsctl = TIOCM_RTS;
	ioctl(fd, TIOCMBIS, &rtsctl);          // RTS Set

	rc = write(fd, buf, size);
	if(rc < 0)
	{
		printf("UART Write Error : %s\n", strerror(errno));
		return 0;
	}

	while(1)                             // Waiting.. Tx
	{
		ioctl(fd, TIOCSERGETLSR, &txempty);
		if (txempty) break;
		if(cnt > 1000)
		{
			cnt = 0;
			printf("Tx Empty Error !!!!!!!!!!\n");
			rc = -1;
			goto out;
		}
		cnt++;
		usleep(100);

		/*
		int i, tmp=0;
		for(i=0; i<10000; i++)
		{
			tmp++;
		}
		*/
	}

out:
	rtsctl = TIOCM_RTS;
	ioctl(fd, TIOCMBIC, &rtsctl);          // RTS Clear

	return rc;
}
// 2012.09.07 bygomma : end


static uint8_t sum_xor(uint8_t *buf, int len)
{
	uint8_t sum=0;
	while(len--)
	{
		sum ^= *buf++;
	}

	return sum;
}

static uint8_t sum_add(uint8_t *buf, int len)
{
	uint8_t sum=0;
	while(len--)
	{
		sum += *buf++;
	}

	return sum & 0xff;
}

#if defined(_WIN32)

/* This simple implementation is sort of a substitute of the select() call,
 * working this way: the win32_ser_select() call tries to read some data from
 * the serial port, setting the timeout as the select() call would. Data read is
 * stored into the receive buffer, that is then consumed by the win32_ser_read()
 * call.  So win32_ser_select() does both the event waiting and the reading,
 * while win32_ser_read() only consumes the receive buffer.
 */

static void win32_ser_init(struct win32_ser *ws) {
    /* Clear everything */
    memset(ws, 0x00, sizeof(struct win32_ser));

    /* Set file handle to invalid */
    ws->fd = INVALID_HANDLE_VALUE;
}

/* FIXME Try to remove length_to_read -> max_len argument, only used by win32 */
static int win32_ser_select(struct win32_ser *ws, int max_len,
                            struct timeval *tv) {
    COMMTIMEOUTS comm_to;
    unsigned int msec = 0;

    /* Check if some data still in the buffer to be consumed */
    if (ws->n_bytes > 0) {
        return 1;
    }

    /* Setup timeouts like select() would do.
       FIXME Please someone on Windows can look at this?
       Does it possible to use WaitCommEvent?
       When tv is NULL, MAXDWORD isn't infinite!
     */
    if (tv == NULL) {
        msec = MAXDWORD;
    } else {
        msec = tv->tv_sec * 1000 + tv->tv_usec / 1000;
        if (msec < 1)
            msec = 1;
    }

    comm_to.ReadIntervalTimeout = msec;
    comm_to.ReadTotalTimeoutMultiplier = 0;
    comm_to.ReadTotalTimeoutConstant = msec;
    comm_to.WriteTotalTimeoutMultiplier = 0;
    comm_to.WriteTotalTimeoutConstant = 1000;
    SetCommTimeouts(ws->fd, &comm_to);

    /* Read some bytes */
    if ((max_len > PY_BUF_SIZE) || (max_len < 0)) {
        max_len = PY_BUF_SIZE;
    }

    if (ReadFile(ws->fd, &ws->buf, max_len, &ws->n_bytes, NULL)) {
        /* Check if some bytes available */
        if (ws->n_bytes > 0) {
            /* Some bytes read */
            return 1;
        } else {
            /* Just timed out */
            return 0;
        }
    } else {
        /* Some kind of error */
        return -1;
    }
}

static int win32_ser_read(struct win32_ser *ws, uint8_t *p_msg,
                          unsigned int max_len) {
    unsigned int n = ws->n_bytes;

    if (max_len < n) {
        n = max_len;
    }

    if (n > 0) {
        memcpy(p_msg, ws->buf, n);
    }

    ws->n_bytes -= n;

    return n;
}
#endif

static void _tta_rs485_ioctl_rts(int fd, int on)
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

static int _tta_rs485_prepare_response_tid(const uint8_t *pkt, int *pktlen)
{
    (*pktlen) -= _TTA_RS485_CHECKSUM_LENGTH;
    /* No TID */
    return 0;
}

static ssize_t _tta_rs485_send(tta_t *ctx, const uint8_t *pkt, int pktlen)
{
#if defined(_WIN32)
    tta_rs485_t *ctx_rs485 = ctx->backend_data;
    DWORD n_bytes = 0;
    return (WriteFile(ctx_rs485->w_ser.fd, pkt, pktlen, &n_bytes, NULL)) ? n_bytes : -1;
#else
#if HAVE_DECL_TIOCM_RTS
    tta_rs485_t *ctx_rs485 = ctx->backend_data;
    if (ctx_rs485->rts != TTA_RS485_RTS_NONE) {
        ssize_t size;

        if (ctx->debug) {
            printf("Sending request using RTS signal\n");
        }

        _tta_rs485_ioctl_rts(ctx->s, ctx_rs485->rts == TTA_RS485_RTS_UP);
        usleep(_TTA_RS485_TIME_BETWEEN_RTS_SWITCH);

        size = write(ctx->s, pkt, pktlen);

        usleep(_TTA_RS485_TIME_BETWEEN_RTS_SWITCH);
        _tta_rs485_ioctl_rts(ctx->s, ctx_rs485->rts != TTA_RS485_RTS_UP);

        return size;
    } else {
#endif
		// 2012.05.05 bygomma : RS485
		// return write(ctx->s, pkt, pktlen);
		return write_uart(ctx->s, pkt, pktlen);
		// 2012.05.05 bygomma : end

#if HAVE_DECL_TIOCM_RTS
    }
#endif
#endif
}

static int _tta_rs485_receive(tta_t *ctx, uint8_t *pkt, int fgtimeout)
{
    int rc;
    tta_rs485_t *ctx_rs485 = ctx->backend_data;

    if (ctx_rs485->confirmation_to_ignore) {
        _tta_receive_pkt(ctx, pkt, fgtimeout);
        /* Ignore errors and reset the flag */
        ctx_rs485->confirmation_to_ignore = FALSE;
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
            ctx_rs485->confirmation_to_ignore = TRUE;
        }
    }
    return rc;
}

static ssize_t _tta_rs485_recv(tta_t *ctx, uint8_t *pkt, int pktlen)
{
#if defined(_WIN32)
    return win32_ser_read(&((tta_rs485_t *)ctx->backend_data)->w_ser, pkt,
			pktlen);
#else
    return read(ctx->s, pkt, pktlen);
#endif
}

/* The check_integrity function shall return 0 is the message is ignored and the
   message length if the SUM is valid. Otherwise it shall return -1 and set
   errno to EMBADCRC. */
static int _tta_rs485_check_integrity(tta_t *ctx, uint8_t *pkt,
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
            _tta_rs485_flush(ctx);
        }
        errno = EMBBADCRC;
        return -1;
    }
}

static int _tta_rs485_pre_check_confirmation(tta_t *ctx, const uint8_t *req,
                                       const uint8_t *rsp, int rsp_length)
{
#if 0
    /* Check responding slave is the slave we requested (except for broacast
     * request) */
    if (req[1] != 0 && req[1] != rsp[1]) {
        if (ctx->debug) {
            printf(				"The responding slave %d isn't the requested slave %d",
				rsp[0], req[0]);
        }
        errno = EMBBADSLAVE;
        return -1;
    } else {
        return 0;
    }
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
}

static ttamsg_t *_tta_rs485_conv_pkt2ttamsg(tta_t *ctx, uint8_t *pkt, int pktlen)
{
	ttamsg_t *ttamsg;
	ttamsg_rs485_hd_t *hd; 
	int len;

	hd = (ttamsg_rs485_hd_t *)pkt;
	len = sizeof(ttamsg_hd_t)+hd->datalen;

	ttamsg = malloc(sizeof(ttamsg_t));
	ttamsg->tid = 0;
	ttamsg->len = len;
	ttamsg->buf = malloc(len);
	memcpy(ttamsg->buf, pkt+_TTA_RS485_PREHEADER_LENGTH, len);
	ttamsg->hd = (ttamsg_hd_t *)ttamsg->buf;
	ttamsg->data = ttamsg->buf+sizeof(ttamsg_hd_t);;
	ttamsg->src = ctx;

	return ttamsg;
}

static int _tta_rs485_conv_ttamsg2pkt(tta_t *ctx, ttamsg_t *ttamsg, uint8_t *pkt)
{
//	tta_rs485_t *ctx_rs485 = ctx->backend_data;
	ttamsg_hd_t *hd;
	int i, hdlen, datalen;

	hd = ttamsg->hd;
	datalen = hd->datalen;

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
}

/* Sets up a serial port for RS485 communications */
static int _tta_rs485_connect(tta_t *ctx)
{
#if defined(_WIN32)
    DCB dcb;
#else
    struct termios tios;
    speed_t speed;
    int flags;
#endif
    tta_rs485_t *ctx_rs485 = ctx->backend_data;

    if (ctx->debug) {
        printf("Opening %s at %d bauds (%c, %d, %d)\n",
               ctx_rs485->device, ctx_rs485->baud, ctx_rs485->parity,
               ctx_rs485->data_bit, ctx_rs485->stop_bit);
    }

#if defined(_WIN32)
    /* Some references here:
     * http://msdn.microsoft.com/en-us/library/aa450602.aspx
     */
    win32_ser_init(&ctx_rs485->w_ser);

    /* ctx_rs485->device should contain a string like "COMxx:" xx being a decimal
     * number */
    ctx_rs485->w_ser.fd = CreateFileA(ctx_rs485->device,
                                    GENERIC_READ | GENERIC_WRITE,
                                    0,
                                    NULL,
                                    OPEN_EXISTING,
                                    0,
                                    NULL);

    /* Error checking */
    if (ctx_rs485->w_ser.fd == INVALID_HANDLE_VALUE) {
        printf("ERROR Can't open the device %s (%s)\n",
                ctx_rs485->device, strerror(errno));
        return -1;
    }

    /* Save params */
    ctx_rs485->old_dcb.DCBlength = sizeof(DCB);
    if (!GetCommState(ctx_rs485->w_ser.fd, &ctx_rs485->old_dcb)) {
        printf("ERROR Error getting configuration (LastError %d)\n",
                (int)GetLastError());
        CloseHandle(ctx_rs485->w_ser.fd);
        ctx_rs485->w_ser.fd = INVALID_HANDLE_VALUE;
        return -1;
    }

    /* Build new configuration (starting from current settings) */
    dcb = ctx_rs485->old_dcb;

    /* Speed setting */
    switch (ctx_rs485->baud) {
    case 110:
        dcb.BaudRate = CBR_110;
        break;
    case 300:
        dcb.BaudRate = CBR_300;
        break;
    case 600:
        dcb.BaudRate = CBR_600;
        break;
    case 1200:
        dcb.BaudRate = CBR_1200;
        break;
    case 2400:
        dcb.BaudRate = CBR_2400;
        break;
    case 4800:
        dcb.BaudRate = CBR_4800;
        break;
    case 9600:
        dcb.BaudRate = CBR_9600;
        break;
    case 19200:
        dcb.BaudRate = CBR_19200;
        break;
    case 38400:
        dcb.BaudRate = CBR_38400;
        break;
    case 57600:
        dcb.BaudRate = CBR_57600;
        break;
    case 115200:
        dcb.BaudRate = CBR_115200;
        break;
    default:
        dcb.BaudRate = CBR_9600;
        printf("WARNING Unknown baud rate %d for %s (B9600 used)\n",
               ctx_rs485->baud, ctx_rs485->device);
    }

    /* Data bits */
    switch (ctx_rs485->data_bit) {
    case 5:
        dcb.ByteSize = 5;
        break;
    case 6:
        dcb.ByteSize = 6;
        break;
    case 7:
        dcb.ByteSize = 7;
        break;
    case 8:
    default:
        dcb.ByteSize = 8;
        break;
    }

    /* Stop bits */
    if (ctx_rs485->stop_bit == 1)
        dcb.StopBits = ONESTOPBIT;
    else /* 2 */
        dcb.StopBits = TWOSTOPBITS;

    /* Parity */
    if (ctx_rs485->parity == 'N') {
        dcb.Parity = NOPARITY;
        dcb.fParity = FALSE;
    } else if (ctx_rs485->parity == 'E') {
        dcb.Parity = EVENPARITY;
        dcb.fParity = TRUE;
    } else {
        /* odd */
        dcb.Parity = ODDPARITY;
        dcb.fParity = TRUE;
    }

    /* Hardware handshaking left as default settings retrieved */

    /* No software handshaking */
    dcb.fTXContinueOnXoff = TRUE;
    dcb.fOutX = FALSE;
    dcb.fInX = FALSE;

    /* Binary mode (it's the only supported on Windows anyway) */
    dcb.fBinary = TRUE;

    /* Don't want errors to be blocking */
    dcb.fAbortOnError = FALSE;

    /* TODO: any other flags!? */

    /* Setup port */
    if (!SetCommState(ctx_rs485->w_ser.fd, &dcb)) {
        printf("ERROR Error setting new configuration (LastError %d)\n",
                (int)GetLastError());
        CloseHandle(ctx_rs485->w_ser.fd);
        ctx_rs485->w_ser.fd = INVALID_HANDLE_VALUE;
        return -1;
    }
#else
    /* The O_NOCTTY flag tells UNIX that this program doesn't want
       to be the "controlling terminal" for that port. If you
       don't specify this then any input (such as keyboard abort
       signals and so forth) will affect your process

       Timeouts are ignored in canonical input mode or when the
       NDELAY option is set on the file via open or fcntl */
    flags = O_RDWR | O_NOCTTY | O_NDELAY | O_EXCL;
#ifdef O_CLOEXEC
    flags |= O_CLOEXEC;
#endif

    ctx->s = open(ctx_rs485->device, flags);
    if (ctx->s == -1) {
        printf("ERROR Can't open the device %s (%s)\n",
                ctx_rs485->device, strerror(errno));
        return -1;
    }

    /* Save */
    tcgetattr(ctx->s, &(ctx_rs485->old_tios));

    memset(&tios, 0, sizeof(struct termios));

    /* C_ISPEED     Input baud (new interface)
       C_OSPEED     Output baud (new interface)
    */
    switch (ctx_rs485->baud) {
    case 110:
        speed = B110;
        break;
    case 300:
        speed = B300;
        break;
    case 600:
        speed = B600;
        break;
    case 1200:
        speed = B1200;
        break;
    case 2400:
        speed = B2400;
        break;
    case 4800:
        speed = B4800;
        break;
    case 9600:
        speed = B9600;
        break;
    case 19200:
        speed = B19200;
        break;
    case 38400:
        speed = B38400;
        break;
    case 57600:
        speed = B57600;
        break;
    case 115200:
        speed = B115200;
        break;
    default:
        speed = B9600;
        if (ctx->debug) {
            printf("WARNING Unknown baud rate %d for %s (B9600 used)\n",
                    ctx_rs485->baud, ctx_rs485->device);
        }
    }

    /* Set the baud rate */
    if ((cfsetispeed(&tios, speed) < 0) ||
        (cfsetospeed(&tios, speed) < 0)) {
        close(ctx->s);
        ctx->s = -1;
        return -1;
    }

    /* C_CFLAG      Control options
       CLOCAL       Local line - do not change "owner" of port
       CREAD        Enable receiver
    */
    tios.c_cflag |= (CREAD | CLOCAL);
    /* CSIZE, HUPCL, CRTSCTS (hardware flow control) */

    /* Set data bits (5, 6, 7, 8 bits)
       CSIZE        Bit mask for data bits
    */
    tios.c_cflag &= ~CSIZE;
    switch (ctx_rs485->data_bit) {
    case 5:
        tios.c_cflag |= CS5;
        break;
    case 6:
        tios.c_cflag |= CS6;
        break;
    case 7:
        tios.c_cflag |= CS7;
        break;
    case 8:
    default:
        tios.c_cflag |= CS8;
        break;
    }

    /* Stop bit (1 or 2) */
    if (ctx_rs485->stop_bit == 1)
        tios.c_cflag &=~ CSTOPB;
    else /* 2 */
        tios.c_cflag |= CSTOPB;

    /* PARENB       Enable parity bit
       PARODD       Use odd parity instead of even */
    if (ctx_rs485->parity == 'N') {
        /* None */
        tios.c_cflag &=~ PARENB;
    } else if (ctx_rs485->parity == 'E') {
        /* Even */
        tios.c_cflag |= PARENB;
        tios.c_cflag &=~ PARODD;
    } else {
        /* Odd */
        tios.c_cflag |= PARENB;
        tios.c_cflag |= PARODD;
    }

    /* Read the man page of termios if you need more information. */

    /* This field isn't used on POSIX systems
       tios.c_line = 0;
    */

    /* C_LFLAG      Line options

       ISIG Enable SIGINTR, SIGSUSP, SIGDSUSP, and SIGQUIT signals
       ICANON       Enable canonical input (else raw)
       XCASE        Map uppercase \lowercase (obsolete)
       ECHO Enable echoing of input characters
       ECHOE        Echo erase character as BS-SP-BS
       ECHOK        Echo NL after kill character
       ECHONL       Echo NL
       NOFLSH       Disable flushing of input buffers after
       interrupt or quit characters
       IEXTEN       Enable extended functions
       ECHOCTL      Echo control characters as ^char and delete as ~?
       ECHOPRT      Echo erased character as character erased
       ECHOKE       BS-SP-BS entire line on line kill
       FLUSHO       Output being flushed
       PENDIN       Retype pending input at next read or input char
       TOSTOP       Send SIGTTOU for background output

       Canonical input is line-oriented. Input characters are put
       into a buffer which can be edited interactively by the user
       until a CR (carriage return) or LF (line feed) character is
       received.

       Raw input is unprocessed. Input characters are passed
       through exactly as they are received, when they are
       received. Generally you'll deselect the ICANON, ECHO,
       ECHOE, and ISIG options when using raw input
    */

    /* Raw input */
    tios.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);

    /* C_IFLAG      Input options

       Constant     Description
       INPCK        Enable parity check
       IGNPAR       Ignore parity errors
       PARMRK       Mark parity errors
       ISTRIP       Strip parity bits
       IXON Enable software flow control (outgoing)
       IXOFF        Enable software flow control (incoming)
       IXANY        Allow any character to start flow again
       IGNBRK       Ignore break condition
       BRKINT       Send a SIGINT when a break condition is detected
       INLCR        Map NL to CR
       IGNCR        Ignore CR
       ICRNL        Map CR to NL
       IUCLC        Map uppercase to lowercase
       IMAXBEL      Echo BEL on input line too long
    */
    if (ctx_rs485->parity == 'N') {
        /* None */
        tios.c_iflag &= ~INPCK;
    } else {
        tios.c_iflag |= INPCK;
    }

    /* Software flow control is disabled */
    tios.c_iflag &= ~(IXON | IXOFF | IXANY);

    /* C_OFLAG      Output options
       OPOST        Postprocess output (not set = raw output)
       ONLCR        Map NL to CR-NL

       ONCLR ant others needs OPOST to be enabled
    */

    /* Raw ouput */
    tios.c_oflag &=~ OPOST;

    /* C_CC         Control characters
       VMIN         Minimum number of characters to read
       VTIME        Time to wait for data (tenths of seconds)

       UNIX serial interface drivers provide the ability to
       specify character and packet timeouts. Two elements of the
       c_cc array are used for timeouts: VMIN and VTIME. Timeouts
       are ignored in canonical input mode or when the NDELAY
       option is set on the file via open or fcntl.

       VMIN specifies the minimum number of characters to read. If
       it is set to 0, then the VTIME value specifies the time to
       wait for every character read. Note that this does not mean
       that a read call for N bytes will wait for N characters to
       come in. Rather, the timeout will apply to the first
       character and the read call will return the number of
       characters immediately available (up to the number you
       request).

       If VMIN is non-zero, VTIME specifies the time to wait for
       the first character read. If a character is read within the
       time given, any read will block (wait) until all VMIN
       characters are read. That is, once the first character is
       read, the serial interface driver expects to receive an
       entire packet of characters (VMIN bytes total). If no
       character is read within the time allowed, then the call to
       read returns 0. This method allows you to tell the serial
       driver you need exactly N bytes and any read call will
       return 0 or N bytes. However, the timeout only applies to
       the first character read, so if for some reason the driver
       misses one character inside the N byte packet then the read
       call could block forever waiting for additional input
       characters.

       VTIME specifies the amount of time to wait for incoming
       characters in tenths of seconds. If VTIME is set to 0 (the
       default), reads will block (wait) indefinitely unless the
       NDELAY option is set on the port with open or fcntl.
    */
    /* Unused because we use open with the NDELAY option */
    tios.c_cc[VMIN] = 0;
    tios.c_cc[VTIME] = 0;

    if (tcsetattr(ctx->s, TCSANOW, &tios) < 0) {
        close(ctx->s);
        ctx->s = -1;
        return -1;
    }
#endif

	// 2012.09.07 bygomma : clear RTS to receive
	int rtsctl;
	rtsctl = TIOCM_RTS;
	ioctl(ctx->s, TIOCMBIC, &rtsctl);          // RTS Clear
	// 2012.09.07 bygomma : end

    return 0;
}

int tta_rs485_set_serial_mode(tta_t *ctx, int mode)
{
    if (ctx->backend->backend_type == _TTA_BACKEND_TYPE_RS485) {
#if HAVE_DECL_TIOCSRS485
        tta_rs485_t *ctx_rs485 = ctx->backend_data;
        struct serial_rs485 rs485conf;
        memset(&rs485conf, 0x0, sizeof(struct serial_rs485));

        if (mode == TTA_RS485_RS485) {
            rs485conf.flags = SER_RS485_ENABLED;
            if (ioctl(ctx->s, TIOCSRS485, &rs485conf) < 0) {
                return -1;
            }

            ctx_rs485->serial_mode = TTA_RS485_RS485;
            return 0;
        } else if (mode == TTA_RS485_RS232) {
            /* Turn off RS485 mode only if required */
            if (ctx_rs485->serial_mode == TTA_RS485_RS485) {
                /* The ioctl call is avoided because it can fail on some RS232 ports */
                if (ioctl(ctx->s, TIOCSRS485, &rs485conf) < 0) {
                    return -1;
                }
            }
            ctx_rs485->serial_mode = TTA_RS485_RS232;
            return 0;
        }
#else
        if (ctx->debug) {
            printf("This function isn't supported on your platform\n");
        }
        errno = ENOTSUP;
        return -1;
#endif
    }

    /* Wrong backend and invalid mode specified */
    errno = EINVAL;
    return -1;
}

int tta_rs485_get_serial_mode(tta_t *ctx) {
    if (ctx->backend->backend_type == _TTA_BACKEND_TYPE_RS485) {
#if HAVE_DECL_TIOCSRS485
        tta_rs485_t *ctx_rs485 = ctx->backend_data;
        return ctx_rs485->serial_mode;
#else
        if (ctx->debug) {
            printf("This function isn't supported on your platform\n");
        }
        errno = ENOTSUP;
        return -1;
#endif
    } else {
        errno = EINVAL;
        return -1;
    }
}

int tta_rs485_set_rts(tta_t *ctx, int mode)
{
    if (ctx->backend->backend_type == _TTA_BACKEND_TYPE_RS485) {
#if HAVE_DECL_TIOCM_RTS
        tta_rs485_t *ctx_rs485 = ctx->backend_data;

        if (mode == TTA_RS485_RTS_NONE || mode == TTA_RS485_RTS_UP ||
            mode == TTA_RS485_RTS_DOWN) {
            ctx_rs485->rts = mode;

            /* Set the RTS bit in order to not reserve the RS485 bus */
            _tta_rs485_ioctl_rts(ctx->s, ctx_rs485->rts != TTA_RS485_RTS_UP);

            return 0;
        }
#else
        if (ctx->debug) {
            printf("This function isn't supported on your platform\n");
        }
        errno = ENOTSUP;
        return -1;
#endif
    }
    /* Wrong backend or invalid mode specified */
    errno = EINVAL;
    return -1;
}

int tta_rs485_get_rts(tta_t *ctx) {
    if (ctx->backend->backend_type == _TTA_BACKEND_TYPE_RS485) {
#if HAVE_DECL_TIOCM_RTS
        tta_rs485_t *ctx_rs485 = ctx->backend_data;
        return ctx_rs485->rts;
#else
        if (ctx->debug) {
            printf("This function isn't supported on your platform\n");
        }
        errno = ENOTSUP;
        return -1;
#endif
    } else {
        errno = EINVAL;
        return -1;
    }
}

static void _tta_rs485_close(tta_t *ctx)
{
    /* Closes the file descriptor in RS485 mode */
    tta_rs485_t *ctx_rs485 = ctx->backend_data;

#if defined(_WIN32)
    /* Revert settings */
    if (!SetCommState(ctx_rs485->w_ser.fd, &ctx_rs485->old_dcb))
        printf("ERROR Couldn't revert to configuration (LastError %d)\n",
                (int)GetLastError());

    if (!CloseHandle(ctx_rs485->w_ser.fd))
        printf("ERROR Error while closing handle (LastError %d)\n",
                (int)GetLastError());
#else
    tcsetattr(ctx->s, TCSANOW, &(ctx_rs485->old_tios));
    close(ctx->s);
#endif
}

static int _tta_rs485_flush(tta_t *ctx)
{
#if defined(_WIN32)
    tta_rs485_t *ctx_rs485 = ctx->backend_data;
    ctx_rs485->w_ser.n_bytes = 0;
    return (FlushFileBuffers(ctx_rs485->w_ser.fd) == FALSE);
#else
    return tcflush(ctx->s, TCIOFLUSH);
#endif
}

static int _tta_rs485_select(tta_t *ctx, fd_set *rset,
                       struct timeval *tv, int length_to_read)
{
    int rc;
#if defined(_WIN32)
    rc = win32_ser_select(&(((tta_rs485_t*)ctx->backend_data)->w_ser),
                            length_to_read, tv);
    if (rc == 0) {
        errno = ETIMEDOUT;
        return -1;
    }

    if (rc < 0) {
        return -1;
    }
#else
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
#endif

    return rc;
}

const tta_backend_t _tta_rs485_backend = {
    _TTA_BACKEND_TYPE_RS485,
    _TTA_RS485_HEADER_LENGTH,
    _TTA_RS485_CHECKSUM_LENGTH,
    TTA_RS485_MAX_ADU_LENGTH,

    _tta_rs485_prepare_response_tid,

    _tta_rs485_receive,
    _tta_rs485_check_integrity,
    _tta_rs485_pre_check_confirmation,
	_tta_rs485_conv_pkt2ttamsg,
	_tta_rs485_conv_ttamsg2pkt,

    _tta_rs485_connect,
    _tta_rs485_close,
    _tta_rs485_send,
    _tta_rs485_recv,
    _tta_rs485_flush,
    _tta_rs485_select
};

tta_t* tta_new_rs485(const char *device,
                         int baud, char parity, int data_bit,
                         int stop_bit)
{
    tta_t *ctx;
    tta_rs485_t *ctx_rs485;
    size_t dest_size;
    size_t ret_size;

    ctx = (tta_t *) malloc(sizeof(tta_t));
    _tta_init_common(ctx);

	ctx->proto = TTA_PROTO_RS485;
    ctx->backend = &_tta_rs485_backend;
    ctx->backend_data = (tta_rs485_t *) malloc(sizeof(tta_rs485_t));
    ctx_rs485 = (tta_rs485_t *)ctx->backend_data;

    dest_size = sizeof(ctx_rs485->device);
    ret_size = strlcpy(ctx_rs485->device, device, dest_size);
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

    ctx_rs485->baud = baud;
    if (parity == 'N' || parity == 'E' || parity == 'O') {
        ctx_rs485->parity = parity;
    } else {
        tta_free(ctx);
        errno = EINVAL;
        return NULL;
    }
    ctx_rs485->data_bit = data_bit;
    ctx_rs485->stop_bit = stop_bit;

#if HAVE_DECL_TIOCSRS485
    /* The RS232 mode has been set by default */
    ctx_rs485->serial_mode = TTA_RS485_RS232;
#endif

#if HAVE_DECL_TIOCM_RTS
    /* The RTS use has been set by default */
    ctx_rs485->rts = TTA_RS485_RTS_NONE;
#endif

    ctx_rs485->confirmation_to_ignore = FALSE;

    return ctx;
}
