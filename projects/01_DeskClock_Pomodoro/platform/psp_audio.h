#ifndef PSP_AUDIO_H
#define PSP_AUDIO_H

#ifdef __cplusplus
extern "C" {
#endif

void audio_init(void);
void audio_play_chime(void);
void audio_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif /* PSP_AUDIO_H */
