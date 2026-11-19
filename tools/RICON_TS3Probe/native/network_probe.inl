// TOOL_ONLY SDK integration fixture; GUI helpers run only in the explicit command callback.
static std::atomic<bool> networkRunning{false};
static std::atomic<bool> networkPrepared{false};
static std::atomic<uint64> networkSource{0}, networkListener{0};
static std::atomic<uint64> networkChannel{0};
static std::atomic<anyID> networkSourceClient{0}, networkListenerClient{0};
static bool networkCancelled(uint64 ticket) { return workerStop.load() || workerGeneration.load() != ticket; }
static bool stateUpdateOk(unsigned int result) { return result == ERROR_ok || result == ERROR_ok_no_update; }

static bool flushTestState(uint64 connection, const char* action) {
    const unsigned int result = api.flushClientSelfUpdates(connection, nullptr);
    record(action, connection, result, "owned test connection; code3 means unchanged success");
    return stateUpdateOk(result);
}

static void cancelNetworkConnection(uint64 connection) {
    if (networkPrepared.load() && (connection == networkSource.load() || connection == networkListener.load())) requestWorkerStop();
}

static void cancelNetworkActor(uint64 connection, anyID client) {
    if (client == networkSourceClient.load() || client == networkListenerClient.load()) cancelNetworkConnection(connection);
}

static bool initialBoundArrival(uint64 connection, anyID client, uint64 oldChannel, uint64 newChannel, int visibility) {
    return networkPrepared.load() && !oldChannel && newChannel && newChannel == networkChannel.load()
        && visibility == ENTER_VISIBILITY && (connection == networkSource.load() || connection == networkListener.load())
        && (client == networkSourceClient.load() || client == networkListenerClient.load());
}

struct NetworkBinding {
    uint64 source{}, listener{}, channel{};
    anyID sourceClient{}, listenerClient{};
    char ip[16]{}, serverUid[128]{};
};
static void runLiveProbe(NetworkBinding binding, uint64 ticket);
static bool liveCleanupPending();
static bool setLiveRadioProfile(uint64 generation);

static bool serverUid(uint64 connection, char (&uid)[128]) {
    char* value{};
    const unsigned int result = api.getServerVariableAsString(connection, VIRTUALSERVER_UNIQUE_IDENTIFIER, &value);
    const bool valid = !result && value && value[0] && std::strlen(value) < sizeof(uid);
    if (valid) strcpy_s(uid, value);
    if (value) api.freeMemory(value);
    return valid;
}

static bool sameOwnedConnection(const NetworkBinding& binding, uint64 connection, anyID expectedClient) {
    anyID actual{};
    char uid[128]{};
    return connectionAt(connection, binding.ip, actual) && actual == expectedClient
        && serverUid(connection, uid) && !std::strcmp(uid, binding.serverUid);
}

static bool networkBindingCurrent(const NetworkBinding& binding) {
    char expected[16]{};
    uint64 a{}, b{};
    return voiceTestProfileEnabled() && expectedTestServer(expected) && !std::strcmp(expected, binding.ip)
        && currentConnection.load() == binding.listener
        && sameOwnedConnection(binding, binding.source, binding.sourceClient)
        && sameOwnedConnection(binding, binding.listener, binding.listenerClient)
        && !api.getChannelOfClient(binding.source, binding.sourceClient, &a) && a == binding.channel
        && !api.getChannelOfClient(binding.listener, binding.listenerClient, &b) && b == binding.channel;
}

static bool closeTestConnection(uint64 connection) {
    std::lock_guard<std::mutex> lock(operations);
    const unsigned int capture = api.closeCaptureDevice(connection);
    const unsigned int playback = api.closePlaybackDevice(connection);
    if (!playback && playbackOwned && playbackConnection == connection) playbackOwned = false;
    int status{};
    const unsigned int queried = api.getConnectionStatus(connection, &status);
    const unsigned int disconnected = !queried && status == STATUS_DISCONNECTED ? 0 : api.stopConnection(connection, "RICON test complete");
    record("network-close-capture", connection, capture, "owned GUI handler");
    record("network-close-playback", connection, playback, "owned GUI handler");
    record("network-disconnect", connection, disconnected, "owned GUI handler");
    return !capture && !playback && !queried && !disconnected;
}

