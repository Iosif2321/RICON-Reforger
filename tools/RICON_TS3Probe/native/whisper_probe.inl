// TOOL_ONLY test on the existing isolated custom capture profile.
enum class WhisperProbeMode { proof, ack, tone, toneAbc };
static bool whisperSynthTone(WhisperProbeMode mode) {
    return mode == WhisperProbeMode::tone || mode == WhisperProbeMode::toneAbc;
}
struct WhisperPeer {
    uint64 handler{};
    anyID client{};
    char uid[128]{};
};
struct WhisperBinding {
    NetworkBinding server{};
    WhisperPeer peers[3]{};
    size_t count{3};
    WhisperProbeMode mode{WhisperProbeMode::proof};
    bool abcOutputsPrepared{};
};
static std::atomic<uint64> whisperHandlers[3]{};
static std::atomic<anyID> whisperSender{0};
static std::atomic<uint64> whisperBlocks[2]{};
static std::atomic<uint64> whisperToneWindow{0};
static std::atomic<bool> whisperAbcMeasuring{false};
static std::atomic<unsigned int> whisperAbcCarriers[2][2]{};
static std::atomic<unsigned int> whisperAbcShortCarriers[3][2]{}, whisperAbcPeak[3]{};
static struct {
    std::atomic<bool> talking{false}, seen{false};
    std::atomic<unsigned int> hits{0};
} whisperToneEvidence[2];
static std::mutex whisperAckMutex;
static struct {
    char code[128]{};
    uint64 handler{}, ticket{}, issued{};
    bool pending{}, seen{}, tone{};
    unsigned int error{};
} whisperAck, whisperPriorToneAck;

static bool whisperTestEnabled() {
    char path[4096]{};
    if (!voiceTestProfileEnabled() || !join(path, sizeof(path), configDirectory, "ricon-whisper-test.enabled")) return false;
    const DWORD attributes = GetFileAttributesA(path);
    return attributes != INVALID_FILE_ATTRIBUTES && !(attributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT));
}

static int observeWhisperAck(uint64 connection, unsigned int error, const char* code) {
    if (!code || !code[0]) return 0;
    std::lock_guard<std::mutex> lock(whisperAckMutex);
    auto& ack = whisperPriorToneAck.pending && !std::strcmp(code, whisperPriorToneAck.code) ? whisperPriorToneAck : whisperAck;
    if (!ack.pending || (ack.seen && (!ack.tone || error == ERROR_ok)) || connection != ack.handler || networkCancelled(ack.ticket)
        || (!ack.tone && GetTickCount64() - ack.issued >= 2000) || std::strcmp(code, ack.code)) return 0;
    ack.error = error;
    ack.seen = true;
    if (ack.tone && error != ERROR_ok) requestWorkerStop();
    return 1;
}

static bool whisperCarrier(const short* samples, int frames, int channels, bool high = false, int minimumFrames = 480) {
    if (!samples || frames < minimumFrames || channels < 1 || channels > 2) return false;
    // Integral cycles at the TS3 callback's 48 kHz rate make the Goertzel bin phase independent.
    const int count = frames > 960 ? 960 : frames / 80 * 80;
    const double coefficient = high ? 1.9753766811902755 : 1.993834667466256;
    for (int channel = 0; channel < channels; ++channel) {
        double previous{}, prior{}, sum{}, squares{};
        for (int i = 0; i < count; ++i) {
            const double sample = samples[i * channels + channel];
            const double next = sample + coefficient * previous - prior;
            prior = previous;
            previous = next;
            sum += sample;
            squares += sample * sample;
        }
        const double energy = squares - sum * sum / count;
        const double bin = previous * previous + prior * prior - coefficient * previous * prior;
        if (energy >= 32.0 * 32.0 * count && 2.0 * bin >= 0.6 * energy * count) return true;
    }
    return false;
}

static void observeWhisperAbcPcm(size_t peer, const short* samples, int frames, int channels) {
    if (!samples || frames <= 0 || channels < 1 || channels > 2) return;
    const int count = frames < 960 ? frames : 960;
    unsigned int peak = 0;
    for (int i = 0; i < count * channels; ++i) {
        const auto amplitude = static_cast<unsigned int>(std::abs(static_cast<int>(samples[i])));
        if (amplitude > peak) peak = amplitude;
    }
    auto previous = whisperAbcPeak[peer].load();
    while (peak > previous && !whisperAbcPeak[peer].compare_exchange_weak(previous, peak)) {}
    for (size_t band = 0; band < 2; ++band) {
        for (int offset = 0; offset + 240 <= count; offset += 80) {
            if (!whisperCarrier(samples + offset * channels, 240, channels, band != 0, 240)) continue;
            whisperAbcShortCarriers[peer][band].fetch_add(1);
            break;
        }
    }
}

static void observeWhisperCapture(uint64 connection, const short* samples, int frames, int channels) {
    if (connection && connection == whisperHandlers[0].load() && whisperAbcMeasuring.load())
        observeWhisperAbcPcm(2, samples, frames, channels);
}

