#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SCREEN_WIDTH   480
#define SCREEN_HEIGHT  272
#define BUFFER_STRIDE  512

/* Colores predefinidos en formato PSP 32-bit (ABGR little-endian: 0xAABBGGRR) */
#define COLOR_RGBA(r, g, b, a) ((uint32_t)(((a) << 24) | ((b) << 16) | ((g) << 8) | (r)))
#define COLOR_RGB(r, g, b)     COLOR_RGBA(r, g, b, 255)

#define COLOR_BG_DARK       0xFF0A0A0A
#define COLOR_CARD_DARK     0xFF1A1A1A
#define COLOR_BORDER_DARK   0xFF303030
#define COLOR_TEXT_LIGHT    0xFFEEEEEE
#define COLOR_TEXT_MUTED    0xFF808080
#define COLOR_TEXT_DIM      0xFF444444
#define COLOR_SEGMENT_GHOST 0xFF161616

/* Colores de acento */
#define COLOR_POMO_WORK     0xFF384DF0  /* Coral / Tomate */
#define COLOR_POMO_BREAK    0xFF55CC66  /* Menta verde */
#define COLOR_CYAN_ACCENT   0xFFDDAA20  /* Cian moderno */
#define COLOR_AMBER_ACCENT  0xFF20B0F0  /* Ámbar / Oro */

void gfx_init(void);
void gfx_shutdown(void);
void gfx_start_frame(uint32_t clear_color);
void gfx_end_frame(void);

void gfx_draw_pixel(int x, int y, uint32_t color);
void gfx_draw_rect(int x, int y, int w, int h, uint32_t color);
void gfx_draw_rect_outline(int x, int y, int w, int h, int thickness, uint32_t color);
void gfx_draw_card(int x, int y, int w, int h, uint32_t bg_color, uint32_t border_color);
void gfx_draw_progress_bar(int x, int y, int w, int h, float progress, uint32_t fill_color, uint32_t bg_color);
void gfx_draw_battery_icon(int x, int y, int percent, bool is_charging, bool is_plugged, uint32_t color);

void gfx_draw_char(int x, int y, char c, uint32_t color);
void gfx_draw_text(int x, int y, const char *text, uint32_t color);
void gfx_draw_text_scaled(int x, int y, const char *text, int scale, uint32_t color);
void gfx_draw_text_centered(int y, const char *text, int scale, uint32_t color);

/* Dígito de reloj digital estilizado mediante segmentos geométricos limpios */
void gfx_draw_segment_digit(int x, int y, int w, int h, char digit, uint32_t on_color, uint32_t off_color);

/* Bloque completo de tiempo HH:MM:SS */
void gfx_draw_digital_clock(int center_x, int y, int h, int hours, int minutes, int seconds, bool show_seconds, uint32_t on_color, uint32_t off_color);

#ifdef __cplusplus
}
#endif

#endif /* GRAPHICS_H */
