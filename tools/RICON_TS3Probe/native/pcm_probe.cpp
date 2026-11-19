// TOOL_ONLY RICON PCM output host. Uses the official TS3 API, not Coalition implementation code.
#define _CRT_RAND_S
#include <windows.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <atomic>
#include <mutex>
#include <thread>
#include <exception>
#include <climits>
#include <cmath>
#include <string_view>
#include "ts3_functions.h"
#include "teamspeak/public_errors.h"
#include "audio_chain.h"
#include "live_audio_profile.h"
#include "ricon_release.h"

#define EXPORT extern "C" __declspec(dllexport)
static TS3Functions api{};
static char pluginId[128]{};
static char configDirectory[4096]{}, dataDirectory[4096]{};
static std::mutex operations, logMutex;
static std::thread testWorker;
static std::atomic<bool> workerStop{false};
static std::atomic<uint64> workerGeneration{1};
static std::atomic<uint64> currentConnection{0};
static std::atomic<unsigned int> claimSequence{0};
static uint64 wave{}, waveConnection{};
static bool waveActive{}, playbackOwned{};
static uint64 playbackConnection{};
static const char* files[] = {"600-input.wav", "600-ricon.wav", "1200-input.wav", "1200-ricon.wav"};
static std::mutex voiceOperations;
static RiconAudioState voiceState{};
static RiconAudioBiquadState voiceNoiseLp{};
static RiconAudioChannelParams voiceParams{};
static uint64 voiceConnection{}, voiceDeadline{};
static uint64 voiceGeneration{1};
static anyID voiceClient{};
static const char* knownCaptureId = "ricon-known-pcm-input";
static bool knownCaptureRegistered{};
static bool voiceLive150{};
static bool ts3RouteEnabled();

static RiconAudioMetrics clearVoiceProbe(uint64* clearedGeneration = nullptr) {
    std::lock_guard<std::mutex> lock(voiceOperations);
    voiceClient = 0;
    voiceLive150 = false;
    voiceNoiseLp = {};
    voiceConnection = voiceDeadline = 0;
    if (voiceGeneration < ~uint64{0}) ++voiceGeneration;
    if (clearedGeneration) *clearedGeneration = voiceGeneration;
    return ricon_audio_metrics_get(&voiceState);
}

static bool armVoiceProbe(uint64 connection, anyID client, bool physicalLost, uint64 deadline, uint64 generation, uint64 expectedWorker = 0) {
    if (!connection || !client || !deadline) return false;
    std::lock_guard<std::mutex> lock(voiceOperations);
    if (generation != voiceGeneration || generation == ~uint64{0} || connection != currentConnection.load()) return false;
    if (expectedWorker && (workerStop.load() || workerGeneration.load() != expectedWorker)) return false;
    ricon_audio_state_init(&voiceState, 0x1234567u ^ client);
    voiceNoiseLp = {};
    voiceLive150 = false;
    // Arma3's state0 hard-mutes; physical LOST here stays an open noisy receiver.
    voiceParams = {1, physicalLost ? 1 : 2, physicalLost ? -20.0f : 4.0f, physicalLost ? 0.5f : 0.7f,
                   physicalLost ? 0.0f : -1.0f, physicalLost ? 1.0f : -1.0f};
    voiceConnection = connection;
    voiceDeadline = deadline;
    voiceClient = client;
    return true;
}

static bool editVoiceProbe(uint64 connection, anyID client, short* samples, int frames, int channels, uint64 now, LivePairedLevels* paired = nullptr) {
    if (!samples || frames <= 0 || frames > 5760 || channels < 1 || channels > 2) return false;
    std::unique_lock<std::mutex> lock(voiceOperations, std::try_to_lock);
    if (!lock.owns_lock() || !voiceClient || connection != voiceConnection || client != voiceClient
        || connection != currentConnection.load()) return false;
    if (now >= voiceDeadline) { voiceClient = 0; return false; }
    const uint64_t clippedBefore = voiceState.metrics.sample_values_clipped;
    if (paired) {
        *paired = {}; paired->blocks = 1; paired->values = static_cast<uint64_t>(frames) * channels;
        LARGE_INTEGER stamp{}; QueryPerformanceCounter(&stamp); paired->firstQpc = static_cast<uint64_t>(stamp.QuadPart);
        measurePairedStage(samples, frames * channels, paired->decodedSquares, paired->decodedPeak);
    }
    if (voiceLive150) ricon_live150_audio_process_playback_voice(&voiceState, &voiceParams, samples, frames, channels,
                                                               &voiceNoiseLp, paired ? &paired->noise : nullptr);
    else ricon_audio_process_playback_voice(&voiceState, &voiceParams, samples, frames, channels);
    if (paired) {
        measurePairedStage(samples, frames * channels, paired->postCSquares, paired->postCPeak);
        // No local gain: this presentation point is callback return, before any downstream TS3/system processing.
        paired->finalSquares = paired->postCSquares; paired->finalPeak = paired->postCPeak;
        paired->clipped = voiceState.metrics.sample_values_clipped - clippedBefore;
        LARGE_INTEGER stamp{}; QueryPerformanceCounter(&stamp); paired->lastQpc = static_cast<uint64_t>(stamp.QuadPart);
    }
    return true;
}

