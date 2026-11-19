// Explicit alpha witness for an already selected normal tab. No capture or media authority is granted here.
enum class Ts3BindingRouteResult { waiting, complete, rejected };

struct Ts3BindingRouteProof {
    Ts3BindingRouteResult result{Ts3BindingRouteResult::waiting};
    bool started{}, answered{}, labelled{};
    uint64 ticket{}, deadline{};
    Ts3TabSelection selection{};
    Ts3TabIdentity identity{};
    Ts3TabQuery query{};
    char profile[4096]{}, challengePath[4096]{}, responsePath[4096]{}, queryPath[4096]{};
    char queryRow[96]{}, responseRow[384]{}, staleChallenge[384]{}, challengeRow[384]{};
    size_t querySize{}, staleSize{}, challengeSize{};
    char originalNickname[128]{}, label[29]{};
};

static bool ts3BindingRouteEnabled() {
    return ts3TabActualMarker("ricon-ts3-route.enabled")
        && ts3TabActualMarker("ricon-ts3-binding-proof.enabled");
}

static bool ts3BindingRouteReadNickname(uint64 handler, char (&nickname)[128]) {
    if (!api.getClientSelfVariableAsString || !api.freeMemory) return false;
    char* value{};
    const unsigned int result = api.getClientSelfVariableAsString(handler, CLIENT_NICKNAME, &value);
    if (result || !value) return false;
    size_t length{};
    while (length < sizeof(nickname) && value[length]) ++length;
    const bool valid = length > 0 && length < sizeof(nickname);
    if (valid) std::memcpy(nickname, value, length + 1);
    api.freeMemory(value);
    return valid;
}

// Cleanup may target the old tab after selection changed, but never a different actor/server on its reused handler.
static bool ts3BindingRouteSameOwner(const Ts3BindingRouteProof& proof) {
    anyID client{};
    char server[128]{}, uid[128]{};
    if (!readTs3TabClient(proof.selection.handler, client) || client != proof.identity.client
        || !serverUid(proof.selection.handler, server) || std::strcmp(server, proof.identity.serverUid)) return false;
    char* value{};
    const auto result = api.getClientVariableAsString(proof.selection.handler, client, CLIENT_UNIQUE_IDENTIFIER, &value);
    return copyTs3TabUid(result, value, uid) && !std::strcmp(uid, proof.identity.clientUid);
}

static bool ts3BindingRouteCurrent(const Ts3BindingRouteProof& proof) {
    char profile[4096]{};
    Ts3TabIdentity identity{};
    return GetTickCount64() < proof.deadline && !networkCancelled(proof.ticket) && ts3BindingRouteEnabled()
        && currentConnection.load() == proof.selection.handler && sameTs3TabSelection(proof.selection, currentTs3Tab())
        && gameProfile(profile) && !std::strcmp(profile, proof.profile)
        && readTs3TabIdentity(proof.selection, identity) && sameTs3TabIdentity(identity, proof.identity)
        && knownCapture(proof.selection.handler)
        && ts3TabActualReadInput(proof.selection.handler, identity.client) == Ts3TabActualInput::quiet
        && !networkCancelled(proof.ticket) && GetTickCount64() < proof.deadline
        && sameTs3TabSelection(proof.selection, currentTs3Tab());
}

static bool ts3BindingRouteQueryUnchanged(const Ts3BindingRouteProof& proof) {
    const DWORD attributes = GetFileAttributesA(proof.queryPath);
    if (attributes == INVALID_FILE_ATTRIBUTES) {
        const DWORD error = GetLastError();
        return error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND;
    }
    return ts3TabActualFileEquals(proof.queryPath, {proof.queryRow, proof.querySize});
}

static void finishTs3BindingRouteProof(Ts3BindingRouteProof& proof) {
	if (proof.result == Ts3BindingRouteResult::waiting) proof.result = Ts3BindingRouteResult::rejected;
    if (!proof.labelled) return;
    proof.labelled = false;
    char nickname[128]{};
    if (!ts3BindingRouteSameOwner(proof) || !ts3BindingRouteReadNickname(proof.selection.handler, nickname)
        || std::strcmp(nickname, proof.label) || !ts3BindingRouteSameOwner(proof)
        || !ts3BindingRouteReadNickname(proof.selection.handler, nickname) || std::strcmp(nickname, proof.label)) {
        record("ts3-binding-label-restore", proof.selection.handler, 1, "skipped: owner unavailable or nickname changed");
        return;
    }
    const unsigned int changed = api.setClientSelfVariableAsString(proof.selection.handler, CLIENT_NICKNAME, proof.originalNickname);
    const unsigned int flushed = stateUpdateOk(changed) && ts3BindingRouteSameOwner(proof)
        ? api.flushClientSelfUpdates(proof.selection.handler, nullptr) : ERROR_undefined;
    record("ts3-binding-label-restore", proof.selection.handler,
           stateUpdateOk(changed) && stateUpdateOk(flushed) ? 0 : 1, "only the unchanged owned proof nickname");
}

