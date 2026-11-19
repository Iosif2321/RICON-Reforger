#ifndef RICON_PROTOCOL_H
#define RICON_PROTOCOL_H

/*
 * RICON IPC protocol — shared contract between the Arma extension (pipe server)
 * and the TeamSpeak plugin (pipe client). Stage 1 walking skeleton.
 *
 * Transport: one text frame per line ('\n'-terminated), UTF-8.
 * Endpoint:
 *   Windows : named pipe   \\.\pipe\ricon_comms
 *   Linux   : per-user unix socket resolved by ricon_paths.h
 *             default order: RICON_IPC_PATH, XDG_RUNTIME_DIR, /run/user/<uid>,
 *             then ${TMPDIR:-/tmp}/ricon-<uid>/ricon_comms.sock.
 *
 * Messages (semver RICON_PROTO_VERSION, bumped on any wire change):
 *   HELLO ver=<proto>                       handshake, both directions
 *   SET_AUDIBLE src=<id> dst=<id> v=<0|1> seq=<n>
 *                                           server decision; src/dst=0 => global
 *   SET_VOICE src=<id> dst=<id> gain=<0..1> route=<direct|radio> seq=<u64>
 *                                           experimental listener-scoped Discord
 *                                           voice policy; older clients ignore it.
 *   SET_VOICE ... seq=<u64> generation=<u64>
 *                                           GAME policy; generation must match
 *                                           the active GAME_SESSION descriptor.
 *   GAME_SESSION schema=1 proto=<ver> session=<token> generation=<u64>
 *                guild=<id> channel=<id> secret=<sensitive-token|-> capacity=<2..8>
 *                authority=arma_server authority_id=<token> active=<0|1> seq=<u64>
 *                                           server-authoritative Discord lifecycle.
 *   SET_CHANNEL src=<id> dst=<id> talker=<tsClientId> state=<n> snr=<f> m=<f>
 *               intel=<f> hiss=<f> seq=<n>
 *                                           per-leg mean channel result for DSP:
 *                                           state 0=lost 1=weak 2=degraded 3=clean,
 *                                           snr=mean dB, m=Nakagami shape >=0.5,
 *                                           intel/hiss 0..1; src/dst=0 => global.
 *                                           talker=<TeamSpeak clientID> targets one
 *                                           speaker's voice; talker=0 => global
 *                                           channel (back-compat). Optional clear=1
 *                                           with talker=<id> forgets that talker so
 *                                           their voice returns to clean/global.
 *   WHOAMI clientID=<n> uid=<...>           plugin -> extension: the local TS
 *                                           clientID + unique id, so SQF can map
 *                                           playerUID <-> TS clientID for per-talker
 *                                           routing.
 *   TALK clientID=<n> v=<0|1>               plugin -> extension: voice activity
 *                                           (VAD) of a speaker, so the server only
 *                                           computes channels for active talkers.
 *   CHANNEL_ACK seq=<n> talker=<id> state=<n> clear=<0|1> applied=<0|1> reason=<text>
 *                                           plugin -> extension: confirms that a
 *                                           SET_CHANNEL frame reached the loaded
 *                                           plugin parser and was applied/ignored.
 *   VERIFY_AUDIO id=<id> nonce=<n> talkers=<a,b,c> frames=<n> sampleRate=48000 seq=<n>
 *                                           extension -> plugin: run deterministic
 *                                           in-memory A/B/C synthetic audio through
 *                                           the same DSP core used by playback.
 *   VERIFY_AUDIO_RESULT id=<id> nonce=<n> talker=<id> seq=<n> result=<ok|fail>
 *                       frames=<n> samples=<n> inRms=<f> outRms=<f> zero=<f>
 *                       clip=<n> delta=<f> role=<clean|degraded|lost> rawAudioStored=0
 *                                           plugin -> extension: aggregate metrics
 *                                           only; raw audio is never stored.
 *   VERIFY_AUDIO_REJECT id=<id> reason=<text>
 *                                           plugin -> extension: verifier rejected
 *                                           malformed/unsafe request.
 *   TS_CHANNEL_CONFIG seq=<n> enabled=<0|1> required=<0|1> strict=<0|1>
 *                     targetType=<id|name|path> id=<u64> name=<token> path=<token>
 *                     password=<token> restore=<0|1> expectedServerUid=<token>
 *                                           extension -> plugin: local TeamSpeak
 *                                           voice-channel auto-move policy. Password
 *                                           is redacted in logs/status and should not
 *                                           be treated as secret if distributed by CBA.
 *   TS_CHANNEL_STATUS seq=<n> state=<disabled|pending|already|requested|moved|failed>
 *                     current=<u64> previous=<u64> target=<u64> reason=<text> error=<n>
 *                                           plugin -> extension: cached status for
 *                                           self-test/support diagnostics.
 *   HEARTBEAT seq=<n>                       liveness, extension -> plugin
 *   STATUS text=<...>                       human-readable link state
 *
 * Invariant: thin plugin. The plugin only APPLIES what SET_AUDIBLE tells it; all
 * decisions are made server-side in SQF and routed through the extension.
 */