static void observeWhisperPlayback(uint64 connection, anyID client, const short* samples, int frames, int channels) {
    const anyID sender = whisperSender.load();
    if (!sender || client != sender || !samples || frames <= 0 || channels <= 0) return;
    for (size_t i = 0; i < 2; ++i) if (connection == whisperHandlers[i + 1].load()) {
        whisperBlocks[i].fetch_add(1);
        auto& evidence = whisperToneEvidence[i];
        const bool abc = whisperAbcMeasuring.load();
        const bool selected = connection && connection == whisperToneWindow.load() && evidence.talking.load();
        if (!abc && !selected) continue;
        const bool low = whisperCarrier(samples, frames, channels);
        const bool high = abc && whisperCarrier(samples, frames, channels, true);
        if (abc) {
            observeWhisperAbcPcm(i, samples, frames, channels);
            if (low) whisperAbcCarriers[i][0].fetch_add(1);
            if (high) whisperAbcCarriers[i][1].fetch_add(1);
        }
        if (selected && ((abc && i == 0) ? high : low)) evidence.hits.fetch_add(1);
    }
}

static void observeWhisperTalk(uint64 connection, int status, int isReceivedWhisper, anyID client) {
    if (!connection || connection != whisperToneWindow.load() || !client || client != whisperSender.load()) return;
    const bool talking = status == STATUS_TALKING && isReceivedWhisper;
    for (size_t i = 0; i < 2; ++i) if (connection == whisperHandlers[i + 1].load()) {
        whisperToneEvidence[i].talking.store(talking);
        if (talking) whisperToneEvidence[i].seen.store(true);
    }
}

static void cancelWhisperHandler(uint64 connection) {
    if (!whisperSender.load()) return;
    for (const auto& handler : whisperHandlers) if (connection == handler.load()) { requestWorkerStop(); break; }
}

static bool whisperClientUid(uint64 handler, anyID client, char (&uid)[128]) {
    char* value{};
    const unsigned int result = api.getClientVariableAsString(handler, client, CLIENT_UNIQUE_IDENTIFIER, &value);
    if (result) return false;
    const bool valid = value && value[0] && std::strlen(value) < sizeof(uid);
    if (valid) strcpy_s(uid, value);
    if (value) api.freeMemory(value);
    return valid;
}

static bool whisperPeerCurrent(const WhisperBinding& binding, const WhisperPeer& peer) {
    char uid[128]{};
    return sameOwnedConnection(binding.server, peer.handler, peer.client)
        && whisperClientUid(peer.handler, peer.client, uid) && !std::strcmp(uid, peer.uid);
}

static bool whisperRosterCurrent(const WhisperBinding& binding) {
    anyID* occupants{};
    if (api.getChannelClientList(binding.peers[0].handler, binding.server.channel, &occupants) || !occupants) return false;
    unsigned int mask = 0;
    bool isolated = true;
    for (size_t i = 0; occupants[i]; ++i) {
        size_t peer = 0;
        while (peer < binding.count && binding.peers[peer].client != occupants[i]) ++peer;
        if (peer == binding.count || (mask & (1u << peer))) { isolated = false; break; }
        mask |= 1u << peer;
    }
    api.freeMemory(occupants);
    return isolated && mask == (1u << binding.count) - 1;
}

static bool whisperOutputPresent(uint64 handler, unsigned int& queried) {
    char* name{};
    int isDefault{};
    queried = api.getCurrentPlaybackDeviceName(handler, &name, &isDefault);
    if (queried != ERROR_ok) return false;
    const bool ready = name && name[0];
    if (name) api.freeMemory(name);
    return ready;
}

static bool whisperCurrent(const WhisperBinding& binding, uint64 ticket) {
    char ip[16]{};
    if (networkCancelled(ticket) || !whisperTestEnabled() || !expectedTestServer(ip) || std::strcmp(ip, binding.server.ip)) return false;
    for (size_t i = 0; i < binding.count; ++i) {
        const auto& peer = binding.peers[i];
        uint64 channel{};
        char uid[128]{};
        if (!whisperPeerCurrent(binding, peer) || !knownCapture(peer.handler)
            || api.getChannelOfClient(peer.handler, peer.client, &channel) || channel != binding.server.channel
            || (i && (!whisperClientUid(binding.peers[0].handler, peer.client, uid) || std::strcmp(uid, peer.uid)))) return false;
        if (i && binding.abcOutputsPrepared) {
            unsigned int queried{};
            int mute{};
            if (!whisperOutputPresent(peer.handler, queried)
                || api.getClientSelfVariableAsInt(peer.handler, CLIENT_OUTPUT_MUTED, &mute) || mute != MUTEOUTPUT_NONE) return false;
        }
    }
    return whisperRosterCurrent(binding) && !networkCancelled(ticket);
}

