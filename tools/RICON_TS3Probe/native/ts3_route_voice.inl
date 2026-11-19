// First normal-tab media adapter: one server route, one bounded known-PCM burst.
struct Ts3RoutePeer { char uid[128]{}; anyID client{}; };
struct Ts3RouteGrant {
    unsigned int sequence{}, session{};
    bool active{};
    char domain[37]{}, bindingId[37]{}, serverUid[128]{}, sourceUid[128]{};
    unsigned int epoch{}, player{}, connection{}, generation{}, entity{}, policyRevision{}, device{}, index{};
    unsigned int txGeneration{}, network{}, channel{}, hold{}, startRequest{}, lease{}, bindingRequest{}, tabRevision{};
    anyID sourceClient{};
    unsigned int remainingMs{}, count{};
    Ts3RoutePeer recipients[64]{};
};
static constexpr size_t ts3RouteMaxRow = 12288;
static std::atomic<uint64> ts3RouteCaptureHandler{0}, ts3RouteCaptureTicket{0}, ts3RouteCaptureDeadline{0};
static std::atomic<uint64> ts3RouteRevocation{1}, ts3RouteCaptureRevision{0};
static unsigned int ts3RouteSequence{};

static bool ts3RouteUuid(std::string_view value) {
    if (value.size() != 36 || value[14] != '4' || std::string_view("89ab").find(value[19]) == value.npos) return false;
    for (size_t i = 0; i < value.size(); ++i) {
        const char c = value[i];
        const bool dash = i == 8 || i == 13 || i == 18 || i == 23;
        if (dash ? c != '-' : (c < '0' || c > '9') && (c < 'a' || c > 'f')) return false;
    }
    return true;
}

static bool parseTs3Route(std::string_view row, std::string_view nonce, unsigned int sequence, Ts3RouteGrant& out) {
    if (row.empty() || row.size() > ts3RouteMaxRow || row.find('\0') != row.npos || nonce.size() != 32) return false;
    for (char c : nonce) if ((c < '0' || c > '9') && (c < 'a' || c > 'f')) return false;
    if (row.back() == '\n') {
        row.remove_suffix(1);
        if (!row.empty() && row.back() == '\r') row.remove_suffix(1);
    }
    const auto take = [&] {
        const auto space = row.find(' ');
        const auto token = row.substr(0, space);
        row.remove_prefix(space == row.npos ? row.size() : space + 1);
        return token;
    };
    const auto integer = [&](unsigned int& value, bool zero = false) {
        const auto token = take();
        if (token.empty() || (token.size() > 1 && token.front() == '0')) return false;
        value = 0;
        for (const char c : token) {
            if (c < '0' || c > '9') return false;
            const auto digit = static_cast<unsigned int>(c - '0');
            if (value > (static_cast<unsigned int>(INT_MAX) - digit) / 10) return false;
            value = value * 10 + digit;
        }
        return value || zero;
    };
    const auto text = [&](char* destination, size_t capacity, bool uuid = false) {
        const auto token = take();
        if (token.size() >= capacity || !(uuid ? ts3RouteUuid(token) : validTs3TabUid(token))) return false;
        std::memcpy(destination, token.data(), token.size());
        return true;
    };
    Ts3RouteGrant parsed{};
    unsigned int active{};
    if (take() != "RICON_TS3_ROUTE_1" || take() != nonce || !integer(parsed.sequence) || parsed.sequence != sequence
        || !integer(parsed.session) || !integer(active, true) || active > 1) return false;
    parsed.active = active == 1;
    if (parsed.active) {
        unsigned int source{};
        if (!text(parsed.domain, sizeof(parsed.domain), true) || !integer(parsed.epoch) || !integer(parsed.player)
            || !integer(parsed.connection) || !integer(parsed.generation) || !integer(parsed.entity) || !integer(parsed.policyRevision)
            || !integer(parsed.device) || !integer(parsed.index, true) || !integer(parsed.txGeneration)
            || !integer(parsed.network) || !integer(parsed.channel) || !integer(parsed.hold) || !integer(parsed.startRequest)
            || !integer(parsed.lease) || !text(parsed.bindingId, sizeof(parsed.bindingId), true) || !integer(parsed.bindingRequest)
            || !integer(parsed.tabRevision) || !text(parsed.serverUid, sizeof(parsed.serverUid))
            || !text(parsed.sourceUid, sizeof(parsed.sourceUid)) || !integer(source) || source > 65535
            || !integer(parsed.remainingMs) || parsed.remainingMs > 500 || !integer(parsed.count) || parsed.count > 64) return false;
        parsed.sourceClient = static_cast<anyID>(source);
        for (unsigned int i = 0; i < parsed.count; ++i) {
            auto& target = parsed.recipients[i];
            unsigned int client{};
            if (!text(target.uid, sizeof(target.uid)) || !integer(client) || client > 65535 || client == source
                || !std::strcmp(target.uid, parsed.sourceUid)) return false;
            target.client = static_cast<anyID>(client);
            for (unsigned int j = 0; j < i; ++j)
                if (target.client == parsed.recipients[j].client || !std::strcmp(target.uid, parsed.recipients[j].uid)) return false;
        }
    }
    if (row != "END") return false;
    out = parsed;
    return true;
}

