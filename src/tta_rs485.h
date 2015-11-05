#ifndef _TTA_RS485_H_
#define _TTA_RS485_H_

#include "tta.h"

TTA_BEGIN_DECLS

#define STX		0xF7

#define TTA_RS485_MAX_ADU_LENGTH 256

tta_t* tta_new_rs485(const char *device, int baud, char parity,
                                int data_bit, int stop_bit);

#define TTA_RS485_RS232 0
#define TTA_RS485_RS485 1

int tta_rs485_set_serial_mode(tta_t *ctx, int mode);
int tta_rs485_get_serial_mode(tta_t *ctx);

#define TTA_RS485_RTS_NONE   0
#define TTA_RS485_RTS_UP     1
#define TTA_RS485_RTS_DOWN   2

int tta_rs485_set_rts(tta_t *ctx, int mode);
int tta_rs485_get_rts(tta_t *ctx);

TTA_END_DECLS

#endif /* _TTA_RS485_H_ */