static bool whisperBind(WhisperBinding& binding, uint64 ticket) {
    auto fresh = binding;
    for (size_t i = 0; i < fresh.count; ++i) {
        auto& peer = fresh.peers[i];
        if (!connectionAt(peer.handler, fresh.server.ip, peer.client) || !whisperClientUid(peer.handler, peer.client, peer.uid)) return false;
        for (size_t j = 0; j < i; ++j)
            if (peer.client == fresh.peers[j].client || !std::strcmp(peer.uid, fresh.peers[j].uid)) return false;
    }
    const auto& a = fresh.peers[0];
    if (!serverUid(a.handler, fresh.server.serverUid) || api.getChannelOfClient(a.handler, a.client, &fresh.server.channel)
        || !fresh.server.channel || !whisperCurrent(fresh, ticket)) return false;
    binding = fresh;
    return true;
}

static bool whisperSetInput(uint64 handler, int state) {
    return stateUpdateOk(api.setClientSelfVariableAsInt(handler, CLIENT_INPUT_DEACTIVATED, state))
        && flushTestState(handler, "whisper-input-flush");
}

static bool whisperInputs(const WhisperBinding& binding, bool sourceActive) {
    for (size_t i = 0; i < binding.count; ++i) {
        int state{};
        if (api.getClientSelfVariableAsInt(binding.peers[i].handler, CLIENT_INPUT_DEACTIVATED, &state)
            || state != (!i && sourceActive ? INPUT_ACTIVE : INPUT_DEACTIVATED)) return false;
    }
    return true;
}

static bool prepareWhisperAck(uint64 handler, uint64 ticket, char (&code)[128], uint64& issued, bool tone = false) {
    api.createReturnCode(pluginId, code, sizeof(code));
    if (!code[0] || !std::memchr(code, 0, sizeof(code)) || networkCancelled(ticket)) {
        record("whisper-request-code", handler, 1, "missing SDK return code");
        return false;
    }
    issued = GetTickCount64();
    std::lock_guard<std::mutex> lock(whisperAckMutex);
    if (!std::strcmp(whisperAck.code, code) || (whisperPriorToneAck.pending && !std::strcmp(whisperPriorToneAck.code, code))) return false;
    if (tone && whisperAck.pending && whisperAck.tone && whisperAck.ticket == ticket) {
        if (whisperPriorToneAck.pending || whisperAck.handler != handler) return false;
        whisperPriorToneAck = whisperAck;
    }
    strcpy_s(whisperAck.code, code);
    whisperAck.handler = handler;
    whisperAck.ticket = ticket;
    whisperAck.issued = issued;
    whisperAck.pending = true;
    whisperAck.seen = false;
    whisperAck.tone = tone;
    whisperAck.error = ERROR_undefined;
    return true;
}

static bool whisperRequest(const WhisperBinding& binding, size_t target, uint64 ticket, bool control = false) {
    if (!target || target >= binding.count || !whisperCurrent(binding, ticket) || !whisperInputs(binding, false)) return false;
    const auto& source = binding.peers[0];
    if (control && !api.requestClientVariables) {
        record("whisper-control-ack", source.handler, ERROR_not_implemented, "requestClientVariables unavailable");
        return false;
    }
    const anyID targets[] = {binding.peers[target].client, 0};
    char code[128]{};
    uint64 issued{};
    if (!prepareWhisperAck(source.handler, ticket, code, issued)) return false;
    const unsigned int request = control ? api.requestClientVariables(source.handler, targets[0], code)
                                         : api.requestClientSetWhisperList(source.handler, source.client, nullptr, targets, code);
    unsigned int callback = ERROR_undefined;
    bool seen = false;
    while (request == ERROR_ok && !networkCancelled(ticket) && GetTickCount64() - issued < 2000) {
        {
            std::lock_guard<std::mutex> lock(whisperAckMutex);
            seen = whisperAck.seen;
            callback = whisperAck.error;
        }
        if (seen) break;
        Sleep(10);
    }
    {
        std::lock_guard<std::mutex> lock(whisperAckMutex);
        whisperAck.pending = false;
    }
    const bool accepted = request == ERROR_ok && seen && callback == ERROR_ok && !networkCancelled(ticket);
    char detail[144]{};
    std::snprintf(detail, sizeof(detail), "target=%c invoke=%u callbackSeen=%u callbackError=%u ticket=%llu",
                  target == 1 ? 'B' : 'C', request, static_cast<unsigned int>(seen), callback, ticket);
    record(control ? "whisper-control-ack" : "whisper-route-ack", source.handler, accepted ? 0 : 1, detail);
    return accepted;
}

struct WhisperCounters { uint64 packets{}, bytes{}, blocks{}; };
static constexpr size_t whisperToneTx = 2;

static bool whisperNoDelivery(const WhisperCounters& counters) {
    return !counters.packets && !counters.bytes && !counters.blocks;
}