static bool ts3RouteEnabled() { return ts3TabActualMarker("ricon-ts3-route.enabled"); }
static void revokeTs3Route() {
    ts3RouteCaptureDeadline.store(0);
    uint64 revision = ts3RouteRevocation.load();
    while (revision != ~uint64{0} && !ts3RouteRevocation.compare_exchange_weak(revision, revision + 1)) {}
}

static bool ts3RouteTimeCurrent(const Ts3RouteGrant& grant, uint64 issued, uint64 now) {
    return grant.active && grant.remainingMs && grant.remainingMs <= 500 && now >= issued && now - issued < grant.remainingMs;
}

static bool ts3RouteHasBlockBudget(uint64 now, uint64 deadline) {
    return now < deadline && deadline - now > 20;
}

static bool ts3RouteCaptureAllowed(uint64 connection, uint64 now) {
    const uint64 deadline = ts3RouteCaptureDeadline.load();
    return connection && connection == ts3RouteCaptureHandler.load() && deadline && now < deadline
        && ts3RouteCaptureRevision.load() == ts3RouteRevocation.load()
        && ts3RouteCaptureRevision.load() != ~uint64{0} && !networkCancelled(ts3RouteCaptureTicket.load());
}

static bool publishTs3RouteCapture(bool acknowledged, uint64 revision, uint64 ticket, uint64 deadline, uint64 now) {
    if (!acknowledged || !revision || revision == ~uint64{0} || revision != ts3RouteRevocation.load()
        || now >= deadline || networkCancelled(ticket)) return false;
    ts3RouteCaptureRevision.store(revision);
    ts3RouteCaptureTicket.store(ticket);
    ts3RouteCaptureDeadline.store(deadline);
    return revision == ts3RouteRevocation.load() && !networkCancelled(ticket);
}

static bool editTs3RouteCapture(uint64 connection, short* samples, int frames, int channels, int* edited, uint64 now) {
    if (!connection || connection != ts3RouteCaptureHandler.load()) return false;
    if (!edited) return true;
    if (!samples || frames <= 0 || frames > 5760 || channels < 1 || channels > 2 || !ts3RouteCaptureAllowed(connection, now)) {
        *edited &= ~2;
        if (samples && frames > 0 && frames <= 5760 && channels >= 1 && channels <= 2) {
            std::memset(samples, 0, static_cast<size_t>(frames) * channels * sizeof(short));
            *edited |= 1;
        }
    }
    return true;
}

static bool ts3RouteSource(const Ts3TabSelection& selection, const Ts3TabIdentity& source, uint64 ticket) {
    Ts3TabIdentity current{};
    return !networkCancelled(ticket) && ts3RouteEnabled() && sameTs3TabSelection(selection, currentTs3Tab())
        && currentConnection.load() == selection.handler && api.getCurrentServerConnectionHandlerID() == selection.handler
        && readTs3TabIdentity(selection, current) && sameTs3TabIdentity(source, current) && knownCapture(selection.handler);
}

static bool ts3RouteOwns(uint64 handler, const Ts3TabIdentity& source) {
    anyID actual{};
    if (!readTs3TabClient(handler, actual) || actual != source.client) return false;
    char current[128]{};
    char* value{};
    const auto serverResult = api.getServerVariableAsString(handler, VIRTUALSERVER_UNIQUE_IDENTIFIER, &value);
    if (!copyTs3TabUid(serverResult, value, current) || std::strcmp(current, source.serverUid)) return false;
    value = nullptr;
    const auto clientResult = api.getClientVariableAsString(handler, actual, CLIENT_UNIQUE_IDENTIFIER, &value);
    if (!copyTs3TabUid(clientResult, value, current) || std::strcmp(current, source.clientUid)) return false;
    return readTs3TabClient(handler, actual) && actual == source.client && knownCapture(handler);
}

static bool ts3RouteCurrent(const Ts3TabSelection& selection, const Ts3TabIdentity& source,
                            const Ts3RouteGrant& grant, uint64 deadline, uint64 ticket) {
    if (!grant.active || GetTickCount64() >= deadline || grant.tabRevision != selection.revision
        || grant.sourceClient != source.client || std::strcmp(grant.sourceUid, source.clientUid)
        || std::strcmp(grant.serverUid, source.serverUid) || !ts3RouteSource(selection, source, ticket)) return false;
    for (unsigned int i = 0; i < grant.count; ++i) {
        char uid[128]{};
        if (!whisperClientUid(selection.handler, grant.recipients[i].client, uid)
            || std::strcmp(uid, grant.recipients[i].uid)) return false;
    }
    return GetTickCount64() < deadline && !networkCancelled(ticket);
}

