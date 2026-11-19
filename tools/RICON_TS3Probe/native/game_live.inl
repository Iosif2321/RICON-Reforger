// Public Enforce FileIO metadata bridge. Audio remains in the supported TS3 capture/codec/render path.
static std::atomic<uint64> liveSource{0}, liveListener{0}, liveDeadline{0};
static std::atomic<anyID> livePeer{0};
static std::atomic<uint64> liveCaptured{0}, liveNonzero{0}, liveBlocked{0}, liveSending{0};
static bool liveCleanupPending() { return liveSource.load() != 0; }

struct PcmLevel {
    uint64 values{};
    double squares{};
    unsigned int peak{};
    double rms() const { return values ? std::sqrt(squares / static_cast<double>(values)) : 0.0; }
};
static std::mutex liveLevelOperations;
static PcmLevel liveLevels[3]{};
static std::atomic<uint64> liveLevelDrops{0};
static LivePairedLevels livePaired{};
static std::atomic<uint64> livePairedDrops{0};

static void observeLivePaired(const LivePairedLevels& block) {
    if (!block.values) return;
    std::unique_lock<std::mutex> lock(liveLevelOperations, std::try_to_lock);
    if (!lock.owns_lock()) { livePairedDrops.fetch_add(1); return; }
    livePaired.blocks += block.blocks; livePaired.values += block.values; livePaired.clipped += block.clipped;
    if (!livePaired.firstQpc || block.firstQpc < livePaired.firstQpc) livePaired.firstQpc = block.firstQpc;
    if (block.lastQpc > livePaired.lastQpc) livePaired.lastQpc = block.lastQpc;
    livePaired.decodedSquares += block.decodedSquares; livePaired.postCSquares += block.postCSquares; livePaired.finalSquares += block.finalSquares;
    livePaired.noise.values += block.noise.values;
    livePaired.noise.sourceSquares += block.noise.sourceSquares; livePaired.noise.rawRfSquares += block.noise.rawRfSquares;
    livePaired.noise.bandRfSquares += block.noise.bandRfSquares; livePaired.noise.sourceRfCross += block.noise.sourceRfCross;
    livePaired.noise.mixedSquares += block.noise.mixedSquares; livePaired.noise.outputSquares += block.noise.outputSquares;
    livePaired.noise.residualSquares += block.noise.residualSquares;
    if (block.decodedPeak > livePaired.decodedPeak) livePaired.decodedPeak = block.decodedPeak;
    if (block.postCPeak > livePaired.postCPeak) livePaired.postCPeak = block.postCPeak;
    if (block.finalPeak > livePaired.finalPeak) livePaired.finalPeak = block.finalPeak;
}

static LivePairedLevels takeLivePaired() {
    std::lock_guard<std::mutex> lock(liveLevelOperations);
    const LivePairedLevels snapshot = livePaired; livePaired = {}; return snapshot;
}

static PcmLevel measurePcm(const short* samples, int count) {
    PcmLevel level{};
    if (!samples || count < 1 || count > 11520) return level;
    level.values = static_cast<uint64>(count);
    for (int i = 0; i < count; ++i) {
        const int value = samples[i];
        const unsigned int magnitude = static_cast<unsigned int>(value < 0 ? -value : value);
        if (magnitude > level.peak) level.peak = magnitude;
        level.squares += static_cast<double>(value) * value;
    }
    return level;
}

static void observeLivePcm(int stage, const short* samples, int count) {
    if (stage < 0 || stage > 2) return;
    const PcmLevel block = measurePcm(samples, count);
    if (!block.values) return;
    std::unique_lock<std::mutex> lock(liveLevelOperations, std::try_to_lock);
    if (!lock.owns_lock()) { liveLevelDrops.fetch_add(1); return; }
    PcmLevel& level = liveLevels[stage];
    level.values += block.values;
    level.squares += block.squares;
    if (block.peak > level.peak) level.peak = block.peak;
}

static uint64 takeLiveLevels(PcmLevel (&snapshot)[3]) {
    std::lock_guard<std::mutex> lock(liveLevelOperations);
    for (int stage = 0; stage < 3; ++stage) { snapshot[stage] = liveLevels[stage]; liveLevels[stage] = {}; }
    return liveLevelDrops.exchange(0);
}

