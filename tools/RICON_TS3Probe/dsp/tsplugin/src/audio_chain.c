#include "audio_chain.h"

#include "../../common/ricon_protocol.h"

#include <math.h>
#include <stddef.h>
#include <string.h>

#define RICON_AUDIO_SAMPLE_RATE 48000.0f
#define RICON_AUDIO_PI 3.14159265358979323846f

typedef struct RiconAudioBiquad {
    float b0, b1, b2;
    float a1, a2;
} RiconAudioBiquad;

static RiconAudioBiquad g_hp750, g_lp4000;
static int g_filters_ready = 0;

static void ricon_biquad_make(RiconAudioBiquad *bq, float hz, float q, int highpass) {
    const float w0 = 2.0f * RICON_AUDIO_PI * hz / RICON_AUDIO_SAMPLE_RATE;
    const float cw = cosf(w0);
    const float alpha = sinf(w0) / (2.0f * q);
    const float a0 = 1.0f + alpha;
    float b0, b1, b2;

    if (highpass) {
        b0 = (1.0f + cw) * 0.5f;
        b1 = -(1.0f + cw);
        b2 = (1.0f + cw) * 0.5f;
    } else {
        b0 = (1.0f - cw) * 0.5f;
        b1 = 1.0f - cw;
        b2 = (1.0f - cw) * 0.5f;
    }

    bq->b0 = b0 / a0;
    bq->b1 = b1 / a0;
    bq->b2 = b2 / a0;
    bq->a1 = (-2.0f * cw) / a0;
    bq->a2 = (1.0f - alpha) / a0;
}

static void ricon_filters_init(void) {
    if (g_filters_ready) return;
    ricon_biquad_make(&g_hp750, 750.0f, 0.7f, 1);
    ricon_biquad_make(&g_lp4000, 4000.0f, 0.7f, 0);
    g_filters_ready = 1;
}

static inline float ricon_biquad_process(const RiconAudioBiquad *bq,
                                         RiconAudioBiquadState *st,
                                         int ch,
                                         float x) {
    const int slot = (ch < RICON_AUDIO_MAX_CHANNELS) ? ch : (RICON_AUDIO_MAX_CHANNELS - 1);
    const float y = bq->b0 * x + bq->b1 * st->x1[slot] + bq->b2 * st->x2[slot]
                  - bq->a1 * st->y1[slot] - bq->a2 * st->y2[slot];
    st->x2[slot] = st->x1[slot];
    st->x1[slot] = x;
    st->y2[slot] = st->y1[slot];
    st->y1[slot] = y;
    return y;
}

static inline float ricon_uniform_open(RiconAudioState *state) {
    unsigned int v;
    state->noise_seed = state->noise_seed * 1664525u + 1013904223u;
    v = (state->noise_seed >> 8) | 1u;
    return ((float)v + 0.5f) / 16777216.0f;
}

static inline float ricon_white(RiconAudioState *state) {
    return 2.0f * ricon_uniform_open(state) - 1.0f;
}

static float ricon_normal_box_muller(RiconAudioState *state) {
    const float u1 = ricon_uniform_open(state);
    const float u2 = ricon_uniform_open(state);
    return sqrtf(-2.0f * logf(u1)) * cosf(2.0f * RICON_AUDIO_PI * u2);
}

static float ricon_gamma_unit_scale(RiconAudioState *state, float shape) {
    if (shape < 0.5f) shape = 0.5f;
    if (shape < 1.0f) {
        return ricon_gamma_unit_scale(state, shape + 1.0f) * powf(ricon_uniform_open(state), 1.0f / shape);
    }

    {
        const float d = shape - (1.0f / 3.0f);
        const float c = 1.0f / sqrtf(9.0f * d);
        for (;;) {
            const float x = ricon_normal_box_muller(state);
            const float base = 1.0f + c * x;
            float v, u;
            if (base <= 0.0f) continue;
            v = base * base * base;
            u = ricon_uniform_open(state);
            if (u < 1.0f - 0.0331f * x * x * x * x) return d * v;
            if (logf(u) < 0.5f * x * x + d * (1.0f - v + logf(v))) return d * v;
        }
    }
}

static float ricon_nakagami_power(RiconAudioState *state, float m) {
    if (m < 0.5f) m = 0.5f;
    return ricon_gamma_unit_scale(state, m) / m;
}