static bool ts3RouteReplyUnchanged(const char* responsePath, const char* queryPath, std::string_view response,
                                  std::string_view query) {
    char actual[ts3RouteMaxRow + 1]{};
    size_t length{};
    if (!plainRead(queryPath, actual, sizeof(actual), length) || std::string_view(actual, length) != query) return false;
    return plainRead(responsePath, actual, sizeof(actual), length) && std::string_view(actual, length) == response;
}

static bool deactivateTs3Route(uint64 handler) {
    if (!stateUpdateOk(api.setClientSelfVariableAsInt(handler, CLIENT_INPUT_DEACTIVATED, INPUT_DEACTIVATED))
        || !stateUpdateOk(api.flushClientSelfUpdates(handler, nullptr))) return false;
    int state{};
    return !api.getClientSelfVariableAsInt(handler, CLIENT_INPUT_DEACTIVATED, &state) && state == INPUT_DEACTIVATED;
}

static void runTs3Route(Ts3TabSelection selection, Ts3TabIdentity source, uint64 ticket, uint64 revision, unsigned int sequence) {
    char profile[4096]{}, responsePath[4096]{}, queryPath[4096]{}, nonce[33]{}, query[96]{}, response[ts3RouteMaxRow + 1]{};
    size_t responseSize{};
    Ts3RouteGrant grant{};
    uint64 deadline{};
    unsigned int submitted{};
    const char* outcome = "SETUP_FAILED";
    bool routeSubmitted{};
    Ts3BindingRouteProof proof{};
    do {
        if (!ts3RouteSource(selection, source, ticket) || !deactivateTs3Route(selection.handler) || !gameProfile(profile)
            || !join(responsePath, sizeof(responsePath), profile, "ricon-ts3-route-response.txt")
            || !join(queryPath, sizeof(queryPath), profile, "ricon-ts3-route-query.txt") || !makeGameNonce(nonce)) break;
        outcome = "BINDING_PROOF_UNAVAILABLE";
        if (!ts3BindingRouteEnabled()) break;
        while (proof.result == Ts3BindingRouteResult::waiting && revision == ts3RouteRevocation.load()
            && ts3RouteSource(selection, source, ticket)) {
            serviceTs3BindingRouteProof(profile, selection, ticket, proof);
            if (proof.result == Ts3BindingRouteResult::waiting) Sleep(20);
        }
        if (proof.result != Ts3BindingRouteResult::complete || revision != ts3RouteRevocation.load()
            || !ts3RouteSource(selection, source, ticket)) break;
        const int querySize = std::snprintf(query, sizeof(query), "RICON_TS3_ROUTE_QUERY_1 %s %u END\n", nonce, sequence);
        if (querySize <= 0 || static_cast<size_t>(querySize) >= sizeof(query)) break;
        const uint64 issued = GetTickCount64();
        if (!writeProfileRequest(profile, "ricon-ts3-route-query.txt", "ricon-ts3-route-query.tmp",
                                 query, static_cast<size_t>(querySize), ticket)) break;
        outcome = "NO_FRESH_ROUTE";
        while (GetTickCount64() - issued < 500 && ts3RouteSource(selection, source, ticket)) {
            if (plainRead(responsePath, response, sizeof(response), responseSize)
                && parseTs3Route({response, responseSize}, nonce, sequence, grant)) break;
            Sleep(5);
        }
        if (!grant.session) break;
        if (!grant.active) { outcome = "INACTIVE"; break; }
        deadline = issued + grant.remainingMs;
        const auto current = [&] {
            return revision == ts3RouteRevocation.load() && ts3RouteCurrent(selection, source, grant, deadline, ticket)
                && ts3RouteReplyUnchanged(responsePath, queryPath, {response, responseSize}, {query, static_cast<size_t>(querySize)});
        };
        if (!ts3RouteTimeCurrent(grant, issued, GetTickCount64()) || !current()) { outcome = "STALE_OR_FOREIGN_ROUTE"; break; }
        anyID targets[65]{};
        for (unsigned int i = 0; i < grant.count; ++i) targets[i] = grant.recipients[i].client;
        char code[128]{};
        uint64 routeIssued{};
        // Keep correlated negative errors observable through cleanup, while still requiring a positive ACK before PCM.
        if (!prepareWhisperAck(selection.handler, ticket, code, routeIssued, true) || !current()) break;
        const unsigned int requested = api.requestClientSetWhisperList(selection.handler, source.client, nullptr, targets, code);
        routeSubmitted = requested == ERROR_ok;
        bool acknowledged{};
        while (routeSubmitted && current()) {
            {
                std::lock_guard<std::mutex> lock(whisperAckMutex);
                acknowledged = whisperAck.seen && whisperAck.error == ERROR_ok;
                if (whisperAck.seen) break;
            }
            Sleep(1);
        }
        if (!acknowledged || !current()) { outcome = "ROUTE_ACK_MISSING_OR_EXPIRED"; break; }
        if (!publishTs3RouteCapture(acknowledged, revision, ticket, deadline, GetTickCount64()) || !current()
            || !stateUpdateOk(api.activateCaptureDevice(selection.handler)) || !current()
            || !stateUpdateOk(api.setClientSelfVariableAsInt(selection.handler, CLIENT_INPUT_DEACTIVATED, INPUT_ACTIVE))
            || !current() || !stateUpdateOk(api.flushClientSelfUpdates(selection.handler, nullptr))) break;
        short tone[960]{};
        constexpr double cycle = 6.28318530717958647692;
        for (int i = 0; i < 960; ++i) tone[i] = static_cast<short>(std::lround(1024 * std::sin(cycle * i / 80.0)));
        const uint64 started = GetTickCount64();
        while (submitted < 10 && current()) {
            const uint64 now = GetTickCount64();
            if (!ts3RouteHasBlockBudget(now, deadline) || !ts3RouteCaptureAllowed(selection.handler, now)
                || api.processCustomCaptureData(knownCaptureId, tone, 960)) break;
            ++submitted;
            while (GetTickCount64() - started < static_cast<uint64>(submitted) * 20 && current()) Sleep(1);
        }
        outcome = submitted ? "KNOWN_PCM_SUBMITTED" : "NO_PCM_BUDGET";
    } while (false);
    revokeTs3Route();
    finishTs3BindingRouteProof(proof);
    bool closed{}, parked{};
    if (ts3RouteOwns(selection.handler, source)) {
        closed = deactivateTs3Route(selection.handler);
        if (closed && routeSubmitted) {
            const uint64 noChannels[] = {0};
            const anyID noClients[] = {0};
            parked = api.requestClientSetWhisperList(selection.handler, source.client, noChannels, noClients, nullptr) == ERROR_ok;
        }
    }
    {
        std::lock_guard<std::mutex> lock(whisperAckMutex);
        if (whisperAck.ticket == ticket) whisperAck.pending = false;
    }
    char detail[208]{};
    std::snprintf(detail, sizeof(detail), "outcome=%s sequence=%u session=%u recipients=%u submittedBlocks=%u inputClosed=%u parkSubmitted=%u source=known-custom-PCM",
                  outcome, sequence, grant.session, grant.count, submitted, static_cast<unsigned int>(closed), static_cast<unsigned int>(parked));
    record("ts3-route-result", selection.handler, closed && submitted && !networkCancelled(ticket) ? 0 : 1, detail);
    networkRunning.store(false);
}

