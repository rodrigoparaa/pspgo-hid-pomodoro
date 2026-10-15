#include "input_logic.h"

#define TAP_MAX_MS       600
#define LR_COMBO_HOLD_MS 1000
#define R_HOLD_MS        1500

void input_init(InputProcessor *proc) {
    if (!proc) return;
    proc->prev_buttons = 0;
    proc->l_press_time = 0;
    proc->r_press_time = 0;
    proc->l_down = false;
    proc->r_down = false;
    proc->l_consumed = false;
    proc->r_consumed = false;
    proc->lr_combo_fired = false;
}

InputCmd input_process(InputProcessor *proc, uint32_t current_buttons, uint64_t now_ms) {
    if (!proc) return CMD_NONE;

    bool l_now = (current_buttons & BTN_LTRIGGER) != 0;
    bool r_now = (current_buttons & BTN_RTRIGGER) != 0;
    InputCmd emitted = CMD_NONE;

    /* Detectar nueva pulsación de L */
    if (l_now && !proc->l_down) {
        proc->l_down = true;
        proc->l_press_time = now_ms;
        proc->l_consumed = false;
    }

    /* Detectar nueva pulsación de R */
    if (r_now && !proc->r_down) {
        proc->r_down = true;
        proc->r_press_time = now_ms;
        proc->r_consumed = false;
    }

    /* Comprobar Combo L + R sostenido */
    if (proc->l_down && proc->r_down && !proc->lr_combo_fired) {
        uint64_t l_dur = now_ms - proc->l_press_time;
        uint64_t r_dur = now_ms - proc->r_press_time;
        uint64_t combo_dur = (l_dur < r_dur) ? l_dur : r_dur;

        if (combo_dur >= LR_COMBO_HOLD_MS) {
            proc->lr_combo_fired = true;
            proc->l_consumed = true;
            proc->r_consumed = true;
            emitted = CMD_LR_COMBO;
        }
    }

    /* Comprobar R sostenido (Reset) */
    if (proc->r_down && !proc->l_down && !proc->r_consumed && !proc->lr_combo_fired) {
        if ((now_ms - proc->r_press_time) >= R_HOLD_MS) {
            proc->r_consumed = true;
            emitted = CMD_R_HOLD;
        }
    }

    /* Detectar liberación de L (Tap) */
    if (!l_now && proc->l_down) {
        proc->l_down = false;
        if (!proc->l_consumed && (now_ms - proc->l_press_time) < TAP_MAX_MS && emitted == CMD_NONE) {
            emitted = CMD_L_TAP;
        }
    }

    /* Detectar liberación de R (Tap) */
    if (!r_now && proc->r_down) {
        proc->r_down = false;
        if (!proc->r_consumed && (now_ms - proc->r_press_time) < TAP_MAX_MS && emitted == CMD_NONE) {
            emitted = CMD_R_TAP;
        }
    }

    /* Si se liberaron ambos gatillos, resetear flag de combo para la próxima vez */
    if (!l_now && !r_now) {
        proc->lr_combo_fired = false;
    }

    /* Si ya se emitió un comando de gatillo prioritario, retornar */
    if (emitted != CMD_NONE) {
        proc->prev_buttons = current_buttons;
        return emitted;
    }

    /* Detección de flanco ascendente (nuevo botón presionado) para controles estándar */
    uint32_t pressed = current_buttons & ~(proc->prev_buttons);

    if (pressed & BTN_CROSS)    emitted = CMD_CONFIRM;
    else if (pressed & BTN_CIRCLE)   emitted = CMD_BACK;
    else if (pressed & BTN_TRIANGLE) emitted = CMD_TOGGLE_UI;
    else if (pressed & BTN_START)    emitted = CMD_START;
    else if (pressed & BTN_SELECT)   emitted = CMD_SELECT;
    else if (pressed & BTN_UP)       emitted = CMD_UP;
    else if (pressed & BTN_DOWN)     emitted = CMD_DOWN;
    else if (pressed & BTN_LEFT)     emitted = CMD_LEFT;
    else if (pressed & BTN_RIGHT)    emitted = CMD_RIGHT;

    proc->prev_buttons = current_buttons;
    return emitted;
}