static Ts3BindingRouteResult rejectTs3BindingRouteProof(Ts3BindingRouteProof& proof, const char* reason) {
    finishTs3BindingRouteProof(proof);
    proof.result = Ts3BindingRouteResult::rejected;
    record("ts3-binding-proof-result", proof.selection.handler, 1, reason);
    return proof.result;
}

static bool matchesTs3BindingRouteChallenge(const Ts3TabProofChallenge& challenge, const Ts3BindingRouteProof& proof) {
    return challenge.requestId == proof.query.sequence && challenge.revision == proof.selection.revision
        && challenge.serverUid == proof.identity.serverUid && challenge.clientUid == proof.identity.clientUid;
}

//! Worker-owned state; call until complete/rejected, and always call finish on worker exit.
//! complete means the witness was consumed and cleanup attempted, not that a server granted a route.
static Ts3BindingRouteResult serviceTs3BindingRouteProof(const char* profile, const Ts3TabSelection& selection,
                                                         uint64 ticket, Ts3BindingRouteProof& proof) {
    if (proof.result != Ts3BindingRouteResult::waiting) return proof.result;
    if (!proof.started) {
        if (!profile || !profile[0] || std::strlen(profile) >= sizeof(proof.profile) || !selection.handler || !selection.revision
            || !api.getClientSelfVariableAsString || !api.setClientSelfVariableAsString || !api.flushClientSelfUpdates
            || !api.getConnectionVariableAsUInt64 || !api.getClientSelfVariableAsInt || !api.getCurrentCaptureMode
            || !api.getCurrentCaptureDeviceName || !api.getClientVariableAsString || !api.freeMemory)
            return rejectTs3BindingRouteProof(proof, "unavailable alpha proof API/profile");
        proof.started = true;
        proof.ticket = ticket;
        proof.selection = selection;
        proof.deadline = GetTickCount64() + 10000;
        strcpy_s(proof.profile, profile);
        if (!join(proof.challengePath, sizeof(proof.challengePath), profile, "ricon-ts3-binding-challenge.txt")
            || !join(proof.queryPath, sizeof(proof.queryPath), profile, "ricon-ts3-tab-query.txt")
            || !join(proof.responsePath, sizeof(proof.responsePath), profile, "ricon-ts3-tab-response.txt")
            || !readTs3TabIdentity(selection, proof.identity))
            return rejectTs3BindingRouteProof(proof, "selected tab identity unavailable");
    }
    if (ticket != proof.ticket || !sameTs3TabSelection(selection, proof.selection) || !profile
        || std::strcmp(profile, proof.profile) || !ts3BindingRouteCurrent(proof))
        return rejectTs3BindingRouteProof(proof, "proof scope lost or original deadline expired");

    if (!proof.answered) {
        if (!plainRead(proof.queryPath, proof.queryRow, sizeof(proof.queryRow), proof.querySize)
            || !parseTs3TabQuery({proof.queryRow, proof.querySize}, proof.query)) return proof.result;
        // An identical challenge already present before this reply is stale, even if its request number matches.
        const DWORD prior = GetFileAttributesA(proof.challengePath);
        if (prior != INVALID_FILE_ATTRIBUTES) {
            if (!plainRead(proof.challengePath, proof.staleChallenge, sizeof(proof.staleChallenge), proof.staleSize))
                return rejectTs3BindingRouteProof(proof, "preexisting challenge is unreadable");
        } else {
            const DWORD error = GetLastError();
            if (error != ERROR_FILE_NOT_FOUND && error != ERROR_PATH_NOT_FOUND)
                return rejectTs3BindingRouteProof(proof, "preexisting challenge status unavailable");
        }
        const int count = std::snprintf(proof.responseRow, sizeof(proof.responseRow), "RICON_TS3_TAB_1 %s %u %u %s %s END\n",
                                        proof.query.nonce, proof.query.sequence, proof.selection.revision,
                                        proof.identity.serverUid, proof.identity.clientUid);
        if (count <= 0 || static_cast<size_t>(count) >= sizeof(proof.responseRow)
            || !respondTs3Tab(profile, selection, ticket) || !ts3BindingRouteCurrent(proof)
            || !ts3TabActualFileEquals(proof.responsePath, {proof.responseRow, static_cast<size_t>(count)})
            || !ts3BindingRouteQueryUnchanged(proof))
            return rejectTs3BindingRouteProof(proof, "exact local TAB reply not published");
        proof.answered = true;
        return proof.result;
    }

    if (!ts3BindingRouteQueryUnchanged(proof)
        || !ts3TabActualFileEquals(proof.responsePath, proof.responseRow))
        return rejectTs3BindingRouteProof(proof, "local TAB query/reply replaced");
    const DWORD attributes = GetFileAttributesA(proof.challengePath);
    if (attributes == INVALID_FILE_ATTRIBUTES) {
        const DWORD error = GetLastError();
        if (error != ERROR_FILE_NOT_FOUND && error != ERROR_PATH_NOT_FOUND)
            return rejectTs3BindingRouteProof(proof, "challenge status unavailable");
        if (proof.labelled) {
            finishTs3BindingRouteProof(proof);
            proof.result = Ts3BindingRouteResult::complete;
            record("ts3-binding-proof-result", proof.selection.handler, 0, "challenge disappeared; server route authority remains separate");
        }
        return proof.result;
    }
    char row[384]{};
    size_t size{};
    if (!plainRead(proof.challengePath, row, sizeof(row), size))
        return rejectTs3BindingRouteProof(proof, "challenge unreadable");
    if (proof.labelled) {
        char nickname[128]{};
        if (std::string_view(row, size) != std::string_view(proof.challengeRow, proof.challengeSize)
            || !ts3BindingRouteReadNickname(proof.selection.handler, nickname) || std::strcmp(nickname, proof.label))
            return rejectTs3BindingRouteProof(proof, "challenge or owned nickname changed");
        return proof.result;
    }
    if (proof.staleSize && std::string_view(row, size) == std::string_view(proof.staleChallenge, proof.staleSize)) return proof.result;
    Ts3TabProofChallenge challenge{};
    if (!parseTs3TabProofChallenge({row, size}, challenge) || !matchesTs3BindingRouteChallenge(challenge, proof))
        return rejectTs3BindingRouteProof(proof, "foreign or malformed proof challenge");
    if (!ts3BindingRouteReadNickname(proof.selection.handler, proof.originalNickname) || !ts3BindingRouteCurrent(proof))
        return rejectTs3BindingRouteProof(proof, "original nickname unavailable");
    strcpy_s(proof.label, "RICON-");
    std::memcpy(proof.label + 6, challenge.nonce.data(), 22);
    std::memcpy(proof.challengeRow, row, size);
    proof.challengeSize = size;
    HANDLE pinned = CreateFileA(proof.challengePath, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
    if (pinned == INVALID_HANDLE_VALUE) return rejectTs3BindingRouteProof(proof, "challenge pin failed");
    char unchangedNickname[128]{};
    const bool applicable = ts3TabActualFileEquals(proof.challengePath, {row, size}) && ts3BindingRouteCurrent(proof)
        && ts3BindingRouteQueryUnchanged(proof) && ts3TabActualFileEquals(proof.responsePath, proof.responseRow)
        && ts3BindingRouteReadNickname(proof.selection.handler, unchangedNickname)
        && !std::strcmp(unchangedNickname, proof.originalNickname);
    unsigned int changed = ERROR_undefined, flushed = ERROR_undefined;
    if (applicable) {
        // Track ownership before the call so error/late-cancel cleanup still restores an actually applied label.
        proof.labelled = true;
        changed = api.setClientSelfVariableAsString(proof.selection.handler, CLIENT_NICKNAME, proof.label);
        if (stateUpdateOk(changed) && ts3BindingRouteCurrent(proof)) flushed = api.flushClientSelfUpdates(proof.selection.handler, nullptr);
    }
    const bool applied = applicable && stateUpdateOk(changed) && stateUpdateOk(flushed) && ts3BindingRouteCurrent(proof)
        && ts3BindingRouteQueryUnchanged(proof) && ts3TabActualFileEquals(proof.responsePath, proof.responseRow);
    CloseHandle(pinned);
    if (!applied) return rejectTs3BindingRouteProof(proof, "bounded proof label submission failed");
    record("ts3-binding-label", proof.selection.handler, 0, "bounded P39 label submitted; input remains deactivated known PCM");
    return proof.result;
}
