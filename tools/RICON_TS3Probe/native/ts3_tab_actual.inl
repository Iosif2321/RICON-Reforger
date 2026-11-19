// TOOL_ONLY: one fresh tab in the disposable SDK runtime fixture.
static std::atomic<bool> ts3TabActualHostReady{false}, ts3TabActualTried{false};

static bool ts3TabActualStagePath(std::string_view path, std::string_view stage = "ts3-tab-actual-01") {
    bool exactStage{};
    while (!path.empty()) {
        const size_t separator = path.find_first_of("/\\");
        const auto part = path.substr(0, separator);
        if (part == "." || part == "..") return false;
        if (part == stage) exactStage = true;
        if (separator == std::string_view::npos) break;
        path.remove_prefix(separator + 1);
    }
    return exactStage;
}

static bool ts3TabActualMarker(const char* name) {
    char path[4096]{};
    if (!join(path, sizeof(path), configDirectory, name)) return false;
    const DWORD attributes = GetFileAttributesA(path);
    return attributes != INVALID_FILE_ATTRIBUTES && !(attributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT));
}

static bool ts3TabActualEnabled() {
    const bool proof = ts3TabActualStagePath(configDirectory, "ts3-binding-proof-01");
    return voiceTestProfileEnabled() && ts3TabActualMarker("ricon-ts3-tab-actual.enabled")
        && (proof ? ts3TabActualMarker("ricon-ts3-proof-actual.enabled") : ts3TabActualStagePath(configDirectory));
}

struct Ts3TabActualTarget {
    uint64 handler{};
    char ip[16]{};
    bool proof{};
};

static bool ts3TabActualSelected(const Ts3TabActualTarget& target, const Ts3TabSelection& selection, uint64 ticket) {
    return !networkCancelled(ticket) && selection.handler == target.handler
        && target.handler == currentConnection.load()
        && sameTs3TabSelection(selection, currentTs3Tab());
}

static bool ts3TabActualOwns(const Ts3TabActualTarget& target, const Ts3TabSelection& selection, anyID& own, uint64 ticket) {
    if (!ts3TabActualSelected(target, selection, ticket)) return false;
    char expected[16]{};
    if (!ts3TabActualEnabled() || !expectedTestServer(expected) || std::strcmp(expected, target.ip)) return false;
    if (!connectionAt(target.handler, target.ip, own) || !knownCapture(target.handler)) return false;
    return ts3TabActualSelected(target, selection, ticket);
}

enum class Ts3TabActualInput { unavailable, pending, unsafe, quiet };

static Ts3TabActualInput ts3TabActualReadInput(uint64 handler, anyID client) {
    uint64 packets = ~uint64{0};
    if (api.getConnectionVariableAsUInt64(handler, client, CONNECTION_PACKETS_SENT_SPEECH, &packets))
        return Ts3TabActualInput::unavailable;
    if (packets != 0) return Ts3TabActualInput::unsafe;
    int state = -1;
    if (api.getClientSelfVariableAsInt(handler, CLIENT_INPUT_DEACTIVATED, &state)) return Ts3TabActualInput::unavailable;
    return state == INPUT_DEACTIVATED ? Ts3TabActualInput::quiet : Ts3TabActualInput::pending;
}

struct Ts3TabProofChallenge {
    std::string_view bindingId, serverUid, clientUid, nonce;
    unsigned int requestId{}, revision{};
};

static bool parseTs3TabProofChallenge(std::string_view row, Ts3TabProofChallenge& out) {
    if (!row.empty() && row.back() == '\n') {
        row.remove_suffix(1);
        if (!row.empty() && row.back() == '\r') row.remove_suffix(1);
    }
    const auto take = [&] {
        const auto split = row.find(' ');
        const auto token = row.substr(0, split);
        row.remove_prefix(split == row.npos ? row.size() : split + 1);
        return token;
    };
    const auto readInt = [&](unsigned int& result) {
        const auto token = take();
        if (token.empty() || token.front() < '1' || token.front() > '9') return false;
        for (const char c : token) {
            if (c < '0' || c > '9') return false;
            const auto digit = static_cast<unsigned int>(c - '0');
            if (result > (static_cast<unsigned int>(INT_MAX) - digit) / 10) return false;
            result = result * 10 + digit;
        }
        return true;
    };
    Ts3TabProofChallenge parsed{};
    if (take() != "RICON_TS3_PROOF_CHALLENGE_1") return false;
    parsed.bindingId = take();
    const auto id = parsed.bindingId;
    if (id.size() != 36 || id[14] != '4' || std::string_view("89ab").find(id[19]) == row.npos) return false;
    for (size_t i = 0; i < id.size(); ++i) {
        const char c = id[i];
        const bool dash = i == 8 || i == 13 || i == 18 || i == 23;
        if (dash ? c != '-' : (c < '0' || c > '9') && (c < 'a' || c > 'f')) return false;
    }
    if (!readInt(parsed.requestId) || !readInt(parsed.revision)) return false;
    parsed.serverUid = take();
    parsed.clientUid = take();
    parsed.nonce = take();
    if (row != "END" || !validTs3TabUid(parsed.serverUid) || !validTs3TabUid(parsed.clientUid)
        || parsed.nonce.size() != 22) return false;
    for (const char c : parsed.nonce) {
        if ((c < 'A' || c > 'Z') && (c < 'a' || c > 'z') && (c < '0' || c > '9') && c != '-' && c != '_') return false;
    }
    out = parsed;
    return true;
}