static bool stillOwnGuiConnection(const NetworkBinding& binding, uint64 connection, anyID expectedClient, bool bound) {
    int status{};
    if (api.getConnectionStatus(connection, &status)) return false;
    if (status == STATUS_DISCONNECTED) return true;
    anyID client{};
    if (!api.getClientID(connection, &client) && client) {
        if ((bound || expectedClient) && client != expectedClient) return false;
        char* ip{};
        const unsigned int result = api.getConnectionVariableAsString(connection, client, CONNECTION_SERVER_IP, &ip);
        const bool foreign = !result && ip && std::strcmp(ip, binding.ip);
        if (!result && ip) api.freeMemory(ip);
        uint64 port{};
        if (foreign || (!api.getConnectionVariableAsUInt64(connection, client, CONNECTION_SERVER_PORT, &port) && port != 19987)) return false;
        char uid[128]{};
        if ((bound || binding.serverUid[0]) && serverUid(connection, uid) && std::strcmp(uid, binding.serverUid)) return false;
    }
    // A successful NEW GUI handler remains ours during setup unless a foreign binding is observed.
    return true;
}

static bool knownCapture(uint64 connection) {
    char* mode{};
    const unsigned int modeResult = api.getCurrentCaptureMode(connection, &mode);
    const bool custom = !modeResult && mode && !std::strcmp(mode, "custom");
    if (!modeResult && mode) api.freeMemory(mode);
    if (!custom) return false;
    char* name{};
    int isDefault{};
    const unsigned int nameResult = api.getCurrentCaptureDeviceName(connection, &name, &isDefault);
    const bool known = !nameResult && name && (!std::strcmp(name, knownCaptureId) || !std::strcmp(name, "RICON Known PCM (TOOL_ONLY)"));
    if (!nameResult && name) api.freeMemory(name);
    return known;
}

static bool testIdentities(char (&source)[256], char (&listener)[256]) {
    if (!api.getProfileList) return false;
    char** profiles{};
    int preferred{};
    const unsigned int result = api.getProfileList(PLUGIN_GUI_IDENTITY, &preferred, &profiles);
    if (result) return false;
    if (profiles) {
        for (size_t i = 0; profiles[i]; ++i) {
            if (profiles[i][0] && std::strlen(profiles[i]) < sizeof(source)) {
                if (!std::strcmp(profiles[i], "RICON_B")) strcpy_s(listener, profiles[i]);
                else if (!source[0] || static_cast<int>(i) == preferred) strcpy_s(source, profiles[i]);
            }
            api.freeMemory(profiles[i]);
        }
        api.freeMemory(profiles);
    }
    return source[0] && listener[0] && std::strcmp(source, listener);
}

static unsigned int little16(const unsigned char* bytes) { return bytes[0] | (static_cast<unsigned int>(bytes[1]) << 8); }
static unsigned int little32(const unsigned char* bytes) { return little16(bytes) | (little16(bytes + 2) << 16); }

static bool readKnownWave(unsigned int index, short (&samples)[48000]) {
    char path[4096]{};
    if (index >= 4 || !join(path, sizeof(path), dataDirectory, files[index])) return false;
    FILE* file{};
    if (fopen_s(&file, path, "rb") || !file) return false;
    unsigned char header[44]{};
    const bool headerOk = std::fread(header, 1, sizeof(header), file) == sizeof(header)
        && !std::memcmp(header, "RIFF", 4) && little32(header + 4) == 96036 && !std::memcmp(header + 8, "WAVEfmt ", 8)
        && little32(header + 16) == 16 && little16(header + 20) == 1 && little16(header + 22) == 1
        && little32(header + 24) == 48000 && little32(header + 28) == 96000 && little16(header + 32) == 2
        && little16(header + 34) == 16 && !std::memcmp(header + 36, "data", 4) && little32(header + 40) == 96000;
    const bool valid = headerOk && std::fread(samples, sizeof(short), 48000, file) == 48000 && std::fgetc(file) == EOF;
    std::fclose(file);
    return valid;
}

