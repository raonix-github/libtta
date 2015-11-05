#ifndef _TTA_RF_PRIVATE_H_
#define _TTA_RF_PRIVATE_H_

#ifndef _MSC_VER
#include <stdint.h>
#else
#include "stdint.h"
#endif

// TODO : for test
#define _TTA_RF_PREHEADER_LENGTH		4
#define _TTA_RF_HEADER_LENGTH			8
// #define _TTA_RF_PREHEADER_LENGTH		1
// #define _TTA_RF_HEADER_LENGTH		5
#define _TTA_RF_CHECKSUM_LENGTH			2

#define _TTA_RF_ID_LENGTH				4
#define _TTA_RF_MAX_CH					10

typedef struct _tta_rf {
    char device[16]; // device drvier

	// RF
    int ch;
	int power;
	uint8_t id[_TTA_RF_ID_LENGTH];
	int rssi;

    /* To handle many slaves on the same link */
    int confirmation_to_ignore;
} tta_rf_t;

#endif /* _TTA_RF_PRIVATE_H_ */
