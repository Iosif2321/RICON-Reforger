// TEST_ONLY device audio-band profile. Generated C retains the RF/RNG/noise implementation.
#pragma once
#include "audio_chain.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct RiconLiveNoiseMetrics {
    uint64_t values;
    double sourceSquares, rawRfSquares, bandRfSquares, sourceRfCross;
    double mixedSquares, outputSquares, residualSquares;
} RiconLiveNoiseMetrics;

void ricon_live150_audio_init(void);
void ricon_live150_audio_state_init(RiconAudioState*, unsigned int);
void ricon_live150_audio_metrics_reset(RiconAudioState*);
RiconAudioMetrics ricon_live150_audio_metrics_get(const RiconAudioState*);
// A null noise-LP state is the TEST_ONLY v0.5.8 reference; hardware LIVE always supplies its owner state.
void ricon_live150_audio_process_playback_voice(RiconAudioState*, const RiconAudioChannelParams*, short*, int, int,
                                               RiconAudioBiquadState* noise_lp_state, RiconLiveNoiseMetrics*);
#ifdef __cplusplus
}

struct LivePairedLevels {
    uint64_t blocks{}, values{}, clipped{};
    uint64_t firstQpc{}, lastQpc{};
    double decodedSquares{}, postCSquares{}, finalSquares{};
    unsigned int decodedPeak{}, postCPeak{}, finalPeak{};
    RiconLiveNoiseMetrics noise{};
};

static void observeLivePaired(const LivePairedLevels& block);

static void measurePairedStage(const short* samples, int count, double& squares, unsigned int& peak) {
    for (int i = 0; i < count; ++i) {
        const int value = samples[i];
        const unsigned int magnitude = static_cast<unsigned int>(value < 0 ? -value : value);
        squares += static_cast<double>(value) * value;
        if (magnitude > peak) peak = magnitude;
    }
}
#endif