static void recordLiveLevels(uint64 connection) {
    PcmLevel level[3]{};
    const uint64 dropped = takeLiveLevels(level);
    char detail[768]{};
    std::snprintf(detail, sizeof(detail), "units=PCM16 captureValues=%llu captureRms=%.3f capturePeak=%u decodedValues=%llu decodedRms=%.3f decodedPeak=%u outputValues=%llu outputRms=%.3f outputPeak=%u droppedBlocks=%llu",
                  level[0].values, level[0].rms(), level[0].peak, level[1].values, level[1].rms(), level[1].peak,
                  level[2].values, level[2].rms(), level[2].peak, dropped);
    record("live-levels", connection, 0, detail);
    const LivePairedLevels paired = takeLivePaired();
    const double n = paired.values ? static_cast<double>(paired.values) : 1.0;
    std::snprintf(detail, sizeof(detail), "units=PCM16 blocks=%llu values=%llu decodedRms=%.3f decodedPeak=%u postCRms=%.3f postCPeak=%u finalRms=%.3f finalPeak=%u clipped=%llu droppedBlocks=%llu presentation=callback-return gain=1",
                  paired.blocks, paired.values, std::sqrt(paired.decodedSquares / n), paired.decodedPeak,
                  std::sqrt(paired.postCSquares / n), paired.postCPeak, std::sqrt(paired.finalSquares / n), paired.finalPeak,
                  paired.clipped, livePairedDrops.exchange(0));
    record("live-paired-levels", connection, 0, detail);
    const RiconLiveNoiseMetrics& noise = paired.noise;
    const double count = noise.values ? static_cast<double>(noise.values) : 1.0;
    const double closure = noise.mixedSquares - noise.sourceSquares - noise.bandRfSquares - noise.sourceRfCross;
    std::snprintf(detail, sizeof(detail), "units=PCM16 values=%llu firstQpc=%llu lastQpc=%llu sourcePathRms=%.6f rawRfRms=%.6f bandRfRms=%.6f crossMean=%.9f preclipMixRms=%.6f outputRms=%.6f energyResidualMean=%.12g pcmResidualRms=%.6f source=filtered-mic-codec-path residual=quantization-rounding-clipping rfAudioLP=4000",
                  noise.values, paired.firstQpc, paired.lastQpc, std::sqrt(noise.sourceSquares / count), std::sqrt(noise.rawRfSquares / count),
                  std::sqrt(noise.bandRfSquares / count), noise.sourceRfCross / count, std::sqrt(noise.mixedSquares / count),
                  std::sqrt(noise.outputSquares / count), closure / count, std::sqrt(noise.residualSquares / count));
    record("live-noise-components", connection, 0, detail);
}

static const char* liveTerminalAction(bool mediaObserved, bool controlHealthy, bool cleanup) {
    if (!controlHealthy || !cleanup) return "live-failed";
    return mediaObserved ? "live-complete" : "live-stopped";
}

struct GameReply { unsigned int sequence{}, session{}, player{}, active{}; };

static bool setLiveRadioProfile(uint64 generation) {
    std::lock_guard<std::mutex> lock(voiceOperations);
    if (voiceGeneration != generation || !voiceClient) return false;
    // shortcut: fixed good-link profile; game RF/per-RX parameters are a separate integration step.
    voiceParams.mean_snr = 30.0f;
    voiceParams.m = 20.0f;
    return true;
}

static bool setLiveDeviceAudioProfile(uint64 generation) {
    std::lock_guard<std::mutex> lock(voiceOperations);
    if (voiceGeneration != generation || !voiceClient || voiceConnection != liveListener.load()) return false;
    voiceLive150 = true;
    return true;
}

static void recordLivePlaybackState(uint64 listener, anyID peer) {
    float modifier{};
    const unsigned int globalResult = api.getPlaybackConfigValueAsFloat
        ? api.getPlaybackConfigValueAsFloat(listener, "volume_modifier", &modifier) : ERROR_not_implemented;
    int clientModifier{};
    const unsigned int clientResult = api.getClientVariableAsInt
        ? api.getClientVariableAsInt(listener, peer, CLIENT_VOLUME_MODIFICATOR, &clientModifier) : ERROR_not_implemented;
    char detail[224]{};
    std::snprintf(detail, sizeof(detail), "voiceVolumeDb=%.3f voiceQuery=%u clientVolumeModifierInt=%d clientQuery=%u sourceClient=%u mutations=0",
                  static_cast<double>(modifier), globalResult, clientModifier, clientResult, static_cast<unsigned int>(peer));
    record("live-playback-readback", listener, globalResult ? globalResult : clientResult, detail);
}

static bool parseGameReply(const char* row, size_t length, const char* nonce, GameReply& reply) {
    if (!row || !length || length >= 192 || std::memchr(row, 0, length)) return false;
    char copy[192]{}; std::memcpy(copy, row, length);
    if (length && copy[length - 1] == '\n') copy[--length] = 0;
    if (length && copy[length - 1] == '\r') copy[--length] = 0;
    char echoed[33]{}, tail[4]{};
    unsigned int seq{}, session{}, player{}, active{};
    int consumed{};
    if (sscanf_s(copy, "RICON_GAME_1 %32s %u %u %u %u %3s%n", echoed, static_cast<unsigned int>(sizeof(echoed)),
                 &seq, &session, &player, &active, tail, static_cast<unsigned int>(sizeof(tail)), &consumed) != 6
        || std::strcmp(echoed, nonce) || !seq || seq > INT_MAX || !session || session > INT_MAX
        || player > INT_MAX || active > 1 || (active && !player) || (!active && player)
        || std::strcmp(tail, "END") || consumed != static_cast<int>(length)) return false;
    char canonical[192]{};
    std::snprintf(canonical, sizeof(canonical), "RICON_GAME_1 %s %u %u %u %u END", nonce, seq, session, player, active);
    if (std::strcmp(copy, canonical)) return false;
    reply = {seq, session, player, active};
    return true;
}

static bool plainRead(const char* path, char* bytes, size_t capacity, size_t& length) {
    length = 0;
    const DWORD attributes = GetFileAttributesA(path);
    if (attributes == INVALID_FILE_ATTRIBUTES || attributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)) return false;
    HANDLE file = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, 0, nullptr);
    if (file == INVALID_HANDLE_VALUE) return false;
    LARGE_INTEGER before{}, after{}; DWORD read{};
    const bool ok = GetFileSizeEx(file, &before) && before.QuadPart > 0 && before.QuadPart < static_cast<LONGLONG>(capacity)
        && ReadFile(file, bytes, static_cast<DWORD>(before.QuadPart), &read, nullptr)
        && GetFileSizeEx(file, &after) && before.QuadPart == after.QuadPart && read == before.QuadPart;
    CloseHandle(file);
    if (ok) { length = read; bytes[length] = 0; }
    return ok;
}

