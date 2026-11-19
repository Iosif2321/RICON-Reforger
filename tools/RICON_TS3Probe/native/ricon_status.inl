// Main-menu metadata/update status is independent of all voice and test workers.
#include <shlobj.h>
#include <bcrypt.h>
#include <string>
#include <condition_variable>
#include <chrono>
#include <utility>
#pragma comment(lib, "bcrypt.lib")

struct RiconSemver { unsigned int part[3]{}; };

static bool parseRiconSemver(std::string_view value, RiconSemver& out) {
    RiconSemver parsed{};
    for (int i = 0; i < 3; ++i) {
        const size_t separator = value.find('.');
        const auto token = value.substr(0, separator);
        if (token.empty() || token.size() > 5 || (token.size() > 1 && token.front() == '0')) return false;
        for (const char digit : token) {
            if (digit < '0' || digit > '9') return false;
            parsed.part[i] = parsed.part[i] * 10 + static_cast<unsigned int>(digit - '0');
            if (parsed.part[i] > 65535) return false;
        }
        if ((i < 2) == (separator == value.npos)) return false;
        value.remove_prefix(separator == value.npos ? value.size() : separator + 1);
    }
    if (!value.empty()) return false;
    out = parsed;
    return true;
}

static int compareRiconSemver(const RiconSemver& a, const RiconSemver& b) {
    for (int i = 0; i < 3; ++i) if (a.part[i] != b.part[i]) return a.part[i] < b.part[i] ? -1 : 1;
    return 0;
}

static bool validRiconMenuToken(std::string_view token) {
    const size_t dash = token.find('-');
    if (dash == token.npos || token.find('-', dash + 1) != token.npos) return false;
    for (const auto number : {token.substr(0, dash), token.substr(dash + 1)}) {
        if (number.empty() || number.size() > 10 || number.front() < '1' || number.front() > '9') return false;
        for (const char digit : number) if (digit < '0' || digit > '9') return false;
    }
    return true;
}

static bool validRiconStatusNonce(std::string_view nonce) {
    if (nonce.size() != 32) return false;
    for (const char c : nonce) if ((c < '0' || c > '9') && (c < 'a' || c > 'f')) return false;
    return true;
}

static std::string_view takeRiconStatusToken(std::string_view& row) {
    const size_t space = row.find(' ');
    const auto token = row.substr(0, space);
    row.remove_prefix(space == row.npos ? row.size() : space + 1);
    return token;
}

static void trimRiconStatusLine(std::string_view& row) {
    if (!row.empty() && row.back() == '\n') {
        row.remove_suffix(1);
        if (!row.empty() && row.back() == '\r') row.remove_suffix(1);
    }
}

struct RiconMenuStatus {
    std::string token, mod, wanted;
    RiconSemver modVersion{}, wantedVersion{};
};

static bool parseRiconMenuStatus(std::string_view row, RiconMenuStatus& out) {
    trimRiconStatusLine(row);
    if (takeRiconStatusToken(row) != "RICON_MENU_1") return false;
    RiconMenuStatus parsed{};
    parsed.token = takeRiconStatusToken(row);
    parsed.mod = takeRiconStatusToken(row);
    parsed.wanted = takeRiconStatusToken(row);
    if (row != "END" || !validRiconMenuToken(parsed.token) || !parseRiconSemver(parsed.mod, parsed.modVersion)
        || !parseRiconSemver(parsed.wanted, parsed.wantedVersion)) return false;
    out = parsed;
    return true;
}

static bool matchesRiconStatusAck(std::string_view row, std::string_view token, std::string_view nonce) {
    trimRiconStatusLine(row);
    return validRiconMenuToken(token) && validRiconStatusNonce(nonce)
        && takeRiconStatusToken(row) == "RICON_ACK_1" && takeRiconStatusToken(row) == token
        && takeRiconStatusToken(row) == nonce && row == "END";
}

static bool freshRiconStatusAck(std::string_view row, std::string_view token, std::string_view nonce, uint64 issued, uint64 now) {
    return issued && now >= issued && now - issued < 3000 && matchesRiconStatusAck(row, token, nonce);
}