static bool whisperTonePhasePass(const WhisperCounters (&deltas)[3], size_t target, bool abc, bool received,
                                 unsigned int carrierBlocks) {
    if (!target || target > 2) return false;
    const auto& source = deltas[whisperToneTx];
    const auto& included = deltas[target - 1];
    return source.packets && source.bytes && included.packets && included.bytes && included.blocks
        && received && carrierBlocks >= 3 && (!abc || whisperNoDelivery(deltas[2 - target]));
}

static bool whisperToneTrigger(const WhisperBinding& binding, size_t target, uint64 ticket) {
    const bool abc = binding.mode == WhisperProbeMode::toneAbc;
    if (!whisperSynthTone(binding.mode) || binding.count != (abc ? 3u : 2u) || !target || target >= binding.count
        || !whisperCurrent(binding, ticket) || !whisperInputs(binding, false) || !binding.peers[target].client || networkCancelled(ticket)) return false;
    const auto& source = binding.peers[0];
    const anyID targets[] = {binding.peers[target].client, 0};
    char code[128]{};
    uint64 issued{};
    if (!prepareWhisperAck(source.handler, ticket, code, issued, true) || networkCancelled(ticket)) return false;
    const unsigned int invoked = api.requestClientSetWhisperList(source.handler, source.client, nullptr, targets, code);
    if (abc) {
        record("whisper-tone-abc-trigger", source.handler, invoked, target == 1
            ? "scope=SYNTHETIC_A_B_C_SYNC_TRIGGER target=B callbackProof=0"
            : "scope=SYNTHETIC_A_B_C_SYNC_TRIGGER target=C callbackProof=0");
    } else {
        record("whisper-tone-trigger", source.handler, invoked, "scope=SYNTHETIC_A_B_SYNC_TRIGGER target=B callbackProof=0");
    }
    return invoked == ERROR_ok && !networkCancelled(ticket);
}

static bool whisperCounters(const WhisperBinding& binding, WhisperCounters (&counters)[3]) {
    for (size_t i = 0; i + 1 < binding.count; ++i) {
        const auto& peer = binding.peers[i + 1];
        if (api.getConnectionVariableAsUInt64(peer.handler, peer.client, CONNECTION_PACKETS_RECEIVED_SPEECH, &counters[i].packets)
            || api.getConnectionVariableAsUInt64(peer.handler, peer.client, CONNECTION_BYTES_RECEIVED_SPEECH, &counters[i].bytes)) return false;
        counters[i].blocks = whisperBlocks[i].load();
    }
    if (whisperSynthTone(binding.mode)) {
        const auto& source = binding.peers[0];
        auto& sent = counters[whisperToneTx];
        const unsigned int packets = api.getConnectionVariableAsUInt64(source.handler, source.client, CONNECTION_PACKETS_SENT_SPEECH, &sent.packets);
        const unsigned int bytes = api.getConnectionVariableAsUInt64(source.handler, source.client, CONNECTION_BYTES_SENT_SPEECH, &sent.bytes);
        if (packets || bytes) {
            char error[80]{};
            std::snprintf(error, sizeof(error), "packetsQuery=%u bytesQuery=%u", packets, bytes);
            record("whisper-tone-tx-counters", source.handler, packets ? packets : bytes, error);
            return false;
        }
    }
    return true;
}

static bool whisperQuiet(const WhisperBinding& binding, uint64 ticket, WhisperCounters (&stable)[3]) {
    if (!whisperCounters(binding, stable)) return false;
    for (int attempt = 0, unchanged = 0; attempt < 30 && !networkCancelled(ticket); ++attempt) {
        Sleep(100);
        WhisperCounters next[3]{};
        if (!whisperCurrent(binding, ticket) || !whisperInputs(binding, false) || !whisperCounters(binding, next)) return false;
        bool same = true;
        for (size_t i = 0; i < 3; ++i) {
            if (next[i].packets < stable[i].packets || next[i].bytes < stable[i].bytes || next[i].blocks < stable[i].blocks) return false;
            same &= next[i].packets == stable[i].packets && next[i].bytes == stable[i].bytes && next[i].blocks == stable[i].blocks;
            stable[i] = next[i];
        }
        unchanged = same ? unchanged + 1 : 0;
        if (unchanged == 7) return true;
    }
    return false;
}

static bool resetWhisperAbcCapture(const WhisperBinding& binding, uint64 ticket) {
    const auto& source = binding.peers[0];
    if (!api.openCaptureDevice || !whisperCurrent(binding, ticket) || !whisperInputs(binding, false)) return false;
    const unsigned int closed = api.closeCaptureDevice(source.handler);
    record("whisper-tone-abc-close-capture", source.handler, closed, "replace only A's known custom input");
    if (closed != ERROR_ok || networkCancelled(ticket) || !whisperPeerCurrent(binding, source)
        || !whisperInputs(binding, false)) return false;

    const unsigned int opened = api.openCaptureDevice(source.handler, "custom", knownCaptureId);
    record("whisper-tone-abc-open-capture", source.handler, opened, "reopen A's explicit virtual device");
    return opened == ERROR_ok && whisperCurrent(binding, ticket) && whisperInputs(binding, false)
        && stateUpdateOk(api.setPreProcessorConfigValue(source.handler, "vad", "false"));
}

