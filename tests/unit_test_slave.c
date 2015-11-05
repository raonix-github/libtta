#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <tta.h>

#include "unit_test.h"

enum {
    TCP,
    RS485
};

int main(int argc, char*argv[])
{
    tta_t *ctx;
	ttamsg_t *ttarx=NULL;
//	ttamsg_t *ttarsp=NULL;
    int s = -1;
    int rc;
    int use_backend;

    if (argc > 1) {
        if (strcmp(argv[1], "tcp") == 0) {
            use_backend = TCP;
        } else if (strcmp(argv[1], "rs485") == 0) {
            use_backend = RS485;
        } else {
            printf("Usage:\n  %s [tcp|tcppi|rs485] - Modbus server for unit testing\n\n", argv[0]);
            return -1;
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

    tta_set_debug(ctx, TRUE);

    if (use_backend == TCP) {
        s = tta_tcp_listen(ctx, 1);
        tta_tcp_accept(ctx, &s);
    } else {
        rc = tta_connect(ctx);
        if (rc == -1) {
            fprintf(stderr, "Unable to connect %s\n", tta_strerror(errno));
            tta_free(ctx);
            return -1;
        }
    }

    for (;;) {
        do {
            rc = tta_receive(ctx, &ttarx, 0);
            /* Filtered queries return 0 */
        } while (rc == 0);

        if (rc == -1) {
            /* Connection closed by the client or error */
            break;
        }
		printf(">>>> Recevie\n");
		ttamsg_print(ttarx);

		// Loobpack receve packet
        rc = tta_reply(ctx, ttarx, ttarx->data, ttarx->hd->datalen);
        if (rc == -1) {
            break;
        }
    }

    if (use_backend == TCP) {
        if (s != -1) {
            close(s);
        }
    }

    tta_close(ctx);
    tta_free(ctx);

    return 0;
}