static std::atomic<bool> riconStatusStop{true}, riconStatusHostReady{false};
static std::thread riconStatusWorker;
static std::mutex riconStatusMutex;
static std::condition_variable riconStatusWake;
static std::atomic<unsigned int> riconStatusWriteSerial{0};

static bool riconStatusWide(const char* value, std::wstring& out) {
    if (!value || !value[0]) return false;
    const int size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value, -1, nullptr, 0);
    if (size <= 1 || size > 32768) return false;
    std::wstring result(static_cast<size_t>(size), L'\0');
    if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value, -1, &result[0], size)) return false;
    result.pop_back();
    while (result.size() > 3 && (result.back() == L'\\' || result.back() == L'/')) result.pop_back();
    if (result.find(L'"') != result.npos || result.find_first_of(L"\r\n") != result.npos) return false;
    out = result;
    return true;
}

static bool riconStatusPlain(const std::wstring& path, bool directory = false) {
    const DWORD attributes = GetFileAttributesW(path.c_str());
    return attributes != INVALID_FILE_ATTRIBUTES && !(attributes & FILE_ATTRIBUTE_REPARSE_POINT)
        && ((attributes & FILE_ATTRIBUTE_DIRECTORY) != 0) == directory;
}

static bool riconStatusFullPath(const char* value, std::wstring& out) {
    std::wstring decoded;
    if (!riconStatusWide(value, decoded)) return false;
    wchar_t canonical[32768]{};
    const DWORD count = GetFullPathNameW(decoded.c_str(), static_cast<DWORD>(sizeof(canonical) / sizeof(*canonical)), canonical, nullptr);
    if (!count || count >= sizeof(canonical) / sizeof(*canonical)) return false;
    out = canonical;
    while (out.size() > 3 && (out.back() == L'\\' || out.back() == L'/')) out.pop_back();
    return true;
}

static bool riconStatusSafeDirectories(const std::wstring& path) {
    if (!riconStatusPlain(path, true)) return false;
    size_t start = path.size() > 2 && path[1] == L':' ? 3 : 2;
    for (size_t i = start; i < path.size(); ++i) {
        if (path[i] != L'\\' && path[i] != L'/') continue;
        const std::wstring parent = path.substr(0, i);
        if (!riconStatusPlain(parent, true)) return false;
    }
    return true;
}

template <size_t Capacity>
static bool readRiconStatusFile(const std::wstring& path, char (&buffer)[Capacity], size_t& length) {
    length = 0;
    if (!riconStatusPlain(path)) return false;
    HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                              nullptr, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
    if (file == INVALID_HANDLE_VALUE) return false;
    LARGE_INTEGER size{}, after{};
    BY_HANDLE_FILE_INFORMATION info{};
    DWORD read{};
    const bool ok = GetFileInformationByHandle(file, &info) && !(info.dwFileAttributes & (FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_DIRECTORY))
        && GetFileSizeEx(file, &size) && size.QuadPart > 0 && size.QuadPart < static_cast<LONGLONG>(sizeof(buffer))
        && ReadFile(file, buffer, static_cast<DWORD>(size.QuadPart), &read, nullptr)
        && GetFileSizeEx(file, &after) && after.QuadPart == size.QuadPart && read == static_cast<DWORD>(size.QuadPart);
    CloseHandle(file);
    if (!ok || std::memchr(buffer, 0, read)) return false;
    length = read;
    buffer[length] = 0;
    return true;
}