/* Compatibility rule: same MAJOR = compatible (negotiate to the lower MINOR);
 * different MAJOR = incompatible -> safe mode + human-readable message.
 * MINOR 1 added SET_CHANNEL (DSP params); a 1.0 plugin simply ignores it and
 * keeps the SET_AUDIBLE gate, so the bump is backward compatible.
 * MINOR 2 sends mean SNR + Nakagami m; fast fading is generated by the plugin.
 * MINOR 3 adds WHOAMI (plugin->extension TS clientID) and an optional talker=<id>
 * field on SET_CHANNEL for per-talker routing; talker=0 keeps the old global
 * behaviour, so older peers stay compatible.
 * MINOR 4 adds TALK (plugin->extension VAD); an extension that doesn't know it
 * simply ignores the frame, so the bump stays backward compatible.
 * MINOR 5 adds CHANNEL_ACK and VERIFY_AUDIO synthetic in-memory proof frames; older
 * peers ignore the new verbs, while same-major peers remain fail-open compatible.
 * MINOR 6 adds TS_CHANNEL_CONFIG/TS_CHANNEL_STATUS for dedicated TeamSpeak voice
 * channel auto-move/status. Older same-major peers ignore the new verbs safely.
 * MINOR 7 adds experimental SET_VOICE for the isolated Discord voice spike.
 * Existing TeamSpeak clients ignore the unknown verb and retain their 1.6
 * SET_AUDIBLE/SET_CHANNEL behavior.
 * MINOR 8 adds GAME_SESSION, a server-authoritative descriptor consumed by
 * RICON Core. Same-major TeamSpeak clients ignore the unknown verb. */
#define RICON_PROTO_MAJOR 1
#define RICON_PROTO_MINOR 9
#define RICON_PROTO_STRINGIFY2(x) #x
#define RICON_PROTO_STRINGIFY(x) RICON_PROTO_STRINGIFY2(x)
#define RICON_PROTO_VERSION RICON_PROTO_STRINGIFY(RICON_PROTO_MAJOR) "." RICON_PROTO_STRINGIFY(RICON_PROTO_MINOR)

#ifdef RICON_TESTING
#define RICON_PIPE_NAME_WIN  "\\\\.\\pipe\\ricon_comms_test"
#define RICON_PIPE_PATH_NIX  "/tmp/ricon-comms-test/ricon_comms_test.sock"
#else
#define RICON_PIPE_NAME_WIN  "\\\\.\\pipe\\ricon_comms"
#define RICON_PIPE_PATH_NIX  "/tmp/ricon-comms-legacy/ricon_comms.sock"
#endif