static bool resolveGameProfile(const char* value, size_t length, char (&profile)[4096]) {
    if (length < 4 || value[1] != ':' || (value[2] != '\\' && value[2] != '/') || std::memchr(value, 0, length)
        || std::strchr(value, '\n') || std::strchr(value, '\r')) return false;
    const DWORD size = GetFullPathNameA(value, static_cast<DWORD>(sizeof(profile)), profile, nullptr);
    const DWORD attributes = GetFileAttributesA(profile);
    return size && size < sizeof(profile) && attributes != INVALID_FILE_ATTRIBUTES
        && (attributes & FILE_ATTRIBUTE_DIRECTORY) && !(attributes & FILE_ATTRIBUTE_REPARSE_POINT);
}

static bool gameProfile(char (&profile)[4096]) {
    char config[4096]{}, value[4096]{}; size_t length{};
    if (!join(config, sizeof(config), configDirectory, "ricon-game-profile.txt") || !plainRead(config, value, sizeof(value), length)) return false;
    if (length && value[length - 1] == '\n') value[--length] = 0;
    if (length && value[length - 1] == '\r') value[--length] = 0;
    return resolveGameProfile(value, length, profile);
}

static bool writeProfileRequest(const char* profile, const char* requestLeaf, const char* temporaryLeaf,
                                const char* row, size_t length, uint64 ticket) {
    char path[4096]{}, temporary[4096]{};
    if (!row || !length || length >= 96 || networkCancelled(ticket) || !join(path, sizeof(path), profile, requestLeaf)
        || !join(temporary, sizeof(temporary), profile, temporaryLeaf)) return false;
    const DWORD attributes = GetFileAttributesA(path), tempAttributes = GetFileAttributesA(temporary);
    if ((attributes != INVALID_FILE_ATTRIBUTES && attributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT))
        || (tempAttributes != INVALID_FILE_ATTRIBUTES && tempAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT))) return false;
    HANDLE file = CreateFileA(temporary, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) { record("game-request-open-failed", 0, GetLastError(), "temporary request could not be opened"); return false; }
    DWORD written{};
    const BOOL complete = WriteFile(file, row, static_cast<DWORD>(length), &written, nullptr);
    const DWORD writeError = !complete ? GetLastError() : written == static_cast<DWORD>(length) ? ERROR_SUCCESS : ERROR_WRITE_FAULT;
    CloseHandle(file);
    if (writeError) { record("game-request-write-failed", 0, writeError, "incomplete temporary request"); return false; }
    DWORD error{};
    for (unsigned int attempt = 0; attempt < 20 && !networkCancelled(ticket); ++attempt) {
        if (MoveFileExA(temporary, path, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) return true;
        error = GetLastError();
        if (error != ERROR_SHARING_VIOLATION && error != ERROR_ACCESS_DENIED) break;
        if (!attempt) record("game-request-retry", 0, error, "bounded retry on replace sharing/access conflict");
        Sleep(5);
    }
    record(networkCancelled(ticket) ? "game-request-cancelled" : "game-request-publish-failed", 0, error, "not published; lease is not renewed");
    return false;
}

static bool writeGameRequest(const char* profile, const char* nonce, unsigned int sequence, uint64 ticket, bool pttStatus = false) {
    char row[96]{};
    const int length = std::snprintf(row, sizeof(row), "%s %s %u END\n",
                                     pttStatus ? "RICON_PTT_QUERY_1" : "RICON_TS3_1", nonce, sequence);
    if (length <= 0 || static_cast<size_t>(length) >= sizeof(row)) return false;
    return writeProfileRequest(profile, pttStatus ? "ricon-ptt-request.txt" : "ricon-ts3-request.txt",
                               pttStatus ? "ricon-ptt-request.tmp" : "ricon-ts3-request.tmp", row, static_cast<size_t>(length), ticket);
}

static bool makeGameNonce(char (&nonce)[33]) {
    for (int i = 0; i < 4; ++i) {
        unsigned int random{};
        if (rand_s(&random)) return false;
        std::snprintf(nonce + i * 8, 9, "%08x", random);
    }
    return true;
}

struct PttStatusReply {
    unsigned int session{};
    bool active{};
    char domain[37]{};
    unsigned int epoch{}, player{}, connection{}, generation{}, entity{}, policyRevision{};
    unsigned int device{}, index{}, network{}, channel{}, rxGeneration{}, hold{}, request{}, lease{}, remainingMs{};
};
static constexpr uint64 pttStatusMaxAgeMs = 1500;
static constexpr size_t pttStatusRowCapacity = 320;

