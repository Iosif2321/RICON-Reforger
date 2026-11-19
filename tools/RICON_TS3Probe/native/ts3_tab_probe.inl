// One explicit metadata reply for the selected normal TS3 tab.
struct Ts3TabQuery {
    char nonce[33]{};
    unsigned int sequence{};
};

static bool parseTs3TabQuery(std::string_view row, Ts3TabQuery& out) {
    constexpr std::string_view prefix = "RICON_TS3_TAB_QUERY_1 ", suffix = " END";
    if (!row.empty() && row.back() == '\n') {
        row.remove_suffix(1);
        if (!row.empty() && row.back() == '\r') row.remove_suffix(1);
    }
    if (row.size() < prefix.size() + 34 + suffix.size() || row.substr(0, prefix.size()) != prefix
        || row.substr(row.size() - suffix.size()) != suffix) return false;
    row.remove_prefix(prefix.size());
    row.remove_suffix(suffix.size());
    for (const char digit : row.substr(0, 32)) {
        if ((digit < '0' || digit > '9') && (digit < 'a' || digit > 'f')) return false;
    }
    if (row[32] != ' ' || row[33] < '1' || row[33] > '9') return false;

    unsigned int sequence{};
    for (const char digit : row.substr(33)) {
        if (digit < '0' || digit > '9') return false;
        const auto value = static_cast<unsigned int>(digit - '0');
        if (sequence > (static_cast<unsigned int>(INT_MAX) - value) / 10) return false;
        sequence = sequence * 10 + value;
    }
    out = {};
    std::memcpy(out.nonce, row.data(), 32);
    out.sequence = sequence;
    return true;
}

static bool validTs3TabUid(std::string_view uid) {
    if (uid.empty() || uid.size() > 127) return false;
    for (const char c : uid) {
        if ((c < 'A' || c > 'Z') && (c < 'a' || c > 'z') && (c < '0' || c > '9')
            && c != '+' && c != '/' && c != '_' && c != '=' && c != '-') return false;
    }
    return true;
}

struct Ts3TabSelection { uint64 handler{}; unsigned int revision{}; };
struct Ts3TabIdentity { anyID client{}; char serverUid[128]{}, clientUid[128]{}; };

static std::mutex ts3TabSelectionMutex;
static Ts3TabSelection selectedTs3Tab{};

// The selection mutex fences the final rename against TS3 callbacks.
static void advanceTs3TabSelection(uint64 handler) {
    if (selectedTs3Tab.revision == static_cast<unsigned int>(INT_MAX)) selectedTs3Tab.handler = 0;
    else selectedTs3Tab = {handler, selectedTs3Tab.revision + 1};
}

static void selectTs3Tab(uint64 handler) {
    std::lock_guard<std::mutex> lock(ts3TabSelectionMutex);
    advanceTs3TabSelection(handler);
}

static void invalidateTs3Tab(uint64 handler) {
    std::lock_guard<std::mutex> lock(ts3TabSelectionMutex);
    if (handler && selectedTs3Tab.handler == handler) advanceTs3TabSelection(handler);
}

static Ts3TabSelection currentTs3Tab() {
    std::lock_guard<std::mutex> lock(ts3TabSelectionMutex);
    return selectedTs3Tab;
}

static bool sameTs3TabSelection(const Ts3TabSelection& a, const Ts3TabSelection& b) {
    return a.handler && a.revision && a.handler == b.handler && a.revision == b.revision;
}

static bool readTs3TabClient(uint64 handler, anyID& client) {
    int status{};
    return api.getConnectionStatus(handler, &status) == ERROR_ok && status == STATUS_CONNECTION_ESTABLISHED
        && api.getClientID(handler, &client) == ERROR_ok && client;
}

static bool copyTs3TabUid(unsigned int result, char* value, char (&uid)[128]) {
    if (result != ERROR_ok || !value) return false;
    size_t length{};
    while (length < sizeof(uid) && value[length]) ++length;
    const bool valid = validTs3TabUid({value, length});
    if (valid) std::memcpy(uid, value, length + 1);
    api.freeMemory(value);
    return valid;
}

static bool readTs3TabIdentity(const Ts3TabSelection& selection, Ts3TabIdentity& out) {
    if (!sameTs3TabSelection(selection, currentTs3Tab()) || !api.getConnectionStatus || !api.getClientID
        || !api.getServerVariableAsString || !api.getClientVariableAsString || !api.freeMemory) return false;

    Ts3TabIdentity identity{};
    const uint64 handler = selection.handler;
    if (!readTs3TabClient(handler, identity.client)) return false;
    char* value{};
    const auto serverResult = api.getServerVariableAsString(handler, VIRTUALSERVER_UNIQUE_IDENTIFIER, &value);
    if (!copyTs3TabUid(serverResult, value, identity.serverUid)) return false;
    value = nullptr;
    const auto clientResult = api.getClientVariableAsString(handler, identity.client, CLIENT_UNIQUE_IDENTIFIER, &value);
    if (!copyTs3TabUid(clientResult, value, identity.clientUid)) return false;
    anyID confirmedClient{};
    if (!readTs3TabClient(handler, confirmedClient) || identity.client != confirmedClient
        || !sameTs3TabSelection(selection, currentTs3Tab())) return false;
    out = identity;
    return true;
}