static bool writeRiconStatusFile(const std::wstring& path, const std::string& row) {
    char existing[256]{};
    size_t existingSize{};
    if (readRiconStatusFile(path, existing, existingSize) && std::string_view(existing, existingSize) == row) return true;
    const DWORD attributes = GetFileAttributesW(path.c_str());
    if (attributes != INVALID_FILE_ATTRIBUTES && !riconStatusPlain(path)) return false;
    const auto temporary = path + L"." + std::to_wstring(GetCurrentProcessId()) + L"."
        + std::to_wstring(++riconStatusWriteSerial) + L".tmp";
    if (GetFileAttributesW(temporary.c_str()) != INVALID_FILE_ATTRIBUTES) return false;
    HANDLE file = CreateFileW(temporary.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) return false;
    DWORD written{};
    const bool complete = WriteFile(file, row.data(), static_cast<DWORD>(row.size()), &written, nullptr) && written == row.size();
    CloseHandle(file);
    const bool published = complete && MoveFileExW(temporary.c_str(), path.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
    if (!published) DeleteFileW(temporary.c_str());
    return published;
}

static bool riconStatusDocuments(std::wstring& profile) {
    // Resolve only in the real host worker; mock startup does not import Shell32/Ole32.
    HMODULE shell = LoadLibraryExW(L"shell32.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    HMODULE ole = LoadLibraryExW(L"ole32.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
    using KnownFolder = HRESULT (WINAPI*)(REFKNOWNFOLDERID, DWORD, HANDLE, PWSTR*);
    using FreeTask = void (WINAPI*)(LPVOID);
    const auto known = shell ? reinterpret_cast<KnownFolder>(GetProcAddress(shell, "SHGetKnownFolderPath")) : nullptr;
    const auto freeTask = ole ? reinterpret_cast<FreeTask>(GetProcAddress(ole, "CoTaskMemFree")) : nullptr;
    const GUID documentsId = {0xfdd39ad0, 0x238f, 0x46af, {0xad, 0xb4, 0x6c, 0x85, 0x48, 0x03, 0x69, 0xc7}};
    PWSTR documents{};
    const bool found = known && freeTask && SUCCEEDED(known(documentsId, KF_FLAG_DEFAULT, nullptr, &documents)) && documents;
    if (found) profile = documents;
    if (documents && freeTask) freeTask(documents);
    if (shell) FreeLibrary(shell);
    if (ole) FreeLibrary(ole);
    return found;
}

static bool riconStatusProfile(const std::wstring& config, std::wstring& documents, std::wstring& profile) {
    const auto overridePath = config + L"\\ricon-game-profile.txt";
    if (GetFileAttributesW(overridePath.c_str()) != INVALID_FILE_ATTRIBUTES) {
        char selected[4096]{};
        size_t size{};
        if (!readRiconStatusFile(overridePath, selected, size)) return false;
        std::string_view row(selected, size);
        trimRiconStatusLine(row);
        if (row.size() < 3 || row[1] != ':' || (row[2] != '\\' && row[2] != '/')) return false;
        const std::string utf8(row);
        return riconStatusFullPath(utf8.c_str(), profile) && riconStatusSafeDirectories(profile);
    }
    if (documents.empty() && !riconStatusDocuments(documents)) return false;
    profile = documents + L"\\My Games\\ArmaReforger\\profile";
    return riconStatusSafeDirectories(profile);
}

static bool parseRiconWorkerLeaf(std::wstring_view leaf, DWORD& pid, uint64& creation, std::string& version) {
    constexpr std::wstring_view prefix = L"worker-", suffix = L".exe";
    if (leaf.size() < prefix.size() + suffix.size() || leaf.substr(0, prefix.size()) != prefix
        || leaf.substr(leaf.size() - suffix.size()) != suffix) return false;
    leaf.remove_prefix(prefix.size()); leaf.remove_suffix(suffix.size());
    const auto readNumber = [&](uint64& value, size_t maxDigits) {
        const auto split = leaf.find(L'-');
        const auto token = leaf.substr(0, split);
        if (split == leaf.npos || token.empty() || token.size() > maxDigits || token.front() < L'1' || token.front() > L'9') return false;
        for (const wchar_t digit : token) {
            if (digit < L'0' || digit > L'9' || value > (~uint64{0} - static_cast<uint64>(digit - L'0')) / 10) return false;
            value = value * 10 + static_cast<uint64>(digit - L'0');
        }
        leaf.remove_prefix(split + 1);
        return true;
    };
    uint64 parsedPid{}, parsedCreation{};
    if (!readNumber(parsedPid, 10) || parsedPid > MAXDWORD || !readNumber(parsedCreation, 20)) return false;
    std::string parsed;
    for (const wchar_t c : leaf) { if (c > 127) return false; parsed.push_back(static_cast<char>(c)); }
    RiconSemver wanted{};
    if (!parseRiconSemver(parsed, wanted)) return false;
    pid = static_cast<DWORD>(parsedPid); creation = parsedCreation; version = parsed;
    return true;
}

static bool riconStatusHostExited(DWORD pid, uint64 creation) {
    HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | SYNCHRONIZE, FALSE, pid);
    if (!process) return GetLastError() == ERROR_INVALID_PARAMETER; // Access denied or unknown liveness is never deletion permission.
    FILETIME born{}, exited{}, kernel{}, user{};
    bool gone{};
    if (GetProcessTimes(process, &born, &exited, &kernel, &user)) {
        const uint64 actual = (static_cast<uint64>(born.dwHighDateTime) << 32) | born.dwLowDateTime;
        gone = actual != creation || WaitForSingleObject(process, 0) == WAIT_OBJECT_0;
    }
    CloseHandle(process);
    return gone;
}

static void cleanupRiconStatusWorkers(const std::wstring& directory, uint64 creation) {
    if (!riconStatusSafeDirectories(directory)) return;
    WIN32_FIND_DATAW item{};
    HANDLE search = FindFirstFileW((directory + L"\\worker-*.exe").c_str(), &item);
    if (search == INVALID_HANDLE_VALUE) return;
    unsigned int scanned{};
    do {
        if (++scanned > 64 || riconStatusStop.load()) break;
        DWORD pid{}; uint64 born{}; std::string version;
        if ((item.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT))
            || !parseRiconWorkerLeaf(item.cFileName, pid, born, version)
            || (pid == GetCurrentProcessId() && born == creation) || !riconStatusHostExited(pid, born)) continue;
        const auto path = directory + L"\\" + item.cFileName;
        HANDLE file = CreateFileW(path.c_str(), DELETE | FILE_READ_ATTRIBUTES, 0, nullptr, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
        if (file == INVALID_HANDLE_VALUE) continue; // In-use helpers and uncertain handles are left alone.
        BY_HANDLE_FILE_INFORMATION info{};
        if (GetFileInformationByHandle(file, &info) && !(info.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT))) {
            FILE_DISPOSITION_INFO disposition{}; disposition.DeleteFile = TRUE;
            SetFileInformationByHandle(file, FileDispositionInfo, &disposition, sizeof(disposition));
        }
        CloseHandle(file);
    } while (FindNextFileW(search, &item));
    FindClose(search);
}

