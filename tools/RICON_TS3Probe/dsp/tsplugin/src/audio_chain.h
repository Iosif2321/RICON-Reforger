#ifndef RICON_TS_AUDIO_CHAIN_H
#define RICON_TS_AUDIO_CHAIN_H

/*
 * Testable TeamSpeak playback DSP core. The TeamSpeak plugin stays thin: it
 * gathers the current server-authoritative channel snapshot, then calls this
 * pure C audio chain. The core stores no raw audio; diagnostics are aggregate
 * counters only.
 */

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RICON_AUDIO_MAX_CHANNELS 16

typedef struct RiconAudioChannelParams {
    int audible;      /* 0 => gate/mute, non-zero => process/pass */
    int state;        /* RICON_CH_* wire code: 0 lost .. 3 clean */
    float mean_snr;   /* mean SNR in dB */
    float m;          /* Nakagami shape, clamped to >= 0.5 */
    float intel;      /* optional precomputed intelligibility, <0 => derive from SNR */
    float hiss;       /* optional precomputed hiss/noise level, <0 => derive from SNR */
} RiconAudioChannelParams;

typedef struct RiconAudioBiquadState {
    float x1[RICON_AUDIO_MAX_CHANNELS], x2[RICON_AUDIO_MAX_CHANNELS];
    float y1[RICON_AUDIO_MAX_CHANNELS], y2[RICON_AUDIO_MAX_CHANNELS];
} RiconAudioBiquadState;

typedef struct RiconAudioMetrics {
    uint64_t callbacks_total;
    uint64_t callbacks_ignored;
    uint64_t callbacks_passthrough;
    uint64_t callbacks_muted;
    uint64_t callbacks_dsp;
    uint64_t sample_values_processed;
    uint64_t sample_values_zeroed;
    uint64_t sample_values_clipped;
} RiconAudioMetrics;

typedef struct RiconAudioState {
    unsigned int noise_seed;
    float fade_db;
    RiconAudioBiquadState hp_state;
    RiconAudioBiquadState lp_state;
    RiconAudioMetrics metrics;
} RiconAudioState;

void ricon_audio_init(void);
void ricon_audio_state_init(RiconAudioState *state, unsigned int seed);
void ricon_audio_metrics_reset(RiconAudioState *state);
RiconAudioMetrics ricon_audio_metrics_get(const RiconAudioState *state);

void ricon_audio_process_playback_voice(RiconAudioState *state,
                                        const RiconAudioChannelParams *params,
                                        short *samples,
                                        int sample_count,
                                        int channels);

#ifdef __cplusplus
}
#endif

#endif /* RICON_TS_AUDIO_CHAIN_H */