static bool join(char* out, size_t capacity, const char* directory, const char* leaf) {
    const size_t n = std::strlen(directory);
    if (!n) return false;
    const char* separator = directory[n - 1] == '/' || directory[n - 1] == '\\' ? "" : "/";
    const int written = std::snprintf(out, capacity, "%s%s%s", directory, separator, leaf);
    return written >= 0 && static_cast<size_t>(written) < capacity;
}

static void requestWorkerStop() {
    workerStop.store(true);
    uint64 generation = workerGeneration.load();
    while (generation != ~uint64{0} && !workerGeneration.compare_exchange_weak(generation, generation + 1)) {}
}

static void record(const char* action, uint64 connection, unsigned int result, const char* detail, uint64 recordedWave = 0) {
    std::lock_guard<std::mutex> lock(logMutex);
    char logPath[4096]{};
    if (!join(logPath, sizeof(logPath), configDirectory, "ricon-pcm-probe.log")) return;
    FILE* log{};
    if (fopen_s(&log, logPath, "ab") || !log) return;
    LARGE_INTEGER now{}, frequency{};
    QueryPerformanceCounter(&now);
    QueryPerformanceFrequency(&frequency);
    std::fprintf(log, "qpc=%lld frequency=%lld action=%s connection=%llu result=%u wave=%llu detail=%s\n",
                 now.QuadPart, frequency.QuadPart, action, connection, result, recordedWave, detail);
    std::fclose(log);
}

static bool voiceCommand(const char* command, anyID& client, bool& physicalLost) {
    if (!command) return false;
    const char* number{};
    if (!std::strncmp(command, "voice ", 6)) { physicalLost = false; number = command + 6; }
    else if (!std::strncmp(command, "noise ", 6)) { physicalLost = true; number = command + 6; }
    else return false;
    if (*number < '1' || *number > '9') return false;
    unsigned int value{};
    for (; *number; ++number) {
        if (*number < '0' || *number > '9') return false;
        value = value * 10 + static_cast<unsigned int>(*number - '0');
        if (value > 65535) return false;
    }
    client = static_cast<anyID>(value);
    return true;
}

static bool voiceTestProfileEnabled() {
    char enabled[4096]{};
    if (!std::strstr(configDirectory, "ts3-pcm-prototype-01") || !std::strstr(configDirectory, "ricon-probe")
        || !join(enabled, sizeof(enabled), configDirectory, "ricon-voice-test.enabled")) return false;
    const DWORD attributes = GetFileAttributesA(enabled);
    return attributes != INVALID_FILE_ATTRIBUTES && !(attributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT));
}

static bool parsePrivateTestIp(const char* bytes, size_t length, char (&ip)[16]) {
    if (!bytes) return false;
    if (length && bytes[length - 1] == '\n') --length;
    if (length && bytes[length - 1] == '\r') --length;
    if (!length || length > 15 || std::memchr(bytes, 0, length)) return false;
    unsigned int octets[4]{}, group{}, digits{};
    bool leadingZero{};
    for (size_t i = 0; i < length; ++i) {
        const char ch = bytes[i];
        if (ch == '.') {
            if (!digits || group == 3) return false;
            ++group; digits = 0; leadingZero = false;
        } else {
            if (ch < '0' || ch > '9' || digits == 3 || (digits && leadingZero)) return false;
            if (!digits) leadingZero = ch == '0';
            octets[group] = octets[group] * 10 + static_cast<unsigned int>(ch - '0');
            if (octets[group] > 255) return false;
            ++digits;
        }
    }
    if (group != 3 || !digits) return false;
    const bool local = (octets[0] == 127 && !octets[1] && !octets[2] && octets[3] == 1)
        || octets[0] == 10 || (octets[0] == 172 && octets[1] >= 16 && octets[1] <= 31)
        || (octets[0] == 192 && octets[1] == 168);
    if (!local) return false;
    std::memcpy(ip, bytes, length); ip[length] = 0;
    return true;
}

