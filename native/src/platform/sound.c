/* Queued SDL3 stream: all synthesis and frequency changes stay on the main thread. */
#include "platform.h"
static SDL_AudioStream *speaker;
static unsigned frequency;
static double phase;

void dz_audio_service(void)
{
    if (!speaker || !frequency) return;
    float samples[480]; /* 10 ms at 48 kHz; queue at most 30 ms */
    while (SDL_GetAudioStreamQueued(speaker) < (int)(sizeof samples * 2)) {
        for (unsigned i = 0; i < sizeof samples / sizeof *samples; ++i) {
            samples[i] = phase < 0.5 ? 0.12f : -0.12f;
            phase += (double)frequency / 48000.0;
            phase -= (unsigned)phase;
        }
        if (!SDL_PutAudioStreamData(speaker, samples, sizeof samples)) break;
    }
}
void dz_sound(unsigned hz)
{
    dz_charge(1);
    if (dz_options.headless || dz_options.deterministic) return;
    if (!speaker) {
        if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) return;
        const SDL_AudioSpec spec = {SDL_AUDIO_F32, 1, 48000};
        speaker = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL, NULL);
        if (!speaker) return;
        SDL_ResumeAudioStreamDevice(speaker);
    }
    SDL_ClearAudioStream(speaker);
    frequency = hz; phase = 0;
    dz_audio_service();
}
void dz_nosound(void)
{
    dz_charge(1);
    frequency = 0;
    if (speaker) SDL_ClearAudioStream(speaker);
}
void dz_sound_shutdown(void)
{
    SDL_DestroyAudioStream(speaker); speaker = NULL;
    frequency = 0; phase = 0;
}