enum class RiconStatusClaim { none, sameTarget, otherOrUncertain };

static RiconStatusClaim readRiconStatusHostClaim(const std::wstring& directory, const std::wstring& claim,
                                                uint64 creation, const std::string& wanted) {
    const DWORD attributes = GetFileAttributesW(claim.c_str());
    if (attributes != INVALID_FILE_ATTRIBUTES) {
        char row[256]{}; size_t size{};
        if (!readRiconStatusFile(claim, row, size)) return RiconStatusClaim::otherOrUncertain;
        return std::string_view(row, size) == wanted ? RiconStatusClaim::sameTarget : RiconStatusClaim::otherOrUncertain;
    }
    if (GetLastError() != ERROR_FILE_NOT_FOUND && GetLastError() != ERROR_PATH_NOT_FOUND) return RiconStatusClaim::otherOrUncertain;
    // A pre-claim plugin generation may already have launched a target-specific helper in this host incarnation.
    WIN32_FIND_DATAW item{};
    const std::wstring mask = directory + L"\\worker-" + std::to_wstring(GetCurrentProcessId()) + L"-" + std::to_wstring(creation) + L"-*.exe";
    HANDLE search = FindFirstFileW(mask.c_str(), &item);
    if (search == INVALID_HANDLE_VALUE) {
        const DWORD error = GetLastError();
        return error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND ? RiconStatusClaim::none : RiconStatusClaim::otherOrUncertain;
    }
    DWORD pid{}; uint64 born{}; std::string target;
    const bool same = parseRiconWorkerLeaf(item.cFileName, pid, born, target) && pid == GetCurrentProcessId() && born == creation && target == wanted;
    const bool more = FindNextFileW(search, &item) != FALSE;
    FindClose(search);
    return same && !more ? RiconStatusClaim::sameTarget : RiconStatusClaim::otherOrUncertain;
}

