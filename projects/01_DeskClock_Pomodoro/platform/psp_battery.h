#ifndef PSP_BATTERY_H
#define PSP_BATTERY_H

#include "../core/app_state.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Actualiza la estructura de batería consultando las APIs de psppower */
void battery_update(BatteryState *state);

#ifdef __cplusplus
}
#endif

#endif /* PSP_BATTERY_H */
