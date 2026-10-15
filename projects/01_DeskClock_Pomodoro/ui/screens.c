#include "screens.h"
#include "graphics.h"
#include <stdio.h>
#include <string.h>

static const char *months_es[] = {
    "", "ENERO", "FEBRERO", "MARZO", "ABRIL", "MAYO", "JUNIO",
    "JULIO", "AGOSTO", "SEPTIEMBRE", "OCTUBRE", "NOVIEMBRE", "DICIEMBRE"
};

static void draw_top_bar(const AppState *app, const char *title) {
    /* Barra superior con línea divisoria fina */
    gfx_draw_rect(0, 0, SCREEN_WIDTH, 20, COLOR_CARD_DARK);
    gfx_draw_rect(0, 20, SCREEN_WIDTH, 1, COLOR_BORDER_DARK);

    /* Título / Modo en esquina izquierda */
    gfx_draw_text(12, 6, title, COLOR_TEXT_LIGHT);

    /* Batería en esquina derecha */
    char batt_str[16];
    if (app->battery.percent >= 0) {
        snprintf(batt_str, sizeof(batt_str), "%d%%", app->battery.percent);
        gfx_draw_text(SCREEN_WIDTH - 72, 6, batt_str, COLOR_TEXT_LIGHT);
        gfx_draw_battery_icon(SCREEN_WIDTH - 38, 4, app->battery.percent, 
                              app->battery.is_charging, app->battery.is_plugged, COLOR_TEXT_LIGHT);
    } else {
        gfx_draw_text(SCREEN_WIDTH - 60, 6, "--%", COLOR_TEXT_MUTED);
    }
}

static void draw_bottom_bar(const char *left_hint, const char *right_hint) {
    int y = SCREEN_HEIGHT - 18;
    gfx_draw_rect(0, y - 2, SCREEN_WIDTH, 1, COLOR_BORDER_DARK);
    gfx_draw_rect(0, y - 1, SCREEN_WIDTH, 19, COLOR_CARD_DARK);

    if (left_hint) {
        gfx_draw_text(12, y + 4, left_hint, COLOR_TEXT_MUTED);
    }
    if (right_hint) {
        int w = (int)strlen(right_hint) * 8;
        gfx_draw_text(SCREEN_WIDTH - w - 12, y + 4, right_hint, COLOR_TEXT_DIM);
    }
}

/* ========================================================================= */
/* 1. PANTALLA: RELOJ MINIMALISTA (DESK CLOCK)                                */
/* ========================================================================= */
static void render_clock(const AppState *app, const ScePspDateTime *time) {
    draw_top_bar(app, "DESK CLOCK  •  PSP GO");

    /* Fecha formateada: ej. 15 DE OCTUBRE DE 2026 */
    char date_buf[48];
    int m = time->month;
    if (m < 1 || m > 12) m = 1;
    snprintf(date_buf, sizeof(date_buf), "%02d DE %s DE %04d", time->day, months_es[m], time->year);
    gfx_draw_text_centered(34, date_buf, 1, COLOR_TEXT_MUTED);

    /* Dígitos gigantes de 7 segmentos */
    int hours = time->hour;
    int mins = time->minute;
    int secs = time->second;

    /* Dibujar reloj digital grande (altura 92 px) */
    gfx_draw_digital_clock(SCREEN_WIDTH / 2, 54, 92, hours, mins, secs, 
                           app->display_cfg.show_seconds, COLOR_TEXT_LIGHT, COLOR_SEGMENT_GHOST);

    /* Tarjeta inferior dinámica */
    int card_y = 160;
    int card_w = 440;
    int card_x = (SCREEN_WIDTH - card_w) / 2;

    if (app->pomo.state == POMO_STATE_RUNNING || app->pomo.state == POMO_STATE_PAUSED) {
        /* Mini estado del Pomodoro activo */
        gfx_draw_card(card_x, card_y, card_w, 64, COLOR_CARD_DARK, COLOR_BORDER_DARK);

        const char *phase_txt = (app->pomo.phase == POMO_PHASE_WORK) ? "POMODORO EN CURSO" : "TIEMPO DE DESCANSO";
        uint32_t col = (app->pomo.phase == POMO_PHASE_WORK) ? COLOR_POMO_WORK : COLOR_POMO_BREAK;
        gfx_draw_text(card_x + 16, card_y + 12, phase_txt, col);

        char pomo_time[16];
        uint32_t rem_sec = pomo_remaining_ms(&app->pomo, 0) / 1000;
        snprintf(pomo_time, sizeof(pomo_time), "%02u:%02u", 
                 (unsigned)(rem_sec / 60), (unsigned)(rem_sec % 60));
        gfx_draw_text_scaled(card_x + 16, card_y + 30, pomo_time, 2, COLOR_TEXT_LIGHT);

        const char *state_txt = (app->pomo.state == POMO_STATE_RUNNING) ? "Activo" : "Pausa";
        gfx_draw_text(card_x + 115, card_y + 36, state_txt, COLOR_TEXT_MUTED);

        /* Barra de progreso miniatura */
        float p = pomo_progress(&app->pomo, 0);
        gfx_draw_progress_bar(card_x + 190, card_y + 34, 230, 14, p, col, COLOR_BG_DARK);
    } else {
        /* Tarjeta de bienvenida / atajos */
        gfx_draw_card(card_x, card_y, card_w, 64, COLOR_CARD_DARK, COLOR_BORDER_DARK);
        gfx_draw_text(card_x + 16, card_y + 14, "DISPOSITIVO DE ESCRITORIO", COLOR_TEXT_MUTED);
        gfx_draw_text(card_x + 16, card_y + 34, "[L] Iniciar Pomodoro  |  [L+R] Cambiar Modo  |  [SELECT] Ayuda", COLOR_TEXT_LIGHT);
    }

    draw_bottom_bar("[L+R] Siguiente Pantalla", "PSP Go Desk App");
}

