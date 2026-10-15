#ifndef SCREENS_H
#define SCREENS_H

#include "../core/app_state.h"
#include <psprtc.h>

#ifdef __cplusplus
extern "C" {
#endif

void render_active_screen(const AppState *app, const ScePspDateTime *time, uint64_t now_ms);

#ifdef __cplusplus
}
#endif

#endif /* SCREENS_H */