static bool waitNetworkArm(uint64 ticket) {
    FILETIME created{}, exited{}, kernel{}, user{};
    if (!GetProcessTimes(GetCurrentProcess(), &created, &exited, &kernel, &user)) return false;
    const uint64 creation = (static_cast<uint64>(created.dwHighDateTime) << 32) | created.dwLowDateTime;
    char arm[4096]{}, claim[4096]{}, leaf[128]{};
    if (!join(arm, sizeof(arm), configDirectory, "ricon-network-run-once.arm")) return false;
    std::snprintf(leaf, sizeof(leaf), "network-claim-%lu-%llx-%u.arm", GetCurrentProcessId(), GetTickCount64(), ++claimSequence);
    if (!join(claim, sizeof(claim), configDirectory, leaf)) return false;
    for (int attempt = 0; attempt < 2400 && !networkCancelled(ticket); ++attempt) {
        const DWORD attributes = GetFileAttributesA(arm);
        if (attributes == INVALID_FILE_ATTRIBUTES) { Sleep(50); continue; }
        if (attributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT) || !MoveFileA(arm, claim)) return false;
        FILE* file{}; char token[128]{}; size_t count{};
        if (!fopen_s(&file, claim, "rb") && file) { count = std::fread(token, 1, sizeof(token), file); std::fclose(file); }
        return matchesArm(token, count, GetCurrentProcessId(), creation);
    }
    return false;
}

static bool sendKnownFrames(const NetworkBinding& binding, const short* samples, int blocks, uint64& next, uint64 ticket) {
    short silence[960]{};
    for (int block = 0; block < blocks; ++block) {
        int listenerInput{};
        if (networkCancelled(ticket) || !networkBindingCurrent(binding) || !knownCapture(binding.source) || !knownCapture(binding.listener)
            || api.getClientSelfVariableAsInt(binding.listener, CLIENT_INPUT_DEACTIVATED, &listenerInput) || listenerInput != INPUT_DEACTIVATED) return false;
        if (!stateUpdateOk(api.activateCaptureDevice(binding.source)) || networkCancelled(ticket)) return false;
        const unsigned int result = api.processCustomCaptureData(knownCaptureId, samples ? samples + block * 960 : silence, 960);
        if (result) { record("network-submit-error", binding.source, result, "known custom source only"); return false; }
        next += 20;
        while (!networkCancelled(ticket) && GetTickCount64() < next) Sleep(1);
    }
    return !networkCancelled(ticket);
}

static RiconAudioMetrics networkMetrics(uint64 listener, const char* phase) {
    const RiconAudioMetrics metrics = clearVoiceProbe();
    char detail[192]{};
    std::snprintf(detail, sizeof(detail), "phase=%s callbacks=%llu dsp=%llu muted=%llu sampleValues=%llu",
                  phase, metrics.callbacks_total, metrics.callbacks_dsp, metrics.callbacks_muted, metrics.sample_values_processed);
    record("network-metrics", listener, 0, detail);
    return metrics;
}