static bool expectedTestServer(char (&ip)[16]) {
    char path[4096]{};
    if (!join(path, sizeof(path), configDirectory, "ricon-voice-test-server.txt")) return false;
    const DWORD attributes = GetFileAttributesA(path);
    if (attributes == INVALID_FILE_ATTRIBUTES) {
        if (GetLastError() != ERROR_FILE_NOT_FOUND) return false;
        strcpy_s(ip, "127.0.0.1"); return true;
    }
    if (attributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)) return false;
    FILE* file{};
    if (fopen_s(&file, path, "rb") || !file) return false;
    char bytes[32]{};
    const size_t length = std::fread(bytes, 1, sizeof(bytes), file);
    std::fclose(file);
    return parsePrivateTestIp(bytes, length, ip);
}

static void registerKnownCapture() {
    if (knownCaptureRegistered || (!voiceTestProfileEnabled() && !ts3RouteEnabled()) || !api.registerCustomDevice || !api.unregisterCustomDevice) return;
    const unsigned int result = api.registerCustomDevice(knownCaptureId, "RICON Known PCM (TOOL_ONLY)", 48000, 1, 48000, 2);
    knownCaptureRegistered = result == ERROR_ok;
    record("known-capture-register", 0, result, "virtual device only; hardware capture not opened");
}

static void unregisterKnownCapture() {
    if (!knownCaptureRegistered) return;
    const unsigned int result = api.unregisterCustomDevice(knownCaptureId);
    record("known-capture-unregister", 0, result, "only owned virtual device");
    if (!result) knownCaptureRegistered = false;
}

static bool connectionAt(uint64 connection, const char* expected, anyID& own) {
    if (!api.getConnectionStatus || !api.getClientID || !api.getConnectionVariableAsString
        || !api.getConnectionVariableAsUInt64 || !api.freeMemory) return false;
    int status{};
    if (api.getConnectionStatus(connection, &status) || status != STATUS_CONNECTION_ESTABLISHED
        || api.getClientID(connection, &own) || !own) return false;
    char* ip{};
    const unsigned int ipResult = api.getConnectionVariableAsString(connection, own, CONNECTION_SERVER_IP, &ip);
    const bool matches = !ipResult && ip && !std::strcmp(ip, expected);
    if (ip) api.freeMemory(ip);
    uint64 port{};
    return matches && !api.getConnectionVariableAsUInt64(connection, own, CONNECTION_SERVER_PORT, &port) && port == 19987;
}

static bool testServerConnection(uint64 connection, anyID& own) {
    char expected[16]{};
    return voiceTestProfileEnabled() && expectedTestServer(expected) && connectionAt(connection, expected, own);
}

static bool isLocalVoiceTest(uint64 connection, anyID client) {
    anyID own{};
    if (connection != currentConnection.load() || !testServerConnection(connection, own) || own == client || !api.getClientVariableAsString) return false;
    char* uid{};
    const unsigned int peerResult = api.getClientVariableAsString(connection, client, CLIENT_UNIQUE_IDENTIFIER, &uid);
    const bool peerPresent = !peerResult && uid && uid[0];
    if (!peerResult && uid) api.freeMemory(uid);
    return peerPresent;
}

static bool stop() {
    if (waveActive) {
        const unsigned int result = api.closeWaveFileHandle(waveConnection, wave);
        record("stop", waveConnection, result, "owned wave only", wave);
        if (result) return false;
        wave = 0;
        waveActive = false;
    }
    return true;
}

static bool closeOwnedPlayback() {
    if (!playbackOwned) return true;
    const unsigned int result = api.closePlaybackDevice(playbackConnection);
    record("playback-close", playbackConnection, result, "owned output only");
    if (!result) playbackOwned = false;
    return result == ERROR_ok;
}