static bool parsePttStatusReply(std::string_view row, std::string_view nonce, unsigned int sequence, PttStatusReply& reply) {
    if (row.empty() || row.size() >= pttStatusRowCapacity || nonce.size() != 32 || row.find('\0') != row.npos) return false;
    const auto lowerHex = [](char value) { return (value >= '0' && value <= '9') || (value >= 'a' && value <= 'f'); };
    for (char value : nonce) if (!lowerHex(value)) return false;
    if (row.back() == '\n') {
        row.remove_suffix(1);
        if (!row.empty() && row.back() == '\r') row.remove_suffix(1);
    }
    const auto take = [&]() {
        const auto separator = row.find(' ');
        const auto token = row.substr(0, separator);
        row.remove_prefix(separator == row.npos ? row.size() : separator + 1);
        return token;
    };
    const auto readInt = [&](unsigned int& result, bool allowZero = false) {
        const auto token = take();
        if (token.empty() || (token.size() > 1 && token.front() == '0')) return false;
        result = 0;
        for (char value : token) {
            if (value < '0' || value > '9') return false;
            const auto digit = static_cast<unsigned int>(value - '0');
            if (result > (static_cast<unsigned int>(INT_MAX) - digit) / 10) return false;
            result = result * 10 + digit;
        }
        return result || allowZero;
    };
    PttStatusReply parsed{};
    unsigned int echoedSequence{}, active{};
    if (take() != "RICON_PTT_1" || take() != nonce || !readInt(echoedSequence) || echoedSequence != sequence
        || !readInt(parsed.session) || !readInt(active, true) || active > 1) return false;
    parsed.active = active == 1;
    if (parsed.active) {
        const auto domain = take();
        if (domain.size() != 36 || domain[14] != '4' || std::string_view("89ab").find(domain[19]) == row.npos) return false;
        for (size_t position = 0; position < domain.size(); ++position) {
            const bool hyphen = position == 8 || position == 13 || position == 18 || position == 23;
            if (hyphen ? domain[position] != '-' : !lowerHex(domain[position])) return false;
        }
        std::memcpy(parsed.domain, domain.data(), domain.size());
        if (!readInt(parsed.epoch) || !readInt(parsed.player) || !readInt(parsed.connection) || !readInt(parsed.generation)
            || !readInt(parsed.entity) || !readInt(parsed.policyRevision) || !readInt(parsed.device) || !readInt(parsed.index, true)
            || !readInt(parsed.network) || !readInt(parsed.channel) || !readInt(parsed.rxGeneration) || !readInt(parsed.hold)
            || !readInt(parsed.request) || !readInt(parsed.lease) || !readInt(parsed.remainingMs) || parsed.remainingMs > 60000) return false;
    }
    if (row != "END") return false;
    reply = parsed;
    return true;
}

static bool pttStatusReplyCurrent(const PttStatusReply& reply, uint64 issued, uint64 now) {
    return now >= issued && now - issued < pttStatusMaxAgeMs && (!reply.active || now - issued < reply.remainingMs);
}

static const char* pollPttStatus(const char* profile, uint64 ticket, PttStatusReply& outcome) {
    outcome = {};
    char response[4096]{}, nonce[33]{};
    if (!join(response, sizeof(response), profile, "ricon-ptt-response.txt") || !makeGameNonce(nonce)) return "SETUP_FAILED";
    const unsigned int sequence = 1;
    const uint64 issued = GetTickCount64();
    if (!writeGameRequest(profile, nonce, sequence, ticket, true))
        return networkCancelled(ticket) ? "CANCELLED" : "PUBLISH_FAILED";
    while (!networkCancelled(ticket) && GetTickCount64() - issued < pttStatusMaxAgeMs) {
        char row[pttStatusRowCapacity]{};
        size_t length{};
        PttStatusReply candidate{};
        if (plainRead(response, row, sizeof(row), length) && parsePttStatusReply({row, length}, nonce, sequence, candidate)
            && pttStatusReplyCurrent(candidate, issued, GetTickCount64()) && !networkCancelled(ticket)) {
            outcome = candidate;
            return candidate.active ? "ACTIVE" : "INACTIVE";
        }
        Sleep(20);
    }
    return networkCancelled(ticket) ? "CANCELLED" : "NO_FRESH_REPLY";
}

static void runPttStatus(uint64 connection, uint64 ticket) {
    char profile[4096]{}, detail[128]{};
    PttStatusReply reply{};
    const char* status = gameProfile(profile) ? pollPttStatus(profile, ticket, reply) : "PROFILE_UNAVAILABLE";
    std::snprintf(detail, sizeof(detail), "outcome=%s gameSession=%u reportedRemainingMs=%u", status, reply.session, reply.remainingMs);
    record("ptt-status", connection, reply.session ? 0 : 1, detail);
    networkRunning.store(false);
}

static bool beginPttStatus(uint64 connection) {
    const uint64 ticket = workerGeneration.load();
    if (ticket == ~uint64{0} || liveCleanupPending() || networkRunning.exchange(true)) return false;
    if (testWorker.joinable()) {
        if (WaitForSingleObject(testWorker.native_handle(), 0) != WAIT_OBJECT_0) {
            networkRunning.store(false);
            return false;
        }
        testWorker.join();
    }
    workerStop.store(false);
    try { testWorker = std::thread(runPttStatus, connection, ticket); }
    catch (const std::exception&) { networkRunning.store(false); return false; }
    return true;
}

static bool liveAllowed(uint64 now) {
    const uint64 deadline = liveDeadline.load();
    return deadline && now < deadline && !workerStop.load();
}

static bool gameReplyCurrent(const GameReply& reply, unsigned int sequence, uint64 issued, uint64 now) {
    return reply.sequence == sequence && now >= issued && now - issued < 1500;
}

static void editLiveCapture(uint64 connection, short* samples, int frames, int channels, int* edited, uint64 now) {
    if (connection != liveSource.load() || !liveSource.load() || !edited) return;
    if (!samples || frames <= 0 || frames > 5760 || channels < 1 || channels > 2) { *edited &= ~2; return; }
    if (!liveAllowed(now)) {
        std::memset(samples, 0, static_cast<size_t>(frames) * channels * sizeof(short));
        *edited = (*edited | 1) & ~2;
        liveBlocked.fetch_add(1);
        return;
    }
    liveCaptured.fetch_add(static_cast<uint64>(frames) * channels);
    if (*edited & 2) liveSending.fetch_add(static_cast<uint64>(frames) * channels);
    uint64 nonzero{};
    for (int i = 0; i < frames * channels; ++i) if (samples[i]) ++nonzero;
    liveNonzero.fetch_add(nonzero);
    observeLivePcm(0, samples, frames * channels);
}