/* Message verbs (kept as plain tokens for trivial parsing). */
#define RICON_MSG_HELLO       "HELLO"
#define RICON_MSG_SET_AUDIBLE "SET_AUDIBLE"
#define RICON_MSG_SET_VOICE   "SET_VOICE"
#define RICON_MSG_GAME_SESSION "GAME_SESSION"
#define RICON_MSG_GAME_IDENTITY "GAME_IDENTITY"
#define RICON_MSG_GAME_ROSTER "GAME_ROSTER"
#define RICON_MSG_SET_CHANNEL "SET_CHANNEL"
#define RICON_MSG_HEARTBEAT   "HEARTBEAT"
#define RICON_MSG_STATUS      "STATUS"
#define RICON_MSG_WHOAMI      "WHOAMI"
#define RICON_MSG_TALK        "TALK"
#define RICON_MSG_CHANNEL_ACK "CHANNEL_ACK"
#define RICON_MSG_VERIFY_AUDIO "VERIFY_AUDIO"
#define RICON_MSG_VERIFY_AUDIO_RESULT "VERIFY_AUDIO_RESULT"
#define RICON_MSG_VERIFY_AUDIO_REJECT "VERIFY_AUDIO_REJECT"
#define RICON_MSG_TS_CHANNEL_CONFIG "TS_CHANNEL_CONFIG"
#define RICON_MSG_TS_CHANNEL_STATUS "TS_CHANNEL_STATUS"

/* Channel state codes (SET_CHANNEL state=...). Mirror the SQF "state" string. */
#define RICON_CH_LOST     0
#define RICON_CH_WEAK     1
#define RICON_CH_DEGRADED 2
#define RICON_CH_CLEAN    3

/* Global scope id used when a flag applies to every talker (skeleton default). */
#define RICON_SCOPE_GLOBAL 0

#ifdef __cplusplus
#include <charconv>
#include <iomanip>
#include <locale>
#include <map>
#include <sstream>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace ricon::protocol {

constexpr std::size_t kMaximumGameSessionFrameBytes = 8192;

struct GameSessionFrame {
    int schema = 1;
    int protocolMajor = RICON_PROTO_MAJOR;
    int protocolMinor = RICON_PROTO_MINOR;
    std::string sessionId;
    unsigned long long generation = 0;
    std::string guildId;
    std::string channelId;
    std::string lobbySecret;
    unsigned int capacity = 0;
    std::string authority;
    std::string authorityId;
    bool active = false;
    unsigned long long sequence = 0;
};

enum class GameSessionParseError {
    None,
    TooLarge,
    WrongVerb,
    MalformedField,
    DuplicateField,
    MissingField,
    UnsupportedSchema,
    ProtocolMismatch,
    InvalidValue,
};

struct ParsedGameSessionFrame {
    GameSessionFrame frame;
    GameSessionParseError error = GameSessionParseError::None;
    bool ok() const noexcept { return error == GameSessionParseError::None; }
};

inline void wipeSensitiveToken(std::string &value) noexcept {
    volatile char *bytes = value.empty() ? nullptr : &value[0];
    for (std::size_t index = 0; index < value.size(); ++index) bytes[index] = 0;
    value.clear();
}

inline bool validWireToken(std::string_view value, std::size_t maximum) noexcept {
    if (value.empty() || value.size() > maximum) return false;
    for (const unsigned char byte : value) {
        if (byte <= 0x20 || byte == 0x7f) return false;
    }
    return true;
}

inline bool validDecimalId(std::string_view value) noexcept {
    if (value.empty() || value.size() > 20) return false;
    for (const char byte : value) if (byte < '0' || byte > '9') return false;
    return value != "0";
}

template <typename Integer>
inline bool parseUnsignedDecimal(std::string_view value, Integer &out) noexcept {
    if (value.empty()) return false;
    Integer parsed{};
    const auto result = std::from_chars(value.data(), value.data() + value.size(), parsed);
    if (result.ec != std::errc{} || result.ptr != value.data() + value.size()) return false;
    out = parsed;
    return true;
}

