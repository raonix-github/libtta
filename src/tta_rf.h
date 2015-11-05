#ifndef _TTA_RF_H_
#define _TTA_RF_H_

#include "tta.h"

TTA_BEGIN_DECLS

#define TTA_RF_MAX_ADU_LENGTH 256

tta_t* tta_new_rf (const char *device);
int tta_rf_set_id (tta_t *ctx, uint8_t *id);
int tta_rf_get_id (tta_t *ctx, uint8_t *id);
int tta_rf_set_ch (tta_t *ctx, int ch);
int tta_rf_set_power (tta_t *ctx, int power);
int tta_rf_set_agc_gain_adjust (tta_t *ctx, int val);
int tta_rf_set_agc_cs_thr (tta_t *ctx, int thr);
int tta_rf_get_rssi (tta_t *ctx, int *rssi);

TTA_END_DECLS

#endif /* _TTA_RF_H_ */