static void ricon_hill_from_snr(float snr_db, float *intel, float *hiss) {
    const float half = 10.0f;
    float snr_lin = powf(10.0f, snr_db / 10.0f);
    if (snr_lin < 0.0f) snr_lin = 0.0f;
    *intel = snr_lin / (snr_lin + half);
    if (*intel < 0.0f) *intel = 0.0f;
    if (*intel > 1.0f) *intel = 1.0f;
    *hiss = 1.0f - *intel;
}

void ricon_audio_init(void) {
    ricon_filters_init();
}

void ricon_audio_state_init(RiconAudioState *state, unsigned int seed) {
    if (state == NULL) return;
    memset(state, 0, sizeof(*state));
    state->noise_seed = (seed != 0u) ? seed : 0x1234567u;
}

void ricon_audio_metrics_reset(RiconAudioState *state) {
    if (state == NULL) return;
    memset(&state->metrics, 0, sizeof(state->metrics));
}

RiconAudioMetrics ricon_audio_metrics_get(const RiconAudioState *state) {
    RiconAudioMetrics empty;
    memset(&empty, 0, sizeof(empty));
    return state != NULL ? state->metrics : empty;
}

void ricon_audio_process_playback_voice(RiconAudioState *state,
                                        const RiconAudioChannelParams *params,
                                        short *samples,
                                        int sample_count,
                                        int channels) {
    int frame, ch;
    int ch_state = RICON_CH_CLEAN;
    float mean_snr = 60.0f;
    float m = 20.0f;
    float pre_intel = -1.0f;
    float pre_hiss = -1.0f;
    float power, new_fade_db, eff_snr, intel, hiss, gain, noise_amp;
    size_t sample_values;

    if (state == NULL) return;
    state->metrics.callbacks_total++;

    if (samples == NULL || sample_count <= 0 || channels <= 0) {
        state->metrics.callbacks_ignored++;
        return;
    }

    sample_values = (size_t)sample_count * (size_t)channels;
    state->metrics.sample_values_processed += (uint64_t)sample_values;

    if (params != NULL) {
        if (!params->audible) {
            ch_state = RICON_CH_LOST;
        } else {
            ch_state = params->state;
        }
        mean_snr = params->mean_snr;
        m = params->m;
        pre_intel = params->intel;
        pre_hiss = params->hiss;
    }

    if (ch_state == RICON_CH_LOST) {
        memset(samples, 0, sample_values * sizeof(short));
        state->metrics.callbacks_muted++;
        state->metrics.sample_values_zeroed += (uint64_t)sample_values;
        return;
    }

    if (m < 0.5f) m = 0.5f;

    if (mean_snr >= 55.0f && m >= 15.0f) {
        state->fade_db = 0.0f;
        state->metrics.callbacks_passthrough++;
        return;
    }

    power = ricon_nakagami_power(state, m);
    if (power < 0.000001f) power = 0.000001f;
    new_fade_db = 10.0f * log10f(power);
    state->fade_db += 0.15f * (new_fade_db - state->fade_db);
    eff_snr = mean_snr + state->fade_db;
    if (pre_intel >= 0.0f && pre_hiss >= 0.0f) {
        intel = pre_intel;
        hiss = pre_hiss;
        if (intel < 0.0f) intel = 0.0f;
        if (intel > 1.0f) intel = 1.0f;
        if (hiss < 0.0f) hiss = 0.0f;
        if (hiss > 1.0f) hiss = 1.0f;
    } else {
        ricon_hill_from_snr(eff_snr, &intel, &hiss);
    }

    gain = 0.35f + 0.65f * intel;
    noise_amp = hiss * 0.12f * 32767.0f;
    state->metrics.callbacks_dsp++;

    for (frame = 0; frame < sample_count; frame++) {
        for (ch = 0; ch < channels; ch++) {
            const int i = frame * channels + ch;
            float s = (float)samples[i] * gain;
            s = ricon_biquad_process(&g_hp750, &state->hp_state, ch, s);
            s = ricon_biquad_process(&g_lp4000, &state->lp_state, ch, s);
            s += ricon_white(state) * noise_amp;
            if (s > 32767.0f) {
                s = 32767.0f;
                state->metrics.sample_values_clipped++;
            }
            if (s < -32768.0f) {
                s = -32768.0f;
                state->metrics.sample_values_clipped++;
            }
            samples[i] = (short)s;
        }
    }
}
