#include "pcm_probe.cpp"
#include <cassert>
#include <string>
#include <utility>

static char folder[4096]{};
static void verifyRiconStatusProtocol() {
    RiconSemver a{}, b{};
    assert(parseRiconSemver("0.0.0", a) && parseRiconSemver("65535.65535.65535", b));
    assert(compareRiconSemver(a, b) < 0 && compareRiconSemver(b, a) > 0 && compareRiconSemver(a, a) == 0);
    assert(parseRiconSemver(RICON_TS3_VERSION, a));
    for (const char* invalid : {"", "1.2", "1.2.3.4", "01.2.3", "1.02.3", "1.2.03", "1.2.65536", "-1.2.3", "1.2.3 ", "1.2.3-beta"})
        assert(!parseRiconSemver(invalid, b));
    assert(validRiconMenuToken("1-1") && validRiconMenuToken("9999999999-1234567890"));
    for (const char* invalid : {"", "0-1", "1-0", "01-1", "1-01", "1-1-1", "1-", "1-a", "12345678901-1"})
        assert(!validRiconMenuToken(invalid));
    RiconMenuStatus request{};
    assert(parseRiconMenuStatus("RICON_MENU_1 123-9 1.2.3 1.3.0 END\r\n", request));
    assert(request.token == "123-9" && request.mod == "1.2.3" && request.wanted == "1.3.0");
    for (const char* invalid : {"RICON_MENU_1 1-1 1.2.3 1.3.0 END x", "RICON_MENU_1 1-1 1.2.3 1.3.0  END",
                               "RICON_MENU_1 1-1 1.2.3 1.3.0 END\r", "RICON_MENU_1 1-1 1.2.3 1.03.0 END"})
        assert(!parseRiconMenuStatus(invalid, request));
    const char* nonce = "0123456789abcdef0123456789abcdef";
    assert(matchesRiconStatusAck("RICON_ACK_1 123-9 0123456789abcdef0123456789abcdef END", "123-9", nonce));
    assert(!matchesRiconStatusAck("RICON_ACK_1 123-8 0123456789abcdef0123456789abcdef END", "123-9", nonce));
    assert(!matchesRiconStatusAck("RICON_ACK_1 123-9 0123456789abcdef0123456789abcdeF END", "123-9", nonce));
    assert(!matchesRiconStatusAck("RICON_ACK_1 123-9 0123456789abcdef0123456789abcdef END extra", "123-9", nonce));
    constexpr std::string_view ack = "RICON_ACK_1 123-9 0123456789abcdef0123456789abcdef END";
    assert(freshRiconStatusAck(ack, "123-9", nonce, 100, 3099));
    assert(!freshRiconStatusAck(ack, "123-9", nonce, 100, 3100));
    assert(!freshRiconStatusAck(ack, "123-9", nonce, 100, 99));
    assert(!freshRiconStatusAck(ack, "123-9", "1123456789abcdef0123456789abcdef", 100, 101));
    assert(!freshRiconStatusAck(ack, "123-10", nonce, 100, 101));
    DWORD workerPid{}; uint64 workerBorn{}; std::string workerVersion;
    assert(parseRiconWorkerLeaf(L"worker-123-456-1.2.3.exe", workerPid, workerBorn, workerVersion)
        && workerPid == 123 && workerBorn == 456 && workerVersion == "1.2.3");
    for (const wchar_t* invalid : {L"worker-0-456-1.2.3.exe", L"worker-123-0-1.2.3.exe", L"worker-01-456-1.2.3.exe",
                                   L"worker-123-456-01.2.3.exe", L"worker-123-18446744073709551616-1.2.3.exe",
                                   L"worker-4294967296-456-1.2.3.exe", L"worker-123-456-1.2.3.exe.extra"})
        assert(!parseRiconWorkerLeaf(invalid, workerPid, workerBorn, workerVersion));
    std::wstring unicodePath;
    assert(riconStatusWide(u8"C:\\Users\\\u0414\u0435\u0434\\profile", unicodePath)
        && unicodePath == L"C:\\Users\\\u0414\u0435\u0434\\profile");
    assert(!riconStatusWide("C:\\Users\\\xff\\profile", unicodePath)); // Invalid UTF-8 is not reinterpreted as the active code page.
    std::wstring statusDirectory;
    assert(riconStatusFullPath(folder, statusDirectory));
    statusDirectory += L"\\ricon-status-selftest";
    assert(CreateDirectoryW(statusDirectory.c_str(), nullptr));
    const auto statusFile = statusDirectory + L"\\status.txt";
    const std::string statusRow = "RICON_TS_1 123-9 0123456789abcdef0123456789abcdef " RICON_TS3_VERSION " CURRENT END\n";
    assert(writeRiconStatusFile(statusFile, statusRow));
    HANDLE before = CreateFileW(statusFile.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    assert(before != INVALID_HANDLE_VALUE);
    BY_HANDLE_FILE_INFORMATION initial{}, unchanged{};
    assert(GetFileInformationByHandle(before, &initial));
    assert(writeRiconStatusFile(statusFile, statusRow));
    HANDLE after = CreateFileW(statusFile.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                               nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    assert(after != INVALID_HANDLE_VALUE && GetFileInformationByHandle(after, &unchanged));
    assert(initial.nFileIndexHigh == unchanged.nFileIndexHigh && initial.nFileIndexLow == unchanged.nFileIndexLow
        && CompareFileTime(&initial.ftLastWriteTime, &unchanged.ftLastWriteTime) == 0);
    CloseHandle(after); CloseHandle(before);
    assert(writeRiconStatusFile(statusFile, "replaced"));
    assert(writeRiconStatusFile(statusFile, statusRow));
    char restored[256]{}; size_t restoredSize{};
    assert(readRiconStatusFile(statusFile, restored, restoredSize) && std::string_view(restored, restoredSize) == statusRow);
    const auto hostClaim = statusDirectory + L"\\host.claim";
    assert(readRiconStatusHostClaim(statusDirectory, hostClaim, 1, "1.2.3") == RiconStatusClaim::none);
    assert(writeRiconStatusFile(hostClaim, "1.2.3"));
    assert(readRiconStatusHostClaim(statusDirectory, hostClaim, 1, "1.2.3") == RiconStatusClaim::sameTarget);
    assert(readRiconStatusHostClaim(statusDirectory, hostClaim, 1, "1.2.4") == RiconStatusClaim::otherOrUncertain);
    assert(DeleteFileW(hostClaim.c_str()));
    const auto legacyCopy = statusDirectory + L"\\worker-" + std::to_wstring(GetCurrentProcessId()) + L"-1-1.2.3.exe";
    assert(writeRiconStatusFile(legacyCopy, "never executable"));
    assert(readRiconStatusHostClaim(statusDirectory, hostClaim, 1, "1.2.3") == RiconStatusClaim::sameTarget);
    assert(readRiconStatusHostClaim(statusDirectory, hostClaim, 1, "1.2.4") == RiconStatusClaim::otherOrUncertain);
    assert(DeleteFileW(legacyCopy.c_str()));
    FILETIME born{}, exited{}, kernel{}, user{};
    assert(GetProcessTimes(GetCurrentProcess(), &born, &exited, &kernel, &user));
    const uint64 hostBorn = (static_cast<uint64>(born.dwHighDateTime) << 32) | born.dwLowDateTime;
    const auto liveCopy = statusDirectory + L"\\worker-" + std::to_wstring(GetCurrentProcessId()) + L"-" + std::to_wstring(hostBorn) + L"-1.2.3.exe";
    const auto oldCopy = statusDirectory + L"\\worker-" + std::to_wstring(GetCurrentProcessId()) + L"-" + std::to_wstring(hostBorn - 1) + L"-1.2.3.exe";
    assert(writeRiconStatusFile(liveCopy, "current host") && writeRiconStatusFile(oldCopy, "old host"));
    HANDLE inUse = CreateFileW(oldCopy.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
    assert(inUse != INVALID_HANDLE_VALUE);
    const bool oldStop = riconStatusStop.exchange(false);
    cleanupRiconStatusWorkers(statusDirectory, hostBorn);
    assert(GetFileAttributesW(oldCopy.c_str()) != INVALID_FILE_ATTRIBUTES && GetFileAttributesW(liveCopy.c_str()) != INVALID_FILE_ATTRIBUTES);
    CloseHandle(inUse);
    cleanupRiconStatusWorkers(statusDirectory, hostBorn);
    assert(GetFileAttributesW(oldCopy.c_str()) == INVALID_FILE_ATTRIBUTES && GetFileAttributesW(liveCopy.c_str()) != INVALID_FILE_ATTRIBUTES);
    riconStatusStop.store(oldStop);
    assert(DeleteFileW(liveCopy.c_str()) && DeleteFileW(statusFile.c_str()) && RemoveDirectoryW(statusDirectory.c_str()));
    riconStatusHostReady.store(true);
    startRiconStatus();
    assert(!riconStatusWorker.joinable()); // This selftest executable is never a real initialized TS host.
    riconStatusHostReady.store(false);
    std::puts("RICON_STATUS_PROTOCOL_SELFTEST_PASS");
}
static void verifyTs3RouteGate() {
    const char* nonce = "0123456789abcdef0123456789abcdef";
    const std::string prefix = "RICON_TS3_ROUTE_1 " + std::string(nonce) + " 9 2 ";
    const std::string active = prefix
        + "1 12345678-1234-4234-9234-123456789abc 1 2 3 4 5 6 7 0 8 9 10 11 12 13 "
          "12345678-1234-4234-9234-123456789abd 14 15 ServerUid= SourceUid= 1 500 2 TargetB= 2 TargetC= 3 END";
    Ts3RouteGrant grant{};
    assert(parseTs3Route(active, nonce, 9, grant));
    assert(grant.sequence == 9 && grant.session == 2 && grant.active && grant.epoch == 1 && grant.player == 2);
    assert(grant.connection == 3 && grant.generation == 4 && grant.entity == 5 && grant.policyRevision == 6);
    assert(grant.device == 7 && grant.index == 0 && grant.txGeneration == 8 && grant.network == 9 && grant.channel == 10);
    assert(grant.hold == 11 && grant.startRequest == 12 && grant.lease == 13 && grant.bindingRequest == 14 && grant.tabRevision == 15);
    assert(grant.sourceClient == 1 && grant.remainingMs == 500 && grant.count == 2);
    assert(!std::strcmp(grant.recipients[0].uid, "TargetB=") && grant.recipients[0].client == 2);
    assert(!std::strcmp(grant.recipients[1].uid, "TargetC=") && grant.recipients[1].client == 3);
    assert(ts3RouteTimeCurrent(grant, 1000, 1499));
    assert(!ts3RouteTimeCurrent(grant, 1000, 1500) && !ts3RouteTimeCurrent(grant, 1000, 999));
    assert(ts3RouteHasBlockBudget(1479, 1500));
    assert(!ts3RouteHasBlockBudget(1480, 1500) && !ts3RouteHasBlockBudget(1499, 1500));
    assert(!ts3RouteHasBlockBudget(1501, 1500));
    assert(parseTs3Route(active + "\r\n", nonce, 9, grant));
    assert(!parseTs3Route(active, nonce, 10, grant));
    assert(!parseTs3Route(active, "0123456789abcdef0123456789abcdee", 9, grant));
    for (size_t size = 0; size < active.size(); ++size) assert(!parseTs3Route({active.data(), size}, nonce, 9, grant));
    for (const auto& replacement : {
        std::pair{" 500 2 ", " 501 2 "}, std::pair{" 500 2 ", " 0 2 "}, std::pair{" 500 2 ", " 500 0 "},
        std::pair{" 500 2 ", " 500 65 "}, std::pair{" 1 2 3 4 5 6 ", " 01 2 3 4 5 6 "},
        std::pair{" 1 2 3 4 5 6 ", " 2147483648 2 3 4 5 6 "},
        std::pair{" 7 0 8 ", " 7 +0 8 "}, std::pair{" TargetC= 3 ", " TargetB= 3 "},
        std::pair{" TargetC= 3 ", " TargetC= 2 "}, std::pair{" TargetB= 2 ", " SourceUid= 2 "},
        std::pair{" TargetB= 2 ", " TargetB= 1 "}, std::pair{" TargetC= 3 ", " TargetC= 65536 "},
        std::pair{"-4234-", "-5234-"}, std::pair{"-9234-", "-7234-"}, std::pair{"ServerUid=", "Server:Uid"},
        std::pair{" END", " END extra"}, std::pair{" END", " END\n\n"}, std::pair{" 9 2 1 ", " 9  2 1 "}
    }) {
        std::string invalid = active;
        const auto where = invalid.find(replacement.first);
        assert(where != invalid.npos);
        invalid.replace(where, std::strlen(replacement.first), replacement.second);
        assert(!parseTs3Route(invalid, nonce, 9, grant));
    }
    std::string oversize = active + std::string(ts3RouteMaxRow, ' ');
    assert(!parseTs3Route(oversize, nonce, 9, grant));
    std::string embedded = active;
    embedded[30] = 0;
    assert(!parseTs3Route(embedded, nonce, 9, grant));
    assert(parseTs3Route(prefix + "0 END", nonce, 9, grant) && !grant.active && !grant.count && !grant.sourceUid[0]);

    const uint64 oldHandler = ts3RouteCaptureHandler.load(), oldTicket = ts3RouteCaptureTicket.load();
    const uint64 oldDeadline = ts3RouteCaptureDeadline.load(), ticket = workerGeneration.load();
    const uint64 oldRevision = ts3RouteCaptureRevision.load(), oldRevocation = ts3RouteRevocation.load();
    const bool stopped = workerStop.load();
    workerStop.store(false);
    ts3RouteCaptureHandler.store(81); ts3RouteCaptureTicket.store(ticket); revokeTs3Route();
    ts3RouteCaptureRevision.store(ts3RouteRevocation.load());
    short pcm[] = {123, -456};
    int edited = 2;
    assert(editTs3RouteCapture(81, pcm, 2, 1, &edited, 1000));
    assert(!pcm[0] && !pcm[1] && edited == 1); // No grant and no matched route ACK.
    pcm[0] = 123; edited = 2;
    assert(!editTs3RouteCapture(82, pcm, 2, 1, &edited, 1000) && pcm[0] == 123 && edited == 2);
    ts3RouteCaptureDeadline.store(1100);
    assert(editTs3RouteCapture(81, pcm, 2, 1, &edited, 1099) && pcm[0] == 123 && edited == 2);
    assert(editTs3RouteCapture(81, pcm, 2, 1, &edited, 1100) && !pcm[0] && edited == 1);
    ts3RouteCaptureDeadline.store(1200); ts3RouteCaptureTicket.store(ticket ^ 1); pcm[0] = 123; edited = 2;
    assert(editTs3RouteCapture(81, pcm, 2, 1, &edited, 1100) && !pcm[0] && edited == 1);
    ts3RouteCaptureTicket.store(ticket); workerStop.store(true); pcm[0] = 123; edited = 2;
    assert(editTs3RouteCapture(81, pcm, 2, 1, &edited, 1100) && !pcm[0] && edited == 1);
    workerStop.store(false);
    const uint64 revokedBeforePublish = ts3RouteRevocation.load();
    revokeTs3Route();
    const uint64 currentRevision = ts3RouteRevocation.load();
    assert(!publishTs3RouteCapture(false, currentRevision, ticket, 1300, 1100));
    assert(!ts3RouteCaptureAllowed(81, 1100));
    assert(publishTs3RouteCapture(true, currentRevision, ticket, 1300, 1100));
    assert(ts3RouteCaptureAllowed(81, 1100));
    revokeTs3Route();
    assert(!publishTs3RouteCapture(true, revokedBeforePublish, ticket, 1300, 1100));
    assert(!ts3RouteCaptureAllowed(81, 1100));
    ts3RouteCaptureHandler.store(oldHandler); ts3RouteCaptureTicket.store(oldTicket); ts3RouteCaptureDeadline.store(oldDeadline);
    ts3RouteCaptureRevision.store(oldRevision); ts3RouteRevocation.store(oldRevocation);
    workerStop.store(stopped);
}

static void verifyTs3TabActualGuard() {
    assert(ts3TabActualStagePath("C:\\test\\ts3-tab-actual-01\\ts3-pcm-prototype-01-client\\ricon-probe"));
    assert(!ts3TabActualStagePath("C:\\test\\ts3-tab-actual-01-extra\\ricon-probe"));
    assert(!ts3TabActualStagePath("C:\\test\\ts3-tab-actual-01\\..\\ricon-probe"));
    const TS3Functions saved = api;
    static uint64 speechPackets;
    static int inputState = INPUT_DEACTIVATED;
    api.getConnectionVariableAsUInt64 = [](uint64 handler, anyID client, size_t field, uint64* count) -> unsigned int {
        assert(handler == 177 && client == 9 && field == CONNECTION_PACKETS_SENT_SPEECH);
        *count = speechPackets;
        return ERROR_ok;
    };
    api.getClientSelfVariableAsInt = [](uint64 handler, size_t field, int* state) -> unsigned int {
        assert(handler == 177 && field == CLIENT_INPUT_DEACTIVATED);
        *state = inputState;
        return ERROR_ok;
    };
    assert(ts3TabActualReadInput(177, 9) == Ts3TabActualInput::quiet);
    inputState = INPUT_ACTIVE;
    assert(ts3TabActualReadInput(177, 9) == Ts3TabActualInput::pending);
    inputState = INPUT_DEACTIVATED;
    speechPackets = 1;
    assert(ts3TabActualReadInput(177, 9) == Ts3TabActualInput::unsafe);
    api.getConnectionVariableAsUInt64 = [](uint64, anyID, size_t, uint64*) -> unsigned int { return ERROR_undefined; };
    assert(ts3TabActualReadInput(177, 9) == Ts3TabActualInput::unavailable);
    api = saved;
    std::puts("RICON_TS3_TAB_ACTUAL_GUARD_SELFTEST_PASS");
}

static void verifyTs3TabProofChallenge() {
    constexpr std::string_view row = "RICON_TS3_PROOF_CHALLENGE_1 12345678-1234-4234-9234-123456789abc 7 5 Server/UID+= Client_ID- ABCDEFGHIJKLMNOPQRSTUw END";
    Ts3TabProofChallenge proof{};
    assert(parseTs3TabProofChallenge(row, proof));
    assert(proof.requestId == 7 && proof.revision == 5 && proof.nonce == "ABCDEFGHIJKLMNOPQRSTUw");
    assert(proof.serverUid == "Server/UID+=" && proof.clientUid == "Client_ID-");
    assert(!parseTs3TabProofChallenge(row.substr(0, row.size() - 1), proof));
    assert(!parseTs3TabProofChallenge(std::string(row) + " EXTRA", proof));
    assert(!parseTs3TabProofChallenge("RICON_TS3_PROOF_CHALLENGE_1 12345678-1234-4234-9234-123456789ABC 7 5 S C ABCDEFGHIJKLMNOPQRSTUw END", proof));
    assert(!parseTs3TabProofChallenge("RICON_TS3_PROOF_CHALLENGE_1 12345678-1234-4234-9234-123456789abc 7 5 S C ABCDEFGHIJKLMNOPQRSTU+ END", proof));
    assert(ts3TabProofProfilePath("C:\\lab\\remote-radio-01\\ts3-binding-proof-runtime01\\client\\profile\\profile"));
    assert(!ts3TabProofProfilePath("C:\\lab\\remote-radio-01\\ts3-binding-proof-runtime01\\server\\profile\\profile"));
    std::puts("RICON_TS3_TAB_PROOF_SELFTEST_PASS");
}

static void verifyTs3TabProfile() {
    Ts3TabQuery query{};
    const char* normal = "RICON_TS3_TAB_QUERY_1 0123456789abcdef0123456789abcdef 8 END";
    assert(parseTs3TabQuery(normal, query));
    assert(query.sequence == 8 && !std::strcmp(query.nonce, "0123456789abcdef0123456789abcdef"));
    assert(parseTs3TabQuery("RICON_TS3_TAB_QUERY_1 0123456789abcdef0123456789abcdef 2147483647 END\r\n", query));
    const char* invalid[] = {
        "RICON_PTT_QUERY_1 0123456789abcdef0123456789abcdef 8 END",
        "RICON_TS3_TAB_QUERY_1 0123456789abcdef0123456789abcdeF 8 END",
        "RICON_TS3_TAB_QUERY_1 0123456789abcdef0123456789abcdef 08 END",
        "RICON_TS3_TAB_QUERY_1 0123456789abcdef0123456789abcdef 2147483648 END",
        "RICON_TS3_TAB_QUERY_1 0123456789abcdef0123456789abcdef 8  END",
        "RICON_TS3_TAB_QUERY_1 0123456789abcdef0123456789abcdef 8 END\r",
        "RICON_TS3_TAB_QUERY_1 0123456789abcdef0123456789abcdef 8 END\nextra"
    };
    for (const char* row : invalid) assert(!parseTs3TabQuery(row, query));
    for (size_t count = 0; count < std::strlen(normal); ++count) assert(!parseTs3TabQuery({normal, count}, query));
    assert(validTs3TabUid("Aa0+/_=-") && validTs3TabUid(std::string(127, 'x')));
    assert(!validTs3TabUid("") && !validTs3TabUid("bad uid") && !validTs3TabUid("bad.uid")
           && !validTs3TabUid(std::string(128, 'x')) && !validTs3TabUid({"id\0suffix", 9}));

    static int frees;
    const TS3Functions saved = api;
    const uint64 previous = currentConnection.load();
    currentConnection.store(191);
    selectTs3Tab(191);
    Ts3TabSelection selection = currentTs3Tab();
    assert(selection.handler == 191 && selection.revision > 0);
    api.getConnectionStatus = [](uint64 handler, int* status) -> unsigned int {
        assert(handler == 191);
        *status = STATUS_CONNECTION_ESTABLISHED;
        return ERROR_ok;
    };
    api.getClientID = [](uint64 handler, anyID* client) -> unsigned int {
        assert(handler == 191);
        *client = 9;
        return ERROR_ok;
    };
    api.freeMemory = [](void* ptr) -> unsigned int { ++frees; std::free(ptr); return ERROR_ok; };
    api.getServerVariableAsString = [](uint64 handler, size_t flag, char** value) -> unsigned int {
        assert(handler == 191 && flag == VIRTUALSERVER_UNIQUE_IDENTIFIER);
        *value = _strdup("Server/UID+=");
        assert(*value);
        return ERROR_ok;
    };
    api.getClientVariableAsString = [](uint64 handler, anyID client, size_t flag, char** value) -> unsigned int {
        assert(handler == 191 && client == 9 && flag == CLIENT_UNIQUE_IDENTIFIER);
        *value = _strdup("Client_ID-");
        assert(*value);
        return ERROR_ok;
    };
    Ts3TabIdentity identity{};
    assert(readTs3TabIdentity(selection, identity));
    assert(frees == 2 && identity.client == 9 && !std::strcmp(identity.serverUid, "Server/UID+=")
           && !std::strcmp(identity.clientUid, "Client_ID-"));

    char queryPath[4096]{}, responsePath[4096]{}, row[400]{}, expected[400]{};
    assert(join(queryPath, sizeof(queryPath), folder, "ricon-ts3-tab-query.txt"));
    assert(join(responsePath, sizeof(responsePath), folder, "ricon-ts3-tab-response.txt"));
    FILE* request{};
    assert(!fopen_s(&request, queryPath, "wb") && request);
    assert(std::fputs(normal, request) != EOF && std::fclose(request) == 0);
    workerStop.store(false);
    const uint64 ticket = workerGeneration.load();
    assert(respondTs3Tab(folder, selection, ticket));
    size_t length{};
    assert(plainRead(responsePath, row, sizeof(row), length));
    std::snprintf(expected, sizeof(expected), "RICON_TS3_TAB_1 0123456789abcdef0123456789abcdef 8 %u Server/UID+= Client_ID- END\n", selection.revision);
    assert(std::string_view(row, length) == expected);

    static char rotatingQueryPath[4096]{};
    static int uidReads;
    strcpy_s(rotatingQueryPath, queryPath);
    const auto goodUidRead = api.getClientVariableAsString;
    api.getClientVariableAsString = [](uint64, anyID, size_t, char** value) -> unsigned int {
        if (++uidReads == 2) {
            FILE* replaced{};
            assert(!fopen_s(&replaced, rotatingQueryPath, "wb") && replaced);
            assert(std::fputs("RICON_TS3_TAB_QUERY_1 0123456789abcdef0123456789abcdef 9 END", replaced) != EOF);
            assert(std::fclose(replaced) == 0);
        }
        *value = _strdup("Client_ID-");
        assert(*value);
        return ERROR_ok;
    };
    assert(!respondTs3Tab(folder, selection, ticket));
    assert(uidReads == 2 && plainRead(responsePath, row, sizeof(row), length) && std::string_view(row, length) == expected);
    api.getClientVariableAsString = goodUidRead;
    assert(DeleteFileA(queryPath));
    assert(!respondTs3Tab(folder, selection, ticket));
    assert(plainRead(responsePath, row, sizeof(row), length) && std::string_view(row, length) == expected);
    assert(DeleteFileA(responsePath));

    frees = 0;
    invalidateTs3Tab(191);
    assert(!readTs3TabIdentity(selection, identity) && frees == 0);
    currentConnection.store(191);
    selectTs3Tab(191);
    const Ts3TabSelection reselected = currentTs3Tab();
    assert(reselected.handler == 191 && reselected.revision != selection.revision);
    api.getClientVariableAsString = [](uint64, anyID, size_t, char** value) -> unsigned int {
        static char undefinedResult[] = "unowned-on-error";
        *value = undefinedResult;
        return ERROR_undefined;
    };
    assert(!readTs3TabIdentity(reselected, identity) && frees == 1);
    api.getClientVariableAsString = [](uint64, anyID, size_t, char** value) -> unsigned int {
        selectTs3Tab(192);
        *value = _strdup("Client_ID-");
        assert(*value);
        return ERROR_ok;
    };
    assert(!readTs3TabIdentity(reselected, identity) && frees == 3);
    currentConnection.store(previous);
    selectTs3Tab(previous);
    api = saved;
    std::puts("RICON_TS3_TAB_PROFILE_SELFTEST_PASS");
}

static void verifyWhisperAckGate() {
    const uint64 current = workerGeneration.load();
    whisperAck = {};
    strcpy_s(whisperAck.code, "test-whisper-return-code");
    whisperAck.handler = 81;
    whisperAck.ticket = current;
    whisperAck.issued = GetTickCount64();
    whisperAck.pending = true;
    const auto ack = [](uint64 handler, unsigned int error, const char* code) {
        return ts3plugin_onServerErrorEvent(handler, "", error, code, nullptr);
    };
    assert(!ack(82, ERROR_ok, whisperAck.code));
    assert(!ack(81, ERROR_ok, "other-return-code"));
    whisperAck.ticket = current ^ 1;
    assert(!ack(81, ERROR_ok, whisperAck.code));
    whisperAck.ticket = current;
    whisperAck.issued = GetTickCount64() - 2000;
    assert(!ack(81, ERROR_ok, whisperAck.code) && !whisperAck.seen);
    whisperAck.issued = GetTickCount64();
    assert(ack(81, ERROR_ok, whisperAck.code) == 1 && whisperAck.seen && whisperAck.error == ERROR_ok);
    assert(!ack(81, ERROR_undefined, whisperAck.code) && whisperAck.error == ERROR_ok);
    whisperAck.tone = true;
    whisperAck.issued = GetTickCount64() - 2300;
    assert(ack(81, ERROR_undefined, whisperAck.code) == 1 && whisperAck.error == ERROR_undefined);
    assert(networkCancelled(current));
    workerStop.store(false);
    const uint64 rerouted = workerGeneration.load();
    whisperAck.ticket = rerouted;
    whisperAck.seen = false;
    whisperPriorToneAck = whisperAck;
    strcpy_s(whisperAck.code, "test-whisper-next-target");
    assert(!ack(82, ERROR_undefined, whisperPriorToneAck.code));
    assert(ack(81, ERROR_undefined, whisperPriorToneAck.code) == 1 && networkCancelled(rerouted));
    assert(whisperPriorToneAck.seen && !whisperAck.seen);
    workerStop.store(false);
    whisperAck = {};
    whisperPriorToneAck = {};
    short pure[960]{}, harmonic[960]{}, quiet[960]{};
    constexpr double cycle = 6.28318530717958647692;
    for (int i = 0; i < 960; ++i) {
        pure[i] = static_cast<short>(std::lround(1024.0 * std::sin(cycle * i / 80.0 + 0.31)));
        harmonic[i] = static_cast<short>(std::lround(1024.0 * std::sin(cycle * i / 40.0)));
    }
    assert(whisperCarrier(pure, 960, 1));
    assert(!whisperCarrier(harmonic, 960, 1) && !whisperCarrier(quiet, 960, 1));
    assert(whisperCarrier(harmonic, 960, 1, true));
    assert(!whisperCarrier(pure, 960, 1, true) && !whisperCarrier(quiet, 960, 1, true));
    short onset[960]{};
    std::copy(harmonic + 480, harmonic + 720, onset + 480);
    observeWhisperAbcPcm(2, onset, 960, 1);
    assert(!whisperCarrier(onset, 960, 1, true));
    assert(whisperAbcShortCarriers[2][0].load() == 0 && whisperAbcShortCarriers[2][1].load() == 1);
    assert(whisperAbcPeak[2].load() == 1024);
    whisperSender.store(3);
    whisperHandlers[1].store(82);
    whisperHandlers[2].store(83);
    whisperToneWindow.store(83);
    observeWhisperTalk(82, STATUS_TALKING, 1, 3);
    observeWhisperTalk(83, STATUS_TALKING, 1, 4);
    observeWhisperPlayback(83, 3, pure, 960, 1);
    assert(!whisperToneEvidence[0].seen.load() && !whisperToneEvidence[1].hits.load());
    observeWhisperTalk(83, STATUS_TALKING, 1, 3);
    observeWhisperPlayback(83, 3, pure, 960, 1);
    assert(whisperToneEvidence[1].hits.load() == 1);
    whisperToneWindow.store(82);
    observeWhisperPlayback(82, 3, pure, 960, 1);
    assert(!whisperToneEvidence[0].hits.load());
    observeWhisperTalk(82, STATUS_TALKING, 1, 3);
    observeWhisperPlayback(82, 3, pure, 960, 1);
    assert(whisperToneEvidence[0].hits.load() == 1);
    whisperAbcMeasuring.store(true);
    observeWhisperPlayback(82, 3, harmonic, 960, 1);
    observeWhisperPlayback(83, 3, pure, 960, 1);
    assert(whisperToneEvidence[0].hits.load() == 2);
    assert(whisperAbcCarriers[0][0].load() == 0 && whisperAbcCarriers[0][1].load() == 1);
    assert(whisperAbcCarriers[1][0].load() == 1 && whisperAbcCarriers[1][1].load() == 0);
    whisperAbcMeasuring.store(false);
    whisperToneWindow.store(0);
    whisperSender.store(0);
    for (auto& handler : whisperHandlers) handler.store(0);
    for (auto& count : whisperBlocks) count.store(0);

    WhisperCounters toC[3]{{}, {50, 1200, 49}, {50, 1200, 0}};
    WhisperCounters toB[3]{{50, 1200, 49}, {}, {50, 1200, 0}};
    assert(whisperTonePhasePass(toC, 2, true, true, 48));
    assert(whisperTonePhasePass(toB, 1, true, true, 48));
    for (auto metric : {&WhisperCounters::packets, &WhisperCounters::bytes, &WhisperCounters::blocks}) {
        toC[0].*metric = 1;
        toB[1].*metric = 1;
        assert(!whisperTonePhasePass(toC, 2, true, true, 48));
        assert(!whisperTonePhasePass(toB, 1, true, true, 48));
        toC[0].*metric = 0;
        toB[1].*metric = 0;
    }
    toC[1] = {};
    assert(!whisperTonePhasePass(toC, 2, true, true, 48));
}

static void verifyPttStatusGate() {
    const char* nonce = "0123456789abcdef0123456789abcdef";
    const char* uuid = "12345678-1234-4234-9234-123456789abc";
    PttStatusReply reply{};
    const char* inactive = "RICON_PTT_1 0123456789abcdef0123456789abcdef 12 7 0 END\n";
    assert(parsePttStatusReply(inactive, nonce, 12, reply));
    assert(reply.session == 7 && !reply.active && !reply.remainingMs);
    assert(pttStatusReplyCurrent(reply, 1000, 2499));
    assert(!pttStatusReplyCurrent(reply, 1000, 2500));
    assert(!parsePttStatusReply(inactive, nonce, 13, reply));
    assert(!parsePttStatusReply(inactive, "0123456789abcdef0123456789abcdee", 12, reply));
    const char* invalid[] = {
        "RICON_PTT_1 0123456789abcdef0123456789abcdef 012 7 0 END",
        "RICON_PTT_1 0123456789abcdef0123456789abcdef 12  7 0 END",
        "RICON_PTT_1 0123456789abcdef0123456789abcdef 12 0 0 END",
        "RICON_PTT_1 0123456789abcdef0123456789abcdef 12 7 00 END",
        "RICON_PTT_1 0123456789abcdef0123456789abcdef 12 7 0 extra END",
        "RICON_PTT_1 0123456789abcdef0123456789abcdef 12 7 0 END ",
        "RICON_PTT_1 0123456789abcdef0123456789abcdef 12 7 0 END\n\n"
    };
    for (const char* row : invalid) assert(!parsePttStatusReply(row, nonce, 12, reply));
    const auto parseActive = [&](const char* domain, const char* epoch, const char* index, const char* ttl) {
        char row[pttStatusRowCapacity]{};
        std::snprintf(row, sizeof(row), "RICON_PTT_1 %s 12 7 1 %s %s 2 3 4 5 6 7 %s 9 10 11 12 13 14 %s END\r\n",
                      nonce, domain, epoch, index, ttl);
        return parsePttStatusReply(row, nonce, 12, reply);
    };
    assert(parseActive(uuid, "2147483647", "0", "100"));
    assert(reply.session == 7 && reply.active && reply.remainingMs == 100);
    assert(!std::strcmp(reply.domain, uuid) && reply.epoch == INT_MAX && reply.player == 2 && reply.connection == 3);
    assert(reply.generation == 4 && reply.entity == 5 && reply.policyRevision == 6 && reply.device == 7 && reply.index == 0);
    assert(reply.network == 9 && reply.channel == 10 && reply.rxGeneration == 11 && reply.hold == 12 && reply.request == 13 && reply.lease == 14);
    assert(pttStatusReplyCurrent(reply, 1000, 1099));
    assert(!pttStatusReplyCurrent(reply, 1000, 1100));
    assert(!pttStatusReplyCurrent(reply, 1000, 1150)); // A re-read cannot refresh the issue time.
    assert(!pttStatusReplyCurrent(reply, 1000, 999));
    assert(parseActive(uuid, "1", "17", "60000"));
    assert(pttStatusReplyCurrent(reply, 1000, 2499) && !pttStatusReplyCurrent(reply, 1000, 2500));
    assert(!parseActive(uuid, "2147483648", "0", "100"));
    assert(!parseActive(uuid, "0", "0", "100"));
    assert(!parseActive(uuid, "1", "+0", "100"));
    assert(!parseActive(uuid, "1", "00", "100"));
    assert(!parseActive(uuid, "1", "0", "60001"));
    assert(!parseActive(uuid, "1", "0", "0"));
    assert(!parseActive("12345678-1234-4234-9234-123456789abC", "1", "0", "100"));
    assert(!parseActive("12345678-1234-5234-9234-123456789abc", "1", "0", "100"));
    assert(!parseActive("12345678-1234-4234-7234-123456789abc", "1", "0", "100"));
    const char* active = "RICON_PTT_1 0123456789abcdef0123456789abcdef 12 7 1 12345678-1234-4234-9234-123456789abc 1 2 3 4 5 6 7 0 9 10 11 12 13 14 100 END";
    for (size_t length = 0; length < std::strlen(active); ++length)
        assert(!parsePttStatusReply({active, length}, nonce, 12, reply));
    assert(parsePttStatusReply(active, nonce, 12, reply));
    assert(parsePttStatusReply(inactive, nonce, 12, reply));
    assert(!reply.active && !reply.domain[0] && !reply.hold && !reply.lease);

    const uint64 ticket = workerGeneration.load();
    assert(writeGameRequest(folder, nonce, 12, ticket, true));
    char request[4096]{}, bytes[96]{};
    size_t length{};
    assert(join(request, sizeof(request), folder, "ricon-ptt-request.txt"));
    assert(plainRead(request, bytes, sizeof(bytes), length));
    assert(std::string_view(bytes, length) == "RICON_PTT_QUERY_1 0123456789abcdef0123456789abcdef 12 END\n");
    assert(join(request, sizeof(request), folder, "ricon-ts3-request.txt"));
    assert(plainRead(request, bytes, sizeof(bytes), length));
    assert(std::string_view(bytes, length) == "RICON_TS3_1 0123456789abcdef0123456789abcdef 5 END\n");
}

static void verifyLiveGameGate() {
    networkPrepared.store(true); networkSource.store(81); networkListener.store(82);
    networkSourceClient.store(3); networkListenerClient.store(4); networkChannel.store(9);
    const auto statusApi = api.getConnectionStatus;
    api.getConnectionStatus = [](uint64, int* status) -> unsigned int { *status = STATUS_CONNECTION_ESTABLISHED; return 0; };
    for (int late = 1; late < STATUS_CONNECTION_ESTABLISHED; ++late) {
        workerStop.store(false);
        ts3plugin_onConnectStatusChangeEvent(82, late, 0);
        assert(!workerStop.load());
    }
    workerStop.store(false);
    ts3plugin_onConnectStatusChangeEvent(82, STATUS_DISCONNECTED, 0);
    assert(workerStop.load());
    workerStop.store(false);
    ts3plugin_onConnectStatusChangeEvent(82, STATUS_CONNECTION_ESTABLISHED, 1);
    assert(workerStop.load());
    api.getConnectionStatus = [](uint64, int* status) -> unsigned int { *status = 1; return 0; };
    workerStop.store(false);
    ts3plugin_onConnectStatusChangeEvent(82, 1, 0);
    assert(workerStop.load());
    api.getConnectionStatus = [](uint64, int*) -> unsigned int { return 1; };
    workerStop.store(false);
    ts3plugin_onConnectStatusChangeEvent(82, 1, 0);
    assert(workerStop.load());
    api.getConnectionStatus = statusApi;
    workerStop.store(false);
    ts3plugin_onClientMoveEvent(82, 3, 0, 9, ENTER_VISIBILITY, "");
    assert(!workerStop.load()); // Late initial visibility must not cancel an already validated same-channel binding.
    assert(!initialBoundArrival(82, 3, 8, 9, RETAIN_VISIBILITY));
    assert(!initialBoundArrival(82, 3, 0, 10, ENTER_VISIBILITY));
    assert(!initialBoundArrival(83, 3, 0, 9, ENTER_VISIBILITY));
    assert(!initialBoundArrival(82, 5, 0, 9, ENTER_VISIBILITY));
    ts3plugin_onClientMoveEvent(82, 3, 9, 0, LEAVE_VISIBILITY, "");
    assert(workerStop.load());
    workerStop.store(false);
    ts3plugin_onClientMoveMovedEvent(82, 3, 9, 10, RETAIN_VISIBILITY, 0, "", "", "");
    assert(workerStop.load());
    workerStop.store(false);
    ts3plugin_onClientMoveMovedEvent(82, 3, 0, 9, ENTER_VISIBILITY, 0, "", "", "");
    assert(workerStop.load());
    workerStop.store(false);
    ts3plugin_onClientKickFromChannelEvent(82, 3, 0, 9, ENTER_VISIBILITY, 0, "", "", "");
    assert(workerStop.load());
    workerStop.store(false);
    ts3plugin_onClientKickFromServerEvent(82, 3, 0, 9, ENTER_VISIBILITY, 0, "", "", "");
    assert(workerStop.load());
    workerStop.store(false);
    ts3plugin_onClientMoveTimeoutEvent(82, 3, 0, 9, ENTER_VISIBILITY, "");
    assert(workerStop.load());
    networkPrepared.store(false);
    const char* nonce = "0123456789abcdef0123456789abcdef";
    GameReply reply{};
    const char* valid = "RICON_GAME_1 0123456789abcdef0123456789abcdef 12 7 1 1 END\r\n";
    assert(parseGameReply(valid, std::strlen(valid), nonce, reply));
    assert(reply.sequence == 12 && reply.session == 7 && reply.player == 1 && reply.active == 1);
    assert(gameReplyCurrent(reply, 12, 1000, 2499));
    assert(!gameReplyCurrent(reply, 12, 1000, 2500));
    assert(!gameReplyCurrent(reply, 13, 1000, 1100));
    assert(!gameReplyCurrent(reply, 12, 1000, 999));
    const char* invalid[] = {
        "RICON_GAME_1 0123456789abcdef0123456789abcdef 12 7 1 1 ENDjunk",
        "RICON_GAME_1 0123456789abcdef0123456789abcdef 012 7 1 1 END",
        "RICON_GAME_1 0123456789abcdef0123456789abcdef -1 7 1 1 END",
        "RICON_GAME_1 0123456789abcdef0123456789abcdef 2147483648 7 1 1 END",
        "RICON_GAME_1 0123456789abcdef0123456789abcdef 12 7 0 1 END",
        "RICON_GAME_1 0123456789abcdef0123456789abcdef 12 7 1 0 END",
        "RICON_GAME_1 0123456789abcdef0123456789abcdef 12 7 1 1 END\n\n",
        "RICON_GAME_1 0123456789abcdef0123456789abcdee 12 7 1 1 END"
    };
    for (const char* row : invalid) assert(!parseGameReply(row, std::strlen(row), nonce, reply));
    for (size_t length = 0; length < std::strlen(valid) - 2; ++length) assert(!parseGameReply(valid, length, nonce, reply));
    liveSource.store(81); liveDeadline.store(2000); workerStop.store(false);
    short samples[] = {123, -123}; int edited = 2;
    editLiveCapture(81, samples, 2, 1, &edited, 1999);
    assert(samples[0] == 123 && edited == 2);
    editLiveCapture(81, samples, 2, 1, &edited, 2000);
    assert(!samples[0] && !samples[1] && edited == 1);
    samples[0] = 123; edited = 2;
    editLiveCapture(82, samples, 2, 1, &edited, 2001);
    assert(samples[0] == 123 && edited == 2);
    workerStop.store(true); edited = 2;
    editLiveCapture(81, samples, 2, 1, &edited, 1999);
    assert(!samples[0] && edited == 1);
    const auto originalClose = api.closeCaptureDevice;
    api.closeCaptureDevice = [](uint64) -> unsigned int { requestWorkerStop(); return 0; };
    workerStop.store(false); liveDeadline.store(GetTickCount64() + 1500);
    const uint64 ticket = workerGeneration.load();
    assert(!replaceLiveCapture(81, ticket)); // Stop during close must prevent the following hardware open.
    api.closeCaptureDevice = originalClose;
    const auto savedApi = api;
    api.getDefaultCaptureMode = [](char** value) -> unsigned int { *value = _strdup("test-mode"); return 0; };
    api.getCaptureModeList = [](char***) -> unsigned int { assert(false); return 1; };
    api.getDefaultCaptureDevice = [](const char*, char*** value) -> unsigned int {
        *value = static_cast<char**>(std::calloc(3, sizeof(char*)));
        (*value)[0] = _strdup("test input"); (*value)[1] = _strdup("test device");
        requestWorkerStop(); return 0;
    };
    api.openCaptureDevice = [](uint64, const char*, const char*) -> unsigned int { assert(false); return 1; };
    workerStop.store(false); liveDeadline.store(GetTickCount64() + 1500);
    assert(openLiveCapture(81, workerGeneration.load()) != ERROR_ok);
    api = savedApi;
    liveSource.store(0); liveDeadline.store(0);
    const short extrema[] = {-32768, 32767, 0};
    assert(measurePcm(extrema, 3).peak == 32768);
    const uint64 savedCurrent = currentConnection.load();
    currentConnection.store(81); workerStop.store(false);
    uint64 radioGeneration{}; clearVoiceProbe(&radioGeneration);
    assert(armVoiceProbe(81, 3, false, GetTickCount64() + 1500, radioGeneration, workerGeneration.load()));
    assert(setLiveRadioProfile(radioGeneration));
    short quiet[960]{};
    assert(editVoiceProbe(81, 3, quiet, 960, 1, GetTickCount64()));
    assert(measurePcm(quiet, 960).peak < 64 && voiceState.metrics.callbacks_dsp == 1 && !voiceState.metrics.callbacks_passthrough);
    clearVoiceProbe();
    assert(!setLiveRadioProfile(radioGeneration));
    currentConnection.store(savedCurrent);
    const uint64 publishTicket = workerGeneration.load();
    assert(writeGameRequest(folder, nonce, 1, publishTicket));
    char request[4096]{}, temporary[4096]{};
    assert(join(request, sizeof(request), folder, "ricon-ts3-request.txt"));
    assert(join(temporary, sizeof(temporary), folder, "ricon-ts3-request.tmp"));
    HANDLE held = CreateFileA(request, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    assert(held != INVALID_HANDLE_VALUE && GetFileAttributesA(temporary) == INVALID_FILE_ATTRIBUTES);
    std::atomic<bool> published{false};
    std::thread publisher([&] { published.store(writeGameRequest(folder, nonce, 2, publishTicket)); });
    for (int wait = 0; wait < 1000 && GetFileAttributesA(temporary) == INVALID_FILE_ATTRIBUTES; ++wait) Sleep(1);
    assert(GetFileAttributesA(temporary) != INVALID_FILE_ATTRIBUTES);
    Sleep(20); CloseHandle(held); publisher.join();
    assert(published.load());
    held = CreateFileA(request, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    assert(held != INVALID_HANDLE_VALUE);
    assert(!writeGameRequest(folder, nonce, 3, publishTicket));
    CloseHandle(held);
    char bytes[96]{}; size_t length{};
    assert(plainRead(request, bytes, sizeof(bytes), length) && std::strstr(bytes, " 2 END\n"));
    workerStop.store(true);
    assert(!writeGameRequest(folder, nonce, 4, publishTicket));
    workerStop.store(false);
    assert(writeGameRequest(folder, nonce, 5, publishTicket));
    verifyPttStatusGate();
    verifyWhisperAckGate();
    std::puts("RICON_TS3_LIVE_GAME_GATE_SELFTEST_PASS");
}
static std::atomic<unsigned int> plays{0};
static unsigned int closes{}, nextError{}, closeError{};
static bool orderedRun{};
static unsigned int orderedIndex{};
static uint64 lastConnection{}, lastClosed{};
static char lastPath[4096]{};
static bool devicePresent{};
static unsigned int opens{}, deviceCloses{}, openError{}, queryError{};
static const char* defaultMode = "test-mode";
static unsigned int defaultModeError{}, defaultDeviceError{};
static unsigned int queryDefaultMode(char** mode) { *mode = defaultModeError ? nullptr : _strdup(defaultMode); return defaultModeError; }
static unsigned int queryDefaultDevice(const char* mode, char*** device) {
    assert(!std::strcmp(mode, "test-mode"));
    *device = nullptr;
    if (defaultDeviceError) return defaultDeviceError;
    *device = static_cast<char**>(std::calloc(3, sizeof(char*)));
    (*device)[0] = _strdup("test output");
    (*device)[1] = _strdup("test-device");
    return 0;
}
static unsigned int queryPlaybackModes(char*** modes) {
    *modes = static_cast<char**>(std::calloc(3, sizeof(char*)));
    (*modes)[0] = _strdup("custom");
    (*modes)[1] = _strdup("test-mode");
    return 0;
}
static unsigned int queryDevice(uint64, char** result, int* isDefault) {
    if (queryError) { *result = nullptr; return queryError; }
    *result = devicePresent ? _strdup("test output") : nullptr;
    *isDefault = 1;
    return devicePresent ? ERROR_ok : ERROR_sound_no_playback_device_available;
}
static unsigned int freeSdk(void* value) { std::free(value); return 0; }
static unsigned int queryGain(uint64, const char* key, float* value) { assert(!std::strcmp(key, "volume_factor_wave")); *value = 0.0f; return 0; }
static unsigned int openDevice(uint64 connection, const char* mode, const char* device) {
    assert(connection == 77 && !std::strcmp(mode, "test-mode") && !std::strcmp(device, "test-device"));
    ++opens;
    if (!openError) devicePresent = true;
    return openError;
}
static unsigned int closeDevice(uint64 connection) {
    assert(connection == 77 && playbackOwned);
    ++deviceCloses;
    devicePresent = false;
    return 0;
}
static void configPath(char* out, size_t size) { strncpy_s(out, size, folder, _TRUNCATE); }
static void pluginPath(char* out, size_t size, const char* id) { assert(!std::strcmp(id, "test-id")); configPath(out, size); }
static uint64 currentConnectionMock() { return 77; }
static unsigned int playFile(uint64 connection, const char* path, int loop, uint64* handle) {
    assert(loop == 0);
    ++plays;
    lastConnection = connection;
    strncpy_s(lastPath, path, _TRUNCATE);
    if (orderedRun) { assert(orderedIndex < 4 && std::strstr(path, files[orderedIndex])); ++orderedIndex; }
    *handle = nextError || plays == 1 ? 0 : 100 + plays;
    return nextError;
}
static unsigned int closeFile(uint64 connection, uint64 handle) {
    assert(connection == waveConnection && handle == wave);
    ++closes;
    lastClosed = handle;
    return closeError;
}

static int testConnectionStatus = STATUS_CONNECTION_ESTABLISHED;
static const char* testServerIp = "127.0.0.1";
static uint64 testServerPort = 19987;
static bool invalidateDuringValidation{};
static unsigned int captureRegisters{}, captureUnregisters{}, captureRegisterError{};
static anyID sourceOwnClient = 77;
static unsigned int registerCaptureMock(const char* id, const char* name, int capRate, int capChannels, int playRate, int playChannels) {
    assert(!std::strcmp(id, knownCaptureId) && !std::strcmp(name, "RICON Known PCM (TOOL_ONLY)"));
    assert(capRate == 48000 && capChannels == 1 && playRate == 48000 && playChannels == 2);
    ++captureRegisters;
    return captureRegisterError;
}
static unsigned int unregisterCaptureMock(const char* id) { assert(!std::strcmp(id, knownCaptureId)); ++captureUnregisters; return 0; }
static unsigned int connectionStatusMock(uint64, int* status) { *status = testConnectionStatus; return 0; }
static unsigned int ownClientMock(uint64 connection, anyID* client) { *client = connection == 78 ? 78 : sourceOwnClient; return 0; }
static unsigned int serverIpMock(uint64, anyID client, size_t flag, char** value) {
    assert((client == sourceOwnClient || client == 78) && flag == CONNECTION_SERVER_IP);
    if (invalidateDuringValidation) ts3plugin_onClientMoveEvent(77, 41, 1, 0, 0, "");
    *value = _strdup(testServerIp);
    return 0;
}

static bool armVoiceTest(uint64 connection, anyID client, bool lost, uint64 deadline) {
    uint64 generation{};
    {
        std::lock_guard<std::mutex> lock(voiceOperations);
        generation = voiceGeneration;
    }
    return armVoiceProbe(connection, client, lost, deadline, generation);
}
static unsigned int serverPortMock(uint64, anyID client, size_t flag, uint64* value) {
    assert((client == sourceOwnClient || client == 78) && flag == CONNECTION_SERVER_PORT);
    *value = testServerPort;
    return 0;
}
static unsigned int peerMock(uint64, anyID client, size_t flag, char** value) {
    assert(flag == CLIENT_UNIQUE_IDENTIFIER);
    *value = client == 41 ? _strdup("test-only-peer") : nullptr;
    return client == 41 ? 0 : ERROR_client_invalid_id;
}

static void verifyVoiceAdapter() {
    short source[1920]{}, output[1920]{}, expected[1920]{};
    for (int i = 0; i < 1920; ++i) source[i] = i & 1 ? 12000 : -12000;
    std::memcpy(output, source, sizeof(source));
    assert(!editVoiceProbe(77, 41, output, 960, 2, 1) && !std::memcmp(output, source, sizeof(source)));
    assert(!armVoiceTest(0, 41, false, 100) && !armVoiceTest(77, 0, false, 100));
    const uint64 deadline = GetTickCount64() + 10000;
    assert(armVoiceTest(77, 41, false, deadline));
    assert(!editVoiceProbe(78, 41, output, 960, 2, 1));
    assert(!editVoiceProbe(77, 42, output, 960, 2, 1));
    assert(!editVoiceProbe(77, 41, nullptr, 960, 2, 1));
    assert(!editVoiceProbe(77, 41, output, -1, 2, 1));
    assert(!editVoiceProbe(77, 41, output, 5761, 2, 1));
    assert(!editVoiceProbe(77, 41, output, 960, 3, 1));
    std::atomic<bool> held{false}, release{false};
    std::thread controller([&] {
        std::lock_guard<std::mutex> lock(voiceOperations);
        held.store(true);
        while (!release.load()) Sleep(1);
    });
    for (int wait = 0; !held.load() && wait < 5000; ++wait) Sleep(1);
    assert(held.load());
    assert(!editVoiceProbe(77, 41, output, 960, 2, 1)); // Audio never waits on another control thread.
    release.store(true);
    controller.join();
    assert(!std::memcmp(output, source, sizeof(source)) && voiceState.metrics.callbacks_total == 0);
    RiconAudioState reference{};
    ricon_audio_state_init(&reference, 0x1234567u ^ 41);
    RiconAudioChannelParams parameters{1, 2, 4.0f, 0.7f, -1.0f, -1.0f};
    std::memcpy(expected, source, sizeof(source));
    ricon_audio_process_playback_voice(&reference, &parameters, expected, 960, 2);
    ts3plugin_onEditPlaybackVoiceDataEvent(77, 41, output, 960, 2);
    assert(!std::memcmp(output, expected, sizeof(output)) && std::memcmp(output, source, sizeof(source)));
    assert(voiceState.metrics.callbacks_dsp == 1 && voiceState.metrics.callbacks_muted == 0);
    assert(armVoiceTest(77, 41, true, deadline));
    std::memset(output, 0, sizeof(output));
    ts3plugin_onEditPlaybackVoiceDataEvent(77, 41, output, 960, 2);
    bool nonzero = false;
    for (short value : output) nonzero |= value != 0;
    assert(nonzero && voiceState.metrics.callbacks_dsp == 1 && voiceState.metrics.callbacks_muted == 0);
    assert(armVoiceTest(77, 41, false, 100));
    assert(!editVoiceProbe(77, 41, output, 960, 2, 100) && !voiceClient);
    assert(!editVoiceProbe(77, 41, output, 960, 2, 99)); // Expiry does not resurrect.
    assert(armVoiceTest(77, 41, false, deadline));
    ts3plugin_currentServerConnectionChanged(78);
    assert(!voiceClient);
    ts3plugin_currentServerConnectionChanged(77);
    assert(armVoiceTest(77, 41, false, deadline));
    ts3plugin_onConnectStatusChangeEvent(77, STATUS_DISCONNECTED, 0);
    assert(!voiceClient);
    assert(armVoiceTest(77, 41, false, deadline));
    ts3plugin_onClientMoveEvent(77, 41, 1, 0, 0, "");
    assert(!voiceClient);
    assert(armVoiceTest(77, 41, false, deadline));
    ts3plugin_onMenuItemEvent(77, PLUGIN_MENU_TYPE_GLOBAL, 4, 0);
    assert(!voiceClient);
    anyID client{};
    bool lost{};
    assert(voiceCommand("voice 41", client, lost) && client == 41 && !lost);
    assert(voiceCommand("noise 65535", client, lost) && client == 65535 && lost);
    for (const char* invalid : {"voice 0", "voice +1", "voice -1", "voice 01", "voice 65536", "voice 1 ", "voice 1 x", "voice 999999999999999999999999999999", "noise ", "unknown 1"})
        assert(!voiceCommand(invalid, client, lost));
    assert(!voiceCommand(nullptr, client, lost));
    assert(ts3plugin_processCommand(77, "voice 41") == 0 && !voiceClient); // No test-profile opt-in.
    char profile[4096]{}, enabled[4096]{};
    assert(join(profile, sizeof(profile), folder, "ts3-pcm-prototype-01"));
    assert(CreateDirectoryA(profile, nullptr));
    assert(join(configDirectory, sizeof(configDirectory), profile, "ricon-probe"));
    assert(CreateDirectoryA(configDirectory, nullptr));
    assert(join(enabled, sizeof(enabled), configDirectory, "ricon-voice-test.enabled"));
    FILE* marker{};
    assert(!fopen_s(&marker, enabled, "wb") && marker);
    std::fclose(marker);
    api.registerCustomDevice = registerCaptureMock;
    api.unregisterCustomDevice = unregisterCaptureMock;
    captureRegisterError = 1;
    registerKnownCapture();
    assert(!knownCaptureRegistered && captureRegisters == 1);
    unregisterKnownCapture();
    assert(captureUnregisters == 0);
    captureRegisterError = 0;
    registerKnownCapture();
    registerKnownCapture();
    assert(knownCaptureRegistered && captureRegisters == 2);
    unregisterKnownCapture();
    assert(!knownCaptureRegistered && captureUnregisters == 1);
    api.getConnectionStatus = connectionStatusMock;
    api.getClientID = ownClientMock;
    api.getConnectionVariableAsString = serverIpMock;
    api.getConnectionVariableAsUInt64 = serverPortMock;
    api.getClientVariableAsString = peerMock;
    assert(ts3plugin_processCommand(77, "voice 41") == 0 && voiceClient == 41);
    assert(ts3plugin_processCommand(77, "stop") == 0 && !voiceClient);
    assert(ts3plugin_processCommand(77, "noise 41") == 0 && voiceClient == 41 && voiceParams.state == 1);
    testServerIp = "192.0.2.1";
    assert(ts3plugin_processCommand(77, "voice 41") == 0 && !voiceClient);
    testServerIp = "127.0.0.1";
    testServerPort = 9987;
    assert(ts3plugin_processCommand(77, "voice 41") == 0 && !voiceClient);
    testServerPort = 19987;
    testConnectionStatus = STATUS_DISCONNECTED;
    assert(ts3plugin_processCommand(77, "voice 41") == 0 && !voiceClient);
    testConnectionStatus = STATUS_CONNECTION_ESTABLISHED;
    assert(ts3plugin_processCommand(78, "voice 41") == 0 && !voiceClient);
    assert(ts3plugin_processCommand(77, "voice 77") == 0 && !voiceClient);
    assert(ts3plugin_processCommand(77, "voice 42") == 0 && !voiceClient);
    invalidateDuringValidation = true;
    assert(ts3plugin_processCommand(77, "voice 41") == 0 && !voiceClient); // A clear event during SDK queries wins.
    invalidateDuringValidation = false;
    assert(DeleteFileA(enabled));
    assert(ts3plugin_processCommand(77, "voice 41") == 0 && !voiceClient);
    strncpy_s(configDirectory, folder, _TRUNCATE);
    std::puts("RICON_ARMA3_CALLBACK_ADAPTER_SELFTEST_PASS");
}

static const char* captureModeMock = "custom";
static unsigned int customSubmissions{};
static const char* testUid = "test-server";
static uint64 listenerChannel = 1;
static unsigned int captureCloseError{}, captureCloses{}, networkOutputCloses{}, networkStops{};
static unsigned int serverUidQuery(uint64, size_t flag, char** value) { assert(flag == VIRTUALSERVER_UNIQUE_IDENTIFIER); *value = _strdup(testUid); return 0; }
static unsigned int channelQuery(uint64 connection, anyID, uint64* channel) { *channel = connection == 78 ? listenerChannel : 1; return 0; }
static unsigned int closeCaptureMock(uint64) { ++captureCloses; return captureCloseError; }
static unsigned int closeNetworkOutputMock(uint64) { ++networkOutputCloses; return 0; }
static unsigned int stopNetworkMock(uint64, const char*) { ++networkStops; return 0; }
static unsigned int activateMock(uint64 connection) { assert(connection == 77); return ERROR_ok_no_update; }
static unsigned int flushResult = ERROR_ok_no_update;
static unsigned int flushStateMock(uint64, const char*) { return flushResult; }
static bool includeListenerIdentity = true;
static unsigned int identityProfilesMock(PluginGuiProfile kind, int* preferred, char*** result) {
    assert(kind == PLUGIN_GUI_IDENTITY);
    *preferred = 1; // Even when RICON_B becomes the GUI default, A must use the other identity.
    *result = static_cast<char**>(std::calloc(3, sizeof(char*)));
    (*result)[0] = _strdup("Default");
    if (includeListenerIdentity) (*result)[1] = _strdup("RICON_B");
    return 0;
}
static unsigned int captureModeQuery(uint64, char** mode) { *mode = _strdup(captureModeMock); return 0; }
static unsigned int captureNameQuery(uint64, char** name, int* isDefault) { *name = _strdup(knownCaptureId); *isDefault = 0; return 0; }
static unsigned int listenerInputQuery(uint64, size_t flag, int* value) { assert(flag == CLIENT_INPUT_DEACTIVATED); *value = INPUT_DEACTIVATED; return 0; }
static unsigned int submitCustom(const char* id, const short* samples, int count) {
    assert(!std::strcmp(id, knownCaptureId) && samples && count == 960);
    ++customSubmissions;
    return 0;
}

static unsigned int bootstrapModeFailures[2]{}, bootstrapDeviceFailures[2]{}, bootstrapDeviceReads[2]{};
static bool bootstrapWrongMode{}, bootstrapWrongDevice{}, bootstrapCancelMode{}, bootstrapReconnectName{}, bootstrapForeignName{};
static unsigned int bootstrapSlowDeviceMs{};
static unsigned int bootstrapFreedErrors{}, bootstrapFreedControlErrors{};
static unsigned int bootstrapFree(void* value) {
    if (value && (!std::strcmp(static_cast<const char*>(value), "error-mode") || !std::strcmp(static_cast<const char*>(value), "error-device"))) ++bootstrapFreedErrors;
    if (value && (!std::strcmp(static_cast<const char*>(value), "error-ip") || !std::strcmp(static_cast<const char*>(value), "error-uid"))) ++bootstrapFreedControlErrors;
    return freeSdk(value);
}
static unsigned int bootstrapModeQuery(uint64 connection, char** mode) {
    const int actor = connection == 77 ? 0 : 1;
    if (bootstrapCancelMode) { bootstrapCancelMode = false; requestWorkerStop(); }
    if (bootstrapModeFailures[actor]) { --bootstrapModeFailures[actor]; *mode = _strdup("error-mode"); return ERROR_undefined; }
    *mode = _strdup(bootstrapWrongMode ? "Windows Audio Session" : "custom"); return 0;
}
static unsigned int bootstrapDeviceQuery(uint64 connection, char** name, int* isDefault) {
    const int actor = connection == 77 ? 0 : 1;
    ++bootstrapDeviceReads[actor];
    if (connection == 78 && bootstrapSlowDeviceMs) Sleep(bootstrapSlowDeviceMs);
    if (bootstrapReconnectName) { bootstrapReconnectName = false; ts3plugin_onConnectStatusChangeEvent(77, STATUS_DISCONNECTED, 0); }
    if (bootstrapForeignName) { bootstrapForeignName = false; sourceOwnClient = 79; }
    if (bootstrapDeviceFailures[actor]) { --bootstrapDeviceFailures[actor]; *name = _strdup("error-device"); return ERROR_sound_unknown_device; }
    *name = _strdup(bootstrapWrongDevice ? "another virtual input" : knownCaptureId); *isDefault = 0; return 0;
}

static void verifyLiveBootstrap() {
    const auto saved = api;
    const uint64 savedCurrent = currentConnection.load();
    api.getCurrentCaptureMode = bootstrapModeQuery;
    api.getCurrentCaptureDeviceName = bootstrapDeviceQuery;
    api.freeMemory = bootstrapFree;
    api.openCaptureDevice = [](uint64, const char*, const char*) -> unsigned int { assert(false); return 1; };
    NetworkBinding binding{}; bool bound{};
    const auto reset = [&] {
        binding = {}; binding.source = 77; binding.listener = 78; strcpy_s(binding.ip, "127.0.0.1"); bound = false;
        networkSource.store(77); networkListener.store(78); networkPrepared.store(false); currentConnection.store(78);
        workerStop.store(false); testConnectionStatus = STATUS_CONNECTION_ESTABLISHED; sourceOwnClient = 77;
        testServerIp = "127.0.0.1"; testServerPort = 19987; testUid = "test-server"; listenerChannel = 1;
        bootstrapWrongMode = bootstrapWrongDevice = bootstrapCancelMode = bootstrapReconnectName = bootstrapForeignName = false;
        bootstrapSlowDeviceMs = 0;
        bootstrapFreedErrors = 0;
        for (int i = 0; i < 2; ++i) bootstrapModeFailures[i] = bootstrapDeviceFailures[i] = bootstrapDeviceReads[i] = 0;
    };
    LiveCaptureReadiness capture[2]{};
    api.getConnectionVariableAsString = [](uint64, anyID, size_t, char** value) -> unsigned int { *value = _strdup("error-ip"); return ERROR_undefined; };
    anyID ignored{};
    bootstrapFreedControlErrors = 0;
    assert(!connectionAt(77, "127.0.0.1", ignored) && bootstrapFreedControlErrors == 1);
    api.getConnectionVariableAsString = saved.getConnectionVariableAsString;
    api.getServerVariableAsString = [](uint64, size_t, char** value) -> unsigned int { *value = _strdup("error-uid"); return ERROR_undefined; };
    char ignoredUid[128]{};
    assert(!serverUid(77, ignoredUid) && !ignoredUid[0] && bootstrapFreedControlErrors == 2);
    api.getServerVariableAsString = saved.getServerVariableAsString;
    reset();
    uint64 ticket = workerGeneration.load(), deadline = GetTickCount64() + 15000;
    testConnectionStatus = STATUS_CONNECTING;
    assert(pollLiveBootstrap(binding, bound, ticket, deadline, capture) == LiveBootstrapResult::waiting && !bound);
    testConnectionStatus = STATUS_CONNECTION_ESTABLISHED;
    bootstrapModeFailures[0] = 1; bootstrapDeviceFailures[1] = 2;
    assert(pollLiveBootstrap(binding, bound, ticket, deadline, capture) == LiveBootstrapResult::waiting && bound);
    assert(capture[0].modeQueried && capture[1].deviceQueried && capture[0].modeResult == ERROR_undefined
           && capture[1].deviceResult == ERROR_sound_unknown_device && !capture[0].known && !capture[1].known);
    assert(!capture[0].mode[0] && !capture[1].device[0]);
    assert(pollLiveBootstrap(binding, bound, ticket, deadline, capture) == LiveBootstrapResult::waiting);
    assert(pollLiveBootstrap(binding, bound, ticket, deadline, capture) == LiveBootstrapResult::ready && capture[0].known && capture[1].known);
    assert(bootstrapFreedErrors == 3); // SDK-owned output buffers are released even when their query reports an error.
    assert(binding.sourceClient == 77 && binding.listenerClient == 78 && !std::strcmp(binding.serverUid, "test-server"));
    recordLiveBootstrapCapture(77, "mock-A", capture[0]);

    reset(); ticket = workerGeneration.load(); deadline = GetTickCount64() + 15000;
    bootstrapWrongMode = true;
    assert(pollLiveBootstrap(binding, bound, ticket, deadline, capture) == LiveBootstrapResult::waiting && !capture[0].known && !capture[1].known);
    bootstrapWrongMode = false; bootstrapWrongDevice = true;
    assert(pollLiveBootstrap(binding, bound, ticket, deadline, capture) == LiveBootstrapResult::waiting && !capture[0].known && !capture[1].known);
    const unsigned int reads = bootstrapDeviceReads[0] + bootstrapDeviceReads[1];
    assert(pollLiveBootstrap(binding, bound, ticket, GetTickCount64(), capture) == LiveBootstrapResult::expired);
    assert(bootstrapDeviceReads[0] + bootstrapDeviceReads[1] == reads); // Timeout does not renew or perform more queries.

    reset(); ticket = workerGeneration.load(); bootstrapCancelMode = true;
    assert(pollLiveBootstrap(binding, bound, ticket, GetTickCount64() + 15000, capture) == LiveBootstrapResult::cancelled);
    assert(!capture[0].deviceQueried && !capture[1].modeQueried);
    reset(); ticket = workerGeneration.load(); bootstrapReconnectName = true;
    assert(pollLiveBootstrap(binding, bound, ticket, GetTickCount64() + 15000, capture) == LiveBootstrapResult::cancelled);
    reset(); ticket = workerGeneration.load(); bootstrapForeignName = true;
    assert(pollLiveBootstrap(binding, bound, ticket, GetTickCount64() + 15000, capture) == LiveBootstrapResult::changed);
    assert(!stillOwnGuiConnection(binding, 77, 77, false)); // Observed foreign reconnect is not ours even before full setup.
    reset(); ticket = workerGeneration.load(); sourceOwnClient = 78;
    assert(pollLiveBootstrap(binding, bound, ticket, GetTickCount64() + 15000, capture) == LiveBootstrapResult::changed && !bound);
    reset(); ticket = workerGeneration.load();
    assert(pollLiveBootstrap(binding, bound, ticket, GetTickCount64() + 15000, capture) == LiveBootstrapResult::ready);
    testUid = "another-server";
    assert(pollLiveBootstrap(binding, bound, ticket, GetTickCount64() + 15000, capture) == LiveBootstrapResult::changed);
    assert(!stillOwnGuiConnection(binding, 78, 78, false));
    reset(); ticket = workerGeneration.load(); bootstrapSlowDeviceMs = 1100;
    assert(pollLiveBootstrap(binding, bound, ticket, GetTickCount64() + 1000, capture) == LiveBootstrapResult::expired);
    assert(capture[1].deviceQueried); // Last SDK query completing after the original deadline cannot authorize READY.
    reset(); api = saved; currentConnection.store(savedCurrent); networkPrepared.store(false);
    std::puts("RICON_TS3_LIVE_BOOTSTRAP_SELFTEST_PASS");
}

static void verifyNetworkInput() {
    assert(stateUpdateOk(ERROR_ok) && stateUpdateOk(ERROR_ok_no_update));
    assert(!stateUpdateOk(ERROR_undefined) && !stateUpdateOk(ERROR_not_implemented) && !stateUpdateOk(ERROR_sound_unknown_device));
    api.flushClientSelfUpdates = flushStateMock;
    assert(flushTestState(77, "test-flush-unchanged"));
    flushResult = ERROR_not_implemented;
    assert(!flushTestState(77, "test-flush-error"));
    flushResult = ERROR_ok_no_update;
    api.getProfileList = identityProfilesMock;
    char sourceIdentity[256]{}, listenerIdentity[256]{};
    assert(testIdentities(sourceIdentity, listenerIdentity) && !std::strcmp(sourceIdentity, "Default") && !std::strcmp(listenerIdentity, "RICON_B"));
    includeListenerIdentity = false;
    sourceIdentity[0] = listenerIdentity[0] = 0;
    assert(!testIdentities(sourceIdentity, listenerIdentity));
    includeListenerIdentity = true;
    char ip[16]{};
    assert(parsePrivateTestIp("172.16.100.20\r\n", 15, ip) && !std::strcmp(ip, "172.16.100.20"));
    assert(parsePrivateTestIp("127.0.0.1", 9, ip));
    assert(parsePrivateTestIp("10.0.0.1", 8, ip));
    for (const char* bad : {"8.8.8.8", "0.0.0.0", "localhost", "172.32.0.1", "172.020.1.1", "172.20.1.256", "172.20.1", "172.20.1.1 x"})
        assert(!parsePrivateTestIp(bad, std::strlen(bad), ip));
    assert(!parsePrivateTestIp(nullptr, 1, ip) && !parsePrivateTestIp("127.0.0.1\0x", 11, ip));
    char path[4096]{};
    assert(join(path, sizeof(path), configDirectory, "ricon-voice-test-server.txt"));
    assert(expectedTestServer(ip) && !std::strcmp(ip, "127.0.0.1"));
    FILE* file{};
    assert(!fopen_s(&file, path, "wb") && file);
    std::fputs("172.16.100.20\r\n", file); std::fclose(file);
    assert(expectedTestServer(ip) && !std::strcmp(ip, "172.16.100.20"));
    assert(DeleteFileA(path));
    assert(CreateDirectoryA(path, nullptr));
    assert(!expectedTestServer(ip));
    assert(RemoveDirectoryA(path));
    api.getCurrentCaptureMode = captureModeQuery;
    api.getCurrentCaptureDeviceName = captureNameQuery;
    api.getClientSelfVariableAsInt = listenerInputQuery;
    api.processCustomCaptureData = submitCustom;
    api.getServerVariableAsString = serverUidQuery;
    api.getChannelOfClient = channelQuery;
    api.activateCaptureDevice = activateMock;
    char savedConfig[4096]{};
    strcpy_s(savedConfig, configDirectory);
    char parent[4096]{};
    assert(join(parent, sizeof(parent), folder, "ts3-pcm-prototype-01"));
    assert(join(configDirectory, sizeof(configDirectory), parent, "ricon-probe"));
    char flag[4096]{};
    assert(join(flag, sizeof(flag), configDirectory, "ricon-voice-test.enabled"));
    assert(!fopen_s(&file, flag, "wb") && file); std::fclose(file);
    NetworkBinding binding{};
    binding.source = 77; binding.listener = 78; binding.sourceClient = 77; binding.listenerClient = 78; binding.channel = 1;
    strcpy_s(binding.ip, "127.0.0.1"); strcpy_s(binding.serverUid, "test-server");
    currentConnection.store(78);
    workerStop.store(false);
    const uint64 ticket = workerGeneration.load();
    uint64 next = GetTickCount64();
    assert(sendKnownFrames(binding, nullptr, 2, next, ticket) && customSubmissions == 2);
    captureModeMock = "Windows Audio Session";
    assert(!sendKnownFrames(binding, nullptr, 1, next, ticket) && customSubmissions == 2);
    captureModeMock = "custom";
    testServerIp = "192.0.2.1";
    assert(!sendKnownFrames(binding, nullptr, 1, next, ticket) && customSubmissions == 2);
    testServerIp = "127.0.0.1";
    sourceOwnClient = 79;
    assert(!sendKnownFrames(binding, nullptr, 1, next, ticket) && customSubmissions == 2);
    sourceOwnClient = 77;
    testUid = "new-server";
    assert(!sendKnownFrames(binding, nullptr, 1, next, ticket) && customSubmissions == 2);
    testUid = "test-server";
    listenerChannel = 2;
    assert(!sendKnownFrames(binding, nullptr, 1, next, ticket) && customSubmissions == 2);
    listenerChannel = 1;
    networkSource.store(77); networkListener.store(78); networkSourceClient.store(77); networkListenerClient.store(78); networkPrepared.store(true);
    ts3plugin_onConnectStatusChangeEvent(77, STATUS_DISCONNECTED, 0);
    workerStop.store(false);
    assert(!sendKnownFrames(binding, nullptr, 1, next, ticket) && customSubmissions == 2);
    networkPrepared.store(false);
    currentConnection.store(77);
    assert(!armVoiceProbe(77, 41, false, GetTickCount64() + 1000, voiceGeneration, ticket));
    api.closeCaptureDevice = closeCaptureMock; api.closePlaybackDevice = closeNetworkOutputMock; api.stopConnection = stopNetworkMock;
    assert(closeTestConnection(77) && captureCloses == 1 && networkOutputCloses == 1 && networkStops == 1);
    captureCloseError = ERROR_sound_device_in_use;
    assert(!closeTestConnection(77) && captureCloses == 2 && networkOutputCloses == 2 && networkStops == 2);
    captureCloseError = 0;
    NetworkBinding fresh{};
    fresh.source = 77; fresh.listener = 78; strcpy_s(fresh.ip, "127.0.0.1");
    currentConnection.store(78);
    captureModeMock = "Windows Audio Session";
    workerStop.store(false); networkRunning.store(true);
    runNetworkProbe(fresh, workerGeneration.load()); // Successful GUI handlers, then failed capture validation.
    assert(!networkRunning.load() && customSubmissions == 2 && captureCloses == 4 && networkOutputCloses == 4 && networkStops == 4);
    testConnectionStatus = STATUS_CONNECTING;
    workerStop.store(true); networkRunning.store(true);
    runNetworkProbe(fresh, workerGeneration.load()); // Cancel before an initial connection can establish.
    assert(!networkRunning.load() && customSubmissions == 2 && captureCloses == 6 && networkOutputCloses == 6 && networkStops == 6);
    testConnectionStatus = STATUS_CONNECTION_ESTABLISHED;
    testServerIp = "192.0.2.1";
    assert(!stillOwnGuiConnection(fresh, 77, 0, false)); // Positively foreign reconnect is never rolled back.
    testServerIp = "127.0.0.1"; captureModeMock = "custom";
    currentConnection.store(77);
    verifyLiveBootstrap();
    assert(DeleteFileA(flag));
    strcpy_s(configDirectory, savedConfig);
    char savedData[4096]{};
    strcpy_s(savedData, dataDirectory);
    assert(join(dataDirectory, sizeof(dataDirectory), folder, "known-wave"));
    assert(CreateDirectoryA(dataDirectory, nullptr));
    assert(join(path, sizeof(path), dataDirectory, files[1]));
    unsigned char header[44] = {'R','I','F','F',0x24,0x77,1,0,'W','A','V','E','f','m','t',' ',16,0,0,0,1,0,1,0,0x80,0xbb,0,0,0,0x77,1,0,2,0,16,0,'d','a','t','a',0,0x77,1,0};
    short samples[48000]{};
    assert(!fopen_s(&file, path, "wb") && file);
    assert(std::fwrite(header, 1, 44, file) == 44 && std::fwrite(samples, 2, 48000, file) == 48000); std::fclose(file);
    assert(readKnownWave(1, samples));
    header[34] = 24;
    assert(!fopen_s(&file, path, "wb") && file);
    assert(std::fwrite(header, 1, 44, file) == 44); std::fclose(file);
    assert(!readKnownWave(1, samples));
    strcpy_s(dataDirectory, savedData);
    assert(!beginNetworkProbe()); // No SDK GUI/connection functions in this mock.
    assert(!beginNetworkProbe(false, true)); // Good-profile fixture retains the same SDK/source gates.
    assert(!beginNetworkProbe(true, true)); // Hardware-live and known-input variants cannot be combined.
    assert(ts3plugin_processCommand(77, "network-good") == 0 && !networkRunning.load());
    std::puts("RICON_TS3_NETWORK_INPUT_SELFTEST_PASS");
}

static void verifyLiveLevelsAndOutcome() {
    const short data[] = {0, 3, -4};
    const PcmLevel measured = measurePcm(data, 3);
    assert(measured.values == 3 && measured.squares == 25.0 && measured.peak == 4);
    assert(std::fabs(measured.rms() - std::sqrt(25.0 / 3.0)) < 1e-12);
    assert(!measurePcm(nullptr, 3).values && !measurePcm(data, 0).values && !measurePcm(data, 11521).values);
    PcmLevel levels[3]{}; takeLiveLevels(levels);
    observeLivePcm(0, data, 3); observeLivePcm(1, data, 3); observeLivePcm(2, data, 3);
    assert(!takeLiveLevels(levels));
    for (const PcmLevel& level : levels) assert(level.values == 3 && level.squares == 25.0 && level.peak == 4);
    assert(!takeLiveLevels(levels) && !levels[0].values && !levels[1].values && !levels[2].values);
    std::atomic<bool> held{false}, release{false};
    std::thread holder([&] {
        std::lock_guard<std::mutex> lock(liveLevelOperations);
        held.store(true);
        while (!release.load()) Sleep(1);
    });
    for (int wait = 0; !held.load() && wait < 5000; ++wait) Sleep(1);
    assert(held.load());
    observeLivePcm(0, data, 3);
    release.store(true); holder.join();
    assert(takeLiveLevels(levels) == 1 && !levels[0].values);
    assert(!std::strcmp(liveTerminalAction(true, false, true), "live-failed")); // Historical run03 publisher failure must not report completion.
    assert(!std::strcmp(liveTerminalAction(true, true, false), "live-failed"));
    assert(!std::strcmp(liveTerminalAction(true, true, true), "live-complete"));
    assert(!std::strcmp(liveTerminalAction(false, true, true), "live-stopped"));
    const auto saved = api;
    api.getCurrentCaptureMode = [](uint64, char** value) -> unsigned int { *value = _strdup("mock hardware backend"); return 0; };
    api.getCurrentCaptureDeviceName = [](uint64, char** value, int* isDefault) -> unsigned int { *value = _strdup("synthetic input\nnot a device"); *isDefault = 1; return 0; };
    api.getClientSelfVariableAsInt = [](uint64, size_t, int* value) -> unsigned int { *value = 0; return 0; };
    api.getPreProcessorConfigValue = [](uint64, const char*, char** value) -> unsigned int { *value = _strdup("false"); return 0; };
    api.openCaptureDevice = [](uint64, const char*, const char*) -> unsigned int { assert(false); return 1; };
    recordLiveCaptureState(81);
    unsigned int playbackReads{};
    api.getPlaybackConfigValueAsFloat = [](uint64, const char* key, float* value) -> unsigned int {
        assert(!std::strcmp(key, "volume_modifier")); *value = -6.0f; return 0;
    };
    api.getClientVariableAsInt = [](uint64, anyID, size_t flag, int* value) -> unsigned int {
        assert(flag == CLIENT_VOLUME_MODIFICATOR); *value = -3; return 0;
    };
    recordLivePlaybackState(81, 3);
    (void)playbackReads;
    LivePairedLevels paired{}; paired.blocks = 1; paired.values = 3;
    measurePairedStage(data, 3, paired.decodedSquares, paired.decodedPeak);
    paired.postCSquares = paired.finalSquares = 9; paired.postCPeak = paired.finalPeak = 3;
    observeLivePaired(paired);
    const LivePairedLevels pairRead = takeLivePaired();
    assert(pairRead.values == 3 && pairRead.decodedSquares == 25 && pairRead.postCSquares == 9 && pairRead.finalSquares == 9);
    assert(!takeLivePaired().values);
    api = saved;
    std::puts("RICON_TS3_LEVELS_AND_CONTROL_SELFTEST_PASS");
}

static void verifyDspAudibility() {
    const uint64 savedCurrent = currentConnection.load();
    currentConnection.store(77);
    uint64 generation{}; clearVoiceProbe(&generation);
    assert(armVoiceProbe(77, 41, false, GetTickCount64() + 1500, generation));
    const RiconAudioChannelParams historical = voiceParams;
    assert(setLiveRadioProfile(generation));
    const RiconAudioChannelParams profiles[] = {historical, voiceParams};
    clearVoiceProbe(); currentConnection.store(savedCurrent);
    const char* names[] = {"historical4dB", "good30dB"};
    constexpr double pi = 3.14159265358979323846;
    double oldNoise{}, goodNoise{};
    for (int profile = 0; profile < 2; ++profile) for (double decodeGain : {1.0, 0.25}) for (int amplitude : {0, 128, 512, 2048, 8192}) {
        RiconAudioState signalState{}, noiseState{};
        ricon_audio_state_init(&signalState, 0x1234567u ^ 5u);
        ricon_audio_state_init(&noiseState, 0x1234567u ^ 5u);
        double sourceSquares{}, decodedSquares{}, filteredSquares{}, noiseSquares{};
        for (int start = 0; start < 48000; start += 960) {
            short decoded[960]{}, processed[960]{}, noise[960]{};
            for (int i = 0; i < 960; ++i) {
                const double t = static_cast<double>(start + i) / 48000.0;
                const double envelope = 0.5 - 0.5 * std::cos(2.0 * pi * 4.0 * t);
                const double harmonics = std::sin(2.0 * pi * 160.0 * t) + 0.5 * std::sin(2.0 * pi * 320.0 * t)
                    + 0.25 * std::sin(2.0 * pi * 640.0 * t) + 0.125 * std::sin(2.0 * pi * 1280.0 * t)
                    + 0.0625 * std::sin(2.0 * pi * 2560.0 * t);
                const short source = static_cast<short>(std::lround(amplitude * envelope * harmonics / 1.9375));
                decoded[i] = static_cast<short>(std::lround(source * decodeGain));
                sourceSquares += static_cast<double>(source) * source;
                decodedSquares += static_cast<double>(decoded[i]) * decoded[i];
            }
            std::memcpy(processed, decoded, sizeof(decoded));
            // Equal seed and block layout keep receiver noise identical; subtraction isolates the passed synthetic signal.
            ricon_audio_process_playback_voice(&signalState, &profiles[profile], processed, 960, 1);
            ricon_audio_process_playback_voice(&noiseState, &profiles[profile], noise, 960, 1);
            for (int i = 0; i < 960; ++i) {
                const double signal = static_cast<double>(processed[i]) - noise[i];
                filteredSquares += signal * signal;
                noiseSquares += static_cast<double>(noise[i]) * noise[i];
            }
        }
        const double sourceRms = std::sqrt(sourceSquares / 48000.0), decodedRms = std::sqrt(decodedSquares / 48000.0);
        const double filteredRms = std::sqrt(filteredSquares / 48000.0), noiseRms = std::sqrt(noiseSquares / 48000.0);
        assert(signalState.metrics.callbacks_dsp == 50 && !signalState.metrics.callbacks_passthrough && !signalState.metrics.sample_values_clipped);
        assert(noiseState.metrics.callbacks_dsp == 50 && !noiseState.metrics.sample_values_clipped && noiseRms > 0);
        if (!amplitude) assert(!filteredSquares);
        else assert(filteredRms > decodedRms * 0.03); // Muting the useful signal must not pass as a hiss fix.
        const double audioSnr = filteredSquares > 0 ? 10.0 * std::log10(filteredSquares / noiseSquares) : 0;
        char ratio[32]{};
        if (filteredSquares > 0) std::snprintf(ratio, sizeof(ratio), "%.3f", audioSnr);
        else strcpy_s(ratio, "n/a");
        std::printf("RICON_DSP_LEVEL profile=%s sourceAmplitude=%d modeledDecodeGain=%.2f sourceRms=%.3f decodedRms=%.3f filteredSignalRms=%.3f addedNoiseRms=%.3f audioSnrDb=%s\n",
                    names[profile], amplitude, decodeGain, sourceRms, decodedRms, filteredRms, noiseRms, ratio);
        if (decodeGain == 1.0 && amplitude == 128 && profile == 0) assert(audioSnr < 0);
        if (decodeGain == 1.0 && amplitude == 2048) {
            if (profile == 0) oldNoise = noiseRms;
            else { goodNoise = noiseRms; assert(audioSnr > 10); }
        }
    }
    assert(goodNoise > 0 && goodNoise < oldNoise / 20.0);
    std::puts("RICON_TS3_SYNTHETIC_SIGNAL_NOISE_SELFTEST_PASS");
}

static void verifyLiveAudioProfile() {
    ricon_audio_init(); ricon_live150_audio_init();
    const RiconAudioChannelParams params{1, 2, 30.0f, 20.0f, -1.0f, -1.0f};
    const double pi = 3.14159265358979323846;
    for (int hz : {160, 320, 640, 1200, 2560}) {
        RiconAudioState old{}, body{}, oldNoise{}, bodyNoise{};
        ricon_audio_state_init(&old, 1234); ricon_live150_audio_state_init(&body, 1234);
        ricon_audio_state_init(&oldNoise, 1234); ricon_live150_audio_state_init(&bodyNoise, 1234);
        double oldPower{}, bodyPower{}; unsigned int oldPeak{}, bodyPeak{};
        for (int block = 0; block < 50; ++block) {
            short a[960]{}, b[960]{}, n0[960]{}, n1[960]{};
            for (int i = 0; i < 960; ++i) a[i] = static_cast<short>(std::lround(2048 * std::sin(2 * pi * hz * (block * 960 + i) / 48000.0)));
            std::memcpy(b, a, sizeof(a));
            ricon_audio_process_playback_voice(&old, &params, a, 960, 1);
            ricon_live150_audio_process_playback_voice(&body, &params, b, 960, 1, nullptr, nullptr);
            ricon_audio_process_playback_voice(&oldNoise, &params, n0, 960, 1);
            ricon_live150_audio_process_playback_voice(&bodyNoise, &params, n1, 960, 1, nullptr, nullptr);
            assert(!std::memcmp(n0, n1, sizeof(n0))); // Same noise bytes, not merely similar RMS.
            assert(old.noise_seed == body.noise_seed && old.fade_db == body.fade_db);
            assert(old.noise_seed == oldNoise.noise_seed && body.noise_seed == bodyNoise.noise_seed);
            for (int i = 0; i < 960; ++i) {
                const double sa = static_cast<double>(a[i]) - n0[i], sb = static_cast<double>(b[i]) - n1[i];
                oldPower += sa * sa; bodyPower += sb * sb;
                const unsigned int pa = static_cast<unsigned int>(a[i] < 0 ? -static_cast<int>(a[i]) : a[i]);
                const unsigned int pb = static_cast<unsigned int>(b[i] < 0 ? -static_cast<int>(b[i]) : b[i]);
                if (pa > oldPeak) oldPeak = pa; if (pb > bodyPeak) bodyPeak = pb;
            }
        }
        assert(!old.metrics.callbacks_passthrough && !body.metrics.callbacks_passthrough);
        assert(old.metrics.callbacks_dsp == 50 && body.metrics.callbacks_dsp == 50);
        assert(!old.metrics.sample_values_clipped && !body.metrics.sample_values_clipped);
        const double ratio = std::sqrt(bodyPower / oldPower);
        if (hz == 160) assert(ratio > 10);
        if (hz == 320) assert(ratio > 4);
        if (hz >= 1200) assert(ratio > 0.9 && ratio < 1.25);
        std::printf("RICON_LIVE_AUDIO_BAND hz=%d oldRms=%.3f bodyRms=%.3f ratio=%.4f oldPeak=%u bodyPeak=%u noiseBytesEqual=1 rfStateEqual=1 clips=0\n",
                    hz, std::sqrt(oldPower / 48000), std::sqrt(bodyPower / 48000), ratio, oldPeak, bodyPeak);
    }
    RiconAudioState stress{}; ricon_live150_audio_state_init(&stress, 77);
    const RiconAudioChannelParams loud{1, 2, 30.0f, 20.0f, 1.0f, 1.0f};
    unsigned int stressPeak{};
    for (int block = 0; block < 50; ++block) {
        short samples[960]{};
        for (int i = 0; i < 960; ++i) samples[i] = static_cast<short>(std::lround(32700 * std::sin(2 * pi * 1200 * (block * 960 + i) / 48000.0)));
        ricon_live150_audio_process_playback_voice(&stress, &loud, samples, 960, 1, nullptr, nullptr);
        double ignored{}; measurePairedStage(samples, 960, ignored, stressPeak);
    }
    assert(stress.metrics.sample_values_clipped > 0 && stressPeak >= 32767);
    const uint64 savedCurrent = currentConnection.load(), savedListener = liveListener.load();
    currentConnection.store(81); liveListener.store(81); workerStop.store(false);
    uint64 generation{}; clearVoiceProbe(&generation);
    voiceNoiseLp.x1[0] = 99.0f;
    assert(armVoiceProbe(81, 3, false, GetTickCount64() + 1500, generation));
    assert(!voiceLive150 && setLiveDeviceAudioProfile(generation) && voiceLive150);
    short test[960]{}; LivePairedLevels pair{};
    const RiconAudioBiquadState emptyNoiseLp{};
    assert(!std::memcmp(&voiceNoiseLp, &emptyNoiseLp, sizeof(voiceNoiseLp)));
    assert(editVoiceProbe(81, 3, test, 960, 1, GetTickCount64(), &pair));
    assert(pair.blocks == 1 && pair.values == 960 && pair.postCSquares == pair.finalSquares && pair.postCPeak == pair.finalPeak);
    assert(pair.noise.values == pair.values && pair.noise.rawRfSquares > pair.noise.bandRfSquares);
    assert(std::memcmp(&voiceNoiseLp, &emptyNoiseLp, sizeof(voiceNoiseLp)));
    clearVoiceProbe(); assert(!voiceLive150 && !setLiveDeviceAudioProfile(generation));
    assert(!std::memcmp(&voiceNoiseLp, &emptyNoiseLp, sizeof(voiceNoiseLp)));
    currentConnection.store(savedCurrent); liveListener.store(savedListener);
    std::printf("RICON_LIVE_AUDIO_CLIP_STRESS clipped=%llu peak=%u\n", stress.metrics.sample_values_clipped, stressPeak);
    std::puts("RICON_TS3_LIVE_AUDIO_PROFILE_SELFTEST_PASS");
}

static void verifyLiveNoiseProfile() {
    const double pi = 3.14159265358979323846;
    const RiconAudioChannelParams params{1, 2, 30.0f, 20.0f, -1.0f, -1.0f};
    for (int channels : {1, 2}) for (bool stress : {false, true}) {
        RiconAudioState enabled{}, disabled{}, reference{};
        ricon_live150_audio_state_init(&enabled, 891); ricon_live150_audio_state_init(&disabled, 891);
        ricon_live150_audio_state_init(&reference, 891);
        RiconAudioBiquadState lpEnabled{}, lpDisabled{};
        RiconLiveNoiseMetrics measured{}, rawReference{};
        const RiconAudioChannelParams loud{1, 2, 30.0f, 20.0f, 1.0f, 1.0f};
        const auto& active = stress ? loud : params;
        const int sizes[] = {1, 7, 960, 25};
        int sampleClock{};
        for (int block = 0; block < 80; ++block) {
            const int frames = sizes[block % 4];
            short a[1920]{}, b[1920]{}, old[1920]{};
            for (int frame = 0; frame < frames; ++frame) for (int ch = 0; ch < channels; ++ch) {
                const double phase = 2 * pi * (ch ? 640 : 1200) * (sampleClock + frame) / 48000.0;
                a[frame * channels + ch] = static_cast<short>(std::lround((stress ? 32700.0 : 2048.0) * std::sin(phase)));
            }
            std::memcpy(b, a, sizeof(a)); std::memcpy(old, a, sizeof(a));
            ricon_live150_audio_process_playback_voice(&enabled, &active, a, frames, channels, &lpEnabled, &measured);
            ricon_live150_audio_process_playback_voice(&disabled, &active, b, frames, channels, &lpDisabled, nullptr);
            ricon_live150_audio_process_playback_voice(&reference, &active, old, frames, channels, nullptr, &rawReference);
            assert(!std::memcmp(a, b, sizeof(a)) && !std::memcmp(&enabled, &disabled, sizeof(enabled)));
            assert(!std::memcmp(&lpEnabled, &lpDisabled, sizeof(lpEnabled)));
            assert(enabled.noise_seed == reference.noise_seed && enabled.fade_db == reference.fade_db);
            assert(!std::memcmp(&enabled.hp_state, &reference.hp_state, sizeof(enabled.hp_state)));
            assert(!std::memcmp(&enabled.lp_state, &reference.lp_state, sizeof(enabled.lp_state)));
            assert(measured.sourceSquares == rawReference.sourceSquares && measured.rawRfSquares == rawReference.rawRfSquares);
            sampleClock += frames;
        }
        assert(measured.values == static_cast<uint64_t>(sampleClock) * channels);
        const double values = static_cast<double>(measured.values);
        const double identity = measured.sourceSquares + measured.bandRfSquares + measured.sourceRfCross;
        assert(std::fabs(measured.mixedSquares - identity) < (measured.mixedSquares + 1) * 1e-11);
        assert(measured.bandRfSquares > 0 && measured.bandRfSquares < measured.rawRfSquares);
        if (stress) assert(enabled.metrics.sample_values_clipped > 0 && measured.residualSquares > values);
        else assert(!enabled.metrics.sample_values_clipped && measured.residualSquares <= values * 1.01);
        std::printf("RICON_LIVE_NOISE_PARITY channels=%d stress=%u values=%llu diagnosticsBytesEqual=1 rawRfEqual=1 rfStateEqual=1 sourceEqual=1 energyResidualMean=%.12g clips=%llu\n",
                    channels, stress ? 1u : 0u, measured.values, (measured.mixedSquares - identity) / values,
                    enabled.metrics.sample_values_clipped);
    }

    RiconAudioState raw{}, band{};
    ricon_live150_audio_state_init(&raw, 677); ricon_live150_audio_state_init(&band, 677);
    RiconAudioBiquadState noiseLp{};
    RiconLiveNoiseMetrics rawMetrics{}, bandMetrics{};
    const RiconAudioChannelParams noise{1, 2, 30.0f, 20.0f, 1.0f, 0.3f};
    double spectral[2][4]{};
    for (int block = 0; block < 50; ++block) {
        short a[960]{}, b[960]{};
        ricon_live150_audio_process_playback_voice(&raw, &noise, a, 960, 1, nullptr, &rawMetrics);
        ricon_live150_audio_process_playback_voice(&band, &noise, b, 960, 1, &noiseLp, &bandMetrics);
        for (int i = 0; i < 960; ++i) for (int f = 0; f < 2; ++f) {
            const double phase = 2 * pi * (f ? 12000 : 1000) * (block * 960 + i) / 48000.0;
            spectral[f][0] += a[i] * std::cos(phase); spectral[f][1] += a[i] * std::sin(phase);
            spectral[f][2] += b[i] * std::cos(phase); spectral[f][3] += b[i] * std::sin(phase);
        }
    }
    const double noisePowerRatio = bandMetrics.bandRfSquares / rawMetrics.rawRfSquares;
    assert(rawMetrics.rawRfSquares == bandMetrics.rawRfSquares && !bandMetrics.sourceSquares);
    assert(!std::memcmp(&raw, &band, sizeof(raw)) && !band.metrics.sample_values_clipped);
    assert(noisePowerRatio > 0.1 && noisePowerRatio < 0.3);
    double ratios[2]{};
    for (int f = 0; f < 2; ++f) {
        const double rawPower = spectral[f][0] * spectral[f][0] + spectral[f][1] * spectral[f][1];
        const double bandPower = spectral[f][2] * spectral[f][2] + spectral[f][3] * spectral[f][3];
        assert(rawPower > 1); ratios[f] = std::sqrt(bandPower / rawPower);
    }
    assert(ratios[0] > 0.85 && ratios[0] < 1.15 && ratios[1] < 0.15);
    std::printf("RICON_LIVE_NOISE_BAND rawRms=%.6f bandRms=%.6f powerRatio=%.6f amplitudeRatio1k=%.6f amplitudeRatio12k=%.6f\n",
                std::sqrt(rawMetrics.rawRfSquares / static_cast<double>(rawMetrics.values)),
                std::sqrt(bandMetrics.bandRfSquares / static_cast<double>(bandMetrics.values)),
                noisePowerRatio, ratios[0], ratios[1]);

    // Voice is unchanged when the RF contribution is zero, including independent channel/filter histories.
    RiconAudioState voiceRaw{}, voiceBand{};
    ricon_live150_audio_state_init(&voiceRaw, 77); ricon_live150_audio_state_init(&voiceBand, 77);
    RiconAudioBiquadState voiceLp{};
    const RiconAudioChannelParams noNoise{1, 2, 30.0f, 20.0f, 1.0f, 0.0f};
    for (int block = 0; block < 10; ++block) {
        short a[1920]{}, b[1920]{};
        for (int i = 0; i < 960; ++i) a[i * 2] = static_cast<short>(std::lround(2000 * std::sin(2 * pi * 600 * (block * 960 + i) / 48000.0)));
        std::memcpy(b, a, sizeof(a));
        ricon_live150_audio_process_playback_voice(&voiceRaw, &noNoise, a, 960, 2, nullptr, nullptr);
        ricon_live150_audio_process_playback_voice(&voiceBand, &noNoise, b, 960, 2, &voiceLp, nullptr);
        assert(!std::memcmp(a, b, sizeof(a)) && !std::memcmp(&voiceRaw, &voiceBand, sizeof(voiceRaw)));
        for (int i = 0; i < 960; ++i) assert(!b[i * 2 + 1]);
    }
    const RiconAudioBiquadState empty{};
    for (int ch = 0; ch < RICON_AUDIO_MAX_CHANNELS; ++ch)
        assert(voiceLp.x1[ch] == 0 && voiceLp.x2[ch] == 0 && voiceLp.y1[ch] == 0 && voiceLp.y2[ch] == 0);
    for (bool lost : {false, true}) {
        RiconAudioState state{}; ricon_live150_audio_state_init(&state, 99);
        RiconAudioBiquadState lp{}; lp.x1[0] = 31.0f;
        const auto before = lp;
        RiconLiveNoiseMetrics metrics{};
        const RiconAudioChannelParams skip{lost ? 0 : 1, 2, 60.0f, 20.0f, -1.0f, -1.0f};
        short samples[32]{}; samples[0] = 123;
        ricon_live150_audio_process_playback_voice(&state, &skip, samples, 32, 1, &lp, &metrics);
        assert(!metrics.values && !std::memcmp(&lp, &before, sizeof(lp)));
        if (lost) assert(state.metrics.callbacks_muted == 1 && !samples[0]);
        else assert(state.metrics.callbacks_passthrough == 1 && samples[0] == 123);
    }
    RiconAudioState invalid{}; ricon_live150_audio_state_init(&invalid, 19);
    RiconAudioBiquadState invalidLp{};
    RiconLiveNoiseMetrics invalidMetrics{};
    ricon_live150_audio_process_playback_voice(&invalid, &params, nullptr, 32, 1, &invalidLp, &invalidMetrics);
    assert(invalid.metrics.callbacks_ignored == 1 && !invalidMetrics.values);
    assert(!std::memcmp(&invalidLp, &empty, sizeof(empty)));

    // Synthetic quiet-input backgrounds, not microphone measurements or calibrated RF noise powers.
    const struct { const char* name; int amplitude; float hiss; } backgrounds[] = {
        {"zero", 0, 0.0f}, {"input-only", 16, 0.0f}, {"rf-only", 0, 0.01f},
        {"rf-dominant", 16, 0.01f}, {"input-dominant", 64, 0.01f}
    };
    for (const auto& background : backgrounds) {
        RiconAudioState state{}; ricon_live150_audio_state_init(&state, 677);
        RiconAudioBiquadState lp{};
        RiconLiveNoiseMetrics metrics{};
        const RiconAudioChannelParams budget{1, 2, 30.0f, 20.0f, 1.0f - background.hiss, background.hiss};
        unsigned int inputSeed = 0x6e624eb7u;
        for (int block = 0; block < 50; ++block) {
            short samples[960]{};
            for (int i = 0; i < 960; ++i) {
                inputSeed ^= inputSeed << 13; inputSeed ^= inputSeed >> 17; inputSeed ^= inputSeed << 5;
                const int uniform = static_cast<int>(inputSeed >> 16) - 32768;
                samples[i] = static_cast<short>(std::lround(static_cast<double>(uniform) * background.amplitude / 32768.0));
            }
            ricon_live150_audio_process_playback_voice(&state, &budget, samples, 960, 1, &lp, &metrics);
        }
        assert(metrics.values == 48000 && state.metrics.callbacks_dsp == 50 && !state.metrics.sample_values_clipped);
        const double values = static_cast<double>(metrics.values);
        const double componentPower = metrics.sourceSquares + metrics.bandRfSquares;
        const double closure = metrics.mixedSquares - componentPower - metrics.sourceRfCross;
        assert(std::fabs(closure) < (metrics.mixedSquares + 1) * 1e-11);
        assert(metrics.residualSquares <= values * 1.01);
        if (!background.amplitude) assert(!metrics.sourceSquares && !metrics.sourceRfCross);
        else assert(metrics.sourceSquares > 0);
        if (!background.hiss) assert(!metrics.rawRfSquares && !metrics.bandRfSquares && !metrics.sourceRfCross);
        else assert(metrics.bandRfSquares > 0 && metrics.bandRfSquares < metrics.rawRfSquares);
        if (!componentPower) assert(!metrics.outputSquares);
        if (background.amplitude == 16 && background.hiss) assert(metrics.bandRfSquares > metrics.sourceSquares);
        if (background.amplitude == 64) assert(metrics.sourceSquares > metrics.bandRfSquares);
        const double sourceFraction = componentPower ? metrics.sourceSquares / componentPower : 0;
        const double rfFraction = componentPower ? metrics.bandRfSquares / componentPower : 0;
        if (componentPower) assert(std::fabs(sourceFraction + rfFraction - 1.0) < 1e-14);
        std::printf("RICON_LIVE_SYNTHETIC_BACKGROUND case=%s source=synthetic-uniform inputAmplitude=%d preHiss=%.3f values=%llu sourcePathRms=%.6f rawRfRms=%.6f bandRfRms=%.6f crossMean=%.9f outputRms=%.6f sourcePowerFraction=%.6f rfPowerFraction=%.6f fractionsDefined=%u fractionBasis=source-plus-bandRF energyResidualMean=%.12g clips=0\n",
                    background.name, background.amplitude, static_cast<double>(background.hiss), metrics.values,
                    std::sqrt(metrics.sourceSquares / values), std::sqrt(metrics.rawRfSquares / values),
                    std::sqrt(metrics.bandRfSquares / values), metrics.sourceRfCross / values,
                    std::sqrt(metrics.outputSquares / values), sourceFraction, rfFraction,
                    componentPower ? 1u : 0u, closure / values);
    }
    std::puts("RICON_TS3_LIVE_NOISE_PROFILE_SELFTEST_PASS");
}

int main(int argc, char** argv) {
    if (argc != 1) {
        char profile[4096]{};
        if (argc != 3 || std::strcmp(argv[1], "--ptt-status") || !resolveGameProfile(argv[2], std::strlen(argv[2]), profile)) return 2;
        PttStatusReply reply{};
        const char* status = pollPttStatus(profile, workerGeneration.load(), reply);
        if (!reply.session || reply.active) { std::fprintf(stderr, "PTT status outcome=%s\n", status); return 1; }
        std::puts("RICON_TS3_PTT_STATUS_INACTIVE_PASS");
        return 0;
    }
    assert(GetCurrentDirectoryA(sizeof(folder), folder));
    TS3Functions functions{};
    functions.getConfigPath = configPath;
    functions.getPluginPath = pluginPath;
    functions.getCurrentServerConnectionHandlerID = currentConnectionMock;
    functions.playWaveFileHandle = playFile;
    functions.getCurrentPlaybackDeviceName = queryDevice;
    functions.openPlaybackDevice = openDevice;
    functions.closePlaybackDevice = closeDevice;
    functions.freeMemory = freeSdk;
    functions.getPlaybackConfigValueAsFloat = queryGain;
    functions.getDefaultPlayBackMode = queryDefaultMode;
    functions.getDefaultPlaybackDevice = queryDefaultDevice;
    functions.getPlaybackModeList = queryPlaybackModes;
    ts3plugin_setFunctionPointers(functions);
    assert(ts3plugin_init() != 0 && plays == 0);
    functions.closeWaveFileHandle = closeFile;
    ts3plugin_setFunctionPointers(functions);
    ts3plugin_registerPluginID("test-id");
    assert(!configDirectory[0] && !dataDirectory[0] && plays == 0);
    assert(ts3plugin_init() == 0 && dataDirectory[0] && currentConnection.load() == 77 && plays == 0);
    pluginId[0] = configDirectory[0] = dataDirectory[0] = 0;
    currentConnection.store(0);
    assert(ts3plugin_init() == 0 && plays == 0); // No ID yet: no plugin-path API or playback.
    ts3plugin_registerPluginID("test-id");
    assert(ts3plugin_init() == 0 && plays == 0);
    assert(matchesArm("77 123\r\n", 8, 77, 123));
    assert(!matchesArm("77 123 x", 8, 77, 123));
    assert(!matchesArm("77+123", 6, 77, 123));
    assert(!matchesArm("+77 123", 7, 77, 123));
    assert(!matchesArm("77 +123", 7, 77, 123));
    assert(!matchesArm("4294967373 123", 14, 77, 123));
    assert(!matchesArm("77 123", 6, 78, 123));
    assert(!matchesArm("77 123", 6, 77, 124));
    assert(!matchesArm("77\0 123", 7, 77, 123));
    assert(!matchesArm("", 0, 77, 123));
    char oversized[128]{};
    assert(!matchesArm(oversized, sizeof(oversized), 77, 123));
    char tiny[2]{};
    assert(!join(tiny, sizeof(tiny), "long path", "x") && !join(tiny, sizeof(tiny), "", "x"));
    ts3plugin_onMenuItemEvent(0, PLUGIN_MENU_TYPE_CHANNEL, 0, 0);
    ts3plugin_onMenuItemEvent(0, PLUGIN_MENU_TYPE_GLOBAL, 99, 0);
    assert(plays == 0);
    defaultModeError = 0xdead;
    assert(openDefaultHardwarePlayback(77) == 0xdead && opens == 0);
    defaultModeError = 0;
    defaultDeviceError = 0xbeef;
    assert(openDefaultHardwarePlayback(77) == 0xbeef && opens == 0);
    defaultDeviceError = 0;
    defaultMode = "custom";
    ts3plugin_onMenuItemEvent(0, PLUGIN_MENU_TYPE_GLOBAL, 0, 0);
    defaultMode = "test-mode";
    assert(plays == 1 && wave == 0 && waveActive && opens == 1 && lastConnection == 77 && std::strstr(lastPath, "ricon_pcm_probe/600-input.wav"));
    ts3plugin_onMenuItemEvent(77, PLUGIN_MENU_TYPE_GLOBAL, 1, 0);
    assert(plays == 2 && closes == 1 && lastClosed == 0 && lastConnection == 77 && opens == 1);
    closeError = 9;
    ts3plugin_onMenuItemEvent(77, PLUGIN_MENU_TYPE_GLOBAL, 2, 0);
    assert(plays == 2 && closes == 2 && wave == 102);
    closeError = 0;
    nextError = 7;
    ts3plugin_onMenuItemEvent(77, PLUGIN_MENU_TYPE_GLOBAL, 2, 0);
    assert(plays == 3 && closes == 3 && wave == 0 && !waveActive);
    ts3plugin_shutdown();
    assert(closes == 3 && deviceCloses == 1 && !playbackOwned);
    nextError = 0;
    devicePresent = true; // A pre-existing output must never become ours.
    ts3plugin_onMenuItemEvent(9, PLUGIN_MENU_TYPE_GLOBAL, 3, 0);
    assert(plays == 4 && opens == 1 && !playbackOwned);
    ts3plugin_shutdown();
    assert(deviceCloses == 1);
    devicePresent = false;
    openError = ERROR_sound_could_not_open_playback_device;
    ts3plugin_onMenuItemEvent(77, PLUGIN_MENU_TYPE_GLOBAL, 0, 0);
    assert(plays == 4 && !playbackOwned && opens == 2);
    queryError = 0xdead;
    ts3plugin_onMenuItemEvent(77, PLUGIN_MENU_TYPE_GLOBAL, 0, 0);
    assert(plays == 4 && opens == 2 && !playbackOwned);
    PluginMenuItem** items{};
    char* icon{};
    ts3plugin_initMenus(&items, &icon);
    assert(items && !items[5] && !icon);
    for (int i = 0; i < 5; ++i) {
        assert(items[i] && items[i]->id == i && items[i]->type == PLUGIN_MENU_TYPE_GLOBAL);
        ts3plugin_freeMemory(items[i]);
    }
    ts3plugin_freeMemory(items);
    queryError = openError = nextError = closeError = 0;
    FILETIME created{}, exitTime{}, kernel{}, user{};
    assert(GetProcessTimes(GetCurrentProcess(), &created, &exitTime, &kernel, &user));
    const uint64 creation = (static_cast<uint64>(created.dwHighDateTime) << 32) | created.dwLowDateTime;
    char armPath[4096]{};
    assert(join(armPath, sizeof(armPath), configDirectory, "ricon-pcm-run-once.arm"));
    assert(GetFileAttributesA(armPath) == INVALID_FILE_ATTRIBUTES);
    FILE* arm{};
    assert(!fopen_s(&arm, armPath, "wb") && arm);
    std::fprintf(arm, "%lu %llu\n", GetCurrentProcessId(), creation);
    std::fclose(arm);
    unsigned int baseline = plays.load();
    orderedRun = true;
    orderedIndex = 0;
    workerStop.store(false);
    testWorker = std::thread(runOnceWhenArmed);
    testWorker.join();
    assert(plays.load() == baseline + 4 && orderedIndex == 4 && !waveActive);
    assert(GetFileAttributesA(armPath) == INVALID_FILE_ATTRIBUTES); // Reload cannot replay an already claimed arm.
    orderedRun = false;
    ts3plugin_shutdown();
    assert(!playbackOwned);
    baseline = plays.load();
    assert(!fopen_s(&arm, armPath, "wb") && arm);
    std::fprintf(arm, "%lu %llu\n", GetCurrentProcessId(), creation);
    std::fclose(arm);
    workerStop.store(false);
    testWorker = std::thread(runOnceWhenArmed);
    for (int wait = 0; plays.load() == baseline && wait < 5000; ++wait) Sleep(1);
    ts3plugin_onMenuItemEvent(77, PLUGIN_MENU_TYPE_GLOBAL, 4, 0);
    testWorker.join();
    assert(plays.load() == baseline + 1 && !waveActive);
    ts3plugin_shutdown();
    assert(!testWorker.joinable() && !playbackOwned);
    assert(!fopen_s(&arm, armPath, "wb") && arm);
    std::fputs("1 1\n", arm); // A stale/recycled process token must never play.
    std::fclose(arm);
    baseline = plays.load();
    workerStop.store(false);
    testWorker = std::thread(runOnceWhenArmed);
    Sleep(100);
    ts3plugin_shutdown();
    assert(plays.load() == baseline && !testWorker.joinable());
    verifyVoiceAdapter();
    verifyNetworkInput();
    verifyLiveGameGate();
    verifyLiveLevelsAndOutcome();
    verifyDspAudibility();
    verifyLiveAudioProfile();
    verifyLiveNoiseProfile();
    verifyTs3TabProfile();
    verifyTs3RouteGate();
    verifyTs3TabActualGuard();
    verifyTs3TabProofChallenge();
    verifyRiconStatusProtocol();
    std::puts("RICON_TS3_HOST_SELFTEST_PASS");
}