inline ParsedGameSessionFrame parseGameSessionFrame(std::string_view wire) noexcept {
    ParsedGameSessionFrame result;
    if (wire.size() > kMaximumGameSessionFrameBytes) {
        result.error = GameSessionParseError::TooLarge;
        return result;
    }
    constexpr std::string_view prefix = RICON_MSG_GAME_SESSION " ";
    if (wire.size() <= prefix.size() || wire.substr(0, prefix.size()) != prefix) {
        result.error = GameSessionParseError::WrongVerb;
        return result;
    }
    std::map<std::string_view, std::string_view> fields;
    std::size_t position = prefix.size();
    while (position < wire.size()) {
        const auto end = wire.find(' ', position);
        const auto field = wire.substr(position, end == std::string_view::npos
                                                     ? wire.size() - position
                                                     : end - position);
        const auto equals = field.find('=');
        if (field.empty() || equals == 0 || equals == std::string_view::npos ||
            equals + 1 == field.size()) {
            result.error = GameSessionParseError::MalformedField;
            return result;
        }
        const auto key = field.substr(0, equals);
        const auto value = field.substr(equals + 1);
        if (!fields.emplace(key, value).second) {
            result.error = GameSessionParseError::DuplicateField;
            return result;
        }
        if (end == std::string_view::npos) break;
        position = end + 1;
        if (position == wire.size()) {
            result.error = GameSessionParseError::MalformedField;
            return result;
        }
    }
    static constexpr const char *required[] = {
        "schema", "proto", "session", "generation", "guild", "channel",
        "secret", "capacity", "authority", "authority_id", "active", "seq"};
    if (fields.size() != sizeof(required) / sizeof(required[0])) {
        result.error = GameSessionParseError::MissingField;
        return result;
    }
    for (const auto *key : required) {
        if (fields.find(key) == fields.end()) {
            result.error = GameSessionParseError::MissingField;
            return result;
        }
    }
    unsigned int schema = 0;
    if (!parseUnsignedDecimal(fields["schema"], schema) || schema != 1) {
        result.error = GameSessionParseError::UnsupportedSchema;
        return result;
    }
    const auto proto = fields["proto"];
    const auto dot = proto.find('.');
    unsigned int major = 0, minor = 0;
    if (dot == std::string_view::npos ||
        !parseUnsignedDecimal(proto.substr(0, dot), major) ||
        !parseUnsignedDecimal(proto.substr(dot + 1), minor) ||
        major != RICON_PROTO_MAJOR) {
        result.error = GameSessionParseError::ProtocolMismatch;
        return result;
    }
    unsigned int capacity = 0, active = 0;
    if (!parseUnsignedDecimal(fields["generation"], result.frame.generation) ||
        result.frame.generation == 0 ||
        !parseUnsignedDecimal(fields["capacity"], capacity) || capacity < 2 || capacity > 8 ||
        !parseUnsignedDecimal(fields["active"], active) || active > 1 ||
        !parseUnsignedDecimal(fields["seq"], result.frame.sequence) ||
        result.frame.sequence == 0 ||
        !validWireToken(fields["session"], 128) ||
        !validDecimalId(fields["guild"]) ||
        !validDecimalId(fields["channel"]) ||
        !validWireToken(fields["authority_id"], 128) ||
        fields["authority"] != "arma_server" ||
        (active == 1 && !validWireToken(fields["secret"], 1024)) ||
        (active == 0 && fields["secret"] != "-")) {
        result.error = GameSessionParseError::InvalidValue;
        return result;
    }
    result.frame.schema = static_cast<int>(schema);
    result.frame.protocolMajor = static_cast<int>(major);
    result.frame.protocolMinor = static_cast<int>(minor);
    result.frame.sessionId.assign(fields["session"]);
    result.frame.guildId.assign(fields["guild"]);
    result.frame.channelId.assign(fields["channel"]);
    result.frame.lobbySecret.assign(fields["secret"]);
    result.frame.capacity = capacity;
    result.frame.authority.assign(fields["authority"]);
    result.frame.authorityId.assign(fields["authority_id"]);
    result.frame.active = active == 1;
    return result;
}