static bool hashRiconStatusFile(HANDLE file, char (&hex)[65]) {
    LARGE_INTEGER fileSize{};
    if (!GetFileSizeEx(file, &fileSize) || fileSize.QuadPart <= 0 || fileSize.QuadPart > 64 * 1024 * 1024) return false;
    BCRYPT_ALG_HANDLE algorithm{};
    BCRYPT_HASH_HANDLE hash{};
    DWORD bytes{}, objectSize{};
    bool ok = BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0) >= 0;
    if (ok) ok = BCryptGetProperty(algorithm, BCRYPT_OBJECT_LENGTH, reinterpret_cast<PUCHAR>(&objectSize), sizeof(objectSize), &bytes, 0) >= 0;
    unsigned char* object = ok ? static_cast<unsigned char*>(std::malloc(objectSize)) : nullptr;
    if (ok) ok = object && BCryptCreateHash(algorithm, &hash, object, objectSize, nullptr, 0, 0) >= 0;
    unsigned char buffer[16384]{}, digest[32]{};
    while (ok) {
        if (riconStatusStop.load()) { ok = false; break; }
        DWORD read{};
        if (!ReadFile(file, buffer, sizeof(buffer), &read, nullptr)) { ok = false; break; }
        if (!read) break;
        ok = BCryptHashData(hash, buffer, read, 0) >= 0;
    }
    if (ok) ok = BCryptFinishHash(hash, digest, sizeof(digest), 0) >= 0;
    if (hash) BCryptDestroyHash(hash);
    if (algorithm) BCryptCloseAlgorithmProvider(algorithm, 0);
    std::free(object);
    if (ok) for (size_t i = 0; i < sizeof(digest); ++i) std::snprintf(hex + i * 2, 3, "%02x", digest[i]);
    return ok;
}

static HANDLE pinTrustedRiconStatusHelper(const std::wstring& path) {
    if (!riconStatusPlain(path)) return INVALID_HANDLE_VALUE;
    HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
    if (file == INVALID_HANDLE_VALUE) return file;
    char digest[65]{};
    BY_HANDLE_FILE_INFORMATION info{};
    const bool trusted = GetFileInformationByHandle(file, &info) && !(info.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT))
        && hashRiconStatusFile(file, digest) && !std::strcmp(digest, RICON_UPDATE_HELPER_SHA256);
    if (!trusted) { CloseHandle(file); return INVALID_HANDLE_VALUE; }
    return file;
}