static bool ts3TabProofProfilePath(std::string_view path) {
    constexpr std::string_view tail[] = {"profile", "profile", "client", "ts3-binding-proof-runtime01", "remote-radio-01"};
    for (const auto expected : tail) {
        const auto split = path.find_last_of("/\\");
        if (split == path.npos || path.substr(split + 1) != expected) return false;
        path.remove_suffix(path.size() - split);
    }
    return !path.empty();
}

static bool ts3TabActualFileEquals(const char* path, std::string_view expected) {
    char row[384]{};
    size_t length{};
    return plainRead(path, row, sizeof(row), length) && std::string_view(row, length) == expected;
}

static bool submitTs3TabProof(const char* profile, const Ts3TabActualTarget& target, const Ts3TabSelection& selection,
                              anyID client, uint64 ticket, const Ts3TabQuery& query) {
    char ownerPath[4096]{}, responsePath[4096]{};
    Ts3TabIdentity identity{};
    if (!api.setClientSelfVariableAsString || !ts3TabProofProfilePath(profile)
        || !join(ownerPath, sizeof(ownerPath), profile, "ricon-ts3-proof-owner.txt")
        || !join(responsePath, sizeof(responsePath), profile, "ricon-ts3-tab-response.txt")
        || !readTs3TabIdentity(selection, identity) || identity.client != client) return false;

    char response[384]{};
    const int size = std::snprintf(response, sizeof(response), "RICON_TS3_TAB_1 %s %u %u %s %s END\n",
                                   query.nonce, query.sequence, selection.revision, identity.serverUid, identity.clientUid);
    if (size <= 0 || static_cast<size_t>(size) >= sizeof(response) || !ts3TabActualFileEquals(responsePath, response)) return false;

    const auto current = [&] {
        anyID own{};
        Ts3TabIdentity confirmed{};
        return ts3TabActualOwns(target, selection, own, ticket) && own == client
            && ts3TabActualReadInput(target.handler, client) == Ts3TabActualInput::quiet
            && readTs3TabIdentity(selection, confirmed) && sameTs3TabIdentity(identity, confirmed)
            && ts3TabActualSelected(target, selection, ticket);
    };
    const uint64 started = GetTickCount64();
    while (GetTickCount64() - started < 10000 && current()) {
        char row[384]{};
        size_t length{};
        Ts3TabProofChallenge challenge{};
        if (!plainRead(ownerPath, row, sizeof(row), length) || !parseTs3TabProofChallenge({row, length}, challenge)) {
            Sleep(20);
            continue;
        }
        if (challenge.requestId != query.sequence || challenge.revision != selection.revision
            || challenge.serverUid != identity.serverUid || challenge.clientUid != identity.clientUid) return false;

        char nickname[29] = "RICON-";
        std::memcpy(nickname + 6, challenge.nonce.data(), challenge.nonce.size());
        // Keep the owner challenge unchanged through the SDK submission.
        HANDLE pinned = CreateFileA(ownerPath, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
        if (pinned == INVALID_HANDLE_VALUE) return false;
        char confirmedProfile[4096]{};
        const bool applied = gameProfile(confirmedProfile) && !std::strcmp(profile, confirmedProfile)
            && ts3TabActualFileEquals(ownerPath, {row, length}) && ts3TabActualFileEquals(responsePath, response)
            && current() && GetTickCount64() - started < 10000
            && stateUpdateOk(api.setClientSelfVariableAsString(target.handler, CLIENT_NICKNAME, nickname)) && current()
            && stateUpdateOk(api.flushClientSelfUpdates(target.handler, nullptr)) && current();
        CloseHandle(pinned);
        return applied;
    }
    return false;
}

static void finishTs3TabActual(unsigned int result, const char* detail) {
    record("ts3-tab-actual-result", 0, result, detail);
    networkRunning.store(false);
}

static void runTs3TabActual(Ts3TabActualTarget target, uint64 ticket) {
    Ts3TabSelection selection{};
    anyID client{};
    const uint64 connecting = GetTickCount64();
    while (!networkCancelled(ticket) && GetTickCount64() - connecting < 15000) {
        const auto selected = currentTs3Tab();
        anyID own{};
        if (ts3TabActualOwns(target, selected, own, ticket)) { selection = selected; client = own; break; }
        Sleep(20);
    }
    if (!client) return finishTs3TabActual(1, networkCancelled(ticket) ? "cancelled" : "no established selected owned custom tab");
    record("ts3-tab-actual-connect", 0, 0, "selected fixture tab established on the private endpoint");

    auto current = [&] {
        anyID confirmed{};
        return ts3TabActualOwns(target, selection, confirmed, ticket) && confirmed == client;
    };
    if (!current() || !stateUpdateOk(api.setClientSelfVariableAsInt(target.handler, CLIENT_INPUT_DEACTIVATED, INPUT_DEACTIVATED))
        || !current() || !stateUpdateOk(api.flushClientSelfUpdates(target.handler, nullptr)))
        return finishTs3TabActual(1, "owned custom input was not deactivated");

    bool ready{};
    const uint64 deactivating = GetTickCount64();
    while (current() && GetTickCount64() - deactivating < 3000) {
        const auto observed = ts3TabActualReadInput(target.handler, client);
        if (observed == Ts3TabActualInput::unsafe) return finishTs3TabActual(1, "nonzero speech packet count");
        if (observed == Ts3TabActualInput::quiet && current()) { ready = true; break; }
        Sleep(20);
    }
    if (!ready) return finishTs3TabActual(1, "input or zero speech count unconfirmed");
    record("ts3-tab-actual-ready", 0, 0, "capture=known-custom input=deactivated speechPacketsSent=0");

    char profile[4096]{}, queryPath[4096]{};
    bool profileReady{};
    const uint64 waiting = GetTickCount64();
    while (!networkCancelled(ticket) && GetTickCount64() - waiting < 60000) {
        if (!current() || ts3TabActualReadInput(target.handler, client) != Ts3TabActualInput::quiet)
            return finishTs3TabActual(1, "owned tab or zero speech proof lost");
        if (!profileReady) {
            profileReady = gameProfile(profile) && join(queryPath, sizeof(queryPath), profile, "ricon-ts3-tab-query.txt");
            if (!profileReady) { Sleep(20); continue; }
            if (target.proof && !ts3TabProofProfilePath(profile)) return finishTs3TabActual(1, "wrong proof fixture game profile");
        }
        char bytes[96]{};
        size_t length{};
        Ts3TabQuery query{};
        if (plainRead(queryPath, bytes, sizeof(bytes), length) && parseTs3TabQuery({bytes, length}, query) && current()
            && ts3TabActualReadInput(target.handler, client) == Ts3TabActualInput::quiet
            && respondTs3Tab(profile, selection, ticket)) {
            const bool quiet = current() && ts3TabActualReadInput(target.handler, client) == Ts3TabActualInput::quiet && current();
            if (target.proof) {
                const bool submitted = quiet && submitTs3TabProof(profile, target, selection, client, ticket, query);
                record("ts3-tab-proof-actual-result", 0, submitted ? 0 : 1,
                       submitted ? "nickname-flush=1 capture=known-custom input=deactivated speechPacketsSent=0"
                                 : "challenge submission unconfirmed");
            }
            return finishTs3TabActual(quiet ? 0 : 1, quiet
                ? "published=1 capture=known-custom input=deactivated speechPacketsSent=0"
                : "published=1 final tab or zero speech proof lost");
        }
        Sleep(20);
    }
    if (networkCancelled(ticket)) return finishTs3TabActual(1, "cancelled");
    return finishTs3TabActual(1, profileReady ? "no current game query" : "game profile unavailable");
}

static void maybeBeginTs3TabActual(uint64 selected) {
    if (!ts3TabActualHostReady.load() || ts3TabActualTried.load() || !selected || !ts3TabActualEnabled()) return;
    if (!api.getConnectionStatus || selected != currentConnection.load() || selected != currentTs3Tab().handler) return;
    int status{};
    if (api.getConnectionStatus(selected, &status) || status != STATUS_CONNECTION_ESTABLISHED) return;

    // Selection and connection callbacks can both observe the established fixture tab.
    bool idle{};
    if (!ts3TabActualTried.compare_exchange_strong(idle, true)) return;
    if (!api.getClientID || !api.getConnectionVariableAsString || !api.getConnectionVariableAsUInt64 || !api.getServerVariableAsString
        || !api.getClientVariableAsString || !api.freeMemory || !api.getCurrentCaptureMode || !api.getCurrentCaptureDeviceName
        || !api.getClientSelfVariableAsInt || !api.setClientSelfVariableAsInt || !api.flushClientSelfUpdates
        || testWorker.joinable() || liveCleanupPending() || networkRunning.exchange(true)) {
        record("ts3-tab-actual-result", 0, 1, "required SDK methods or idle test worker unavailable");
        return;
    }
    const uint64 ticket = workerGeneration.load();
    if (ticket == ~uint64{0} || networkCancelled(ticket)) return finishTs3TabActual(1, "cancelled before worker startup");
    Ts3TabActualTarget target{};
    if (!expectedTestServer(target.ip)) return finishTs3TabActual(1, "isolated private server unavailable");
    target.handler = selected;
    target.proof = ts3TabActualStagePath(configDirectory, "ts3-binding-proof-01");
    try { testWorker = std::thread(runTs3TabActual, target, ticket); }
    catch (const std::exception&) { finishTs3TabActual(1, "worker not started"); }
}
