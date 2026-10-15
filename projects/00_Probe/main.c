/*
 * 00_Probe — Fase 0 del DeskClock.
 * Responde a:
 *   P1: ¿se puede detectar el slider en modo usuario? (bits crudos + callbacks de energía)
 *   P2: ¿qué pasa al cerrar el slider con una app abierta? (detección de huecos de tiempo)
 *   P3: ¿qué botones funcionan con la consola cerrada? (máscara acumulada de bits vistos)
 * Todo se muestra en pantalla y se escribe en probe_log.txt junto al EBOOT.
 */
#include <pspkernel.h>
#include <pspdebug.h>
#include <pspdisplay.h>
#include <pspctrl.h>
#include <psppower.h>
#include <psprtc.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

PSP_MODULE_INFO("SlideProbe", 0, 1, 0);
PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER | THREAD_ATTR_VFPU);

#define printf pspDebugScreenPrintf
#define GAP_THRESHOLD_MS 500
#define EVENT_LINES 9

static char g_log_path[256] = "probe_log.txt";
static volatile int g_power_flags = 0;
static volatile int g_power_count = 0;
static volatile int g_power_pending = 0;
static int g_power_reg_ret = 0;
static char g_events[EVENT_LINES][64];
static int g_event_head = 0;
static u64 g_t0_ms = 0;

/* ---------- tiempo ---------- */
static u64 now_ms(void) {
    u64 tick;
    sceRtcGetCurrentTick(&tick);
    return tick / (sceRtcGetTickResolution() / 1000);
}

/* ---------- log ---------- */
static void log_event(const char *fmt, ...) {
    char msg[64];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(msg, sizeof(msg), fmt, ap);
    va_end(ap);

    double t = (now_ms() - g_t0_ms) / 1000.0;
    snprintf(g_events[g_event_head], sizeof(g_events[0]), "%7.2fs %s", t, msg);
    g_event_head = (g_event_head + 1) % EVENT_LINES;

    FILE *f = fopen(g_log_path, "a");
    if (f) {
        fprintf(f, "%9.3f %s\n", t, msg);
        fclose(f);
    }
}

/* ---------- nombres de bits ---------- */
static const char *bit_name(int bit) {
    switch (1u << bit) {
        case PSP_CTRL_SELECT:   return "SELECT";
        case PSP_CTRL_START:    return "START";
        case PSP_CTRL_UP:       return "UP";
        case PSP_CTRL_RIGHT:    return "RIGHT";
        case PSP_CTRL_DOWN:     return "DOWN";
        case PSP_CTRL_LEFT:     return "LEFT";
        case PSP_CTRL_LTRIGGER: return "L";
        case PSP_CTRL_RTRIGGER: return "R";
        case PSP_CTRL_TRIANGLE: return "TRIANGLE";
        case PSP_CTRL_CIRCLE:   return "CIRCLE";
        case PSP_CTRL_CROSS:    return "CROSS";
        case PSP_CTRL_SQUARE:   return "SQUARE";
        case PSP_CTRL_HOME:     return "HOME";
        case PSP_CTRL_HOLD:     return "HOLD";
        case PSP_CTRL_WLAN_UP:  return "WLAN";
        case PSP_CTRL_REMOTE:   return "REMOTE";
        case PSP_CTRL_VOLUP:    return "VOL+";
        case PSP_CTRL_VOLDOWN:  return "VOL-";
        case PSP_CTRL_SCREEN:   return "SCREEN";
        case PSP_CTRL_NOTE:     return "NOTE";
        case PSP_CTRL_DISC:     return "DISC";
        case PSP_CTRL_MS:       return "MS";
        default:                return NULL;
    }
}

static void log_button_changes(u32 prev, u32 cur) {
    u32 diff = prev ^ cur;
    for (int b = 0; b < 32; b++) {
        if (!(diff & (1u << b))) continue;
        const char *n = bit_name(b);
        char unk[16];
        if (!n) { snprintf(unk, sizeof(unk), "BIT%d", b); n = unk; }
        log_event("%s %s (raw %08X)", n, (cur & (1u << b)) ? "DOWN" : "UP  ", (unsigned)cur);
    }
}

/* ---------- callbacks ---------- */
static int exit_callback(int a, int b, void *c) {
    log_event("EXIT callback (HOME)");
    sceKernelExitGame();
    return 0;
}

static int power_callback(int count, int flags, void *arg) {
    g_power_count = count;
    g_power_flags = flags;
    g_power_pending = 1;
    return 0;
}

static int callback_thread(SceSize args, void *argp) {
    int cb = sceKernelCreateCallback("Exit CB", exit_callback, NULL);
    sceKernelRegisterExitCallback(cb);

    int pcb = sceKernelCreateCallback("Power CB", power_callback, NULL);
    g_power_reg_ret = scePowerRegisterCallback(-1, pcb);
    if (g_power_reg_ret < 0) g_power_reg_ret = scePowerRegisterCallback(0, pcb);

    sceKernelSleepThreadCB();
    return 0;
}

static void setup_callbacks(void) {
    int th = sceKernelCreateThread("cb_thread", callback_thread, 0x11, 0xFA0, 0, 0);
    if (th >= 0) sceKernelStartThread(th, 0, 0);
}

