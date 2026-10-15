#include <pspkernel.h>
#include <pspdisplay.h>
#include <pspctrl.h>
#include <psppower.h>
#include <psprtc.h>
#include <stdio.h>

#include "core/app_state.h"
#include "core/input_logic.h"
#include "ui/graphics.h"
#include "ui/screens.h"
#include "platform/psp_audio.h"
#include "platform/psp_battery.h"

PSP_MODULE_INFO("DeskClock", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

static volatile int g_running = 1;

static int exit_callback(int arg1, int arg2, void *common) {
    (void)arg1; (void)arg2; (void)common;
    g_running = 0;
    sceKernelExitGame();
    return 0;
}

static int callback_thread(SceSize args, void *argp) {
    (void)args; (void)argp;
    int cbid = sceKernelCreateCallback("Exit Callback", exit_callback, NULL);
    sceKernelRegisterExitCallback(cbid);
    sceKernelSleepThreadCB();
    return 0;
}

static void setup_callbacks(void) {
    int thid = sceKernelCreateThread("cb_thread", callback_thread, 0x11, 0xFA0, 0, NULL);
    if (thid >= 0) {
        sceKernelStartThread(thid, 0, NULL);
    }
}

static uint64_t get_now_ms(void) {
    uint64_t tick;
    sceRtcGetCurrentTick(&tick);
    return tick / (sceRtcGetTickResolution() / 1000);
}

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;

    setup_callbacks();
    gfx_init();
    audio_init();

    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);

    AppState app;
    app_init(&app);

    InputProcessor input_proc;
    input_init(&input_proc);

    /* Primera lectura de telemetría de batería */
    battery_update(&app.battery);

    uint64_t last_battery_ms = get_now_ms();
    uint64_t last_power_tick_ms = get_now_ms();

    SceCtrlData pad;
    ScePspDateTime current_time;

    while (g_running) {
        uint64_t now_ms = get_now_ms();
        sceRtcGetCurrentClockLocalTime(&current_time);

        /* 1. Muestreo de botones físicos */
        sceCtrlPeekBufferPositive(&pad, 1);
        InputCmd cmd = input_process(&input_proc, pad.Buttons, now_ms);
        if (cmd != CMD_NONE) {
            app_handle_cmd(&app, cmd, now_ms);
        }

        /* 2. Actualización del motor de Pomodoro */
        app_update(&app, now_ms);

        /* 3. Comprobar alertas acústicas */
        if (app.sound_alert_pending) {
            app.sound_alert_pending = false;
            audio_play_chime();
        }

        /* 4. Actualizar estado de batería cada 3 segundos */
        if (now_ms - last_battery_ms >= 3000) {
            battery_update(&app.battery);
            last_battery_ms = now_ms;
        }

        /* 5. Keep-Awake: Evitar que la PSP Go apague o atenúe la pantalla automáticamente */
        if (now_ms - last_power_tick_ms >= 4000) {
            scePowerTick(PSP_POWER_TICK_ALL);
            last_power_tick_ms = now_ms;
        }

        /* 6. Renderizado de la pantalla activa (Double-Buffered Vsync sincronizado) */
        gfx_start_frame(COLOR_BG_DARK);
        render_active_screen(&app, &current_time, now_ms);
        gfx_end_frame();
    }

    gfx_shutdown();
    audio_shutdown();
    return 0;
}