static bool launchRiconStatusHelper(const std::wstring& source, const std::wstring& destination, const std::wstring& config,
                                    uint64 creation, const std::string& wanted, const std::wstring& profile,
                                    const RiconMenuStatus& request, const std::string& nonce, uint64 issued, const std::wstring& claim) {
    if (riconStatusStop.load() || !riconStatusSafeDirectories(config)) return false;
    HANDLE original = pinTrustedRiconStatusHelper(source);
    if (original == INVALID_HANDLE_VALUE) return false;
    HANDLE claimed = CreateFileW(claim.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (claimed == INVALID_HANDLE_VALUE) { CloseHandle(original); return false; }
    DWORD wrote{};
    const bool claimWritten = WriteFile(claimed, wanted.data(), static_cast<DWORD>(wanted.size()), &wrote, nullptr) && wrote == wanted.size();
    CloseHandle(claimed);
    if (!claimWritten) { CloseHandle(original); DeleteFileW(claim.c_str()); return false; }
    const bool copied = CopyFileW(source.c_str(), destination.c_str(), TRUE) != FALSE;
    CloseHandle(original);
    if (!copied) {
        const DWORD attributes = GetFileAttributesW(destination.c_str());
        const DWORD error = attributes == INVALID_FILE_ATTRIBUTES ? GetLastError() : ERROR_SUCCESS;
        if (attributes == INVALID_FILE_ATTRIBUTES && (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND)) DeleteFileW(claim.c_str());
        return false; // Keep an uncertain partial copy/claim closed; never guess whether an existing file was executed.
    }
    HANDLE pinned = pinTrustedRiconStatusHelper(destination);
    if (pinned == INVALID_HANDLE_VALUE) { DeleteFileW(destination.c_str()); DeleteFileW(claim.c_str()); return false; }
    std::wstring installed;
    if (!riconStatusWide(RICON_TS3_VERSION, installed)) { CloseHandle(pinned); DeleteFileW(destination.c_str()); DeleteFileW(claim.c_str()); return false; }
    const std::wstring target(wanted.begin(), wanted.end());
    std::wstring command = L"\"" + destination + L"\" " + std::to_wstring(GetCurrentProcessId()) + L" " + std::to_wstring(creation)
        + L" \"" + config + L"\" " + installed + L" " + target;
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION process{};
    char currentRow[256]{}, ackRow[256]{};
    size_t currentSize{}, ackSize{};
    RiconMenuStatus current{};
    const bool fresh = readRiconStatusFile(profile + L"\\ricon-status-game.txt", currentRow, currentSize)
        && parseRiconMenuStatus({currentRow, currentSize}, current) && current.token == request.token
        && current.mod == request.mod && current.wanted == request.wanted
        && readRiconStatusFile(profile + L"\\ricon-status-ack.txt", ackRow, ackSize)
        && freshRiconStatusAck({ackRow, ackSize}, request.token, nonce, issued, GetTickCount64());
    const bool launched = fresh && !riconStatusStop.load() && CreateProcessW(destination.c_str(), &command[0], nullptr, nullptr, FALSE,
                                                                   CREATE_NO_WINDOW, nullptr, config.c_str(), &startup, &process);
    CloseHandle(pinned);
    if (launched) { CloseHandle(process.hThread); CloseHandle(process.hProcess); }
    else { DeleteFileW(destination.c_str()); DeleteFileW(claim.c_str()); } // Neither file was executed; a bounded retry cannot duplicate a helper.
    return launched;
}

static void runRiconStatus(std::wstring config, std::wstring helper) {
    RiconSemver installed{};
    if (!parseRiconSemver(RICON_TS3_VERSION, installed)) return;
    FILETIME created{}, exited{}, kernel{}, user{};
    if (!GetProcessTimes(GetCurrentProcess(), &created, &exited, &kernel, &user)) return;
    const uint64 creation = (static_cast<uint64>(created.dwHighDateTime) << 32) | created.dwLowDateTime;
    const std::wstring directory = config + L"\\ricon-update";
    const std::wstring hostStem = std::to_wstring(GetCurrentProcessId()) + L"-" + std::to_wstring(creation);
    const std::wstring hostClaim = directory + L"\\host-" + hostStem + L".claim";
    cleanupRiconStatusWorkers(directory, creation);
    std::wstring documents;
    RiconMenuStatus request{};
    std::string nonce, observedToken, target;
    uint64 issued{}, claiming{};
    unsigned int attempts{};
    bool launched{}, claimed{};
    while (!riconStatusStop.load()) {
        std::wstring profile;
        char row[256]{};
        size_t length{};
        RiconMenuStatus current{};
        if (riconStatusProfile(config, documents, profile) && readRiconStatusFile(profile + L"\\ricon-status-game.txt", row, length)
            && parseRiconMenuStatus({row, length}, current)) {
            const uint64 now = GetTickCount64();
            if (observedToken != current.token) {
                request = current;
                observedToken = current.token;
                char generated[33]{};
                if (!makeGameNonce(generated)) break;
                nonce = generated;
                issued = now;
            }
            const bool immutable = request.token == current.token && request.mod == current.mod && request.wanted == current.wanted;
            const int comparison = compareRiconSemver(installed, current.wantedVersion);
            const char* state = !immutable ? "ERROR" : comparison == 0 ? "CURRENT" : comparison > 0 ? "NEWER" : "CHECKING";
            const std::wstring stem = hostStem + L"-" + std::wstring(current.wanted.begin(), current.wanted.end());
            const std::wstring copy = directory + L"\\worker-" + stem + L".exe";
            const std::wstring receipt = directory + L"\\status-" + stem + L".txt";
            char receiptRow[256]{};
            size_t receiptSize{};
            const bool hasReceipt = readRiconStatusFile(receipt, receiptRow, receiptSize);
            const std::string_view receiptValue(receiptRow, receiptSize);
            if (immutable && comparison < 0) {
                if (target != current.wanted) { target = current.wanted; attempts = 0; launched = claimed = false; claiming = 0; }
                const RiconStatusClaim host = readRiconStatusHostClaim(directory, hostClaim, creation, current.wanted);
                if (host == RiconStatusClaim::otherOrUncertain) state = "ERROR"; // Another target owns this TS incarnation; defer until the next client run.
                else if (hasReceipt && (receiptValue == "STAGED" || receiptValue == "APPLIED")) state = "STAGED";
                else if (hasReceipt && receiptValue == "ERROR") state = "ERROR";
                else if (hasReceipt) state = "ERROR";
                else if (host == RiconStatusClaim::sameTarget) {
                    if (!claiming) claiming = now;
                    if (now - claiming >= (launched ? 120000u : 3000u)) state = "ERROR";
                }
                else if (!launched && attempts >= 3) state = "ERROR";
                else if (launched && claiming && now - claiming >= 120000) state = "ERROR";
                else if (now >= issued && now - issued < 3000
                    && readRiconStatusFile(profile + L"\\ricon-status-ack.txt", row, length)
                    && freshRiconStatusAck({row, length}, request.token, nonce, issued, now)) {
                    if (!riconStatusPlain(directory, true)) CreateDirectoryW(directory.c_str(), nullptr);
                    if (!riconStatusSafeDirectories(directory)) state = "ERROR";
                    else if (GetFileAttributesW(copy.c_str()) != INVALID_FILE_ATTRIBUTES) {
                        if (!claimed) { claiming = now; claimed = true; }
                        if (!launched && now - claiming >= 3000) state = "ERROR"; // Unknown reload owner: never start a duplicate.
                    } else if (!launched && attempts < 3) {
                        ++attempts;
                        launched = launchRiconStatusHelper(helper, copy, config, creation, current.wanted, profile, request, nonce, issued, hostClaim);
                        if (launched) { claimed = true; claiming = now; }
                        if (!launched && attempts == 3) state = "ERROR";
                    }
                }
            }
            if (!riconStatusStop.load()) {
                const std::string response = "RICON_TS_1 " + current.token + " " + nonce + " " RICON_TS3_VERSION " " + state + " END\n";
                writeRiconStatusFile(profile + L"\\ricon-status-plugin.txt", response);
            }
        }
        std::unique_lock<std::mutex> lock(riconStatusMutex);
        riconStatusWake.wait_for(lock, std::chrono::milliseconds(350), [] { return riconStatusStop.load(); });
    }
}

static void startRiconStatus() {
    std::lock_guard<std::mutex> lock(riconStatusMutex);
    if (!riconStatusHostReady.load() || riconStatusWorker.joinable() || !configDirectory[0] || !dataDirectory[0]) return;
    wchar_t executable[32768]{};
    const DWORD size = GetModuleFileNameW(nullptr, executable, static_cast<DWORD>(sizeof(executable) / sizeof(*executable)));
    if (!size || size >= sizeof(executable) / sizeof(*executable)) return;
    const wchar_t* leaf = wcsrchr(executable, L'\\');
    if (_wcsicmp(leaf ? leaf + 1 : executable, L"ts3client_win64.exe")) return; // Native mocks never launch a status/update worker.
    std::wstring config, directory;
    if (!riconStatusFullPath(configDirectory, config) || !riconStatusFullPath(dataDirectory, directory)
        || !riconStatusSafeDirectories(config) || !riconStatusSafeDirectories(directory)) return;
    RiconSemver version{};
    if (!parseRiconSemver(RICON_TS3_VERSION, version)) return;
    riconStatusStop.store(false);
    try { riconStatusWorker = std::thread(runRiconStatus, config, directory + L"\\RiconUpdate.exe"); }
    catch (const std::exception&) { riconStatusStop.store(true); }
}

static void stopRiconStatus() {
    riconStatusStop.store(true);
    riconStatusWake.notify_all();
    std::thread owned;
    { std::lock_guard<std::mutex> lock(riconStatusMutex); owned = std::move(riconStatusWorker); }
    if (owned.joinable()) owned.join();
    riconStatusHostReady.store(false);
}
