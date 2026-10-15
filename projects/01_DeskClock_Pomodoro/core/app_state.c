#include "app_state.h"
#include <stddef.h>

void app_init(AppState *app) {
    if (!app) return;

    app->current_screen = SCREEN_CLOCK;
    app->prev_screen = SCREEN_CLOCK;

    pomo_init(&app->pomo, NULL);

    app->display_cfg.format_24h = true;
    app->display_cfg.show_seconds = true;
    app->display_cfg.sound_enabled = true;
    app->display_cfg.pixel_shift = true;
    app->display_cfg.theme = THEME_DARK;

    app->battery.percent = -1;
    app->battery.life_time_min = -1;
    app->battery.voltage_mv = 0;
    app->battery.temp_c = -1;
    app->battery.is_charging = false;
    app->battery.is_plugged = false;

    app->sessions_today = 0;
    app->sessions_total = 0;
    app->focus_minutes_today = 0;

    app->needs_save = false;
    app->sound_alert_pending = false;
}

void app_cycle_screen(AppState *app) {
    if (!app) return;

    switch (app->current_screen) {
        case SCREEN_CLOCK:
            app->current_screen = SCREEN_POMODORO;
            break;
        case SCREEN_POMODORO:
            app->current_screen = SCREEN_BATTERY;
            break;
        case SCREEN_BATTERY:
            app->current_screen = SCREEN_HELP;
            break;
        case SCREEN_HELP:
        case SCREEN_SETTINGS:
        default:
            app->current_screen = SCREEN_CLOCK;
            break;
    }
}

void app_handle_cmd(AppState *app, InputCmd cmd, uint64_t now_ms) {
    if (!app || cmd == CMD_NONE) return;

    /* Comandos globales prioritarios */
    if (cmd == CMD_LR_COMBO) {
        app_cycle_screen(app);
        return;
    }

    /* Acceso directo a la Guía de Controles con SELECT */
    if (cmd == CMD_SELECT) {
        if (app->current_screen == SCREEN_HELP) {
            app->current_screen = app->prev_screen;
        } else {
            app->prev_screen = app->current_screen;
            app->current_screen = SCREEN_HELP;
        }
        return;
    }

    if (cmd == CMD_START) {
        if (app->current_screen == SCREEN_SETTINGS) {
            app->current_screen = app->prev_screen;
        } else {
            app->prev_screen = app->current_screen;
            app->current_screen = SCREEN_SETTINGS;
        }
        return;
    }

    /* Manejo específico por pantalla */
    switch (app->current_screen) {
        case SCREEN_CLOCK:
            if (cmd == CMD_L_TAP || cmd == CMD_CONFIRM) {
                app->current_screen = SCREEN_POMODORO;
                if (app->pomo.state == POMO_STATE_IDLE) {
                    pomo_toggle(&app->pomo, now_ms);
                }
            } else if (cmd == CMD_R_TAP || cmd == CMD_RIGHT) {
                app->current_screen = SCREEN_POMODORO;
            } else if (cmd == CMD_LEFT) {
                app->current_screen = SCREEN_HELP;
            }
            break;

        case SCREEN_POMODORO:
            if (cmd == CMD_L_TAP) {
                pomo_toggle(&app->pomo, now_ms);
            } else if (cmd == CMD_R_TAP) {
                pomo_skip(&app->pomo, now_ms);
            } else if (cmd == CMD_R_HOLD) {
                pomo_reset(&app->pomo);
            } else if (cmd == CMD_BACK || cmd == CMD_LEFT) {
                app->current_screen = SCREEN_CLOCK;
            } else if (cmd == CMD_RIGHT) {
                app->current_screen = SCREEN_BATTERY;
            }
            break;

        case SCREEN_BATTERY:
            if (cmd == CMD_BACK || cmd == CMD_CONFIRM || cmd == CMD_LEFT) {
                app->current_screen = SCREEN_POMODORO;
            } else if (cmd == CMD_RIGHT) {
                app->current_screen = SCREEN_HELP;
            }
            break;

        case SCREEN_HELP:
            if (cmd == CMD_BACK || cmd == CMD_CONFIRM) {
                app->current_screen = app->prev_screen;
            } else if (cmd == CMD_RIGHT) {
                app->current_screen = SCREEN_CLOCK;
            } else if (cmd == CMD_LEFT) {
                app->current_screen = SCREEN_BATTERY;
            }
            break;

        case SCREEN_SETTINGS:
            if (cmd == CMD_BACK) {
                app->current_screen = app->prev_screen;
            }
            break;
    }
}

void app_update(AppState *app, uint64_t now_ms) {
    if (!app) return;

    PomoEvent ev = pomo_update(&app->pomo, now_ms);

    if (ev == POMO_EVENT_WORK_COMPLETED) {
        app->sessions_today++;
        app->sessions_total++;
        app->focus_minutes_today += app->pomo.config.work_min;
        app->needs_save = true;
        if (app->display_cfg.sound_enabled) {
            app->sound_alert_pending = true;
        }
    } else if (ev == POMO_EVENT_BREAK_COMPLETED) {
        if (app->display_cfg.sound_enabled) {
            app->sound_alert_pending = true;
        }
    }
}