static bool whisperPhase(const WhisperBinding& binding, size_t target, const short (&tone)[48000], uint64 ticket) {
    const auto& source = binding.peers[0];
    const bool toneOnly = whisperSynthTone(binding.mode);
    const bool abc = binding.mode == WhisperProbeMode::toneAbc;
    WhisperCounters before[3]{}, after[3]{};
    if (abc) {
        if (target == 1 && !resetWhisperAbcCapture(binding, ticket)) return false;
        if (!whisperCurrent(binding, ticket) || !whisperInputs(binding, false) || !whisperCounters(binding, before)) return false;
        const unsigned int activated = api.activateCaptureDevice(source.handler);
        record("whisper-tone-abc-activate-off", source.handler, activated, "prepare exact custom capture while inputs are off");
        if (!stateUpdateOk(activated) || !whisperInputs(binding, false)) return false;
    }
    if (toneOnly) {
        if (!whisperToneTrigger(binding, target, ticket)) return false;
    } else if (!whisperRequest(binding, target, ticket)) return false;
    WhisperCounters quiet[3]{};
    if (!whisperQuiet(binding, ticket, abc ? quiet : before)) { record("whisper-pre-counters", source.handler, 1, "no stable own receive counters"); return false; }
    if (abc) {
        for (auto& peer : whisperAbcCarriers) for (auto& hits : peer) hits.store(0);
        for (auto& peer : whisperAbcShortCarriers) for (auto& hits : peer) hits.store(0);
        for (auto& peak : whisperAbcPeak) peak.store(0);
        whisperAbcMeasuring.store(true);
    }
    if (toneOnly) {
        auto& evidence = whisperToneEvidence[target - 1];
        evidence.hits.store(0);
        evidence.talking.store(false);
        evidence.seen.store(false);
        whisperToneWindow.store(binding.peers[target].handler);
    }
    if (!whisperCurrent(binding, ticket) || !whisperInputs(binding, false) || !whisperSetInput(source.handler, INPUT_ACTIVE)) return false;
    const uint64 started = GetTickCount64();
    int sent = 0;
    while (sent < 50 && GetTickCount64() - started < 1500) {
        if (!whisperCurrent(binding, ticket) || !whisperInputs(binding, true)
            || (!abc && !stateUpdateOk(api.activateCaptureDevice(source.handler))) || networkCancelled(ticket)
            || api.processCustomCaptureData(knownCaptureId, tone + sent * 960, 960)) break;
        ++sent;
        while (!networkCancelled(ticket) && GetTickCount64() - started < static_cast<uint64>(sent) * 20) Sleep(1);
    }
    if (!whisperPeerCurrent(binding, source) || !whisperSetInput(source.handler, INPUT_DEACTIVATED) || sent != 50
        || !whisperQuiet(binding, ticket, after)) {
        record("whisper-send-or-counters", source.handler, 1, "known PCM incomplete, input stop failed, or receive counters unavailable");
        return false;
    }
    for (size_t i = 0; i < 3; ++i) {
        if (after[i].packets < before[i].packets || after[i].bytes < before[i].bytes || after[i].blocks < before[i].blocks) return false;
        after[i].packets -= before[i].packets;
        after[i].bytes -= before[i].bytes;
        after[i].blocks -= before[i].blocks;
    }
    const auto& included = after[target - 1];
    const auto& excluded = after[2 - target];
    if (toneOnly) {
        whisperToneWindow.store(0);
        whisperAbcMeasuring.store(false);
        const auto& evidence = whisperToneEvidence[target - 1];
        const bool received = evidence.seen.load();
        const unsigned int carrierBlocks = evidence.hits.load();
        const auto& transmitted = after[whisperToneTx];
        const bool pass = whisperTonePhasePass(after, target, abc, received, carrierBlocks);
        char detail[448]{};
        if (abc) {
            std::snprintf(detail, sizeof(detail), "target=%c frames=%d Apackets=%llu Abytes=%llu Bpackets=%llu Bbytes=%llu BpreDspBlocks=%llu Cpackets=%llu Cbytes=%llu CpreDspBlocks=%llu receivedWhisper=%u carrierHits=%u B600=%u B1200=%u C600=%u C1200=%u",
                          target == 1 ? 'B' : 'C', sent, transmitted.packets, transmitted.bytes, after[0].packets, after[0].bytes,
                          after[0].blocks, after[1].packets, after[1].bytes, after[1].blocks, static_cast<unsigned int>(received), carrierBlocks,
                          whisperAbcCarriers[0][0].load(), whisperAbcCarriers[0][1].load(),
                          whisperAbcCarriers[1][0].load(), whisperAbcCarriers[1][1].load());
            char spectra[208]{};
            std::snprintf(spectra, sizeof(spectra), "target=%c Ashort600=%u Ashort1200=%u Apeak=%u Bshort600=%u Bshort1200=%u Bpeak=%u Cshort600=%u Cshort1200=%u Cpeak=%u",
                          target == 1 ? 'B' : 'C', whisperAbcShortCarriers[2][0].load(), whisperAbcShortCarriers[2][1].load(), whisperAbcPeak[2].load(),
                          whisperAbcShortCarriers[0][0].load(), whisperAbcShortCarriers[0][1].load(), whisperAbcPeak[0].load(),
                          whisperAbcShortCarriers[1][0].load(), whisperAbcShortCarriers[1][1].load(), whisperAbcPeak[1].load());
            record("whisper-tone-abc-spectra", source.handler, 0, spectra);
        } else {
            std::snprintf(detail, sizeof(detail), "target=B frames=%d Apackets=%llu Abytes=%llu Bpackets=%llu Bbytes=%llu BpreDspBlocks=%llu receivedWhisper=%u carrier600Blocks=%u",
                      sent, transmitted.packets, transmitted.bytes, included.packets, included.bytes, included.blocks,
                      static_cast<unsigned int>(received), carrierBlocks);
        }
        record(abc ? "whisper-tone-abc-phase" : "whisper-tone-phase", source.handler, pass ? 0 : 1, detail);
        return pass && !networkCancelled(ticket);
    }
    const bool pass = included.packets && included.bytes && included.blocks && whisperNoDelivery(excluded);
    char detail[256]{};
    std::snprintf(detail, sizeof(detail), "target=%c frames=%d Bpackets=%llu Bbytes=%llu BpreDspBlocks=%llu Cpackets=%llu Cbytes=%llu CpreDspBlocks=%llu",
                  target == 1 ? 'B' : 'C', sent, after[0].packets, after[0].bytes, after[0].blocks,
                  after[1].packets, after[1].bytes, after[1].blocks);
    record("whisper-phase", source.handler, pass ? 0 : 1, detail);
    return pass && !networkCancelled(ticket);
}

