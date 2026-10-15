#include "graphics.h"
#include "font.h"
#include <pspdisplay.h>
#include <pspgu.h>
#include <string.h>

static unsigned int __attribute__((aligned(16))) g_gu_dlist[1024];
static uint32_t *g_vram_draw = (uint32_t*)0x44000000;

void gfx_init(void) {
    sceGuInit();
    sceGuStart(GU_DIRECT, g_gu_dlist);
    sceGuDrawBuffer(GU_PSM_8888, (void*)0, BUFFER_STRIDE);
    sceGuDispBuffer(SCREEN_WIDTH, SCREEN_HEIGHT, (void*)0x88000, BUFFER_STRIDE);
    sceGuOffset(2048 - (SCREEN_WIDTH / 2), 2048 - (SCREEN_HEIGHT / 2));
    sceGuViewport(2048, 2048, SCREEN_WIDTH, SCREEN_HEIGHT);
    sceGuScissor(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    sceGuEnable(GU_SCISSOR_TEST);
    sceGuFinish();
    sceGuSync(0, 0);

    sceDisplayWaitVblankStart();
    sceGuDisplay(GU_TRUE);

    g_vram_draw = (uint32_t*)0x44000000;
}

void gfx_shutdown(void) {
    sceGuTerm();
}

void gfx_start_frame(uint32_t clear_color) {
    /* Relleno rápido del buffer trasero (g_vram_draw no está en pantalla) */
    uint32_t *ptr = g_vram_draw;
    for (int y = 0; y < SCREEN_HEIGHT; y++) {
        for (int x = 0; x < SCREEN_WIDTH; x++) {
            ptr[x] = clear_color;
        }
        ptr += BUFFER_STRIDE;
    }
}

void gfx_end_frame(void) {
    /* Sincronización con el barrido vertical físico y cambio de búfer por hardware */
    sceDisplayWaitVblankStart();
    void *new_draw = sceGuSwapBuffers();
    g_vram_draw = (uint32_t*)((uintptr_t)new_draw | 0x44000000);
}

void gfx_draw_pixel(int x, int y, uint32_t color) {
    if (x < 0 || x >= SCREEN_WIDTH || y < 0 || y >= SCREEN_HEIGHT) return;
    g_vram_draw[y * BUFFER_STRIDE + x] = color;
}

void gfx_draw_rect(int x, int y, int w, int h, uint32_t color) {
    if (x >= SCREEN_WIDTH || y >= SCREEN_HEIGHT || w <= 0 || h <= 0) return;

    int x0 = (x < 0) ? 0 : x;
    int y0 = (y < 0) ? 0 : y;
    int x1 = (x + w > SCREEN_WIDTH) ? SCREEN_WIDTH : (x + w);
    int y1 = (y + h > SCREEN_HEIGHT) ? SCREEN_HEIGHT : (y + h);

    for (int r = y0; r < y1; r++) {
        uint32_t *row = &g_vram_draw[r * BUFFER_STRIDE + x0];
        for (int c = x0; c < x1; c++) {
            *row++ = color;
        }
    }
}

void gfx_draw_rect_outline(int x, int y, int w, int h, int thickness, uint32_t color) {
    if (thickness <= 0) return;
    gfx_draw_rect(x, y, w, thickness, color);                         /* Borde superior */
    gfx_draw_rect(x, y + h - thickness, w, thickness, color);         /* Borde inferior */
    gfx_draw_rect(x, y, thickness, h, color);                         /* Borde izquierdo */
    gfx_draw_rect(x + w - thickness, y, thickness, h, color);         /* Borde derecho */
}

void gfx_draw_card(int x, int y, int w, int h, uint32_t bg_color, uint32_t border_color) {
    gfx_draw_rect(x, y, w, h, bg_color);
    gfx_draw_rect_outline(x, y, w, h, 1, border_color);
}

void gfx_draw_progress_bar(int x, int y, int w, int h, float progress, uint32_t fill_color, uint32_t bg_color) {
    if (progress < 0.0f) progress = 0.0f;
    if (progress > 1.0f) progress = 1.0f;

    gfx_draw_card(x, y, w, h, bg_color, COLOR_BORDER_DARK);

    int fill_w = (int)((w - 4) * progress);
    if (fill_w > 0) {
        gfx_draw_rect(x + 2, y + 2, fill_w, h - 4, fill_color);
    }
}

void gfx_draw_battery_icon(int x, int y, int percent, bool is_charging, bool is_plugged, uint32_t color) {
    int w = 26;
    int h = 13;

    /* Cuerpo de la pila */
    gfx_draw_rect_outline(x, y, w, h, 1, color);
    /* Borne positivo derecho */
    gfx_draw_rect(x + w, y + 3, 2, h - 6, color);

    if (percent < 0) return;
    if (percent > 100) percent = 100;

    int fill_w = (int)((w - 4) * (percent / 100.0f));
    uint32_t fill_col = color;
    if (is_charging) fill_col = COLOR_POMO_BREAK;
    else if (percent <= 20) fill_col = COLOR_POMO_WORK;

    if (fill_w > 0) {
        gfx_draw_rect(x + 2, y + 2, fill_w, h - 4, fill_col);
    }

    /* Rayo de carga simple si está enchufada */
    if (is_charging || is_plugged) {
        gfx_draw_rect(x + (w / 2) - 1, y - 2, 3, 2, COLOR_AMBER_ACCENT);
    }
}

void gfx_draw_char(int x, int y, char c, uint32_t color) {
    if ((unsigned char)c < 32 || (unsigned char)c > 127) c = '?';
    const uint8_t *glyph = font_8x8[c - 32];

    for (int r = 0; r < 8; r++) {
        uint8_t row = glyph[r];
        for (int col = 0; col < 8; col++) {
            if (row & (0x80 >> col)) {
                gfx_draw_pixel(x + col, y + r, color);
            }
        }
    }
}

void gfx_draw_text(int x, int y, const char *text, uint32_t color) {
    if (!text) return;
    int cur_x = x;
    while (*text) {
        if (*text == '\n') {
            cur_x = x;
            y += 10;
        } else {
            gfx_draw_char(cur_x, y, *text, color);
            cur_x += 8;
        }
        text++;
    }
}

void gfx_draw_text_scaled(int x, int y, const char *text, int scale, uint32_t color) {
    if (!text || scale <= 0) return;
    int cur_x = x;
    while (*text) {
        if (*text == '\n') {
            cur_x = x;
            y += 9 * scale;
        } else {
            char c = *text;
            if ((unsigned char)c < 32 || (unsigned char)c > 127) c = '?';
            const uint8_t *glyph = font_8x8[c - 32];

            for (int r = 0; r < 8; r++) {
                uint8_t row = glyph[r];
                for (int col = 0; col < 8; col++) {
                    if (row & (0x80 >> col)) {
                        gfx_draw_rect(cur_x + col * scale, y + r * scale, scale, scale, color);
                    }
                }
            }
            cur_x += 8 * scale + scale;
        }
        text++;
    }
}

void gfx_draw_text_centered(int y, const char *text, int scale, uint32_t color) {
    if (!text || scale <= 0) return;
    int len = (int)strlen(text);
    int width = len * (8 * scale + scale);
    int x = (SCREEN_WIDTH - width) / 2;
    gfx_draw_text_scaled(x, y, text, scale, color);
}

/* 7 Segmentos:
      _a_
    f|   |b
      _g_
    e|   |c
      _d_
*/
#define SEG_A (1 << 0)
#define SEG_B (1 << 1)
#define SEG_C (1 << 2)
#define SEG_D (1 << 3)
#define SEG_E (1 << 4)
#define SEG_F (1 << 5)
#define SEG_G (1 << 6)

static const uint8_t segment_masks[11] = {
    SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F,         /* 0 */
    SEG_B | SEG_C,                                         /* 1 */
    SEG_A | SEG_B | SEG_G | SEG_E | SEG_D,                 /* 2 */
    SEG_A | SEG_B | SEG_G | SEG_C | SEG_D,                 /* 3 */
    SEG_F | SEG_G | SEG_B | SEG_C,                         /* 4 */
    SEG_A | SEG_F | SEG_G | SEG_C | SEG_D,                 /* 5 */
    SEG_A | SEG_F | SEG_G | SEG_E | SEG_C | SEG_D,         /* 6 */
    SEG_A | SEG_B | SEG_C,                                 /* 7 */
    SEG_A | SEG_B | SEG_C | SEG_D | SEG_E | SEG_F | SEG_G, /* 8 */
    SEG_A | SEG_B | SEG_C | SEG_D | SEG_F | SEG_G,         /* 9 */
    SEG_G                                                  /* - (10) */
};

void gfx_draw_segment_digit(int x, int y, int w, int h, char digit, uint32_t on_color, uint32_t off_color) {
    uint8_t mask = 0;
    if (digit >= '0' && digit <= '9') {
        mask = segment_masks[digit - '0'];
    } else if (digit == '-') {
        mask = segment_masks[10];
    }

    int t = h / 8; /* Grosor sólido continuo */
    if (t < 3) t = 3;
    int half_h = (h - t) / 2;

    /* Segmento A (superior) */
    if (mask & SEG_A) gfx_draw_rect(x + t, y, w - 2 * t, t, on_color);
    else if (off_color != 0 && off_color != COLOR_BG_DARK) gfx_draw_rect(x + t, y, w - 2 * t, t, off_color);

    /* Segmento B (arriba derecha) */
    if (mask & SEG_B) gfx_draw_rect(x + w - t, y, t, half_h + t, on_color);
    else if (off_color != 0 && off_color != COLOR_BG_DARK) gfx_draw_rect(x + w - t, y, t, half_h + t, off_color);

    /* Segmento C (abajo derecha) */
    if (mask & SEG_C) gfx_draw_rect(x + w - t, y + half_h, t, h - half_h, on_color);
    else if (off_color != 0 && off_color != COLOR_BG_DARK) gfx_draw_rect(x + w - t, y + half_h, t, h - half_h, off_color);

    /* Segmento D (inferior) */
    if (mask & SEG_D) gfx_draw_rect(x + t, y + h - t, w - 2 * t, t, on_color);
    else if (off_color != 0 && off_color != COLOR_BG_DARK) gfx_draw_rect(x + t, y + h - t, w - 2 * t, t, off_color);

    /* Segmento E (abajo izquierda) */
    if (mask & SEG_E) gfx_draw_rect(x, y + half_h, t, h - half_h, on_color);
    else if (off_color != 0 && off_color != COLOR_BG_DARK) gfx_draw_rect(x, y + half_h, t, h - half_h, off_color);

    /* Segmento F (arriba izquierda) */
    if (mask & SEG_F) gfx_draw_rect(x, y, t, half_h + t, on_color);
    else if (off_color != 0 && off_color != COLOR_BG_DARK) gfx_draw_rect(x, y, t, half_h + t, off_color);

    /* Segmento G (centro) */
    if (mask & SEG_G) gfx_draw_rect(x + t, y + half_h, w - 2 * t, t, on_color);
    else if (off_color != 0 && off_color != COLOR_BG_DARK) gfx_draw_rect(x + t, y + half_h, w - 2 * t, t, off_color);
}

void gfx_draw_digital_clock(int center_x, int y, int h, int hours, int minutes, int seconds, bool show_seconds, uint32_t on_color, uint32_t off_color) {
    int digit_w = (int)(h * 0.55f);
    int gap = (int)(h * 0.12f);
    int colon_w = (int)(h * 0.20f);

    int total_w = 4 * digit_w + 3 * gap + colon_w;
    if (show_seconds) {
        total_w += colon_w + gap + 2 * (int)(digit_w * 0.65f) + gap;
    }

    int cur_x = center_x - total_w / 2;

    /* Horas */
    gfx_draw_segment_digit(cur_x, y, digit_w, h, '0' + (hours / 10), on_color, off_color);
    cur_x += digit_w + gap;
    gfx_draw_segment_digit(cur_x, y, digit_w, h, '0' + (hours % 10), on_color, off_color);
    cur_x += digit_w + gap;

    /* Puntos de dos puntos : */
    int dot_size = h / 9;
    if (dot_size < 3) dot_size = 3;
    gfx_draw_rect(cur_x + colon_w / 2 - dot_size / 2, y + h / 3 - dot_size / 2, dot_size, dot_size, on_color);
    gfx_draw_rect(cur_x + colon_w / 2 - dot_size / 2, y + (h * 2) / 3 - dot_size / 2, dot_size, dot_size, on_color);
    cur_x += colon_w + gap;

    /* Minutos */
    gfx_draw_segment_digit(cur_x, y, digit_w, h, '0' + (minutes / 10), on_color, off_color);
    cur_x += digit_w + gap;
    gfx_draw_segment_digit(cur_x, y, digit_w, h, '0' + (minutes % 10), on_color, off_color);
    cur_x += digit_w + gap;

    /* Segundos (más pequeños si están activos) */
    if (show_seconds) {
        int sec_h = (int)(h * 0.65f);
        int sec_w = (int)(digit_w * 0.65f);
        int sec_y = y + (h - sec_h);

        /* Segundo separador */
        gfx_draw_rect(cur_x + colon_w / 2 - dot_size / 2, sec_y + sec_h / 3, dot_size, dot_size, on_color);
        gfx_draw_rect(cur_x + colon_w / 2 - dot_size / 2, sec_y + (sec_h * 2) / 3, dot_size, dot_size, on_color);
        cur_x += colon_w + gap;

        gfx_draw_segment_digit(cur_x, sec_y, sec_w, sec_h, '0' + (seconds / 10), on_color, off_color);
        cur_x += sec_w + gap;
        gfx_draw_segment_digit(cur_x, sec_y, sec_w, sec_h, '0' + (seconds % 10), on_color, off_color);
    }
}