static void runNetworkProbe(NetworkBinding binding, uint64 ticket, bool goodProfile = false) {
    const uint64 source = binding.source, listener = binding.listener;
    const char* keys[] = {"vad", "denoise", "agc", "echo_canceling"};
    char original[4][16]{};
    bool changed[4]{};
    int originalInput{}, originalListenerInput{};
    bool inputChanged{}, listenerChanged{}, success{};
    anyID sourceId{}, listenerId{};
    bool bound{};
    short tone[48000]{};
    do {
        bool connected = false;
        for (int attempt = 0; attempt < 300 && !networkCancelled(ticket); ++attempt) {
            if (testServerConnection(source, sourceId) && testServerConnection(listener, listenerId) && sourceId != listenerId) { connected = true; break; }
            Sleep(50);
        }
        const bool sourceCustom = connected && knownCapture(source);
        const bool listenerCustom = connected && knownCapture(listener);
        const bool peerCurrent = connected && isLocalVoiceTest(listener, sourceId);
        char contract[160]{};
        std::snprintf(contract, sizeof(contract), "connected=%u sourceCustom=%u listenerCustom=%u peerCurrent=%u",
                      static_cast<unsigned int>(connected), static_cast<unsigned int>(sourceCustom),
                      static_cast<unsigned int>(listenerCustom), static_cast<unsigned int>(peerCurrent));
        record("network-capture-contract", listener, 0, contract);
        if (networkCancelled(ticket) || !sourceCustom || !listenerCustom || !peerCurrent) break;
        binding.sourceClient = sourceId; binding.listenerClient = listenerId;
        if (!serverUid(source, binding.serverUid)
            || api.getChannelOfClient(source, sourceId, &binding.channel) || !binding.channel || !networkBindingCurrent(binding)) break;
        bound = true;
        networkChannel.store(binding.channel);
        const unsigned int activate = api.activateCaptureDevice(source);
        record("network-activate-source", source, activate, "known virtual source only");
        if (!stateUpdateOk(activate)) break;
        {
            std::lock_guard<std::mutex> lock(operations);
            if (!ensurePlayback(listener)) break;
        }
        if (api.getClientSelfVariableAsInt(source, CLIENT_INPUT_DEACTIVATED, &originalInput)) break;
        if (api.getClientSelfVariableAsInt(listener, CLIENT_INPUT_DEACTIVATED, &originalListenerInput)
            || !stateUpdateOk(api.setClientSelfVariableAsInt(listener, CLIENT_INPUT_DEACTIVATED, INPUT_DEACTIVATED))) break;
        listenerChanged = true;
        if (!flushTestState(listener, "network-flush-listener-ready")) break;
        bool configured = true;
        for (int i = 0; i < 4; ++i) {
            char* value{};
            const unsigned int result = api.getPreProcessorConfigValue(source, keys[i], &value);
            if (result || !value || (std::strcmp(value, "true") && std::strcmp(value, "false"))) { record("network-preprocessor-error", source, result ? result : 1, keys[i]); configured = false; if (!result && value) api.freeMemory(value); break; }
            strcpy_s(original[i], value); api.freeMemory(value);
            if (!stateUpdateOk(api.setPreProcessorConfigValue(source, keys[i], "false"))) { configured = false; break; }
            changed[i] = true;
        }
        if (!configured) break;
        if (!stateUpdateOk(api.setClientSelfVariableAsInt(source, CLIENT_INPUT_DEACTIVATED, INPUT_ACTIVE))) break;
        inputChanged = true;
        if (!flushTestState(source, "network-flush-source-ready")) break;
        networkSourceClient.store(sourceId); networkListenerClient.store(listenerId);
        networkPrepared.store(true);
        for (int i = 0; i < 50 && !networkCancelled(ticket); ++i) Sleep(20);
        if (networkCancelled(ticket)) break;
        char ready[160]{};
        std::snprintf(ready, sizeof(ready), "sourceHandler=%llu listenerHandler=%llu sourceClient=%u listenerClient=%u wait=PID+creation",
                      source, listener, static_cast<unsigned int>(sourceId), static_cast<unsigned int>(listenerId));
        record("network-ready", listener, 0, ready);
        if (!waitNetworkArm(ticket)) break;
        uint64 next = GetTickCount64();
        clearVoiceProbe();
        if (!readKnownWave(1, tone) || !sendKnownFrames(binding, nullptr, 10, next, ticket)) break;
        record("network-phase", listener, 0, "baseline600 source=600-ricon.wav");
        if (!sendKnownFrames(binding, tone, 50, next, ticket) || !sendKnownFrames(binding, nullptr, 25, next, ticket)) break;
        uint64 generation{}; clearVoiceProbe(&generation);
        if (!isLocalVoiceTest(listener, sourceId) || !armVoiceProbe(listener, sourceId, false, GetTickCount64() + 10000, generation, ticket)) break;
        if (goodProfile) {
            if (!setLiveRadioProfile(generation)) break;
            record("network-good-profile", listener, 0, "meanSnr=30 m=20 source=knownPCM hardwareCapture=0");
        }
        record("network-phase", listener, 0, goodProfile ? "good600 source=600-ricon.wav" : "degraded600 source=600-ricon.wav");
        if (!sendKnownFrames(binding, tone, 50, next, ticket) || !sendKnownFrames(binding, nullptr, 25, next, ticket)) break;
        if (!readKnownWave(3, tone)) break;
        record("network-phase", listener, 0, goodProfile ? "good1200 source=1200-ricon.wav" : "degraded1200 source=1200-ricon.wav");
        if (!sendKnownFrames(binding, tone, 50, next, ticket) || !sendKnownFrames(binding, nullptr, 25, next, ticket)) break;
        const RiconAudioMetrics degraded = networkMetrics(listener, goodProfile ? "good" : "degraded");
        if (!degraded.callbacks_dsp || degraded.callbacks_muted) break;
        clearVoiceProbe(&generation);
        if (!isLocalVoiceTest(listener, sourceId) || !armVoiceProbe(listener, sourceId, true, GetTickCount64() + 10000, generation, ticket)) break;
        record("network-phase", listener, 0, "noise1200 source=1200-ricon.wav");
        if (!sendKnownFrames(binding, tone, 50, next, ticket) || !sendKnownFrames(binding, nullptr, 50, next, ticket)) break;
        const RiconAudioMetrics noise = networkMetrics(listener, "noise");
        if (!noise.callbacks_dsp || noise.callbacks_muted || networkCancelled(ticket)) break;
        success = true;
    } while (false);
    clearVoiceProbe();
    networkPrepared.store(false);
    const bool sourceOwned = stillOwnGuiConnection(binding, source, sourceId, bound);
    const bool listenerOwned = stillOwnGuiConnection(binding, listener, listenerId, bound);
    bool cleanup = sourceOwned && listenerOwned;
    for (int i = 0; i < 4; ++i) if (changed[i] && sourceOwned) {
        const unsigned int result = api.setPreProcessorConfigValue(source, keys[i], original[i]);
        record("network-restore-preprocessor", source, result, keys[i]); cleanup &= stateUpdateOk(result);
    }
    if (inputChanged && sourceOwned) {
        const unsigned int restored = api.setClientSelfVariableAsInt(source, CLIENT_INPUT_DEACTIVATED, originalInput);
        const bool flushed = flushTestState(source, "network-flush-restored");
        record("network-restore-input", source, restored, "owned test connection");
        cleanup &= stateUpdateOk(restored) && flushed;
    }
    if (listenerChanged && listenerOwned) {
        const unsigned int restored = api.setClientSelfVariableAsInt(listener, CLIENT_INPUT_DEACTIVATED, originalListenerInput);
        const bool flushed = flushTestState(listener, "network-flush-listener");
        record("network-restore-listener", listener, restored, "owned listener");
        cleanup &= stateUpdateOk(restored) && flushed;
    }
    if (sourceOwned) cleanup &= closeTestConnection(source);
    if (listenerOwned) cleanup &= closeTestConnection(listener);
    char complete[96]{}; std::snprintf(complete, sizeof(complete), "content=%u cleanup=%u render=UNQUALIFIED", static_cast<unsigned int>(success), static_cast<unsigned int>(cleanup));
    record(success && cleanup ? "network-complete" : "network-failed", listener, success && cleanup ? 0 : 1, complete);
    networkRunning.store(false);
}

