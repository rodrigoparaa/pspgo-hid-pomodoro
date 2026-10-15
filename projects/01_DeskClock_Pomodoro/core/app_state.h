#ifndef APP_STATE_H
#define APP_STATE_H

#include <stdint.h>
#include <stdbool.h>
#include "pomodoro.h"
#include "input_logic.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SCREEN_CLOCK = 0,
    SCREEN_POMODORO,
    SCREEN_BATTERY,
    SCREEN_HELP,
    SCREEN_SETTINGS
} ScreenMode;

typedef enum {
    THEME_DARK = 0,
    THEME_AMBER,
    THEME_GREEN,
    THEME_WHITE
} ColorTheme;

typedef struct {
    bool format_24h;
    bool show_seconds;
    bool sound_enabled;
    bool pixel_shift;
    ColorTheme theme;
} DisplayConfig;

typedef struct {
    int percent;          /* 0 a 100 (-1 si no disponible) */
    int life_time_min;    /* Minutos restantes (-1 si cargando o desconocido) */
    int voltage_mv;       /* Milivoltios (ej: 4050) */
    int temp_c;           /* Grados Celsius (-1 si no soportado por hardware) */
    bool is_charging;
    bool is_plugged;
} BatteryState;

typedef struct {
    ScreenMode current_screen;
    ScreenMode prev_screen;
    Pomo pomo;
    DisplayConfig display_cfg;
    BatteryState battery;
    
    uint32_t sessions_today;
    uint32_t sessions_total;
    uint32_t focus_minutes_today;
    
    bool needs_save;
    bool sound_alert_pending; /* Bandera para reproducir pitido en la plataforma */
} AppState;

void app_init(AppState *app);

/* Enruta comandos de entrada según la pantalla activa */
void app_handle_cmd(AppState *app, InputCmd cmd, uint64_t now_ms);

/* Actualización periódica por frame */
void app_update(AppState *app, uint64_t now_ms);

/* Cambiar cíclicamente de pantalla (invocado por L+R o D-Pad) */
void app_cycle_screen(AppState *app);

#ifdef __cplusplus
}
#endif

#endif /* APP_STATE_H */
