#pragma once

// Veyra IPC wire protocol (Phase 1).
//
// A deliberately small, versioned, framed binary protocol carried over Win32
// message-mode named pipes. Each logical message is one pipe message:
//
//     [ MessageHeader ][ payload bytes ... ]
//
// This stays intentionally minimal; when the message set grows we can swap the
// payload encoding for FlatBuffers without changing the framing or pipe names.

#include <cstdint>
#include <cstring>
#include <string>

namespace veyra::ipc {

inline constexpr wchar_t kControlPipeName[] = L"\\\\.\\pipe\\veyra-control";
inline constexpr wchar_t kTrackerPipeName[] = L"\\\\.\\pipe\\veyra-tracker";

// 'V''Y''R''A' — used to reject foreign clients early.
inline constexpr uint32_t kMagic = 0x56595241u;
inline constexpr uint16_t kProtocolVersion = 1;

// Hard cap on payload size (4 MiB). All real messages are well under 1 MiB;
// this prevents memory exhaustion from a malformed or malicious header.
inline constexpr uint32_t kMaxPayloadBytes = 4u * 1024u * 1024u;

enum class MessageType : uint16_t {
    Ping         = 1,   // -> Pong
    Pong         = 2,
    GetVersion   = 3,   // -> VersionReply (payload: UTF-8 version string)
    VersionReply = 4,
    GetConfig    = 5,   // -> ConfigReply (payload: UTF-8 JSON)
    ConfigReply  = 6,
    SetConfig    = 7,   // payload: UTF-8 JSON -> SetConfigAck
    SetConfigAck = 8,

    // Presets (Phase 5).
    ListPresets  = 9,   // -> PresetsReply (payload: JSON array of presets)
    PresetsReply = 10,
    LoadPreset   = 11,  // payload: preset uuid -> PresetAck
    SavePreset   = 12,  // payload: preset JSON   -> PresetAck
    DeletePreset = 13,  // payload: preset uuid   -> PresetAck
    PresetAck    = 14,

    // Per-app rules (Phase 5).
    GetAppRules   = 15, // -> AppRulesReply (payload: JSON array of rules)
    AppRulesReply = 16,
    SetAppRules   = 17, // payload: JSON array of rules -> AppRulesAck
    AppRulesAck   = 18,

    Error        = 0xFFFF, // payload: UTF-8 human-readable reason
};

#pragma pack(push, 1)
struct MessageHeader {
    uint32_t magic;
    uint16_t version;
    uint16_t type;
    uint32_t payloadSize;
};
#pragma pack(pop)

static_assert(sizeof(MessageHeader) == 12, "MessageHeader must be tightly packed");

struct Message {
    MessageType type{};
    std::string payload; // raw bytes (UTF-8 text for all Phase 1 messages)
};

// Serialise a message to its on-the-wire byte string.
inline std::string serialize(MessageType type, const std::string& payload = {})
{
    MessageHeader header{kMagic, kProtocolVersion, static_cast<uint16_t>(type),
                         static_cast<uint32_t>(payload.size())};
    std::string out;
    out.resize(sizeof(header) + payload.size());
    std::memcpy(out.data(), &header, sizeof(header));
    if (!payload.empty())
        std::memcpy(out.data() + sizeof(header), payload.data(), payload.size());
    return out;
}

inline std::string serialize(const Message& msg)
{
    return serialize(msg.type, msg.payload);
}

// Parse a complete wire byte string into a Message. Returns false on a short
// buffer, bad magic, version mismatch, oversized payload, or truncated data.
inline bool parse(const std::string& bytes, Message& out)
{
    if (bytes.size() < sizeof(MessageHeader))
        return false;

    MessageHeader header{};
    std::memcpy(&header, bytes.data(), sizeof(header));

    if (header.magic != kMagic || header.version != kProtocolVersion)
        return false;

    // Reject oversized payloads before the bounds check to avoid any chance of
    // integer wraparound when sizeof(header) + header.payloadSize is formed.
    if (header.payloadSize > kMaxPayloadBytes)
        return false;
    if (header.payloadSize > bytes.size() - sizeof(header))
        return false;

    out.type = static_cast<MessageType>(header.type);
    out.payload.assign(bytes.data() + sizeof(header), header.payloadSize);
    return true;
}

} // namespace veyra::ipc