inline std::string formatGameSessionFrame(const GameSessionFrame &frame) {
    std::ostringstream stream;
    stream.imbue(std::locale::classic());
    stream << RICON_MSG_GAME_SESSION
           << " schema=" << frame.schema
           << " proto=" << frame.protocolMajor << '.' << frame.protocolMinor
           << " session=" << frame.sessionId
           << " generation=" << frame.generation
           << " guild=" << frame.guildId
           << " channel=" << frame.channelId
           << " secret=" << (frame.active ? frame.lobbySecret : "-")
           << " capacity=" << frame.capacity
           << " authority=" << frame.authority
           << " authority_id=" << frame.authorityId
           << " active=" << (frame.active ? 1 : 0)
           << " seq=" << frame.sequence;
    return stream.str();
}

inline std::string redactSensitiveFrame(std::string_view wire) {
    std::string redacted(wire);
    constexpr std::string_view key = "secret=";
    const auto keyPosition = redacted.find(key);
    if (keyPosition == std::string::npos) return redacted;
    const auto valueStart = keyPosition + key.size();
    const auto valueEnd = redacted.find(' ', valueStart);
    redacted.replace(valueStart, valueEnd == std::string::npos
                                     ? std::string::npos
                                     : valueEnd - valueStart,
                     "<redacted>");
    return redacted;
}

struct GameRosterEntry {
    std::string playerUid;
    unsigned long long discordUserId = 0;
    unsigned int slot = 0;
};

struct GameRosterFrame {
    int schema = 1;
    int protocolMajor = RICON_PROTO_MAJOR;
    int protocolMinor = RICON_PROTO_MINOR;
    std::string sessionId;
    unsigned long long generation = 0;
    std::string authorityId;
    std::vector<GameRosterEntry> entries;
    unsigned long long sequence = 0;
};

struct ParsedGameRosterFrame {
    GameRosterFrame frame;
    GameSessionParseError error = GameSessionParseError::None;
    bool ok() const noexcept { return error == GameSessionParseError::None; }
};

struct GameIdentityFrame {
    int schema = 1;
    int protocolMajor = RICON_PROTO_MAJOR;
    int protocolMinor = RICON_PROTO_MINOR;
    std::string sessionId;
    unsigned long long generation = 0;
    unsigned long long discordUserId = 0;
    unsigned long long sequence = 0;
};

struct GameLeaseFrame { std::string sessionId; unsigned long long generation = 0; unsigned long long sequence = 0; };
struct ParsedGameLeaseFrame { GameLeaseFrame frame; GameSessionParseError error = GameSessionParseError::None; bool ok() const noexcept { return error == GameSessionParseError::None; } };
inline std::string formatGameLeaseFrame(const GameLeaseFrame &frame) {
    return std::string("GAME_LEASE schema=1 proto=") + RICON_PROTO_VERSION +
        " session=" + frame.sessionId + " generation=" + std::to_string(frame.generation) +
        " seq=" + std::to_string(frame.sequence);
}
inline ParsedGameLeaseFrame parseGameLeaseFrame(std::string_view wire) noexcept {
    ParsedGameLeaseFrame result;
    constexpr std::string_view prefix = "GAME_LEASE ";
    if (wire.size() > kMaximumGameSessionFrameBytes) { result.error = GameSessionParseError::TooLarge; return result; }
    if (wire.substr(0, prefix.size()) != prefix) { result.error = GameSessionParseError::WrongVerb; return result; }
    std::map<std::string_view,std::string_view> fields;
    std::size_t p=prefix.size();
    while (p<wire.size()) { auto e=wire.find(' ',p); auto f=wire.substr(p,e==std::string_view::npos?wire.size()-p:e-p); auto q=f.find('=');
        if(q==0||q==std::string_view::npos||q+1==f.size()||!fields.emplace(f.substr(0,q),f.substr(q+1)).second){result.error=GameSessionParseError::MalformedField;return result;} if(e==std::string_view::npos)break;p=e+1; }
    if(fields.size()!=5||fields["schema"]!="1"||fields["proto"]!=RICON_PROTO_VERSION||!validWireToken(fields["session"],128)||
       !parseUnsignedDecimal(fields["generation"],result.frame.generation)||result.frame.generation==0||
       !parseUnsignedDecimal(fields["seq"],result.frame.sequence)||result.frame.sequence==0){result.error=GameSessionParseError::InvalidValue;return result;}
    result.frame.sessionId.assign(fields["session"]); return result;
}