static unsigned int openDefaultHardwarePlayback(uint64 connection) {
    char* mode{};
    const unsigned int modeResult = api.getDefaultPlayBackMode(&mode);
    char chosen[256]{};
    if (!modeResult && mode && std::strcmp(mode, "custom")) strncpy_s(chosen, mode, _TRUNCATE);
    if (!modeResult && mode) api.freeMemory(mode);
    if (modeResult) return modeResult;
    if (!chosen[0]) {
        char** modes{};
        const unsigned int listResult = api.getPlaybackModeList(&modes);
        if (listResult) return listResult;
        if (modes) {
            for (size_t i = 0; modes[i]; ++i) {
                if (!chosen[0] && std::strcmp(modes[i], "custom")) strncpy_s(chosen, modes[i], _TRUNCATE);
                api.freeMemory(modes[i]);
            }
            api.freeMemory(modes);
        }
    }
    if (!chosen[0]) return ERROR_sound_no_playback_device_available;
    char** device{};
    const unsigned int deviceResult = api.getDefaultPlaybackDevice(chosen, &device);
    if (deviceResult) return deviceResult;
    unsigned int result = ERROR_sound_no_playback_device_available;
    if (device && device[0] && device[1] && device[1][0]) result = api.openPlaybackDevice(connection, chosen, device[1]);
    if (device) {
        if (device[0]) api.freeMemory(device[0]);
        if (device[1]) api.freeMemory(device[1]);
        api.freeMemory(device);
    }
    record("playback-default-mode", connection, result, chosen);
    return result;
}

static bool ensurePlayback(uint64 connection) {
    if (playbackOwned && playbackConnection != connection) {
        record("playback-other-handler", connection, 1, "one owned test output only");
        return false;
    }
    char* name{};
    int isDefault{};
    const unsigned int query = api.getCurrentPlaybackDeviceName(connection, &name, &isDefault);
    const bool present = query == ERROR_ok && name && name[0];
    record("playback-query", connection, query, present ? "device present" : "no playback device");
    if (query == ERROR_ok && name) api.freeMemory(name);
    if (present) return true;
    if (query != ERROR_ok && query != ERROR_sound_no_playback_device_available) return false;
    const unsigned int result = openDefaultHardwarePlayback(connection);
    record("playback-open", connection, result, "default output only");
    if (result == ERROR_ok) {
        playbackOwned = true;
        playbackConnection = connection;
        return true;
    }
    return result == ERROR_sound_handler_has_device;
}

static bool play(uint64 connection, unsigned int index) {
    if (index >= sizeof(files) / sizeof(files[0]) || !pluginId[0]) return false;
    char path[4096]{};
    if (!join(path, sizeof(path), dataDirectory, files[index])) {
        record("path-error", connection, 1, "plugin data path");
        return false;
    }
    if (!stop()) return false;
    if (!connection) connection = currentConnection.load();
    if (!connection || !ensurePlayback(connection)) return false;
    float gain{};
    const unsigned int gainResult = api.getPlaybackConfigValueAsFloat(connection, "volume_factor_wave", &gain);
    char gainText[80]{};
    std::snprintf(gainText, sizeof(gainText), "volume_factor_wave=%g", static_cast<double>(gain));
    record("wave-gain-query", connection, gainResult, gainText);
    waveConnection = connection;
    const unsigned int result = api.playWaveFileHandle(connection, path, 0, &wave);
    waveActive = result == ERROR_ok;
    record("play", connection, result, path, wave);
    if (result) wave = 0;
    return result == ERROR_ok;
}

static bool matchesArm(const char* bytes, size_t length, DWORD pid, uint64 creation) {
    char expected[128]{};
    if (!bytes || !length || length >= sizeof(expected) || std::memchr(bytes, 0, length)) return false;
    const int count = std::snprintf(expected, sizeof(expected), "%lu %llu", pid, creation);
    if (count < 0 || static_cast<size_t>(count) >= sizeof(expected)) return false;
    const size_t n = static_cast<size_t>(count);
    if (length < n || std::memcmp(bytes, expected, n)) return false;
    return length == n || (length == n + 1 && bytes[n] == '\n')
        || (length == n + 2 && bytes[n] == '\r' && bytes[n + 1] == '\n');
}