static bool beginTs3Route(uint64 handler) {
    const auto selection = currentTs3Tab();
    Ts3TabIdentity source{};
    const uint64 ticket = workerGeneration.load();
    if (!handler || handler != selection.handler || ticket == ~uint64{0} || !ts3RouteEnabled() || !knownCaptureRegistered
        || !api.getCurrentServerConnectionHandlerID || !api.getClientSelfVariableAsInt || !api.setClientSelfVariableAsInt
        || !api.flushClientSelfUpdates || !api.requestClientSetWhisperList || !api.createReturnCode
        || !api.activateCaptureDevice || !api.processCustomCaptureData || liveCleanupPending()
        || !readTs3TabIdentity(selection, source) || !knownCapture(handler) || networkRunning.exchange(true)) return false;
    const auto fail = [] { networkRunning.store(false); return false; };
    if (testWorker.joinable()) {
        if (WaitForSingleObject(testWorker.native_handle(), 0) != WAIT_OBJECT_0) return fail();
        testWorker.join();
    }
    int input{};
    if (ts3RouteSequence == INT_MAX || api.getClientSelfVariableAsInt(handler, CLIENT_INPUT_DEACTIVATED, &input)
        || input != INPUT_DEACTIVATED || (ts3RouteCaptureHandler.load() && ts3RouteCaptureHandler.load() != handler)) return fail();
    workerStop.store(false);
    revokeTs3Route();
    const uint64 revision = ts3RouteRevocation.load();
    if (revision == ~uint64{0}) return fail();
    ts3RouteCaptureHandler.store(handler);
    try { testWorker = std::thread(runTs3Route, selection, source, ticket, revision, ++ts3RouteSequence); }
    catch (const std::exception&) { return fail(); }
    return true;
}
