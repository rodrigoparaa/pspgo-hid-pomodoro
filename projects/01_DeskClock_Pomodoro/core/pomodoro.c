#include "pomodoro.h"

static uint32_t get_phase_duration_ms(const Pomo *pomo, PomoPhase phase) {
    switch (phase) {
        case POMO_PHASE_WORK:
            return pomo->config.work_min * 60 * 1000;
        case POMO_PHASE_SHORT_BREAK:
            return pomo->config.short_break_min * 60 * 1000;
        case POMO_PHASE_LONG_BREAK:
            return pomo->config.long_break_min * 60 * 1000;
        default:
            return 25 * 60 * 1000;
    }
}

static void transition_to_next_phase(Pomo *pomo, uint64_t now_ms) {
    if (pomo->phase == POMO_PHASE_WORK) {
        if (pomo->current_round >= pomo->config.long_break_interval) {
            pomo->phase = POMO_PHASE_LONG_BREAK;
        } else {
            pomo->phase = POMO_PHASE_SHORT_BREAK;
        }
    } else {
        /* Se terminó un descanso */
        if (pomo->phase == POMO_PHASE_LONG_BREAK) {
            pomo->current_round = 1;
        } else {
            pomo->current_round++;
        }
        pomo->phase = POMO_PHASE_WORK;
    }

    pomo->duration_ms = get_phase_duration_ms(pomo, pomo->phase);
    pomo->accumulated_ms = 0;
    pomo->phase_start_ms = now_ms;

    if (pomo->config.auto_start) {
        pomo->state = POMO_STATE_RUNNING;
    } else {
        pomo->state = POMO_STATE_READY;
    }
}

void pomo_init(Pomo *pomo, const PomoConfig *config) {
    if (!pomo) return;

    if (config) {
        pomo->config = *config;
    } else {
        pomo->config.work_min = 25;
        pomo->config.short_break_min = 5;
        pomo->config.long_break_min = 15;
        pomo->config.long_break_interval = 4;
        pomo->config.auto_start = false;
    }

    /* Validación de límites de seguridad */
    if (pomo->config.work_min == 0) pomo->config.work_min = 25;
    if (pomo->config.short_break_min == 0) pomo->config.short_break_min = 5;
    if (pomo->config.long_break_min == 0) pomo->config.long_break_min = 15;
    if (pomo->config.long_break_interval == 0) pomo->config.long_break_interval = 4;

    pomo->phase = POMO_PHASE_WORK;
    pomo->state = POMO_STATE_IDLE;
    pomo->current_round = 1;
    pomo->completed_sessions = 0;
    pomo->phase_start_ms = 0;
    pomo->accumulated_ms = 0;
    pomo->duration_ms = get_phase_duration_ms(pomo, pomo->phase);
}

void pomo_toggle(Pomo *pomo, uint64_t now_ms) {
    if (!pomo) return;

    switch (pomo->state) {
        case POMO_STATE_IDLE:
        case POMO_STATE_READY:
            pomo->state = POMO_STATE_RUNNING;
            pomo->phase_start_ms = now_ms;
            pomo->accumulated_ms = 0;
            break;

        case POMO_STATE_RUNNING:
            pomo->state = POMO_STATE_PAUSED;
            pomo->accumulated_ms += (uint32_t)(now_ms - pomo->phase_start_ms);
            break;

        case POMO_STATE_PAUSED:
            pomo->state = POMO_STATE_RUNNING;
            pomo->phase_start_ms = now_ms;
            break;
    }
}

void pomo_skip(Pomo *pomo, uint64_t now_ms) {
    if (!pomo) return;
    transition_to_next_phase(pomo, now_ms);
}

void pomo_reset(Pomo *pomo) {
    if (!pomo) return;
    pomo->phase = POMO_PHASE_WORK;
    pomo->state = POMO_STATE_IDLE;
    pomo->current_round = 1;
    pomo->phase_start_ms = 0;
    pomo->accumulated_ms = 0;
    pomo->duration_ms = get_phase_duration_ms(pomo, pomo->phase);
}

PomoEvent pomo_update(Pomo *pomo, uint64_t now_ms) {
    if (!pomo || pomo->state != POMO_STATE_RUNNING) {
        return POMO_EVENT_NONE;
    }

    uint32_t elapsed = pomo->accumulated_ms + (uint32_t)(now_ms - pomo->phase_start_ms);

    if (elapsed >= pomo->duration_ms) {
        PomoEvent event;
        if (pomo->phase == POMO_PHASE_WORK) {
            pomo->completed_sessions++;
            event = POMO_EVENT_WORK_COMPLETED;
        } else {
            event = POMO_EVENT_BREAK_COMPLETED;
        }

        transition_to_next_phase(pomo, now_ms);
        return event;
    }

    return POMO_EVENT_NONE;
}

uint32_t pomo_remaining_ms(const Pomo *pomo, uint64_t now_ms) {
    if (!pomo) return 0;

    uint32_t elapsed = pomo->accumulated_ms;
    if (pomo->state == POMO_STATE_RUNNING) {
        elapsed += (uint32_t)(now_ms - pomo->phase_start_ms);
    }

    if (elapsed >= pomo->duration_ms) {
        return 0;
    }
    return pomo->duration_ms - elapsed;
}

float pomo_progress(const Pomo *pomo, uint64_t now_ms) {
    if (!pomo || pomo->duration_ms == 0) return 0.0f;

    uint32_t remaining = pomo_remaining_ms(pomo, now_ms);
    if (remaining >= pomo->duration_ms) return 0.0f;

    float progress = (float)(pomo->duration_ms - remaining) / (float)pomo->duration_ms;
    if (progress > 1.0f) progress = 1.0f;
    if (progress < 0.0f) progress = 0.0f;
    return progress;
}