static bool whisperClose(const WhisperBinding& binding, bool bound) {
    whisperToneWindow.store(0);
    whisperAbcMeasuring.store(false);
    whisperSender.store(0);
    bool clean = true;
    for (const auto& peer : binding.peers) if (peer.handler) {
        const bool owned = stillOwnGuiConnection(binding.server, peer.handler, peer.client, bound);
        int status{};
        const bool exact = owned && (!bound || (!api.getConnectionStatus(peer.handler, &status)
            && (status == STATUS_DISCONNECTED || whisperPeerCurrent(binding, peer))));
        clean &= exact && closeTestConnection(peer.handler);
    }
    for (auto& handler : whisperHandlers) handler.store(0);
    return clean;
}

static bool ensureWhisperAbcPlayback(const WhisperBinding& binding, size_t target, uint64 ticket) {
    if (binding.mode != WhisperProbeMode::toneAbc || !target || target >= binding.count) return false;
    const uint64 handler = binding.peers[target].handler;
    unsigned int queried{};
    if (!whisperCurrent(binding, ticket)) return false;
    if (whisperOutputPresent(handler, queried)) return true;
    if (queried != ERROR_ok && queried != ERROR_sound_no_playback_device_available) return false;
    if (!whisperCurrent(binding, ticket)) return false;
    const unsigned int opened = openDefaultHardwarePlayback(handler);
    const bool ready = (opened == ERROR_ok || opened == ERROR_sound_handler_has_device) && whisperOutputPresent(handler, queried);
    record("whisper-tone-abc-playback", handler, ready ? ERROR_ok : opened ? opened : ERROR_sound_no_playback_device_available,
           target == 1 ? "target=B owned temporary output" : "target=C owned temporary output");
    return ready && !networkCancelled(ticket);
}

