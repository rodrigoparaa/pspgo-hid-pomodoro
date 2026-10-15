#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "../core/pomodoro.h"
#include "../core/input_logic.h"
#include "../core/app_state.h"

static void test_pomodoro_logic(void) {
    printf("[TEST] Verificando lógica del Pomodoro...\n");

    Pomo pomo;
    PomoConfig cfg = {
        .work_min = 25,
        .short_break_min = 5,
        .long_break_min = 15,
        .long_break_interval = 4,
        .auto_start = false
    };
    pomo_init(&pomo, &cfg);

    assert(pomo.state == POMO_STATE_IDLE);
    assert(pomo.phase == POMO_PHASE_WORK);
    assert(pomo.current_round == 1);
    assert(pomo_remaining_ms(&pomo, 0) == 25 * 60 * 1000);

    /* Iniciar con gatillo L */
    pomo_toggle(&pomo, 1000);
    assert(pomo.state == POMO_STATE_RUNNING);

    /* Avanzar 10 minutos */
    uint64_t t = 1000 + (10 * 60 * 1000);
    assert(pomo_remaining_ms(&pomo, t) == 15 * 60 * 1000);
    assert(pomo_update(&pomo, t) == POMO_EVENT_NONE);

    /* Pausar con gatillo L */
    pomo_toggle(&pomo, t);
    assert(pomo.state == POMO_STATE_PAUSED);

    /* Pasar 5 minutos en pausa: el tiempo restante DEBE congelarse */
    t += (5 * 60 * 1000);
    assert(pomo_remaining_ms(&pomo, t) == 15 * 60 * 1000);
    assert(pomo_update(&pomo, t) == POMO_EVENT_NONE);

    /* Reanudar con gatillo L */
    pomo_toggle(&pomo, t);
    assert(pomo.state == POMO_STATE_RUNNING);

    /* Avanzar 15 minutos exactos para completar los 25 min de trabajo */
    t += (15 * 60 * 1000);
    PomoEvent ev = pomo_update(&pomo, t);
    assert(ev == POMO_EVENT_WORK_COMPLETED);
    assert(pomo.completed_sessions == 1);
    assert(pomo.phase == POMO_PHASE_SHORT_BREAK);
    assert(pomo.state == POMO_STATE_READY);
    assert(pomo_remaining_ms(&pomo, t) == 5 * 60 * 1000);

    /* Saltar descanso corto con R */
    pomo_skip(&pomo, t);
    assert(pomo.phase == POMO_PHASE_WORK);
    assert(pomo.current_round == 2);

    /* Completar rondas 2 y 3 rápidamente */
    pomo.completed_sessions = 3;
    pomo.current_round = 4;
    pomo.phase = POMO_PHASE_WORK;
    pomo.duration_ms = 25 * 60 * 1000;
    pomo.state = POMO_STATE_RUNNING;
    pomo.phase_start_ms = t;
    pomo.accumulated_ms = 0;

    /* Completar la ronda 4 -> Debe pasar a DESCANSO LARGO */
    t += (25 * 60 * 1000);
    ev = pomo_update(&pomo, t);
    assert(ev == POMO_EVENT_WORK_COMPLETED);
    assert(pomo.phase == POMO_PHASE_LONG_BREAK);
    assert(pomo_remaining_ms(&pomo, t) == 15 * 60 * 1000);

    /* Reset con R sostenido */
    pomo_reset(&pomo);
    assert(pomo.state == POMO_STATE_IDLE);
    assert(pomo.phase == POMO_PHASE_WORK);
    assert(pomo.current_round == 1);

    printf("  ✓ Lógica del Pomodoro verificada al 100%%\n");
}

static void test_input_logic(void) {
    printf("[TEST] Verificando filtro de gestos y cancelación de combo...\n");

    InputProcessor proc;
    input_init(&proc);

    /* Test 1: Toque corto en L (< 600 ms) */
    assert(input_process(&proc, BTN_LTRIGGER, 100) == CMD_NONE);
    assert(input_process(&proc, 0, 300) == CMD_L_TAP);

    /* Test 2: Toque corto en R (< 600 ms) */
    assert(input_process(&proc, BTN_RTRIGGER, 500) == CMD_NONE);
    assert(input_process(&proc, 0, 700) == CMD_R_TAP);

    /* Test 3: R sostenido (> 1500 ms) para Reset */
    assert(input_process(&proc, BTN_RTRIGGER, 1000) == CMD_NONE);
    assert(input_process(&proc, BTN_RTRIGGER, 2000) == CMD_NONE);
    assert(input_process(&proc, BTN_RTRIGGER, 2600) == CMD_R_HOLD);
    /* Al soltar R sostenido NO debe emitir R_TAP */
    assert(input_process(&proc, 0, 2700) == CMD_NONE);

    /* Test 4: Combo L + R sostenido (> 1000 ms) */
    assert(input_process(&proc, BTN_LTRIGGER | BTN_RTRIGGER, 3000) == CMD_NONE);
    assert(input_process(&proc, BTN_LTRIGGER | BTN_RTRIGGER, 3500) == CMD_NONE);
    assert(input_process(&proc, BTN_LTRIGGER | BTN_RTRIGGER, 4100) == CMD_LR_COMBO);
    /* Al soltar L o R NO debe emitir toques falsos */
    assert(input_process(&proc, BTN_LTRIGGER, 4200) == CMD_NONE);
    assert(input_process(&proc, 0, 4300) == CMD_NONE);

    printf("  ✓ Gestos L, R y cancelación de combo L+R verificados al 100%%\n");
}

static void test_app_navigation(void) {
    printf("[TEST] Verificando navegación entre pantallas y estados...\n");

    AppState app;
    app_init(&app);

    assert(app.current_screen == SCREEN_CLOCK);

    /* Combo L+R cambia cíclicamente de pantalla: Clock -> Pomo -> Battery -> Help -> Clock */
    app_handle_cmd(&app, CMD_LR_COMBO, 100);
    assert(app.current_screen == SCREEN_POMODORO);

    app_handle_cmd(&app, CMD_LR_COMBO, 200);
    assert(app.current_screen == SCREEN_BATTERY);

    app_handle_cmd(&app, CMD_LR_COMBO, 300);
    assert(app.current_screen == SCREEN_HELP);

    app_handle_cmd(&app, CMD_LR_COMBO, 400);
    assert(app.current_screen == SCREEN_CLOCK);

    /* SELECT abre y cierra la Guía de Controles */
    app_handle_cmd(&app, CMD_SELECT, 500);
    assert(app.current_screen == SCREEN_HELP);

    app_handle_cmd(&app, CMD_SELECT, 600);
    assert(app.current_screen == SCREEN_CLOCK);

    /* Desde Reloj, toque en L entra directo a Pomodoro y arranca */
    app_handle_cmd(&app, CMD_L_TAP, 600);
    assert(app.current_screen == SCREEN_POMODORO);
    assert(app.pomo.state == POMO_STATE_RUNNING);

    printf("  ✓ Navegación de estados verificada al 100%%\n");
}

int main(void) {
    printf("====================================================\n");
    printf("  SUITE DE PRUEBAS UNITARIAS: CORE DESKCLOCK/POMO   \n");
    printf("====================================================\n");

    test_pomodoro_logic();
    test_input_logic();
    test_app_navigation();

    printf("\n🎉 TODAS LAS PRUEBAS PASARON EXITOSAMENTE (3/3)\n");
    return 0;
}