/* ========================================================================= */
/* 2. PANTALLA: POMODORO TIMER                                               */
/* ========================================================================= */
static void render_pomodoro(const AppState *app, uint64_t now_ms) {
    draw_top_bar(app, "TEMPORIZADOR POMODORO");

    const Pomo *p = &app->pomo;
    uint32_t rem_ms = pomo_remaining_ms(p, now_ms);
    uint32_t total_sec = rem_ms / 1000;
    uint32_t mins = total_sec / 60;
    uint32_t secs = total_sec % 60;

    /* Título de fase */
    const char *phase_title = "ENFOQUE Y TRABAJO";
    uint32_t accent = COLOR_POMO_WORK;

    if (p->phase == POMO_PHASE_SHORT_BREAK) {
        phase_title = "DESCANSO CORTO";
        accent = COLOR_POMO_BREAK;
    } else if (p->phase == POMO_PHASE_LONG_BREAK) {
        phase_title = "DESCANSO LARGO";
        accent = COLOR_POMO_BREAK;
    }

    gfx_draw_text_centered(30, phase_title, 1, accent);

    /* Subtítulo de estado */
    char sub[48];
    if (p->state == POMO_STATE_RUNNING) {
        snprintf(sub, sizeof(sub), "EN CURSO  •  RONDA %u/%u", (unsigned)p->current_round, (unsigned)p->config.long_break_interval);
    } else if (p->state == POMO_STATE_PAUSED) {
        snprintf(sub, sizeof(sub), "EN PAUSA  (Pulsa L para reanudar)");
    } else if (p->state == POMO_STATE_READY) {
        snprintf(sub, sizeof(sub), "FASE LISTA  (Pulsa L para iniciar)");
    } else {
        snprintf(sub, sizeof(sub), "LISTO  •  Pulsa L para empezar (25m)");
    }
    gfx_draw_text_centered(46, sub, 1, COLOR_TEXT_MUTED);

    /* Contador gigante MM:SS */
    gfx_draw_digital_clock(SCREEN_WIDTH / 2, 68, 88, (int)mins, (int)secs, 0, false, accent, COLOR_SEGMENT_GHOST);

    /* Barra de progreso suave */
    float prog = pomo_progress(p, now_ms);
    gfx_draw_progress_bar(40, 168, 400, 10, prog, accent, COLOR_CARD_DARK);

    /* Rondas completadas (4 indicadores de puntos) */
    int dots_x = SCREEN_WIDTH / 2 - 40;
    int dots_y = 190;
    for (uint32_t i = 1; i <= p->config.long_break_interval; i++) {
        uint32_t col = (i < p->current_round) ? accent : 
                       (i == p->current_round && p->phase == POMO_PHASE_WORK) ? accent : COLOR_BORDER_DARK;
        gfx_draw_rect(dots_x + (i - 1) * 26, dots_y, 16, 6, col);
    }

    /* Estadísticas del día */
    char stats_buf[64];
    snprintf(stats_buf, sizeof(stats_buf), "Completadas hoy: %u  |  Foco: %u min", 
             (unsigned)app->sessions_today, (unsigned)app->focus_minutes_today);
    gfx_draw_text_centered(210, stats_buf, 1, COLOR_TEXT_MUTED);

    draw_bottom_bar("[L] Play/Pausa   [R] Saltar   [R 1.5s] Reset", "[L+R] Modo");
}

