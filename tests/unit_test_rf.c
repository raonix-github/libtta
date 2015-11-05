#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <tta.h>

#include "dbg.h"

#define	DBG_TAG "TTA RF"

int tta_thermostat_get_chars(tta_t *ttactx , int subid, unsigned char *buf)
{
	ttamsg_t *ttareq = NULL, *ttarsp = NULL;
	int rc;

	ttareq = ttamsg_make(0x36, subid, 0x0f, NULL, 0);
//	ttamsg_print(ttareq);
	rc = tta_request(ttactx, ttareq, &ttarsp);
	if (rc < 0)
	{
		_dbg (DBG_TAG, "tta_request failed(%d)\n", rc);
		return -1;
	}

	if (ttarsp)
	{
//		ttamsg_print(ttarsp);
		printf(">>>> Recevie\n");
		ttamsg_free (ttarsp);
	}

	return rc;
}

int tta_thermostat_get_state(tta_t *ttactx , int subid, unsigned char *buf)
{
	ttamsg_t *ttareq = NULL, *ttarsp = NULL;
	int rc;

	ttareq = ttamsg_make(0x36, subid, 0x01, NULL, 0);
//	ttamsg_print(ttareq);
	rc = tta_request(ttactx, ttareq, &ttarsp);
	if (rc < 0)
	{
		_dbg (DBG_TAG, "tta_request failed(%d)\n", rc);
		return -1;
	}

	if (ttarsp)
	{
//		ttamsg_print(ttarsp);
		printf(">>>> Recevie\n");
		ttamsg_free (ttarsp);
	}

	return rc;
}

int main(int argc, char *argv[])
{
	tta_t *ctx;
	unsigned char buf[64];
	int rc;
	int i, ch;

	ctx = tta_new_rf("/dev/spiS0");
	if (ctx == NULL) {
		fprintf(stderr, "Unable to allocate libtta context\n");
		return -1;
	}

	struct timeval timeout;

	timeout.tv_sec = 5;
	timeout.tv_usec = 0;

	tta_set_response_timeout(ctx, &timeout);

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

	for(i=0; i<(int)sizeof(buf); i++)
		buf[i] = i;

	while ((ch = getchar ()) != 'q')
	{
		printf(">>>> Send\n");
		// tta_thermostat_get_chars(ctx , 0x1f, buf);
		tta_thermostat_get_state(ctx , 0x1f, buf);
	}

close:
	/* Close the connection */
	tta_close(ctx);
	tta_free(ctx);

	return 0;
}