static unsigned int openLiveCapture(uint64 connection, uint64 ticket) {
    if (!api.getDefaultCaptureMode || !api.getCaptureModeList || !api.getDefaultCaptureDevice || !api.openCaptureDevice)
        return ERROR_sound_no_capture_device_available;
    char* mode{}; char chosen[256]{};
    const unsigned int queried = api.getDefaultCaptureMode(&mode);
    record("live-capture-default-mode", connection, queried, "SDK default hardware backend query");
    if (!queried && mode && std::strcmp(mode, "custom")) strncpy_s(chosen, mode, _TRUNCATE);
    if (!queried && mode) api.freeMemory(mode);
    if (queried) return queried;
    if (!chosen[0]) {
        char** modes{};
        const unsigned int listed = api.getCaptureModeList(&modes);
        if (listed) return listed;
        if (modes) {
            for (size_t i = 0; modes[i]; ++i) {
                if (!chosen[0] && std::strcmp(modes[i], "custom")) strncpy_s(chosen, modes[i], _TRUNCATE);
                api.freeMemory(modes[i]);
            }
            api.freeMemory(modes);
        }
    }
    if (!chosen[0]) return ERROR_sound_no_capture_device_available;
    char** device{};
    const unsigned int queriedDevice = api.getDefaultCaptureDevice(chosen, &device);
    record("live-capture-default-device", connection, queriedDevice, chosen);
    if (queriedDevice) return queriedDevice;
    unsigned int result = ERROR_sound_no_capture_device_available;
    if (device && device[0] && device[1] && device[1][0] && !networkCancelled(ticket) && liveAllowed(GetTickCount64()))
        result = api.openCaptureDevice(connection, chosen, device[1]);
    if (device) {
        if (device[0]) api.freeMemory(device[0]);
        if (device[1]) api.freeMemory(device[1]);
        api.freeMemory(device);
    }
    record("live-capture-open", connection, result, chosen);
    return result;
}

static bool replaceLiveCapture(uint64 source, uint64 ticket) {
    if (networkCancelled(ticket) || !liveAllowed(GetTickCount64())) return false;
    const unsigned int closed = api.closeCaptureDevice(source);
    record("live-capture-close-virtual", source, closed, "owned source before hardware replacement");
    if (closed) return false;
    if (networkCancelled(ticket) || !liveAllowed(GetTickCount64())) return false;
    return openLiveCapture(source, ticket) == ERROR_ok;
}

static void recordLiveCaptureState(uint64 source) {
    char* mode{}; char* device{}; int isDefault{};
    const unsigned int modeResult = api.getCurrentCaptureMode(source, &mode);
    const unsigned int deviceResult = api.getCurrentCaptureDeviceName(source, &device, &isDefault);
    char detail[384]{};
    std::snprintf(detail, sizeof(detail), "mode=%.80s device=%.192s default=%d modeQuery=%u deviceQuery=%u",
                  !modeResult && mode ? mode : "UNKNOWN", !deviceResult && device ? device : "UNKNOWN", isDefault, modeResult, deviceResult);
    for (char& ch : detail) if (ch > 0 && ch < 32) ch = ' ';
    record("live-capture-readback", source, modeResult ? modeResult : deviceResult, detail);
    if (!modeResult && mode) api.freeMemory(mode);
    if (!deviceResult && device) api.freeMemory(device);
    for (size_t flag : {static_cast<size_t>(CLIENT_INPUT_DEACTIVATED), static_cast<size_t>(CLIENT_INPUT_MUTED)}) {
        int value{};
        const unsigned int result = api.getClientSelfVariableAsInt(source, flag, &value);
        std::snprintf(detail, sizeof(detail), "flag=%zu value=%d", flag, value);
        record("live-input-readback", source, result, detail);
    }
    for (const char* key : {"vad", "agc", "denoise", "echo_canceling"}) {
        char* value{};
        const unsigned int result = api.getPreProcessorConfigValue(source, key, &value);
        const char* state = !result && value && (!std::strcmp(value, "true") || !std::strcmp(value, "false")) ? value : "UNKNOWN";
        std::snprintf(detail, sizeof(detail), "%s=%s", key, state);
        record("live-preprocessor-readback", source, result, detail);
        if (!result && value) api.freeMemory(value);
    }
}

enum class LiveBootstrapResult { waiting, ready, cancelled, expired, changed };
struct LiveCaptureReadiness {
    bool modeQueried{}, deviceQueried{}, known{};
    unsigned int modeResult{~0u}, deviceResult{~0u};
    int isDefault{};
    char mode[96]{}, device[224]{};
};

static LiveBootstrapResult liveBootstrapLimit(uint64 ticket, uint64 deadline) {
    if (networkCancelled(ticket)) return LiveBootstrapResult::cancelled;
    return GetTickCount64() >= deadline ? LiveBootstrapResult::expired : LiveBootstrapResult::waiting;
}

