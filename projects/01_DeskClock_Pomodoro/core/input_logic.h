#ifndef INPUT_LOGIC_H
#define INPUT_LOGIC_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Definición abstracta de botones para no depender de cabeceras de la PSP en tests */
#define BTN_SELECT   (1u << 0)
#define BTN_START    (1u << 3)
#define BTN_UP       (1u << 4)
#define BTN_RIGHT    (1u << 5)
#define BTN_DOWN     (1u << 6)
#define BTN_LEFT     (1u << 7)
#define BTN_LTRIGGER (1u << 8)
#define BTN_RTRIGGER (1u << 9)
#define BTN_TRIANGLE (1u << 12)
#define BTN_CIRCLE   (1u << 13)
#define BTN_CROSS    (1u << 14)
#define BTN_SQUARE   (1u << 15)

typedef enum {
    CMD_NONE = 0,
    CMD_L_TAP,           /* Toque rápido en L (< 600 ms) */
    CMD_R_TAP,           /* Toque rápido en R (< 600 ms) */
    CMD_R_HOLD,          /* R sostenido (> 1500 ms) */
    CMD_LR_COMBO,        /* L + R sostenidos (> 1000 ms) */
    CMD_UP,
    CMD_DOWN,
    CMD_LEFT,
    CMD_RIGHT,
    CMD_CONFIRM,         /* Botón X */
    CMD_BACK,            /* Botón Círculo */
    CMD_TOGGLE_UI,       /* Botón Triángulo */
    CMD_START,
    CMD_SELECT
} InputCmd;

typedef struct {
    uint32_t prev_buttons;
    
    /* Estados para L y R */
    uint64_t l_press_time;
    uint64_t r_press_time;
    bool l_down;
    bool r_down;
    bool l_consumed;      /* Ya se disparó combo o hold, no emitir tap al soltar */
    bool r_consumed;
    bool lr_combo_fired;
} InputProcessor;

void input_init(InputProcessor *proc);

/* Procesa un frame de botones crudos y retorna el comando semántico detectado (o CMD_NONE) */
InputCmd input_process(InputProcessor *proc, uint32_t current_buttons, uint64_t now_ms);

#ifdef __cplusplus
}
#endif

#endif /* INPUT_LOGIC_H */