struct ParsedGameIdentityFrame {
    GameIdentityFrame frame;
    GameSessionParseError error = GameSessionParseError::None;
    bool ok() const noexcept { return error == GameSessionParseError::None; }
};

inline std::string formatGameIdentityFrame(const GameIdentityFrame &frame) {
    return std::string(RICON_MSG_GAME_IDENTITY) +
        " schema=1 proto=" RICON_PROTO_VERSION " session=" + frame.sessionId +
        " generation=" + std::to_string(frame.generation) +
        " discord=" + std::to_string(frame.discordUserId) +
        " seq=" + std::to_string(frame.sequence);
}

inline ParsedGameIdentityFrame parseGameIdentityFrame(std::string_view wire) noexcept {
    ParsedGameIdentityFrame result;
    if (wire.size() > kMaximumGameSessionFrameBytes) {
        result.error = GameSessionParseError::TooLarge; return result;
    }
    constexpr std::string_view prefix = RICON_MSG_GAME_IDENTITY " ";
    if (wire.size() <= prefix.size() || wire.substr(0, prefix.size()) != prefix) {
        result.error = GameSessionParseError::WrongVerb; return result;
    }
    std::map<std::string_view, std::string_view> fields;
    std::size_t position = prefix.size();
    while (position < wire.size()) {
        const auto end = wire.find(' ', position);
        const auto field = wire.substr(position, end == std::string_view::npos
            ? wire.size() - position : end - position);
        const auto equals = field.find('=');
        if (field.empty() || equals == 0 || equals == std::string_view::npos ||
            equals + 1 == field.size()) {
            result.error = GameSessionParseError::MalformedField; return result;
        }
        if (!fields.emplace(field.substr(0, equals), field.substr(equals + 1)).second) {
            result.error = GameSessionParseError::DuplicateField; return result;
        }
        if (end == std::string_view::npos) break;
        position = end + 1;
    }
    static constexpr const char *required[] = {
        "schema", "proto", "session", "generation", "discord", "seq"};
    if (fields.size() != sizeof(required) / sizeof(required[0])) {
        result.error = GameSessionParseError::MissingField; return result;
    }
    for (const auto *key : required) if (fields.find(key) == fields.end()) {
        result.error = GameSessionParseError::MissingField; return result;
    }
    unsigned int schema = 0, major = 0, minor = 0;
    const auto proto = fields["proto"];
    const auto dot = proto.find('.');
    if (!parseUnsignedDecimal(fields["schema"], schema) || schema != 1 ||
        dot == std::string_view::npos || proto.find('.', dot + 1) != std::string_view::npos ||
        !parseUnsignedDecimal(proto.substr(0, dot), major) ||
        !parseUnsignedDecimal(proto.substr(dot + 1), minor) ||
        major != RICON_PROTO_MAJOR ||
        !validWireToken(fields["session"], 128) ||
        !parseUnsignedDecimal(fields["generation"], result.frame.generation) ||
        result.frame.generation == 0 ||
        (fields["discord"].size() > 1 && fields["discord"].front() == '0') ||
        !parseUnsignedDecimal(fields["discord"], result.frame.discordUserId) ||
        result.frame.discordUserId == 0 ||
        !parseUnsignedDecimal(fields["seq"], result.frame.sequence) ||
        result.frame.sequence == 0) {
        result.error = GameSessionParseError::InvalidValue; return result;
    }
    result.frame.schema = static_cast<int>(schema);
    result.frame.protocolMajor = static_cast<int>(major);
    result.frame.protocolMinor = static_cast<int>(minor);
    result.frame.sessionId.assign(fields["session"]);
    return result;
}

