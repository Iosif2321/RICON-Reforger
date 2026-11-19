#include "audio_chain.h"

#include "../../common/ricon_protocol.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SAMPLE_RATE 48000.0f
#define PI_F 3.14159265358979323846f
#define FRAMES 1024
#define CHANNELS 1

typedef struct AudioStats {
    double rms;
    double peak;
    double zero_ratio;
    double mean_abs_delta;
} AudioStats;

static int g_failures = 0;

static void check_true(const char *name, int condition) {
    if (!condition) {
        fprintf(stderr, "FAIL %s\n", name);
        g_failures++;
    }
}

static void check_u64(const char *name, uint64_t actual, uint64_t expected) {
    if (actual != expected) {
        fprintf(stderr, "FAIL %s: got %llu expected %llu\n",
                name, (unsigned long long)actual, (unsigned long long)expected);
        g_failures++;
    }
}

static void fill_sine(short *samples, int frames, float hz, float amp) {
    int i;
    for (i = 0; i < frames; i++) {
        samples[i] = (short)(sinf(2.0f * PI_F * hz * (float)i / SAMPLE_RATE) * amp);
    }
}

static AudioStats stats_for(const short *samples, const short *baseline, int count) {
    AudioStats stats;
    double sum2 = 0.0;
    double peak = 0.0;
    double zeros = 0.0;
    double delta = 0.0;
    int i;
    for (i = 0; i < count; i++) {
        const double s = (double)samples[i];
        const double a = fabs(s);
        sum2 += s * s;
        if (a > peak) peak = a;
        if (samples[i] == 0) zeros += 1.0;
        if (baseline != NULL) delta += fabs((double)samples[i] - (double)baseline[i]);
    }
    stats.rms = sqrt(sum2 / (double)count);
    stats.peak = peak;
    stats.zero_ratio = zeros / (double)count;
    stats.mean_abs_delta = baseline != NULL ? delta / (double)count : 0.0;
    return stats;
}

static void init_params(RiconAudioChannelParams *params, int audible, int state, float snr, float m) {
    memset(params, 0, sizeof(*params));
    params->audible = audible;
    params->state = state;
    params->mean_snr = snr;
    params->m = m;
    params->intel = -1.0f;
    params->hiss = -1.0f;
}

static void run_clean_case(void) {
    RiconAudioState state;
    RiconAudioChannelParams params;
    RiconAudioMetrics metrics;
    short in[FRAMES];
    short out[FRAMES];
    AudioStats s;

    fill_sine(in, FRAMES, 1000.0f, 12000.0f);
    memcpy(out, in, sizeof(out));
    ricon_audio_state_init(&state, 0x111u);
    init_params(&params, 1, RICON_CH_CLEAN, 60.0f, 20.0f);
    ricon_audio_process_playback_voice(&state, &params, out, FRAMES, CHANNELS);
    metrics = ricon_audio_metrics_get(&state);
    s = stats_for(out, in, FRAMES);

    check_true("clean passthrough samples", memcmp(in, out, sizeof(in)) == 0);
    check_u64("clean callbacks", metrics.callbacks_total, 1);
    check_u64("clean passthrough metric", metrics.callbacks_passthrough, 1);
    check_u64("clean processed samples", metrics.sample_values_processed, FRAMES);
    check_true("clean rms preserved", s.rms > 8000.0 && s.rms < 9000.0);
    check_true("clean no delta", s.mean_abs_delta < 0.1);
}

static void run_degraded_case(void) {
    RiconAudioState state;
    RiconAudioChannelParams params;
    RiconAudioMetrics metrics;
    short in[FRAMES];
    short out[FRAMES];
    AudioStats s;

    fill_sine(in, FRAMES, 1000.0f, 12000.0f);
    memcpy(out, in, sizeof(out));
    ricon_audio_state_init(&state, 0x222u);
    init_params(&params, 1, RICON_CH_DEGRADED, 4.0f, 0.7f);
    ricon_audio_process_playback_voice(&state, &params, out, FRAMES, CHANNELS);
    metrics = ricon_audio_metrics_get(&state);
    s = stats_for(out, in, FRAMES);

    check_u64("degraded callbacks", metrics.callbacks_total, 1);
    check_u64("degraded dsp metric", metrics.callbacks_dsp, 1);
    check_true("degraded nonzero", s.zero_ratio < 0.20);
    check_true("degraded differs", s.mean_abs_delta > 500.0);
    check_true("degraded output bounded", s.peak <= 32767.0);
}

static void run_lost_case(void) {
    RiconAudioState state;
    RiconAudioChannelParams params;
    RiconAudioMetrics metrics;
    short out[FRAMES];
    AudioStats s;

    fill_sine(out, FRAMES, 1000.0f, 12000.0f);
    ricon_audio_state_init(&state, 0x333u);
    init_params(&params, 1, RICON_CH_LOST, -20.0f, 0.5f);
    ricon_audio_process_playback_voice(&state, &params, out, FRAMES, CHANNELS);
    metrics = ricon_audio_metrics_get(&state);
    s = stats_for(out, NULL, FRAMES);

    check_u64("lost callbacks", metrics.callbacks_total, 1);
    check_u64("lost muted metric", metrics.callbacks_muted, 1);
    check_u64("lost zeroed samples", metrics.sample_values_zeroed, FRAMES);
    check_true("lost zero ratio", s.zero_ratio > 0.999);
    check_true("lost rms", s.rms < 0.001);
}

static void run_order_independence_case(void) {
    RiconAudioState state_a, state_b, state_c;
    RiconAudioChannelParams clean, degraded, lost;
    short base[FRAMES], a[FRAMES], b[FRAMES], c[FRAMES];

    fill_sine(base, FRAMES, 1000.0f, 12000.0f);
    memcpy(a, base, sizeof(a));
    memcpy(b, base, sizeof(b));
    memcpy(c, base, sizeof(c));

    init_params(&clean, 1, RICON_CH_CLEAN, 60.0f, 20.0f);
    init_params(&degraded, 1, RICON_CH_WEAK, 2.0f, 0.6f);
    init_params(&lost, 1, RICON_CH_LOST, -30.0f, 0.5f);

    ricon_audio_state_init(&state_b, 0x444u);
    ricon_audio_state_init(&state_c, 0x555u);
    ricon_audio_state_init(&state_a, 0x666u);

    ricon_audio_process_playback_voice(&state_b, &degraded, b, FRAMES, CHANNELS);
    ricon_audio_process_playback_voice(&state_c, &lost, c, FRAMES, CHANNELS);
    ricon_audio_process_playback_voice(&state_a, &clean, a, FRAMES, CHANNELS);

    check_true("order clean still passthrough", memcmp(a, base, sizeof(a)) == 0);
    check_true("order degraded differs", stats_for(b, base, FRAMES).mean_abs_delta > 500.0);
    check_true("order lost zero", stats_for(c, NULL, FRAMES).zero_ratio > 0.999);
}

int main(void) {
    run_clean_case();
    run_degraded_case();
    run_lost_case();
    run_order_independence_case();

    if (g_failures != 0) {
        fprintf(stderr, "audio_chain_smoke failed: %d checks\n", g_failures);
        return 1;
    }
    puts("audio_chain_smoke ok");
    return 0;
}