static void runOnceWhenArmed() {
    FILETIME created{}, exited{}, kernel{}, user{};
    if (!GetProcessTimes(GetCurrentProcess(), &created, &exited, &kernel, &user)) return;
    const uint64 creation = (static_cast<uint64>(created.dwHighDateTime) << 32) | created.dwLowDateTime;
    char armPath[4096]{};
    if (!join(armPath, sizeof(armPath), configDirectory, "ricon-pcm-run-once.arm")) return;
    char claimLeaf[128]{}, claimPath[4096]{};
    std::snprintf(claimLeaf, sizeof(claimLeaf), "claim-%lu-%llx-%u.arm", GetCurrentProcessId(), GetTickCount64(), ++claimSequence);
    if (!join(claimPath, sizeof(claimPath), configDirectory, claimLeaf)) return;
    record("autorun-waiting", 0, 0, "explicit PID+creation token required; no capture/network");
    for (int attempt = 0; attempt < 2400 && !workerStop.load(); ++attempt) {
        FILE* file{};
        char token[128]{};
        size_t length{};
        const DWORD attributes = GetFileAttributesA(armPath);
        if (attributes == INVALID_FILE_ATTRIBUTES) { Sleep(50); continue; }
        if (attributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)) { record("autorun-token-rejected", 0, 1, "arm must be a plain file"); return; }
        if (!MoveFileA(armPath, claimPath)) { record("autorun-claim-failed", 0, GetLastError(), "no playback"); return; }
        if (!fopen_s(&file, claimPath, "rb") && file) {
            length = std::fread(token, 1, sizeof(token), file);
            std::fclose(file);
        }
        if (!matchesArm(token, length, GetCurrentProcessId(), creation)) { record("autorun-token-rejected", 0, 1, "claimed token does not match"); return; }
        const uint64 connection = currentConnection.load();
        record("autorun-start", connection, 0, "four fixed finite files");
        for (unsigned int index = 0; index < 4 && !workerStop.load(); ++index) {
            {
                std::lock_guard<std::mutex> lock(operations);
                if (workerStop.load()) break;
                if (!play(connection, index)) { record("autorun-failed", connection, 1, files[index]); return; }
            }
            for (int tick = 0; tick < 100 && !workerStop.load(); ++tick) Sleep(20);
        }
        std::lock_guard<std::mutex> lock(operations);
        const bool closed = stop();
        record(workerStop.load() ? "autorun-cancelled" : "autorun-complete", connection, closed ? 0 : 1, "one shot only");
        return;
    }
    record("autorun-expired", 0, 1, "no matching process token");
}

#include "network_probe.inl"
#include "game_live.inl"
#include "whisper_probe.inl"
#include "ts3_tab_probe.inl"
#include "ts3_tab_actual.inl"
#include "ts3_binding_route.inl"
#include "ts3_route_voice.inl"
#include "ricon_status.inl"