static void describe_power(int f, char *out, size_t n) {
    snprintf(out, n, "%s%s%s%s%s%s%s%sbatt=%d",
        (f & PSP_POWER_CB_POWER_SWITCH)    ? "PWRSW " : "",
        (f & PSP_POWER_CB_HOLD_SWITCH)     ? "HOLDSW " : "",
        (f & PSP_POWER_CB_STANDBY)         ? "STANDBY " : "",
        (f & PSP_POWER_CB_SUSPENDING)      ? "SUSP " : "",
        (f & PSP_POWER_CB_RESUMING)        ? "RESUMING " : "",
        (f & PSP_POWER_CB_RESUME_COMPLETE) ? "RESUMED " : "",
        (f & PSP_POWER_CB_AC_POWER)        ? "AC " : "",
        (f & PSP_POWER_CB_BATTERY_LOW)     ? "LOW " : "",
        f & PSP_POWER_CB_BATTPOWER);
}

/* ---------- main ---------- */
int main(int argc, char *argv[]) {
    /* Log junto al EBOOT (sin escribir ms0:/ef0: a mano) */
    if (argc > 0 && argv[0]) {
        strncpy(g_log_path, argv[0], sizeof(g_log_path) - 1);
        char *slash = strrchr(g_log_path, '/');
        if (slash) strcpy(slash + 1, "probe_log.txt");
        else strcpy(g_log_path, "probe_log.txt");
    }

    pspDebugScreenInit();
    setup_callbacks();
    sceCtrlSetSamplingCycle(0);
    sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);

    g_t0_ms = now_ms();
    {
        FILE *f = fopen(g_log_path, "a");
        if (f) { fprintf(f, "\n===== NUEVA SESION =====\n"); fclose(f); }
    }
    sceKernelDelayThread(100 * 1000); /* deja que se registre el callback de energía */
    log_event("START power_reg=%d", g_power_reg_ret);

    SceCtrlData pad;
    u32 prev_buttons = 0, seen_mask = 0;
    u64 last_frame_ms = now_ms(), last_tick_ms = 0;
    u32 frames = 0;
    int keep_awake = 1;
    int max_gap = 0;

    while (1) {
        u64 t = now_ms();

        /* P2: huecos de tiempo = suspensión o hilo congelado */
        int gap = (int)(t - last_frame_ms);
        if (gap > GAP_THRESHOLD_MS) {
            log_event("GAP %d ms (frame %u)", gap, (unsigned)frames);
            if (gap > max_gap) max_gap = gap;
        }
        last_frame_ms = t;

        /* P1/P3: bits crudos */
        sceCtrlPeekBufferPositive(&pad, 1);
        u32 cur = pad.Buttons;
        if (cur != prev_buttons) {
            log_button_changes(prev_buttons, cur);
            if ((cur & PSP_CTRL_TRIANGLE) && !(prev_buttons & PSP_CTRL_TRIANGLE)) {
                keep_awake = !keep_awake;
                log_event("keep_awake=%d", keep_awake);
            }
            prev_buttons = cur;
        }
        seen_mask |= cur;

        /* P1: eventos de energía */
        if (g_power_pending) {
            char d[64];
            g_power_pending = 0;
            describe_power(g_power_flags, d, sizeof(d));
            log_event("PWR %08X %s", (unsigned)g_power_flags, d);
        }

        if (keep_awake && t - last_tick_ms > 5000) {
            scePowerTick(PSP_POWER_TICK_ALL);
            last_tick_ms = t;
        }

        /* Pantalla */
        pspDebugScreenSetXY(0, 0);
        printf("=== 00_PROBE  (Fase 0 DeskClock) ===             \n");
        printf("Frame %-8u  Tiempo %7.1fs  MaxGap %5d ms   \n",
               (unsigned)frames, (t - g_t0_ms) / 1000.0, max_gap);
        printf("Raw ahora : %08X                               \n", (unsigned)cur);
        printf("Vistos    : %08X                               \n", (unsigned)seen_mask);
        printf("PowerCB   : reg=%d  ultimos flags=%08X  n=%d     \n",
               g_power_reg_ret, (unsigned)g_power_flags, g_power_count);
        printf("Keep-awake: %s  (TRIANGULO cambia)               \n", keep_awake ? "ON " : "OFF");
        printf("Bateria   : %d%%  %d mV  %d C                    \n",
               scePowerGetBatteryLifePercent(), scePowerGetBatteryVolt(), scePowerGetBatteryTemp());
        printf("-------------------------------------------------\n");
        printf("Ultimos eventos:                                 \n");
        for (int i = 0; i < EVENT_LINES; i++) {
            int idx = (g_event_head + i) % EVENT_LINES;
            printf(" %-55s\n", g_events[idx]);
        }
        printf("-------------------------------------------------\n");
        printf("1) Cierra/abre el slider 3 veces despacio        \n");
        printf("2) Cerrada: pulsa L, R, VOL+, VOL-, PS, etc.     \n");
        printf("3) Cerrada 30 s quieta, luego abre               \n");
        printf("HOME para salir. Log: probe_log.txt              \n");

        frames++;
        sceDisplayWaitVblankStart();
    }
    return 0;
}