/* ========================================================================= */
/* 3. PANTALLA: BATERÍA Y TELEMETRÍA                                         */
/* ========================================================================= */
static void render_battery(const AppState *app) {
    draw_top_bar(app, "TELEMETRIA DE HARDWARE  •  PSP GO");

    /* Tarjeta principal de batería */
    gfx_draw_card(20, 32, 440, 78, COLOR_CARD_DARK, COLOR_BORDER_DARK);

    /* Icono gigante */
    gfx_draw_battery_icon(40, 48, app->battery.percent, app->battery.is_charging, app->battery.is_plugged, COLOR_TEXT_LIGHT);

    /* Porcentaje grande */
    char pct_str[16];
    if (app->battery.percent >= 0) {
        snprintf(pct_str, sizeof(pct_str), "%d%%", app->battery.percent);
    } else {
        strcpy(pct_str, "--%");
    }
    gfx_draw_text_scaled(80, 44, pct_str, 3, COLOR_TEXT_LIGHT);

    /* Estado de carga */
    const char *status_txt = "Descargando con bateria";
    uint32_t status_col = COLOR_TEXT_MUTED;
    if (app->battery.is_charging) {
        status_txt = "Bateria cargando...";
        status_col = COLOR_POMO_BREAK;
    } else if (app->battery.is_plugged) {
        status_txt = "Alimentacion externa conectada";
        status_col = COLOR_CYAN_ACCENT;
    }
    gfx_draw_text(80, 80, status_txt, status_col);

    /* Cuadrícula de 4 datos técnicos */
    int gw = 214;
    int gh = 44;
    int gy = 120;

    /* 1. Voltaje */
    gfx_draw_card(20, gy, gw, gh, COLOR_CARD_DARK, COLOR_BORDER_DARK);
    gfx_draw_text(30, gy + 8, "VOLTAJE DE BATERIA", COLOR_TEXT_MUTED);
    char volt_buf[32];
    if (app->battery.voltage_mv > 0) {
        snprintf(volt_buf, sizeof(volt_buf), "%d mV  (%.2f V)", app->battery.voltage_mv, app->battery.voltage_mv / 1000.0f);
    } else {
        strcpy(volt_buf, "--");
    }
    gfx_draw_text(30, gy + 24, volt_buf, COLOR_TEXT_LIGHT);

    /* 2. Tiempo Restante */
    gfx_draw_card(246, gy, gw, gh, COLOR_CARD_DARK, COLOR_BORDER_DARK);
    gfx_draw_text(256, gy + 8, "TIEMPO ESTIMADO", COLOR_TEXT_MUTED);
    char time_buf[32];
    if (app->battery.life_time_min > 0) {
        snprintf(time_buf, sizeof(time_buf), "%d h %02d min", app->battery.life_time_min / 60, app->battery.life_time_min % 60);
    } else if (app->battery.is_charging || app->battery.is_plugged) {
        strcpy(time_buf, "Conectada a la red");
    } else {
        strcpy(time_buf, "Calculando...");
    }
    gfx_draw_text(256, gy + 24, time_buf, COLOR_TEXT_LIGHT);

    /* 3. Temperatura */
    gfx_draw_card(20, gy + 52, gw, gh, COLOR_CARD_DARK, COLOR_BORDER_DARK);
    gfx_draw_text(30, gy + 60, "TEMPERATURA", COLOR_TEXT_MUTED);
    char temp_buf[32];
    if (app->battery.temp_c >= 0) {
        snprintf(temp_buf, sizeof(temp_buf), "%d C", app->battery.temp_c);
    } else {
        strcpy(temp_buf, "-- (Sensor no expuesto)");
    }
    gfx_draw_text(30, gy + 76, temp_buf, COLOR_TEXT_LIGHT);

    /* 4. Almacenamiento */
    gfx_draw_card(246, gy + 52, gw, gh, COLOR_CARD_DARK, COLOR_BORDER_DARK);
    gfx_draw_text(256, gy + 60, "ALMACENAMIENTO ACTIVO", COLOR_TEXT_MUTED);
    gfx_draw_text(256, gy + 76, "16 GB Flash Interna (ef0:)", COLOR_TEXT_LIGHT);

    draw_bottom_bar("[L+R] Siguiente Pantalla", "PSP-N1000");
}

