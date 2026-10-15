#ifndef FONT_H
#define FONT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Fuente bitmap estándar de 8x8 para caracteres ASCII del 32 (espacio) al 127 */
extern const uint8_t font_8x8[96][8];

#ifdef __cplusplus
}
#endif

#endif /* FONT_H */