static void runWhisperProbe(WhisperBinding binding, uint64 ticket) {
    const auto source = binding.peers[0].handler;
    const bool ackOnly = binding.mode == WhisperProbeMode::ack;
    const bool toneOnly = whisperSynthTone(binding.mode);
    const bool abc = binding.mode == WhisperProbeMode::toneAbc;
    bool bound = false, pass = false, vadChanged = false;
    char vad[6]{};
    do {
        const uint64 started = GetTickCount64();
        while (!networkCancelled(ticket) && GetTickCount64() - started < 15000) {
            if (whisperBind(binding, ticket)) { bound = true; break; }
            Sleep(50);
        }
        if (!bound) { record("whisper-bind", source, 1, "exact same-channel peers with known custom capture required"); break; }
        bool inactive = true;
        for (size_t i = 0; i < binding.count; ++i) inactive &= whisperSetInput(binding.peers[i].handler, INPUT_DEACTIVATED);
        if (!inactive || !whisperInputs(binding, false)) break;
        for (auto& blocks : whisperBlocks) blocks.store(0);
        for (size_t i = 0; i < binding.count; ++i) whisperHandlers[i].store(binding.peers[i].handler);
        whisperSender.store(binding.peers[0].client);
        record("whisper-bind", source, 0, "same UID/channel; all input off; verified custom capture");
        if (ackOnly) {
            whisperRequest(binding, 1, ticket, true); // The read-only control has no authority over the route result.
            pass = whisperRequest(binding, 1, ticket) && whisperCurrent(binding, ticket) && whisperInputs(binding, false);
            break;
        }
        if (toneOnly) {
            std::lock_guard<std::mutex> lock(operations);
            bool playbackReady = whisperCurrent(binding, ticket);
            for (size_t i = 1; playbackReady && i < binding.count; ++i)
                playbackReady = abc ? ensureWhisperAbcPlayback(binding, i, ticket) : ensurePlayback(binding.peers[i].handler);
            if (!playbackReady) break;
            binding.abcOutputsPrepared = abc;
            if (abc && !whisperCurrent(binding, ticket)) break;
        }
        short tone[48000]{};
        if (toneOnly) {
            constexpr double cycle = 6.28318530717958647692;
            for (int i = 0; i < 80; ++i) tone[i] = static_cast<short>(std::lround(1024.0 * std::sin(cycle * i / 80.0)));
            for (int i = 80; i < 48000; ++i) tone[i] = tone[i % 80];
        } else if (!readKnownWave(1, tone)) {
            break;
        }
        if (!whisperCurrent(binding, ticket)) break;
        char* value{};
        const unsigned int gotVad = api.getPreProcessorConfigValue(source, "vad", &value);
        if (!gotVad && value && (!std::strcmp(value, "true") || !std::strcmp(value, "false"))) strcpy_s(vad, value);
        if (!gotVad && value) api.freeMemory(value);
        if (!vad[0] || !stateUpdateOk(api.setPreProcessorConfigValue(source, "vad", "false"))) break;
        vadChanged = true;
        if (binding.count == 3 && !whisperPhase(binding, 2, tone, ticket)) break;
        if (abc) {
            constexpr double cycle = 6.28318530717958647692;
            for (int i = 0; i < 40; ++i) tone[i] = static_cast<short>(std::lround(1024.0 * std::sin(cycle * i / 40.0)));
            for (int i = 40; i < 48000; ++i) tone[i] = tone[i % 40];
        }
        pass = whisperPhase(binding, 1, tone, ticket);
    } while (false);
    const bool current = bound && whisperPeerCurrent(binding, binding.peers[0]);
    const bool inputOff = !bound || (current && whisperSetInput(source, INPUT_DEACTIVATED));
    const bool restored = !vadChanged || (current && stateUpdateOk(api.setPreProcessorConfigValue(source, "vad", vad)));
    const bool cleanup = whisperClose(binding, bound) && inputOff && restored;
    if (toneOnly) {
        unsigned int callbackError = ERROR_ok;
        {
            std::lock_guard<std::mutex> lock(whisperAckMutex);
            for (auto* ack : {&whisperPriorToneAck, &whisperAck}) if (ack->pending && ack->ticket == ticket && ack->tone) {
                ack->pending = false;
                if (ack->seen && ack->error != ERROR_ok) callbackError = ack->error;
            }
        }
        if (callbackError != ERROR_ok) record("whisper-tone-route-error", source, callbackError, "correlated request error revoked synthetic trigger");
    }
    const bool complete = pass && cleanup && !networkCancelled(ticket);
    const char* action = ackOnly ? (complete ? "whisper-ack-complete" : "whisper-ack-failed")
                         : abc ? (complete ? "whisper-tone-abc-complete" : "whisper-tone-abc-failed")
                         : toneOnly ? (complete ? "whisper-tone-complete" : "whisper-tone-failed")
                                 : (complete ? "whisper-complete" : "whisper-failed");
    const char* scope = ackOnly ? "ACK-only-zero-PCM" : abc ? "SYNTHETIC_A_B_C_SYNC_TRIGGER"
                      : toneOnly ? "SYNTHETIC_A_B_SYNC_TRIGGER" : "known-PCM-carrier";
    char detail[96]{};
    std::snprintf(detail, sizeof(detail), "proof=%u cleanup=%u scope=%s", static_cast<unsigned int>(pass),
                  static_cast<unsigned int>(cleanup), scope);
    record(action, source, complete ? 0 : 1, detail);
    networkRunning.store(false);
}

static bool whisperProfiles(PluginGuiProfile type, const char* const* names, size_t required, char (*source)[256] = nullptr,
                            bool soleSource = false) {
    char** profiles{};
    int preferred{};
    if (api.getProfileList(type, &preferred, &profiles) || !profiles) return false;
    unsigned int count[3]{};
    unsigned int otherCount{};
    for (size_t i = 0; profiles[i]; ++i) {
        const char* name = profiles[i];
        size_t match = 0;
        while (match < required && std::strcmp(name, names[match])) ++match;
        if (match < required) ++count[match];
        else if (source && name[0] && std::strcmp(name, "RICON_C")) {
            ++otherCount;
            if (std::strlen(name) < sizeof(*source)) strcpy_s(*source, name);
        }
        api.freeMemory(profiles[i]);
    }
    api.freeMemory(profiles);
    if (source) {
        if (soleSource && count[0] + otherCount != 1) return false;
        if (count[0] == 1) strcpy_s(*source, names[0]);
        // The isolated fixture may have a localized name for its sole A identity.
        else if (!count[0] && otherCount == 1 && (*source)[0]) count[0] = 1;
    }
    for (size_t i = 0; i < required; ++i) if (count[i] != 1) return false;
    return true;
}