static bool sameTs3TabIdentity(const Ts3TabIdentity& a, const Ts3TabIdentity& b) {
    return a.client == b.client && !std::strcmp(a.serverUid, b.serverUid) && !std::strcmp(a.clientUid, b.clientUid);
}

static bool publishTs3Tab(const char* temporary, const char* responsePath, const char* queryPath,
                          std::string_view original, const Ts3TabSelection& selection, uint64 ticket) {
    // Keep the exact query readable and unchanged through the response rename.
    HANDLE query = CreateFileA(queryPath, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
    if (query == INVALID_HANDLE_VALUE) return false;
    char current[96]{};
    size_t size{};
    bool published{};
    if (plainRead(queryPath, current, sizeof(current), size) && std::string_view(current, size) == original) {
        std::lock_guard<std::mutex> lock(ts3TabSelectionMutex);
        const DWORD attributes = GetFileAttributesA(responsePath);
        published = !networkCancelled(ticket) && sameTs3TabSelection(selection, selectedTs3Tab)
            && (attributes == INVALID_FILE_ATTRIBUTES || !(attributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)))
            && MoveFileExA(temporary, responsePath, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
    }
    CloseHandle(query);
    return published;
}

static bool respondTs3Tab(const char* profile, const Ts3TabSelection& selection, uint64 ticket) {
    char queryPath[4096]{}, responsePath[4096]{}, original[96]{};
    size_t size{};
    Ts3TabQuery query{};
    Ts3TabIdentity identity{};
    if (networkCancelled(ticket) || !join(queryPath, sizeof(queryPath), profile, "ricon-ts3-tab-query.txt")
        || !join(responsePath, sizeof(responsePath), profile, "ricon-ts3-tab-response.txt")
        || !plainRead(queryPath, original, sizeof(original), size) || !parseTs3TabQuery({original, size}, query)
        || !readTs3TabIdentity(selection, identity)) return false;

    char nonce[33]{}, leaf[64]{}, temporary[4096]{}, row[384]{};
    if (!makeGameNonce(nonce)) return false;
    std::snprintf(leaf, sizeof(leaf), "ricon-ts3-tab-response.%s.tmp", nonce);
    if (!join(temporary, sizeof(temporary), profile, leaf)) return false;
    const int length = std::snprintf(row, sizeof(row), "RICON_TS3_TAB_1 %s %u %u %s %s END\n",
                                     query.nonce, query.sequence, selection.revision, identity.serverUid, identity.clientUid);
    if (length <= 0 || static_cast<size_t>(length) >= sizeof(row)) return false;

    HANDLE file = CreateFileA(temporary, GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return false;
    DWORD written{};
    const auto count = static_cast<DWORD>(length);
    const bool complete = WriteFile(file, row, count, &written, nullptr) && written == count;
    CloseHandle(file);

    Ts3TabIdentity confirmed{};
    if (complete && !networkCancelled(ticket) && readTs3TabIdentity(selection, confirmed)
        && sameTs3TabIdentity(identity, confirmed) && publishTs3Tab(temporary, responsePath, queryPath, {original, size}, selection, ticket)) return true;
    DeleteFileA(temporary);
    return false;
}

static void runTs3Tab(Ts3TabSelection selection, uint64 ticket) {
    char profile[4096]{};
    const bool published = gameProfile(profile) && respondTs3Tab(profile, selection, ticket);
    record("ts3-tab-result", 0, published ? 0 : 1, published ? "metadata reply published" : "metadata reply skipped");
    networkRunning.store(false);
}

static bool beginTs3Tab(uint64 connection) {
    const Ts3TabSelection selection = currentTs3Tab();
    const uint64 ticket = workerGeneration.load();
    if (!connection || selection.handler != connection || !api.getCurrentServerConnectionHandlerID
        || api.getCurrentServerConnectionHandlerID() != connection || ticket == ~uint64{0}
        || liveCleanupPending() || networkRunning.exchange(true)) return false;
    if (testWorker.joinable()) {
        if (WaitForSingleObject(testWorker.native_handle(), 0) != WAIT_OBJECT_0) {
            networkRunning.store(false);
            return false;
        }
        testWorker.join();
    }
    workerStop.store(false);
    try { testWorker = std::thread(runTs3Tab, selection, ticket); }
    catch (const std::exception&) { networkRunning.store(false); return false; }
    return true;
}