static void readLiveBootstrapCapture(uint64 connection, uint64 ticket, uint64 deadline, LiveCaptureReadiness& state) {
    state = {};
    if (liveBootstrapLimit(ticket, deadline) != LiveBootstrapResult::waiting) return;
    char* mode{};
    state.modeQueried = true;
    state.modeResult = api.getCurrentCaptureMode(connection, &mode);
    const bool custom = !state.modeResult && mode && !std::strcmp(mode, "custom");
    if (mode) { if (!state.modeResult) strncpy_s(state.mode, mode, _TRUNCATE); api.freeMemory(mode); }
    if (liveBootstrapLimit(ticket, deadline) != LiveBootstrapResult::waiting) return;
    char* name{};
    state.deviceQueried = true;
    state.deviceResult = api.getCurrentCaptureDeviceName(connection, &name, &state.isDefault);
    state.known = custom && !state.deviceResult && name
        && (!std::strcmp(name, knownCaptureId) || !std::strcmp(name, "RICON Known PCM (TOOL_ONLY)"));
    if (name) { if (!state.deviceResult) strncpy_s(state.device, name, _TRUNCATE); api.freeMemory(name); }
}

static void recordLiveBootstrapCapture(uint64 connection, const char* actor, const LiveCaptureReadiness& state) {
    char detail[512]{};
    std::snprintf(detail, sizeof(detail), "actor=%s modeQueried=%u modeQuery=%u mode=%.95s deviceQueried=%u deviceQuery=%u device=%.223s default=%d known=%u",
                  actor, static_cast<unsigned int>(state.modeQueried), state.modeResult, state.mode,
                  static_cast<unsigned int>(state.deviceQueried), state.deviceResult, state.device, state.isDefault, static_cast<unsigned int>(state.known));
    for (char& ch : detail) if (ch > 0 && ch < 32) ch = ' ';
    record("live-bootstrap-capture", connection, 0, detail);
}

//! One original deadline covers connect and virtual capture readiness; no hardware or game request here.
static LiveBootstrapResult pollLiveBootstrap(NetworkBinding& binding, bool& bound, uint64 ticket, uint64 deadline,
                                             LiveCaptureReadiness (&captures)[2]) {
    captures[0] = {}; captures[1] = {};
    LiveBootstrapResult limit = liveBootstrapLimit(ticket, deadline);
    if (limit != LiveBootstrapResult::waiting) return limit;
    anyID source{}, listener{};
    const bool a = testServerConnection(binding.source, source);
    limit = liveBootstrapLimit(ticket, deadline); if (limit != LiveBootstrapResult::waiting) return limit;
    const bool b = testServerConnection(binding.listener, listener);
    limit = liveBootstrapLimit(ticket, deadline); if (limit != LiveBootstrapResult::waiting) return limit;
    if ((binding.sourceClient && (!a || source != binding.sourceClient))
        || (binding.listenerClient && (!b || listener != binding.listenerClient))) return LiveBootstrapResult::changed;
    if (a) binding.sourceClient = source;
    if (b) binding.listenerClient = listener;
    if (a && b && source == listener) return LiveBootstrapResult::changed;
    if (bound) {
        const bool current = networkBindingCurrent(binding);
        limit = liveBootstrapLimit(ticket, deadline); if (limit != LiveBootstrapResult::waiting) return limit;
        if (!current) return LiveBootstrapResult::changed;
    }
    limit = liveBootstrapLimit(ticket, deadline); if (limit != LiveBootstrapResult::waiting) return limit;
    if (a && b && !bound) {
        char uid[128]{};
        if (serverUid(binding.source, uid)) {
            if (binding.serverUid[0] && std::strcmp(binding.serverUid, uid)) return LiveBootstrapResult::changed;
            strcpy_s(binding.serverUid, uid);
        }
        limit = liveBootstrapLimit(ticket, deadline); if (limit != LiveBootstrapResult::waiting) return limit;
        uint64 channel{};
        if (!api.getChannelOfClient(binding.source, source, &channel) && channel) {
            if (binding.channel && binding.channel != channel) return LiveBootstrapResult::changed;
            binding.channel = channel;
        }
        limit = liveBootstrapLimit(ticket, deadline); if (limit != LiveBootstrapResult::waiting) return limit;
        if (binding.serverUid[0] && binding.channel && networkBindingCurrent(binding)) {
            limit = liveBootstrapLimit(ticket, deadline); if (limit != LiveBootstrapResult::waiting) return limit;
            bound = true;
            networkChannel.store(binding.channel);
            networkSourceClient.store(source); networkListenerClient.store(listener); networkPrepared.store(true);
        }
    }
    limit = liveBootstrapLimit(ticket, deadline); if (limit != LiveBootstrapResult::waiting) return limit;
    readLiveBootstrapCapture(binding.source, ticket, deadline, captures[0]);
    limit = liveBootstrapLimit(ticket, deadline); if (limit != LiveBootstrapResult::waiting) return limit;
    readLiveBootstrapCapture(binding.listener, ticket, deadline, captures[1]);
    limit = liveBootstrapLimit(ticket, deadline); if (limit != LiveBootstrapResult::waiting) return limit;
    if (bound) {
        const bool current = networkBindingCurrent(binding);
        limit = liveBootstrapLimit(ticket, deadline); if (limit != LiveBootstrapResult::waiting) return limit;
        if (!current) return LiveBootstrapResult::changed;
    }
    limit = liveBootstrapLimit(ticket, deadline); if (limit != LiveBootstrapResult::waiting) return limit;
    return bound && captures[0].known && captures[1].known ? LiveBootstrapResult::ready : LiveBootstrapResult::waiting;
}