/* ========================================================================= */
/* 4. PANTALLA: GUÍA RÁPIDA DE CONTROLES (HELP / CHEATSHEET)                 */
/* ========================================================================= */
static void render_help(const AppState *app) {
    (void)app;
    draw_top_bar(app, "GUIA RAPIDA DE CONTROLES  •  CHEATSHEET");

    int col_w = 214;
    int col_h = 170;
    int y = 32;

    /* Columna 1: Consola CERRADA (Slider plegado) */
    gfx_draw_card(20, y, col_w, col_h, COLOR_CARD_DARK, COLOR_BORDER_DARK);
    gfx_draw_text(30, y + 10, "CONSOLA CERRADA (MESA)", COLOR_AMBER_ACCENT);
    gfx_draw_rect(30, y + 24, col_w - 20, 1, COLOR_BORDER_DARK);

    gfx_draw_text(30, y + 34, "[Gatillo L]", COLOR_TEXT_LIGHT);
    gfx_draw_text(30, y + 46, "Play / Pausar Pomodoro", COLOR_TEXT_MUTED);

    gfx_draw_text(30, y + 62, "[Gatillo R]", COLOR_TEXT_LIGHT);
    gfx_draw_text(30, y + 74, "Saltar a siguiente fase", COLOR_TEXT_MUTED);

    gfx_draw_text(30, y + 90, "[Gatillo R (1.5s)]", COLOR_TEXT_LIGHT);
    gfx_draw_text(30, y + 102, "Reiniciar Pomodoro", COLOR_TEXT_MUTED);

    gfx_draw_text(30, y + 118, "[Gatillos L + R (1s)]", COLOR_CYAN_ACCENT);
    gfx_draw_text(30, y + 130, "Cambiar pantalla", COLOR_TEXT_LIGHT);

    gfx_draw_text(30, y + 146, "[Boton PS / HOME]", COLOR_TEXT_LIGHT);
    gfx_draw_text(30, y + 158, "Salir al menu XMB", COLOR_TEXT_MUTED);

    /* Columna 2: Consola ABIERTA (Desplegada) */
    gfx_draw_card(246, y, col_w, col_h, COLOR_CARD_DARK, COLOR_BORDER_DARK);
    gfx_draw_text(256, y + 10, "CONSOLA ABIERTA", COLOR_CYAN_ACCENT);
    gfx_draw_rect(256, y + 24, col_w - 20, 1, COLOR_BORDER_DARK);

    gfx_draw_text(256, y + 34, "[Cruceta < / >]", COLOR_TEXT_LIGHT);
    gfx_draw_text(256, y + 46, "Navegar pantallas", COLOR_TEXT_MUTED);

    gfx_draw_text(256, y + 62, "[Boton SELECT]", COLOR_TEXT_LIGHT);
    gfx_draw_text(256, y + 74, "Abrir / Cerrar esta guia", COLOR_TEXT_MUTED);

    gfx_draw_text(256, y + 90, "[Boton X (Cruz)]", COLOR_TEXT_LIGHT);
    gfx_draw_text(256, y + 102, "Confirmar / Iniciar foco", COLOR_TEXT_MUTED);

    gfx_draw_text(256, y + 118, "[Boton O (Circulo)]", COLOR_TEXT_LIGHT);
    gfx_draw_text(256, y + 130, "Volver al Reloj", COLOR_TEXT_MUTED);

    gfx_draw_text(256, y + 146, "[Boton START]", COLOR_TEXT_LIGHT);
    gfx_draw_text(256, y + 158, "Menu rapido", COLOR_TEXT_MUTED);

    draw_bottom_bar("Pulsa [SELECT] o [O] para cerrar esta guia", "Modo Ayuda");
}

/* Despacho de la pantalla actual */
void render_active_screen(const AppState *app, const ScePspDateTime *time, uint64_t now_ms) {
    if (!app || !time) return;

    switch (app->current_screen) {
        case SCREEN_CLOCK:
            render_clock(app, time);
            break;
        case SCREEN_POMODORO:
            render_pomodoro(app, now_ms);
            break;
        case SCREEN_BATTERY:
            render_battery(app);
            break;
        case SCREEN_HELP:
        case SCREEN_SETTINGS:
        default:
            render_help(app);
            break;
    }
}