inline std::string formatGameRosterFrame(const GameRosterFrame &frame) {
    std::string entries;
    for (const auto &entry : frame.entries) {
        if (!entries.empty()) entries.push_back(',');
        entries += entry.playerUid + ":" + std::to_string(entry.discordUserId) +
                   ":" + std::to_string(entry.slot);
    }
    return std::string(RICON_MSG_GAME_ROSTER) +
        " schema=1 proto=" + std::to_string(frame.protocolMajor) + "." +
        std::to_string(frame.protocolMinor) + " session=" + frame.sessionId +
        " generation=" + std::to_string(frame.generation) +
        " authority_id=" + frame.authorityId + " entries=" + entries +
        " seq=" + std::to_string(frame.sequence);
}

inline ParsedGameRosterFrame parseGameRosterFrame(std::string_view wire) noexcept {
    ParsedGameRosterFrame result;
    if (wire.size() > kMaximumGameSessionFrameBytes) {
        result.error = GameSessionParseError::TooLarge; return result;
    }
    constexpr std::string_view prefix = RICON_MSG_GAME_ROSTER " ";
    if (wire.size() <= prefix.size() || wire.substr(0, prefix.size()) != prefix) {
        result.error = GameSessionParseError::WrongVerb; return result;
    }
    std::map<std::string_view, std::string_view> fields;
    std::size_t position = prefix.size();
    while (position < wire.size()) {
        const auto end = wire.find(' ', position);
        const auto field = wire.substr(position, end == std::string_view::npos
            ? wire.size() - position : end - position);
        const auto equals = field.find('=');
        if (field.empty() || equals == 0 || equals == std::string_view::npos ||
            equals + 1 == field.size()) {
            result.error = GameSessionParseError::MalformedField; return result;
        }
        if (!fields.emplace(field.substr(0, equals), field.substr(equals + 1)).second) {
            result.error = GameSessionParseError::DuplicateField; return result;
        }
        if (end == std::string_view::npos) break;
        position = end + 1;
    }
    static constexpr const char *required[] = {
        "schema", "proto", "session", "generation", "authority_id", "entries", "seq"};
    if (fields.size() != sizeof(required) / sizeof(required[0])) {
        result.error = GameSessionParseError::MissingField; return result;
    }
    for (const auto *key : required) if (fields.find(key) == fields.end()) {
        result.error = GameSessionParseError::MissingField; return result;
    }
    unsigned int schema = 0, major = 0, minor = 0;
    const auto proto = fields["proto"];
    const auto dot = proto.find('.');
    if (!parseUnsignedDecimal(fields["schema"], schema) || schema != 1 ||
        dot == std::string_view::npos ||
        !parseUnsignedDecimal(proto.substr(0, dot), major) ||
        !parseUnsignedDecimal(proto.substr(dot + 1), minor) ||
        major != RICON_PROTO_MAJOR ||
        !parseUnsignedDecimal(fields["generation"], result.frame.generation) ||
        result.frame.generation == 0 ||
        !parseUnsignedDecimal(fields["seq"], result.frame.sequence) ||
        result.frame.sequence == 0 ||
        !validWireToken(fields["session"], 128) ||
        !validWireToken(fields["authority_id"], 128)) {
        result.error = GameSessionParseError::InvalidValue; return result;
    }
    std::set<std::string> uids;
    std::set<unsigned long long> discordIds;
    std::set<unsigned int> slots;
    const auto encoded = fields["entries"];
    if (encoded == "-") {
        result.frame.schema = static_cast<int>(schema);
        result.frame.protocolMajor = static_cast<int>(major);
        result.frame.protocolMinor = static_cast<int>(minor);
        result.frame.sessionId.assign(fields["session"]);
        result.frame.authorityId.assign(fields["authority_id"]);
        return result;
    }
    std::size_t entryStart = 0;
    while (entryStart < encoded.size()) {
        const auto entryEnd = encoded.find(',', entryStart);
        const auto entry = encoded.substr(entryStart, entryEnd == std::string_view::npos
            ? encoded.size() - entryStart : entryEnd - entryStart);
        const auto first = entry.find(':');
        const auto second = first == std::string_view::npos
            ? std::string_view::npos : entry.find(':', first + 1);
        const auto discordToken = second == std::string_view::npos
            ? std::string_view{} : entry.substr(first + 1, second - first - 1);
        GameRosterEntry parsed;
        if (first == std::string_view::npos || second == std::string_view::npos ||
            entry.find(':', second + 1) != std::string_view::npos ||
            !validWireToken(entry.substr(0, first), 64) ||
            entry.substr(0, first).find(',') != std::string_view::npos ||
            entry.substr(0, first).find(':') != std::string_view::npos ||
            (discordToken.size() > 1 && discordToken.front() == '0') ||
            !parseUnsignedDecimal(discordToken,
                                  parsed.discordUserId) || parsed.discordUserId == 0 ||
            !parseUnsignedDecimal(entry.substr(second + 1), parsed.slot) ||
            parsed.slot < 1 || parsed.slot > 8) {
            result.error = GameSessionParseError::InvalidValue; return result;
        }
        parsed.playerUid.assign(entry.substr(0, first));
        if (!uids.insert(parsed.playerUid).second ||
            !discordIds.insert(parsed.discordUserId).second ||
            !slots.insert(parsed.slot).second) {
            result.error = GameSessionParseError::DuplicateField; return result;
        }
        result.frame.entries.push_back(std::move(parsed));
        if (result.frame.entries.size() > 8) {
            result.error = GameSessionParseError::InvalidValue; return result;
        }
        if (entryEnd == std::string_view::npos) break;
        entryStart = entryEnd + 1;
    }
    if (result.frame.entries.empty()) { result.error = GameSessionParseError::InvalidValue; return result; }
    result.frame.schema = static_cast<int>(schema);
    result.frame.protocolMajor = static_cast<int>(major);
    result.frame.protocolMinor = static_cast<int>(minor);
    result.frame.sessionId.assign(fields["session"]);
    result.frame.authorityId.assign(fields["authority_id"]);
    return result;
}

inline std::string formatSetVoiceFrame(int src, int dst, double gain,
                                       const std::string &route,
                                       unsigned long long seq) {
    std::ostringstream stream;
    stream.imbue(std::locale::classic());
    stream << RICON_MSG_SET_VOICE
           << " src=" << src
           << " dst=" << dst
           << " gain=" << std::setprecision(7) << std::defaultfloat << gain
           << " route=" << route
           << " seq=" << seq;
    return stream.str();
}

inline std::string formatGameVoiceFrame(int src, int dst, double gain,
                                        const std::string &route,
                                        unsigned long long seq,
                                        unsigned long long generation) {
    return formatSetVoiceFrame(src, dst, gain, route, seq) +
           " generation=" + std::to_string(generation);
}

} // namespace ricon::protocol
#endif

#endif /* RICON_PROTOCOL_H */