EXPORT const char* ts3plugin_name() { return "RICON PCM proof (TOOL_ONLY)"; }
EXPORT const char* ts3plugin_version() { return RICON_TS3_VERSION; }
EXPORT int ts3plugin_apiVersion() { return 26; }
EXPORT const char* ts3plugin_author() { return "RICON"; }
EXPORT const char* ts3plugin_description() { return "Known Core PCM output and explicit local Arma3 RICON DSP network proof. Idle until a test command."; }
EXPORT void ts3plugin_setFunctionPointers(const TS3Functions functions) { api = functions; }
static void prepareHost() {
    // Actual clients may register the ID before init; both must finish before setup.
    if (!pluginId[0] || !configDirectory[0]) return;
    char directory[4096]{}, enabled[8]{};
    api.getPluginPath(directory, sizeof(directory), pluginId);
    if (!join(dataDirectory, sizeof(dataDirectory), directory, "ricon_pcm_probe")) return;
    startRiconStatus();
    if (testWorker.joinable()) return;
    const uint64 connection = api.getCurrentServerConnectionHandlerID();
    selectTs3Tab(connection);
    currentConnection.store(connection);
    record("registered-plugin", currentConnection.load(), 0, directory);
    const bool tabActual = knownCaptureRegistered && ts3TabActualEnabled();
    ts3TabActualHostReady.store(tabActual);
    if (tabActual) return;
    char enablePath[4096]{};
    DWORD enabledAttributes = INVALID_FILE_ATTRIBUTES;
    if (join(enablePath, sizeof(enablePath), configDirectory, "ricon-pcm-test.enabled")) enabledAttributes = GetFileAttributesA(enablePath);
    const bool fileEnabled = enabledAttributes != INVALID_FILE_ATTRIBUTES && !(enabledAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT));
    if (((GetEnvironmentVariableA("RICON_TS3_PCM_AUTORUN", enabled, sizeof(enabled)) == 1 && enabled[0] == '1') || fileEnabled)
        && std::strstr(configDirectory, "ts3-pcm-prototype-01") && std::strstr(configDirectory, "ricon-probe")) {
        workerStop.store(false);
        try { testWorker = std::thread(runOnceWhenArmed); }
        catch (const std::exception&) { record("autorun-worker-error", 0, 1, "worker not started"); }
    }
}
EXPORT void ts3plugin_registerPluginID(const char* id) {
    if (!id || testWorker.joinable()) return;
    strncpy_s(pluginId, id, _TRUNCATE);
    prepareHost();
}
EXPORT void ts3plugin_currentServerConnectionChanged(uint64 connection) {
    revokeTs3Route();
    selectTs3Tab(connection);
    record("current-connection", connection, 0, "SDK selected handler callback");
    currentConnection.store(connection);
    if (networkPrepared.load() && connection != networkListener.load()) requestWorkerStop();
    clearVoiceProbe();
    maybeBeginTs3TabActual(connection);
}
EXPORT void ts3plugin_onConnectStatusChangeEvent(uint64 connection, int status, unsigned int error) {
    bool revoke = status == STATUS_DISCONNECTED || error;
    int actual = status;
    unsigned int queried{};
    if (!revoke && status != STATUS_CONNECTION_ESTABLISHED) {
        queried = api.getConnectionStatus ? api.getConnectionStatus(connection, &actual) : ERROR_undefined;
        // ClientUI notifications can lag behind the ClientLib state already used by the worker.
        revoke = queried || actual != STATUS_CONNECTION_ESTABLISHED;
    }
    if (revoke) {
        if (connection == ts3RouteCaptureHandler.load()) revokeTs3Route();
        invalidateTs3Tab(connection);
    }
    char detail[144]{};
    std::snprintf(detail, sizeof(detail), "status=%d actual=%d query=%u error=%u revoke=%u prepared=%u", status, actual, queried, error,
                  static_cast<unsigned int>(revoke), static_cast<unsigned int>(networkPrepared.load()));
    record("connection-status", connection, error, detail);
    if (revoke) { cancelNetworkConnection(connection); cancelWhisperHandler(connection); }
    if (connection == currentConnection.load() && revoke) clearVoiceProbe();
    if (status == STATUS_CONNECTION_ESTABLISHED) maybeBeginTs3TabActual(connection);
}
static void handleClientMove(uint64 connection, anyID client, uint64 oldChannel, uint64 newChannel, int visibility, bool allowInitial) {
    if (connection == ts3RouteCaptureHandler.load()) revokeTs3Route();
    const bool initial = allowInitial && initialBoundArrival(connection, client, oldChannel, newChannel, visibility);
    char detail[160]{};
    std::snprintf(detail, sizeof(detail), "client=%u old=%llu new=%llu visibility=%d initialBoundArrival=%u prepared=%u",
                  static_cast<unsigned int>(client), oldChannel, newChannel, visibility, static_cast<unsigned int>(initial), static_cast<unsigned int>(networkPrepared.load()));
    record("client-move", connection, 0, detail);
    if (initial) return;
    cancelNetworkActor(connection, client);
    if (connection == currentConnection.load()) clearVoiceProbe();
}
EXPORT void ts3plugin_onClientMoveEvent(uint64 connection, anyID client, uint64 oldChannel, uint64 newChannel, int visibility, const char*) {
    handleClientMove(connection, client, oldChannel, newChannel, visibility, true);
}
EXPORT void ts3plugin_onClientMoveTimeoutEvent(uint64 c, anyID id, uint64 oldChannel, uint64 channel, int visibility, const char* message) {
    (void)message;
    handleClientMove(c, id, oldChannel, channel, visibility, false);
}
EXPORT void ts3plugin_onClientMoveMovedEvent(uint64 c, anyID id, uint64 oldChannel, uint64 channel, int visibility, anyID, const char*, const char*, const char* message) {
    (void)message;
    handleClientMove(c, id, oldChannel, channel, visibility, false);
}
EXPORT void ts3plugin_onClientKickFromChannelEvent(uint64 c, anyID id, uint64 oldChannel, uint64 channel, int visibility, anyID, const char*, const char*, const char* message) {
    (void)message;
    handleClientMove(c, id, oldChannel, channel, visibility, false);
}
EXPORT void ts3plugin_onClientKickFromServerEvent(uint64 c, anyID id, uint64 oldChannel, uint64 channel, int visibility, anyID, const char*, const char*, const char* message) {
    (void)message;
    handleClientMove(c, id, oldChannel, channel, visibility, false);
}
EXPORT void ts3plugin_onEditPlaybackVoiceDataEvent(uint64 connection, anyID client, short* samples, int frames, int channels) {
    observeWhisperPlayback(connection, client, samples, frames, channels);
    if (liveListener.load() && connection == liveListener.load()) {
        if (!samples || frames <= 0 || frames > 5760 || channels < 1 || channels > 2) return;
        if (!liveAllowed(GetTickCount64()) || client != livePeer.load()) {
            std::memset(samples, 0, static_cast<size_t>(frames) * channels * sizeof(short));
            return;
        }
        observeLivePcm(1, samples, frames * channels);
        LivePairedLevels paired{};
        if (!editVoiceProbe(connection, client, samples, frames, channels, GetTickCount64(), &paired)) {
            std::memset(samples, 0, static_cast<size_t>(frames) * channels * sizeof(short));
            return;
        }
        observeLivePcm(2, samples, frames * channels);
        observeLivePaired(paired);
        return;
    }
    editVoiceProbe(connection, client, samples, frames, channels, GetTickCount64());
}
EXPORT void ts3plugin_onEditCapturedVoiceDataEvent(uint64 connection, short* samples, int frames, int channels, int* edited) {
    observeWhisperCapture(connection, samples, frames, channels);
    if (editTs3RouteCapture(connection, samples, frames, channels, edited, GetTickCount64())) return;
    editLiveCapture(connection, samples, frames, channels, edited, GetTickCount64());
}
EXPORT void ts3plugin_onTalkStatusChangeEvent(uint64 connection, int status, int isReceivedWhisper, anyID client) {
    observeWhisperTalk(connection, status, isReceivedWhisper, client);
}
EXPORT const char* ts3plugin_commandKeyword() { return "riconproof"; }
EXPORT int ts3plugin_onServerErrorEvent(uint64 connection, const char*, unsigned int error, const char* code, const char*) {
    return observeWhisperAck(connection, error, code);
}
EXPORT int ts3plugin_processCommand(uint64 connection, const char* command) {
    if (command && !std::strcmp(command, "ts3-route")) {
        record("ts3-route-command", connection, beginTs3Route(connection) ? 0 : 1, "selected established custom source; server route and ACK required");
        return 0;
    }
    if (command && !std::strcmp(command, "ts3-tab")) {
        record("ts3-tab-command", 0, beginTs3Tab(connection) ? 0 : 1, "explicit metadata request");
        return 0;
    }
    if (command && !std::strcmp(command, "whisper-ack")) {
        record("whisper-ack-command", connection, beginWhisperProbe(WhisperProbeMode::ack) ? 0 : 1, "isolated A/B exact route ACK; zero PCM");
        return 0;
    }
    if (command && !std::strcmp(command, "whisper-proof")) {
        record("whisper-command", connection, beginWhisperProbe(WhisperProbeMode::proof) ? 0 : 1, "isolated custom PCM; two explicit client targets");
        return 0;
    }
    if (command && !std::strcmp(command, "whisper-tone")) {
        record("whisper-tone-command", connection, beginWhisperProbe(WhisperProbeMode::tone) ? 0 : 1, "SYNTHETIC_A_B_SYNC_TRIGGER; known 600 Hz PCM only");
        return 0;
    }
    if (command && !std::strcmp(command, "whisper-tone-abc")) {
        record("whisper-tone-abc-command", connection, beginWhisperProbe(WhisperProbeMode::toneAbc) ? 0 : 1,
               "SYNTHETIC_A_B_C_SYNC_TRIGGER; known PCM C=600 Hz B=1200 Hz");
        return 0;
    }
    if (command && !std::strcmp(command, "ptt-status")) {
        if (!beginPttStatus(connection)) record("ptt-status", connection, 1, "outcome=BUSY_OR_UNAVAILABLE");
        return 0;
    }
    if (command && !std::strcmp(command, "network-good")) {
        record("network-good-command", connection, beginNetworkProbe(false, true) ? 0 : 1, "known-PCM A/B good30dB-m20; no hardware capture");
        return 0;
    }
    if (command && !std::strcmp(command, "live")) {
        record("live-command", connection, beginNetworkProbe(true) ? 0 : 1, "game-bound hardware microphone A/B prototype");
        return 0;
    }
    if (command && !std::strcmp(command, "network")) {
        record("network-command", connection, beginNetworkProbe() ? 0 : 1, "explicit known-PCM A/B test");
        return 0;
    }
    if (command && !std::strcmp(command, "stop")) {
        revokeTs3Route();
        requestWorkerStop();
        const RiconAudioMetrics metrics = clearVoiceProbe();
        char summary[160]{};
        std::snprintf(summary, sizeof(summary), "callbacks=%llu dsp=%llu muted=%llu sampleValues=%llu",
                      metrics.callbacks_total, metrics.callbacks_dsp, metrics.callbacks_muted, metrics.sample_values_processed);
        record("voice-test-stop", connection, 0, summary);
        return 0;
    }
    anyID client{};
    bool physicalLost{};
    if (networkRunning.load()) return 1;
    if (!voiceCommand(command, client, physicalLost)) return 1;
    uint64 generation{};
    clearVoiceProbe(&generation);
    const bool armed = isLocalVoiceTest(connection, client)
        && armVoiceProbe(connection, client, physicalLost, GetTickCount64() + 10000, generation);
    char summary[128]{};
    std::snprintf(summary, sizeof(summary), "clientID=%u mode=%s sampleRate=48000 maxDurationMs=10000",
                  static_cast<unsigned int>(client), physicalLost ? "physical-LOST-noise" : "Arma3-degraded");
    record("voice-test-arm", connection, armed ? 0 : 1, summary);
    return 0;
}
EXPORT int ts3plugin_init() {
    if (!api.getConfigPath || !api.getPluginPath || !api.getCurrentServerConnectionHandlerID || !api.playWaveFileHandle || !api.closeWaveFileHandle
        || !api.getCurrentPlaybackDeviceName || !api.openPlaybackDevice || !api.closePlaybackDevice || !api.freeMemory || !api.getPlaybackConfigValueAsFloat
        || !api.getDefaultPlayBackMode || !api.getDefaultPlaybackDevice || !api.getPlaybackModeList) return 1;
    char config[4096]{}, directory[4096]{};
    api.getConfigPath(config, sizeof(config));
    strncpy_s(configDirectory, config, _TRUNCATE);
    ricon_audio_init();
    ricon_live150_audio_init();
    if (pluginId[0]) api.getPluginPath(directory, sizeof(directory), pluginId);
    record("init-config", 0, 0, config);
    record("init-plugin", 0, 0, directory);
    riconStatusHostReady.store(true);
    registerKnownCapture();
    prepareHost();
    return 0;
}
EXPORT void ts3plugin_shutdown() {
    stopRiconStatus();
    revokeTs3Route();
    ts3TabActualHostReady.store(false);
    selectTs3Tab(0);
    clearVoiceProbe();
    requestWorkerStop();
    if (testWorker.joinable()) testWorker.join();
    ts3RouteCaptureHandler.store(0);
    unregisterKnownCapture();
    std::lock_guard<std::mutex> lock(operations);
    const bool closed = stop();
    closeOwnedPlayback();
    record("shutdown", 0, closed ? 0 : 1, "owned wave close status");
}
EXPORT void ts3plugin_freeMemory(void* pointer) { std::free(pointer); }
EXPORT void ts3plugin_initMenus(PluginMenuItem*** items, char** icon) {
    *items = nullptr;
    *icon = nullptr;
    auto list = static_cast<PluginMenuItem**>(std::calloc(6, sizeof(PluginMenuItem*)));
    if (!list) return;
    const char* labels[] = {"Play 600 Hz input", "Play 600 Hz RICON", "Play 1200 Hz input", "Play 1200 Hz RICON", "Stop RICON proof"};
    for (int i = 0; i < 5; ++i) {
        list[i] = static_cast<PluginMenuItem*>(std::calloc(1, sizeof(PluginMenuItem)));
        if (!list[i]) {
            for (int j = 0; j < i; ++j) std::free(list[j]);
            std::free(list);
            return;
        }
        list[i]->type = PLUGIN_MENU_TYPE_GLOBAL;
        list[i]->id = i;
        strncpy_s(list[i]->text, labels[i], _TRUNCATE);
    }
    *items = list;
}
EXPORT void ts3plugin_onMenuItemEvent(uint64 connection, PluginMenuType type, int id, uint64) {
    if (type != PLUGIN_MENU_TYPE_GLOBAL) return;
    if (id == 4) { revokeTs3Route(); requestWorkerStop(); clearVoiceProbe(); }
    else if (networkRunning.load()) return;
    std::lock_guard<std::mutex> lock(operations);
    if (id == 4) stop();
    else if (id >= 0 && id < 4) play(connection, static_cast<unsigned int>(id));
}