static void runLiveProbe(NetworkBinding binding, uint64 ticket) {
    const uint64 source = binding.source, listener = binding.listener;
    anyID sourceId{}, listenerId{}; bool bound{}, connected{}, opened{}, mediaObserved{}, setupReady{}, controlFailure{};
    unsigned int session{}, player{}, sequence = 1;
    const char* stage = "game-profile";
    const char* stopReason = "setup-failed";
    char profile[4096]{}, response[4096]{}, nonce[33]{};
    uint64 generation{}, accepted{}, issued{}, started = GetTickCount64();
    liveDeadline.store(0); liveCaptured.store(0); liveNonzero.store(0); liveBlocked.store(0); liveSending.store(0);
    // A wrong/pending bootstrap device must not transmit while the game lease is still absent.
    liveSource.store(source); liveListener.store(listener); livePeer.store(0);
    PcmLevel discarded[3]{};
    takeLiveLevels(discarded);
    takeLivePaired(); livePairedDrops.store(0);
    do {
        if (!gameProfile(profile) || !join(response, sizeof(response), profile, "ricon-ts3-response.txt")) break;
        stage = "random-challenge";
        if (!makeGameNonce(nonce)) break;
        stage = "connections";
        const uint64 bootstrapDeadline = GetTickCount64() + 15000;
        uint64 reported{};
        for (;;) {
            LiveCaptureReadiness captures[2]{};
            const LiveBootstrapResult result = pollLiveBootstrap(binding, bound, ticket, bootstrapDeadline, captures);
            sourceId = binding.sourceClient; listenerId = binding.listenerClient;
            const uint64 now = GetTickCount64();
            if (!reported || now - reported >= 1000 || result != LiveBootstrapResult::waiting) {
                recordLiveBootstrapCapture(source, "A", captures[0]);
                recordLiveBootstrapCapture(listener, "B", captures[1]);
                reported = now;
            }
            if (result == LiveBootstrapResult::ready) { connected = true; break; }
            if (result != LiveBootstrapResult::waiting) {
                stopReason = result == LiveBootstrapResult::cancelled ? "stopped"
                    : result == LiveBootstrapResult::expired ? "bootstrap-timeout" : "bootstrap-binding-changed";
                break;
            }
            Sleep(50);
        }
        if (!connected) break;
        stage = "initial-binding";
        if (networkCancelled(ticket) || !networkBindingCurrent(binding)) break;
        liveSource.store(source); liveListener.store(listener); livePeer.store(sourceId);
        stage = "input-deactivation";
        if (!stateUpdateOk(api.setClientSelfVariableAsInt(source, CLIENT_INPUT_DEACTIVATED, INPUT_DEACTIVATED))
            || !flushTestState(source, "live-source-waiting")
            || !stateUpdateOk(api.setClientSelfVariableAsInt(listener, CLIENT_INPUT_DEACTIVATED, INPUT_DEACTIVATED))
            || !flushTestState(listener, "live-listener-input-off")) break;
        {
            stage = "playback";
            std::lock_guard<std::mutex> lock(operations);
            if (!ensurePlayback(listener)) break;
        }
        issued = GetTickCount64();
        stage = "initial-request";
        if (!writeGameRequest(profile, nonce, sequence, ticket)) { stopReason = "request-publish-failed"; break; }
        setupReady = true;
        record("live-waiting-game", listener, 0, "fresh challenge required; hardware microphone closed");
        while (true) {
            stage = "binding";
            const bool cancelled = networkCancelled(ticket);
            const bool bindingCurrent = !cancelled && networkBindingCurrent(binding);
            if (cancelled || !bindingCurrent) {
                stopReason = cancelled ? "stopped" : "binding-changed";
                char detail[192]{};
                std::snprintf(detail, sizeof(detail), "cancelled=%u bindingCurrent=%u currentHandler=%llu listener=%llu workerGeneration=%llu ticket=%llu",
                              static_cast<unsigned int>(cancelled), static_cast<unsigned int>(bindingCurrent), currentConnection.load(), listener, workerGeneration.load(), ticket);
                record("live-binding-ended", listener, 0, detail);
                break;
            }
            const uint64 now = GetTickCount64();
            stage = "game-poll";
            if ((opened && now >= liveDeadline.load()) || (!opened && now - started >= 120000)) {
                stopReason = opened ? "game-lease-expired" : "game-wait-expired";
                record("live-game-expired", listener, 0, "capture revoked; no heartbeat or game stopped"); break;
            }
            if (!opened && now - issued >= 1500) {
                if (sequence == INT_MAX) { controlFailure = true; stopReason = "sequence-exhausted"; break; }
                issued = GetTickCount64();
                if (!writeGameRequest(profile, nonce, ++sequence, ticket)) {
                    controlFailure = !networkCancelled(ticket); stopReason = "request-publish-failed"; break;
                }
            }
            char row[192]{}; size_t size{}; GameReply reply{};
            if (plainRead(response, row, sizeof(row), size) && parseGameReply(row, size, nonce, reply) && gameReplyCurrent(reply, sequence, issued, GetTickCount64())) {
                if ((session && (session != reply.session || (reply.active && player != reply.player))) || (opened && !reply.active)) {
                    stopReason = reply.active ? "game-binding-changed" : "game-ended";
                    record("live-game-ended", listener, 0, "game/world/player binding changed"); break;
                }
                if (reply.active) {
                    if (!opened) {
                        session = reply.session; player = reply.player;
                        stage = "voice-arm";
                        clearVoiceProbe(&generation);
                        if (!armVoiceProbe(listener, sourceId, false, issued + 1500, generation, ticket) || !setLiveRadioProfile(generation)
                            || !setLiveDeviceAudioProfile(generation)) {
                            controlFailure = !networkCancelled(ticket); stopReason = "voice-arm-failed"; break;
                        }
                        liveDeadline.store(issued + 1500);
                        record("live-audio-profile", listener, 0, "TEST_ONLY_HP150_LP4000_NOISE_LP4000_V2 RF=unchanged rawNoise=unchanged audioNoiseBand=4000 presentationGain=1 sourceGain=1");
                        stage = "capture-replacement";
                        if (!replaceLiveCapture(source, ticket)) { controlFailure = !networkCancelled(ticket); stopReason = "capture-open-failed"; break; }
                        opened = true;
                        stage = "capture-activation";
                        if (networkCancelled(ticket) || !liveAllowed(GetTickCount64())) { stopReason = "capture-start-revoked"; break; }
                        if (!stateUpdateOk(api.activateCaptureDevice(source))
                            || networkCancelled(ticket) || !liveAllowed(GetTickCount64())
                            || !stateUpdateOk(api.setClientSelfVariableAsInt(source, CLIENT_INPUT_DEACTIVATED, INPUT_ACTIVE))
                            || !flushTestState(source, "live-source-active")) {
                            controlFailure = !networkCancelled(ticket); stopReason = "capture-activate-failed"; break;
                        }
                        recordLiveCaptureState(source);
                        recordLivePlaybackState(listener, sourceId);
                        if (networkCancelled(ticket) || !liveAllowed(GetTickCount64())) { stopReason = "capture-start-revoked"; break; }
                        char detail[128]{}; std::snprintf(detail, sizeof(detail), "session=%u player=%u sourceClient=%u listenerClient=%u leaseMs=1500", session, player, static_cast<unsigned int>(sourceId), static_cast<unsigned int>(listenerId));
                        record("live-microphone-active", source, 0, detail);
                    } else {
                        stage = "voice-renewal";
                        std::lock_guard<std::mutex> lock(voiceOperations);
                        if (voiceGeneration != generation || voiceClient != sourceId || voiceConnection != listener) { stopReason = "voice-revoked"; break; }
                        voiceDeadline = issued + 1500;
                        liveDeadline.store(issued + 1500);
                    }
                    ++accepted;
                    if (accepted == 1 || accepted % 50 == 0) {
                        char detail[128]{}; std::snprintf(detail, sizeof(detail), "sequence=%u session=%u player=%u accepted=%llu", sequence, session, player, accepted);
                        record("live-game-ack", listener, 0, detail);
                    }
                    if (accepted == 1 || accepted % 10 == 0) {
                        recordLiveLevels(listener);
                        recordLivePlaybackState(listener, sourceId);
                    }
                }
                issued = GetTickCount64();
                stage = "next-request";
                if (sequence == INT_MAX) { controlFailure = true; stopReason = "sequence-exhausted"; break; }
                if (!writeGameRequest(profile, nonce, ++sequence, ticket)) {
                    controlFailure = !networkCancelled(ticket); stopReason = "request-publish-failed"; break;
                }
            }
            Sleep(50);
        }
        liveDeadline.store(0);
        const RiconAudioMetrics dsp = networkMetrics(listener, "live");
        mediaObserved = opened && accepted > 1 && liveSending.load() && liveNonzero.load() && dsp.callbacks_dsp;
    } while (false);
    liveDeadline.store(0);
    recordLiveLevels(listener);
    char terminal[160]{};
    std::snprintf(terminal, sizeof(terminal), "stage=%s cancelled=%u currentHandler=%llu listener=%llu", stage,
                  static_cast<unsigned int>(networkCancelled(ticket)), currentConnection.load(), listener);
    record("live-terminal-stage", listener, 0, terminal);
    clearVoiceProbe(); networkPrepared.store(false);
    const bool sourceOwned = stillOwnGuiConnection(binding, source, sourceId, bound);
    const bool listenerOwned = stillOwnGuiConnection(binding, listener, listenerId, bound);
    bool cleanup = sourceOwned && listenerOwned;
    if (sourceOwned) {
        const unsigned int deactivated = api.setClientSelfVariableAsInt(source, CLIENT_INPUT_DEACTIVATED, INPUT_DEACTIVATED);
        record("live-input-off", source, deactivated, "owned source only");
        cleanup &= stateUpdateOk(deactivated) && flushTestState(source, "live-flush-input-off");
        cleanup &= closeTestConnection(source);
    }
    if (listenerOwned) cleanup &= closeTestConnection(listener);
    const bool controlHealthy = !controlFailure && (setupReady || networkCancelled(ticket));
    char metrics[384]{};
    std::snprintf(metrics, sizeof(metrics), "mediaObserved=%u controlHealthy=%u stopReason=%s opened=%u accepted=%llu capturedValues=%llu sentValues=%llu nonzeroValues=%llu blockedCallbacks=%llu cleanup=%u speech=UNQUALIFIED render=UNQUALIFIED",
                  static_cast<unsigned int>(mediaObserved), static_cast<unsigned int>(controlHealthy), stopReason, static_cast<unsigned int>(opened),
                  accepted, liveCaptured.load(), liveSending.load(), liveNonzero.load(), liveBlocked.load(), static_cast<unsigned int>(cleanup));
    record(liveTerminalAction(mediaObserved, controlHealthy, cleanup), listener, controlHealthy && cleanup ? 0 : 1, metrics);
    if (cleanup) { liveSource.store(0); liveListener.store(0); livePeer.store(0); }
    networkRunning.store(false);
}
