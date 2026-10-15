#include "psp_battery.h"
#include <psppower.h>

void battery_update(BatteryState *state) {
    if (!state) return;

    /* Porcentaje de batería (0-100) */
    int pct = scePowerGetBatteryLifePercent();
    state->percent = (pct >= 0 && pct <= 100) ? pct : -1;

    /* Tiempo restante en minutos */
    int time_min = scePowerGetBatteryLifeTime();
    state->life_time_min = (time_min >= 0 && time_min < 2000) ? time_min : -1;

    /* Voltaje en mV (ej: 4050 mV) */
    int volt = scePowerGetBatteryVolt();
    state->voltage_mv = (volt > 0 && volt < 6000) ? volt : 0;

    /* Temperatura en °C (Sanitizada: si devuelve negativo/error de hardware, se marca como no disponible) */
    int temp = scePowerGetBatteryTemp();
    if (temp >= 0 && temp <= 80) {
        state->temp_c = temp;
    } else {
        state->temp_c = -1; /* No soportado por el sensor de la PSP Go */
    }

    state->is_charging = (scePowerIsBatteryCharging() == 1);
    state->is_plugged = (scePowerIsPowerOnline() == 1);
}
