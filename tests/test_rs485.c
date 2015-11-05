#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <errno.h>

typedef unsigned int uint32_t;
typedef unsigned char uint8_t;

int write_uart(int fd, uint8_t *buf, size_t size)
{
	int result, rtsctl, txempty;
	int cnt=0;

	rtsctl = TIOCM_RTS;
	ioctl(fd, TIOCMBIS, &rtsctl);          // RTS Set

	result = write(fd, buf, size);
	if(result < 0)
	{
		printf("UART Write Error : %s\n", strerror(errno));
		return 0;
	}

	while(1)                             // Waiting.. Tx
	{
		ioctl(fd, TIOCSERGETLSR, &txempty);
		if (txempty) break;
		if(cnt > 10000)	 // 1 sec
		{
			cnt = 0;
			printf("Tx Empty Error !!!!!!!!!!\n");
			return 0;
		}
		cnt++;
		usleep(100);
	}

	rtsctl = TIOCM_RTS;
	ioctl(fd, TIOCMBIC, &rtsctl);          // RTS Clear

	return 1;
}

static int fd;
int main(void)
{
	struct termios oldtio, newtio;
	int baudrate;

	// open serial device
	fd = open("/dev/ttyS0", O_RDWR | O_NOCTTY);
	if (fd < 0) {
		printf("can't open\n");
		return -1;
	}

	tcgetattr(fd, &oldtio);
	newtio = oldtio;

    /* Speed setting */
	baudrate = B9600;

    /* Set the baud rate */
	newtio.c_cflag &= ~CBAUD;
	newtio.c_cflag |= baudrate;

	/* Set data bits */
	newtio.c_cflag &= ~CSIZE;
	/*
	newtio.c_cflag |= CS5;
	newtio.c_cflag |= CS6;
	newtio.c_cflag |= CS7;
	*/
	newtio.c_cflag |= CS8;

    /* Stop bit (1 or 2) */
	newtio.c_cflag &=~ CSTOPB;	// 1
	// newtio.c_cflag |= CSTOPB;	// 2

	/* None */
	newtio.c_cflag &=~ PARENB;
	/* Even */
	// newtio.c_cflag |= PARENB;
	// newtio.c_cflag &=~ PARODD;
	/* Odd */
	// newtio.c_cflag |= PARENB;
	// newtio.c_cflag |= PARODD;

	// disable to change CR with NL 
	newtio.c_iflag &= (~ICRNL);


	// Flow-Control
#if 0
	if(!strncmp(s2wCurrent.flowctrl, "1", 1)) {
		newtio.c_cflag &= ~CRTSCTS;
		newtio.c_iflag |= (IXON|IXOFF);
	}
	else if(!strncmp(s2wCurrent.flowctrl, "2", 1)) {
		newtio.c_cflag |= CRTSCTS;
		newtio.c_iflag &= ~(IXON|IXOFF);
	}
	else {
		newtio.c_cflag &= ~CRTSCTS;
		newtio.c_iflag &= ~(IXON|IXOFF);
	}
#endif
	newtio.c_cflag &= ~CRTSCTS;
	newtio.c_iflag &= ~(IXON|IXOFF);

	// set mode - NonCanonical Mode Input Processing
	newtio.c_lflag &= (~ECHO);
	newtio.c_lflag &= (~ICANON);

	// c_oflag  - clear "Map NL to CR-NL"
	newtio.c_oflag &= (~ONLCR);

	tcflush(fd, TCIFLUSH);
	tcsetattr(fd, TCSANOW, &newtio);

	unsigned char ch;
	int rc;

	unsigned char buf[256];
	int i;

	for(i=0; i<256; i++)
		buf[i] = i+40;

   	int rtsctl;
//	rtsctl = TIOCM_RTS;
//	ioctl(fd, TIOCMBIC, &rtsctl);			// RTS Clear


	write_uart(fd, buf, 10);

	while(1)
	{
		rc = read(fd, &ch, 1);

		printf("rc: %d\n", rc);
		if (rc == 1)
			printf("%02x\n", ch);

		sleep(1);
	}

	return 0;
}