static bool beginWhisperProbe(WhisperProbeMode mode) {
    const bool ackOnly = mode == WhisperProbeMode::ack;
    if (!whisperTestEnabled() || !knownCaptureRegistered || !pluginId[0] || liveCleanupPending()
        || !api.getProfileList || !api.getServerConnectionHandlerList || !api.getConnectionStatus || !api.getClientID
        || !api.getConnectionVariableAsString || !api.getConnectionVariableAsUInt64 || !api.getClientVariableAsString
        || !api.getServerVariableAsString || !api.getChannelOfClient || !api.getChannelClientList || !api.freeMemory
        || !api.getCurrentCaptureMode || !api.getCurrentCaptureDeviceName || !api.getClientSelfVariableAsInt
        || !api.setClientSelfVariableAsInt || !api.flushClientSelfUpdates || !api.guiConnect
        || !api.createReturnCode || !api.requestClientSetWhisperList || !api.closeCaptureDevice || !api.closePlaybackDevice
        || !api.stopConnection || (!ackOnly && (!api.getPreProcessorConfigValue || !api.setPreProcessorConfigValue
            || !api.activateCaptureDevice || !api.processCustomCaptureData)) || networkRunning.exchange(true)) return false;
    auto failed = [] { networkRunning.store(false); return false; };
    if (testWorker.joinable()) {
        if (WaitForSingleObject(testWorker.native_handle(), 0) != WAIT_OBJECT_0) return failed();
        testWorker.join();
    }
    uint64 generation = workerGeneration.load();
    if (generation >= ~uint64{0} - 1 || !workerGeneration.compare_exchange_strong(generation, generation + 1)) return failed();
    const uint64 ticket = generation + 1;
    workerStop.store(false);
    WhisperBinding binding{};
    const bool abc = mode == WhisperProbeMode::toneAbc;
    binding.count = mode == WhisperProbeMode::proof || abc ? 3 : 2;
    binding.mode = mode;
    if (!expectedTestServer(binding.server.ip)) return failed();
    const char* identities[] = {"Default", "RICON_B", "RICON_C"};
    const char* capture[] = {"RICON_PCM"};
    char sourceIdentity[256]{};
    if (!whisperProfiles(PLUGIN_GUI_IDENTITY, identities, binding.count, &sourceIdentity, abc)
        || !whisperProfiles(PLUGIN_GUI_SOUND_CAPTURE, capture, 1)) {
        record("whisper-profiles", 0, 1, "requires named isolated identities and RICON_PCM capture profile");
        return failed();
    }
    identities[0] = sourceIdentity;
    uint64* oldHandlers{};
    if (api.getServerConnectionHandlerList(&oldHandlers) || !oldHandlers) return failed();
    bool ready = true;
    for (size_t i = 0; oldHandlers[i]; ++i) {
        int status{};
        if (api.getConnectionStatus(oldHandlers[i], &status) || status != STATUS_DISCONNECTED) { ready = false; break; }
    }
    char address[48]{};
    std::snprintf(address, sizeof(address), "%s:19987", binding.server.ip);
    const char* tabs[] = {"RICON whisper A", "RICON whisper B", "RICON whisper C"};
    const char* nicks[] = {"RICON-W-A", "RICON-W-B", "RICON-W-C"};
    for (size_t i = 0; ready && i < binding.count; ++i) {
        if (networkCancelled(ticket)) { ready = false; break; }
        uint64 handler{};
        const unsigned int result = api.guiConnect(PLUGIN_CONNECT_TAB_NEW, tabs[i], address, "", nicks[i], "", "", "RICON_PCM", "", "", "", identities[i], "", "", &handler);
        record("whisper-connect", handler, result, tabs[i]);
        bool fresh = !result && handler;
        for (size_t old = 0; fresh && oldHandlers[old]; ++old) if (handler == oldHandlers[old]) fresh = false;
        for (size_t peer = 0; fresh && peer < i; ++peer) if (handler == binding.peers[peer].handler) fresh = false;
        if (!fresh) { ready = false; break; }
        binding.peers[i].handler = handler;
    }
    api.freeMemory(oldHandlers);
    if (ready && !networkCancelled(ticket)) {
        try { testWorker = std::thread(runWhisperProbe, binding, ticket); return true; }
        catch (const std::exception&) {}
    }
    whisperClose(binding, false);
    return failed();
}
