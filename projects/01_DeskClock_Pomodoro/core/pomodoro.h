#ifndef POMODORO_H
#define POMODORO_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    POMO_PHASE_WORK,
    POMO_PHASE_SHORT_BREAK,
    POMO_PHASE_LONG_BREAK
} PomoPhase;

typedef enum {
    POMO_STATE_IDLE,       /* No iniciado aún */
    POMO_STATE_RUNNING,    /* Cuenta regresiva activa */
    POMO_STATE_PAUSED,     /* En pausa manual */
    POMO_STATE_READY       /* Fase anterior finalizada, esperando inicio de la siguiente */
} PomoState;

typedef enum {
    POMO_EVENT_NONE = 0,
    POMO_EVENT_WORK_COMPLETED,
    POMO_EVENT_BREAK_COMPLETED,
    POMO_EVENT_PHASE_CHANGED
} PomoEvent;

typedef struct {
    uint32_t work_min;
    uint32_t short_break_min;
    uint32_t long_break_min;
    uint32_t long_break_interval; /* Ej: 4 rondas */
    bool auto_start;              /* Iniciar descanso/trabajo automáticamente al terminar */
} PomoConfig;

typedef struct {
    PomoPhase phase;
    PomoState state;
    PomoConfig config;

    uint32_t current_round;       /* Ronda actual dentro del ciclo (1 a long_break_interval) */
    uint32_t completed_sessions;  /* Total de sesiones de trabajo completadas */
    
    uint64_t phase_start_ms;      /* Marca temporal en la que arrancó o reanudó la fase actual */
    uint32_t accumulated_ms;      /* Milisegundos ya transcurridos antes de la última pausa */
    uint32_t duration_ms;         /* Duración objetivo de la fase en milisegundos */
} Pomo;

/* Inicializar estructura Pomodoro con configuración dada */
void pomo_init(Pomo *pomo, const PomoConfig *config);

/* Gatillo L: Iniciar, pausar o reanudar */
void pomo_toggle(Pomo *pomo, uint64_t now_ms);

/* Gatillo R corto: Saltar fase actual y pasar a la siguiente */
void pomo_skip(Pomo *pomo, uint64_t now_ms);

/* Gatillo R sostenido: Reiniciar ciclo a IDLE */
void pomo_reset(Pomo *pomo);

/* Actualización periódica en cada frame. Retorna eventos relevantes (ej. fase completada) */
PomoEvent pomo_update(Pomo *pomo, uint64_t now_ms);

/* Tiempo restante en milisegundos para la fase activa */
uint32_t pomo_remaining_ms(const Pomo *pomo, uint64_t now_ms);

/* Progreso normalizado de 0.0 (inicio) a 1.0 (completo) */
float pomo_progress(const Pomo *pomo, uint64_t now_ms);

#ifdef __cplusplus
}
#endif

#endif /* POMODORO_H */
