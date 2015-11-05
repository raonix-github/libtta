#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <tta.h>

#include "unit_test.h"

#define CMD_TYPE_REQ_LOOPBACK	0xff

enum {
	TCP,
	RS485
};

int main(int argc, char *argv[])
{
	tta_t *ctx;
	ttamsg_t *ttareq=NULL, *ttarsp=NULL;
	unsigned char subid;
	int grid, swid;
	unsigned char buf[64];
	int rc;
	int i, ch;

	int use_backend;

	if (argc > 1) {
		if (strcmp(argv[1], "tcp") == 0) {
			use_backend = TCP;
		} else if (strcmp(argv[1], "rs485") == 0) {
			use_backend = RS485;
		} else {
			printf("Usage:\n  %s [tcp|tcppi|rs485] - Modbus client for unit testing\n\n", argv[0]);
			exit(1);
		}
	} else {
		/* By default */
		use_backend = TCP;
	}

	if (use_backend == TCP) {
		ctx = tta_new_tcp("127.0.0.1", 1502);
	} else {
		ctx = tta_new_rs485("/dev/ttyS0", 9600, 'N', 8, 1);
	}
	if (ctx == NULL) {
		fprintf(stderr, "Unable to allocate libtta context\n");
		return -1;
	}
	tta_set_debug(ctx, TRUE);
	tta_set_error_recovery(ctx,
			TTA_ERROR_RECOVERY_LINK |
			TTA_ERROR_RECOVERY_PROTOCOL);


	if (tta_connect(ctx) == -1) {
		fprintf(stderr, "Connection failed: %s\n", tta_strerror(errno));
		tta_free(ctx);
		return -1;
	}

	printf("** UNIT TESTING **\n");

	grid = 1;
	swid = 1;
	subid = (grid << 4) | swid;
	for(i=0; i<(int)sizeof(buf); i++)
		buf[i] = i;

#if 1
while ((ch = getchar ()) != 'q')
{
    switch (ch)
    {
    case '0':                ttareq = ttamsg_make (0x36, 0x0f, 0x01, NULL, 0); break;
    case '1':                ttareq = ttamsg_make (0x36, 0x11, 0x01, NULL, 0); break;
    case '2':                ttareq = ttamsg_make (0x36, 0x1f, 0x0f, NULL, 0); break;
    case '3': buf[0] = 0x01; ttareq = ttamsg_make (0x36, 0x11, 0x43, buf , 1); break;
    case '4': buf[0] = 0x00; ttareq = ttamsg_make (0x36, 0x11, 0x43, buf , 1); break;
    case '5': buf[0] = 0x20; ttareq = ttamsg_make (0x36, 0x11, 0x44, buf , 1); break;
    case '6': buf[0] = 0x00; ttareq = ttamsg_make (0x36, 0x11, 0x45, buf , 1); break;
    case '7': buf[0] = 0x01; ttareq = ttamsg_make (0x36, 0x11, 0x45, buf , 1); break;
    case '8': buf[0] = 0x00; ttareq = ttamsg_make (0x36, 0x11, 0x46, buf , 1); break;
    case '9': buf[0] = 0x01; ttareq = ttamsg_make (0x36, 0x11, 0x46, buf , 1); break;
    case 'a': buf[0] = 0x01; ttareq = ttamsg_make (0x36, 0x01, 0x43, buf , 1); break;
    case 'b': buf[0] = 0x18; ttareq = ttamsg_make (0x36, 0x01, 0x44, buf , 1); break;
    case 'c': buf[0] = 0x01; ttareq = ttamsg_make (0x36, 0x11, 0x43, buf , 1); break;
    case 'd': buf[0] = 0x18; ttareq = ttamsg_make (0x36, 0x11, 0x44, buf , 1); break;
    default : continue;
    }

	//- ttareq = ttamsg_make(0xff, subid, CMD_TYPE_REQ_LOOPBACK, buf, sizeof(buf));
	//-- ttareq = ttamsg_make(0x36, 0x1f, 0x0f, NULL, 0);

	printf(">>>> Send\n");
	ttamsg_print(ttareq);
	rc = tta_request(ctx, ttareq, &ttarsp);
	if(rc < 0)
	{
		printf("unit_test_master: %d:%d fail to loopback", grid, swid);
		goto close;
	}
	printf(">>>> Recevie\n");
	ttamsg_print(ttarsp);

	if(ttarsp)
	{
		ttamsg_free(ttarsp);
	}
}
#else
	//- ttareq = ttamsg_make(0xff, subid, CMD_TYPE_REQ_LOOPBACK, buf, sizeof(buf));
    buf[0] = 0x00; ttareq = ttamsg_make (0x36, 0x11, 0x43, buf , 1);
    //buf[0] = 0x01; ttareq = ttamsg_make (0x36, 0x11, 0x43, buf , 1);

	printf(">>>> Send\n");
	ttamsg_print(ttareq);
	rc = tta_request(ctx, ttareq, &ttarsp);
	if(rc < 0)
	{
		printf("unit_test_master: %d:%d fail to loopback", grid, swid);
		goto close;
	}
	printf(">>>> Recevie\n");
	ttamsg_print(ttarsp);

	if(ttarsp)
	{
		ttamsg_free(ttarsp);
	}
#endif

close:
	/* Close the connection */
	tta_close(ctx);
	tta_free(ctx);

	return 0;
}