static bool beginNetworkProbe(bool live = false, bool goodProfile = false) {
    if (live && goodProfile) return false;
    const uint64 ticket = workerGeneration.load();
    if (!voiceTestProfileEnabled() || !knownCaptureRegistered || ticket == ~uint64{0} || networkRunning.load() || liveCleanupPending()
        || !api.guiConnect || !api.getServerConnectionHandlerList || !api.getConnectionStatus || !api.getCurrentServerConnectionHandlerID
        || !api.getCurrentCaptureMode || !api.getCurrentCaptureDeviceName || !api.activateCaptureDevice || !api.processCustomCaptureData
        || !api.getPreProcessorConfigValue || !api.setPreProcessorConfigValue || !api.getClientSelfVariableAsInt
        || !api.setClientSelfVariableAsInt || !api.flushClientSelfUpdates || !api.stopConnection || !api.closeCaptureDevice
        || !api.getServerVariableAsString || !api.getChannelOfClient) return false;
    char ip[16]{};
    if (!expectedTestServer(ip)) return false;
    char sourceIdentity[256]{}, listenerIdentity[256]{};
    if (!testIdentities(sourceIdentity, listenerIdentity)) {
        record("network-identities", 0, 1, "requires distinct existing identity and named RICON_B; no key export");
        return false;
    }
    uint64* handlers{};
    if (api.getServerConnectionHandlerList(&handlers) || !handlers) return false;
    bool disconnected = true;
    for (size_t i = 0; handlers[i]; ++i) { int status{}; if (api.getConnectionStatus(handlers[i], &status) || status != STATUS_DISCONNECTED) disconnected = false; }
    api.freeMemory(handlers);
    if (!disconnected || networkRunning.exchange(true)) return false;
    networkPrepared.store(false);
    workerStop.store(true);
    if (testWorker.joinable()) testWorker.join();
    {
        std::lock_guard<std::mutex> lock(operations);
        if (!stop() || !closeOwnedPlayback()) { networkRunning.store(false); return false; }
    }
    if (workerGeneration.load() != ticket) { networkRunning.store(false); return false; }
    workerStop.store(false);
    char address[48]{};
    std::snprintf(address, sizeof(address), "%s:19987", ip);
    uint64 source{}, listener{};
    const unsigned int a = api.guiConnect(PLUGIN_CONNECT_TAB_NEW, "RICON source A", address, "", "RICON-A", "", "", "RICON_PCM", "", "", "", sourceIdentity, "", "", &source);
    record("network-connect-source", source, a, address);
    if (a || !source) { networkRunning.store(false); return false; }
    if (networkCancelled(ticket)) { closeTestConnection(source); networkRunning.store(false); return false; }
    const unsigned int b = api.guiConnect(PLUGIN_CONNECT_TAB_NEW, "RICON listener B", address, "", "RICON-B", "", "", "RICON_PCM", "", "", "", listenerIdentity, "", "", &listener);
    record("network-connect-listener", listener, b, address);
    if (b || !listener || listener == source) { closeTestConnection(source); networkRunning.store(false); return false; }
    networkSource.store(source); networkListener.store(listener);
    currentConnection.store(api.getCurrentServerConnectionHandlerID());
    NetworkBinding binding{};
    binding.source = source; binding.listener = listener; strcpy_s(binding.ip, ip);
    try {
        if (live) testWorker = std::thread(runLiveProbe, binding, ticket);
        else testWorker = std::thread(runNetworkProbe, binding, ticket, goodProfile);
    }
    catch (const std::exception&) {
        networkRunning.store(false);
        closeTestConnection(source); closeTestConnection(listener);
        return false;
    }
    return true;
}
