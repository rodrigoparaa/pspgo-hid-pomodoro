#include "psp_audio.h"
#include <pspaudio.h>
#include <pspkernel.h>
#include <math.h>
#include <string.h>

#define AUDIO_SAMPLES   512
#define SAMPLE_RATE     44100
#define PI              3.14159265358979323846

static SceUID g_audio_thid = -1;
static volatile int g_audio_running = 1;
static volatile int g_chime_trigger = 0;
static short g_audio_buf[AUDIO_SAMPLES * 2];

static void play_tone(int channel, float freq, int duration_ms) {
    int total_samples = (SAMPLE_RATE * duration_ms) / 1000;
    int played = 0;
    float phase = 0.0f;
    float phase_inc = (2.0f * (float)PI * freq) / (float)SAMPLE_RATE;

    while (played < total_samples && g_audio_running) {
        int batch = total_samples - played;
        if (batch > AUDIO_SAMPLES) batch = AUDIO_SAMPLES;

        for (int i = 0; i < batch; i++) {
            /* Envolvente suave para evitar chasquidos */
            float env = 1.0f;
            if (played + i > total_samples - 500) {
                env = (float)(total_samples - (played + i)) / 500.0f;
            }
            short sample = (short)(sinf(phase) * 12000.0f * env);
            g_audio_buf[i * 2] = sample;     /* Canal Izquierdo */
            g_audio_buf[i * 2 + 1] = sample; /* Canal Derecho */
            phase += phase_inc;
            if (phase >= 2.0f * (float)PI) phase -= 2.0f * (float)PI;
        }

        sceAudioOutputBlocking(channel, 0x8000, g_audio_buf);
        played += batch;
    }
}

static int audio_worker_thread(SceSize args, void *argp) {
    (void)args; (void)argp;

    int channel = sceAudioChReserve(-1, AUDIO_SAMPLES, 0x10); /* 0x10 = Formato estéreo 16-bit */
    if (channel < 0) return -1;

    while (g_audio_running) {
        if (g_chime_trigger > 0) {
            g_chime_trigger = 0;
            /* Secuencia armónica suave de dos tonos: La5 (880 Hz) y Re6 (1174 Hz) */
            play_tone(channel, 880.0f, 140);
            sceKernelDelayThread(40000); /* Pausa de 40 ms */
            play_tone(channel, 1174.0f, 220);
        } else {
            sceKernelDelayThread(20000); /* Dormir 20 ms */
        }
    }

    sceAudioChRelease(channel);
    return 0;
}

void audio_init(void) {
    g_audio_running = 1;
    g_chime_trigger = 0;
    g_audio_thid = sceKernelCreateThread("audio_worker", audio_worker_thread, 0x18, 0x4000, 0, NULL);
    if (g_audio_thid >= 0) {
        sceKernelStartThread(g_audio_thid, 0, NULL);
    }
}

void audio_play_chime(void) {
    g_chime_trigger = 1;
}

void audio_shutdown(void) {
    g_audio_running = 0;
    if (g_audio_thid >= 0) {
        sceKernelWaitThreadEnd(g_audio_thid, NULL);
        sceKernelDeleteThread(g_audio_thid);
        g_audio_thid = -1;
    }
}
