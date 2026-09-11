// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "input_replay.h"

#include <array>
#include <atomic>
#include <chrono>
#include <cstring>
#include <fstream>
#include <iostream>
#include <iterator>
#include <mutex>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <vector>

#include "common/logging/log.h"
#include "core/libraries/pad/pad_errors.h"

namespace Libraries::Pad::InputReplay {
namespace {

constexpr std::array<char, 8> Magic{'S', '4', 'P', 'A', 'D', 'R', 'P', 'L'};
constexpr u32 FormatVersion = 1;
constexpr u32 ClockGnmSubmitDone = 1;
constexpr u32 CallTag = 1;
constexpr u32 EndTag = 2;
constexpr size_t CaptureLimit = 32 * 1024 * 1024;

static_assert(std::is_trivially_copyable_v<OrbisPadData>);
static_assert(sizeof(OrbisPadData) == 120, "Replay format is bound to the pinned pad ABI");

enum class Mode {
    Off,
    RecordArmed,
    Recording,
    RecordComplete,
    ReplayArmed,
    Replaying,
    Complete,
    Failed
};

struct Position {
    u64 progression{};
    u32 ordinal{};
};

struct Call {
    Position position;
    ApiKind api{};
    u32 capacity{};
    u32 count{};
    std::array<OrbisPadData, ORBIS_PAD_MAX_DATA_NUM> samples{};
};

template <typename T>
void Append(std::vector<u8>& out, const T& value) {
    static_assert(std::is_trivially_copyable_v<T>);
    const auto* begin = reinterpret_cast<const u8*>(&value);
    out.insert(out.end(), begin, begin + sizeof(T));
}

template <typename T>
bool Take(const std::vector<u8>& input, size_t& offset, T& value) {
    static_assert(std::is_trivially_copyable_v<T>);
    if (offset > input.size() || input.size() - offset < sizeof(T)) {
        return false;
    }
    std::memcpy(&value, input.data() + offset, sizeof(T));
    offset += sizeof(T);
    return true;
}

bool Before(const Position& left, const Position& right) {
    return left.progression < right.progression ||
           (left.progression == right.progression && left.ordinal < right.ordinal);
}

bool Equal(const Position& left, const Position& right) {
    return left.progression == right.progression && left.ordinal == right.ordinal;
}

class Manager {
public:
    bool ConfigureRecord(const std::filesystem::path& requested_path, bool requested_exit) {
        std::lock_guard lock{mutex};
        Reset();
        if (requested_path.empty() || std::filesystem::exists(requested_path)) {
            LOG_ERROR(Lib_Pad, "INPUT_REPLAY RECORDING_FAILED: output is empty or already exists: {}",
                      requested_path.string());
            return false;
        }
        path = requested_path;
        exit_after_replay = requested_exit;
        mode = Mode::RecordArmed;
        LOG_INFO(Lib_Pad, "INPUT_REPLAY record armed; press F10 to start and F10 to stop: {}",
                 path.string());
        return true;
    }

    bool ConfigureReplay(const std::filesystem::path& requested_path, bool requested_exit,
                         bool requested_ordered_match) {
        std::lock_guard lock{mutex};
        Reset();
        path = requested_path;
        exit_after_replay = requested_exit;
        ordered_match = requested_ordered_match;
        std::string reason;
        if (!Load(reason)) {
            mode = Mode::Failed;
            LOG_ERROR(Lib_Pad, "INPUT_REPLAY INVALID_REPLAY: {} ({})", reason, path.string());
            return false;
        }
        mode = Mode::ReplayArmed;
        LOG_INFO(Lib_Pad,
                 "INPUT_REPLAY replay armed; press F10 at the start state: {} calls policy={}",
                 calls.size(), ordered_match ? "ordered-call" : "strict-position");
        return true;
    }

    void RequestToggle() {
        toggle_requested.store(true, std::memory_order_release);
    }

    bool IsEnabled() {
        std::lock_guard lock{mutex};
        return mode != Mode::Off;
    }

    int Dispatch(ApiKind api, s32 handle, OrbisPadData* data, s32 capacity, u32 progression,
                 const LiveRead& live_read) {
        std::lock_guard lock{mutex};
        if (mode == Mode::Off) {
            return live_read(data, capacity);
        }

        ObserveToggle(progression);
        observed_progression = progression;
        have_observed_progression = true;
        if (boundary_progression && progression != *boundary_progression) {
            if (progression < *boundary_progression) {
                return Fail("GNM submit-done counter moved backward", {}, api, capacity, handle);
            }
            if (mode == Mode::RecordArmed) {
                Start(Mode::Recording, progression, handle);
            } else if (mode == Mode::ReplayArmed) {
                Start(Mode::Replaying, progression, handle);
            } else if (mode == Mode::Recording) {
                end = MakePosition(progression);
                if (!Save()) {
                    return RecordingFail("write failed while completing stream");
                }
                LOG_INFO(Lib_Pad,
                         "INPUT_REPLAY RECORD_COMPLETE calls={} end={}:{} gnm_start={} path={}",
                         calls.size(), end.progression, end.ordinal, base_progression, path.string());
                boundary_progression.reset();
                mode = Mode::RecordComplete;
            }
        }

        if (mode == Mode::RecordArmed || mode == Mode::ReplayArmed || mode == Mode::Off ||
            mode == Mode::RecordComplete) {
            return live_read(data, capacity);
        }
        if (mode == Mode::Complete) {
            WriteNeutral(data);
            return 1;
        }
        if (mode == Mode::Failed) {
            return ORBIS_PAD_ERROR_FATAL;
        }

        if (handle != bound_handle) {
            return Fail("active handle changed", CurrentPosition(progression), api, capacity, handle);
        }
        if (progression < last_progression) {
            return Fail("GNM submit-done counter moved backward", CurrentPosition(progression), api,
                        capacity, handle);
        }
        const Position actual = CurrentPosition(progression);

        if (mode == Mode::Recording) {
            const int count = live_read(data, capacity);
            if (count < 0) {
                return count;
            }
            if (count > capacity || count > ORBIS_PAD_MAX_DATA_NUM) {
                return Fail("live provider returned impossible count", actual, api, capacity, handle);
            }
            const size_t added = sizeof(u32) * 5 + sizeof(u64) +
                                 static_cast<size_t>(count) * sizeof(OrbisPadData);
            if (capture_bytes > CaptureLimit || added > CaptureLimit - capture_bytes) {
                return RecordingFail("capture limit exceeded");
            }
            Call call{};
            call.position = actual;
            call.api = api;
            call.capacity = static_cast<u32>(capacity);
            call.count = static_cast<u32>(count);
            if (count > 0) {
                std::memcpy(call.samples.data(), data, static_cast<size_t>(count) * sizeof(*data));
            }
            calls.push_back(call);
            capture_bytes += added;
            AdvanceOrdinal(progression);
            return count;
        }

        if (next_call == calls.size()) {
            if (ordered_match || Equal(actual, end)) {
                Complete(data);
                return 1;
            }
            return Fail("extra call or end position mismatch", actual, api, capacity, handle);
        }

        const Call& expected = calls[next_call];
        if (!ordered_match && !Equal(actual, expected.position)) {
            return Fail("progression/read ordinal mismatch", actual, api, capacity, handle,
                        &expected);
        }
        if (api != expected.api || static_cast<u32>(capacity) != expected.capacity) {
            return Fail("API/capacity mismatch", actual, api, capacity, handle, &expected);
        }
        if (ordered_match && !Equal(actual, expected.position) && !reported_ordered_divergence) {
            LOG_WARNING(Lib_Pad,
                        "INPUT_REPLAY ORDERED_CALL_DIVERGENCE record={} recorded={}:{} "
                        "actual={}:{}; continuing in recorded call order",
                        next_call, expected.position.progression, expected.position.ordinal,
                        actual.progression, actual.ordinal);
            reported_ordered_divergence = true;
        }
        if (expected.count > 0) {
            std::memcpy(data, expected.samples.data(), expected.count * sizeof(*data));
        }
        ++next_call;
        AdvanceOrdinal(progression);
        return static_cast<int>(expected.count);
    }

    void SetSelfTest(bool enabled) {
        std::lock_guard lock{mutex};
        self_test = enabled;
    }

    int RejectUnsupported(const char* api_name, u32 progression) {
        std::lock_guard lock{mutex};
        if (mode != Mode::Recording && mode != Mode::Replaying) {
            return ORBIS_OK;
        }
        return Fail(std::string{"unsupported active pad read: "} + api_name,
                    CurrentPosition(progression), ApiKind::Read, 0, bound_handle);
    }

private:
    void Reset() {
        mode = Mode::Off;
        path.clear();
        calls.clear();
        end = {};
        next_call = 0;
        capture_bytes = 0;
        base_progression = 0;
        last_progression = 0;
        ordinal = 0;
        bound_handle = -1;
        boundary_progression.reset();
        toggle_requested.store(false, std::memory_order_relaxed);
        have_observed_progression = false;
        ordered_match = false;
        reported_ordered_divergence = false;
    }

    void ObserveToggle(u32 progression) {
        if (!toggle_requested.exchange(false, std::memory_order_acq_rel)) {
            return;
        }
        if (mode == Mode::RecordArmed || mode == Mode::ReplayArmed || mode == Mode::Recording) {
            boundary_progression = have_observed_progression
                                       ? std::optional<u32>{observed_progression}
                                       : std::optional<u32>{progression};
            LOG_INFO(Lib_Pad, "INPUT_REPLAY transition requested at GNM submit-done {}", progression);
        }
    }

    void Start(Mode active_mode, u32 progression, s32 handle) {
        mode = active_mode;
        base_progression = progression;
        last_progression = progression;
        ordinal = 0;
        bound_handle = handle;
        boundary_progression.reset();
        LOG_INFO(Lib_Pad, "INPUT_REPLAY {} at GNM submit-done {} handle={}",
                 mode == Mode::Recording ? "RECORD_START" : "REPLAY_START", progression, handle);
    }

    Position MakePosition(u32 progression) const {
        return {static_cast<u64>(progression - base_progression), 0};
    }

    Position CurrentPosition(u32 progression) const {
        return {static_cast<u64>(progression - base_progression),
                progression == last_progression ? ordinal : 0};
    }

    void AdvanceOrdinal(u32 progression) {
        if (progression == last_progression) {
            ++ordinal;
        } else {
            last_progression = progression;
            ordinal = 1;
        }
    }

    int Fail(const std::string& reason, Position actual, ApiKind api, s32 capacity, s32 handle,
             const Call* expected = nullptr) {
        if (mode != Mode::Failed) {
            if (expected) {
                LOG_ERROR(Lib_Pad,
                          "INPUT_REPLAY DESYNC: {} record={} expected={}:{} api={} capacity={} "
                          "actual={}:{} api={} capacity={} handle={}",
                          reason, next_call, expected->position.progression,
                          expected->position.ordinal, static_cast<u32>(expected->api),
                          expected->capacity, actual.progression, actual.ordinal,
                          static_cast<u32>(api), capacity, handle);
            } else {
                LOG_ERROR(Lib_Pad,
                          "INPUT_REPLAY DESYNC: {} record={} actual={}:{} api={} capacity={} "
                          "handle={}",
                          reason, next_call, actual.progression, actual.ordinal,
                          static_cast<u32>(api), capacity, handle);
            }
            mode = Mode::Failed;
        }
        if (!self_test) {
            std::quick_exit(2);
        }
        return ORBIS_PAD_ERROR_FATAL;
    }

    int RecordingFail(const std::string& reason) {
        if (mode != Mode::Failed) {
            LOG_ERROR(Lib_Pad, "INPUT_REPLAY RECORDING_FAILED: {} calls={} path={}", reason,
                      calls.size(), path.string());
            mode = Mode::Failed;
        }
        if (!self_test) {
            std::quick_exit(2);
        }
        return ORBIS_PAD_ERROR_FATAL;
    }

    static void WriteNeutral(OrbisPadData* data) {
        *data = {};
        data->leftStick = {128, 128};
        data->rightStick = {128, 128};
        data->orientation = {0.0f, 0.0f, 0.0f, 1.0f};
        data->connected = true;
    }

    void Complete(OrbisPadData* data) {
        mode = Mode::Complete;
        WriteNeutral(data);
        LOG_INFO(Lib_Pad,
                 "INPUT_REPLAY REPLAY_COMPLETE calls={} recorded_end={}:{} policy={}", calls.size(),
                 end.progression, end.ordinal, ordered_match ? "ordered-call" : "strict-position");
        if (exit_after_replay && !self_test) {
            std::quick_exit(0);
        }
    }

    bool Save() {
        std::vector<u8> payload;
        payload.reserve(capture_bytes + 32);
        for (const Call& call : calls) {
            Append(payload, CallTag);
            Append(payload, call.position.progression);
            Append(payload, call.position.ordinal);
            Append(payload, static_cast<u32>(call.api));
            Append(payload, call.capacity);
            Append(payload, call.count);
            for (u32 i = 0; i < call.count; ++i) {
                Append(payload, call.samples[i]);
            }
        }
        Append(payload, EndTag);
        Append(payload, end.progression);
        Append(payload, end.ordinal);
        Append(payload, static_cast<u64>(calls.size()));

        std::ofstream output(path, std::ios::binary | std::ios::trunc);
        const u64 payload_size = payload.size();
        output.write(Magic.data(), Magic.size());
        output.write(reinterpret_cast<const char*>(&FormatVersion), sizeof(FormatVersion));
        output.write(reinterpret_cast<const char*>(&ClockGnmSubmitDone), sizeof(ClockGnmSubmitDone));
        output.write(reinterpret_cast<const char*>(&payload_size), sizeof(payload_size));
        output.write(reinterpret_cast<const char*>(payload.data()), payload.size());
        output.close();
        return output.good();
    }

    bool Load(std::string& reason) {
        std::ifstream input(path, std::ios::binary | std::ios::ate);
        if (!input) {
            reason = "cannot open file";
            return false;
        }
        const auto length = input.tellg();
        if (length < 0 || static_cast<u64>(length) > CaptureLimit) {
            reason = "file exceeds size bound";
            return false;
        }
        std::vector<u8> bytes(static_cast<size_t>(length));
        input.seekg(0);
        input.read(reinterpret_cast<char*>(bytes.data()), bytes.size());
        if (!input) {
            reason = "read failed";
            return false;
        }
        size_t offset = 0;
        std::array<char, 8> magic{};
        u32 version{}, clock{};
        u64 payload_size{};
        if (!Take(bytes, offset, magic) || !Take(bytes, offset, version) ||
            !Take(bytes, offset, clock) || !Take(bytes, offset, payload_size) || magic != Magic ||
            version != FormatVersion || clock != ClockGnmSubmitDone ||
            payload_size != bytes.size() - offset) {
            reason = "bad header, version, clock, or payload size";
            return false;
        }

        Position previous{};
        bool have_previous = false;
        bool have_end = false;
        while (offset < bytes.size()) {
            u32 tag{};
            if (!Take(bytes, offset, tag)) {
                reason = "truncated record tag";
                return false;
            }
            if (tag == EndTag) {
                u64 total{};
                if (!Take(bytes, offset, end.progression) || !Take(bytes, offset, end.ordinal) ||
                    !Take(bytes, offset, total) || total != calls.size() ||
                    offset != bytes.size() || (have_previous && !Before(previous, end))) {
                    reason = "invalid end record";
                    return false;
                }
                have_end = true;
                break;
            }
            if (tag != CallTag) {
                reason = "unknown record tag";
                return false;
            }
            Call call{};
            u32 api{};
            if (!Take(bytes, offset, call.position.progression) ||
                !Take(bytes, offset, call.position.ordinal) || !Take(bytes, offset, api) ||
                !Take(bytes, offset, call.capacity) || !Take(bytes, offset, call.count) ||
                (api != static_cast<u32>(ApiKind::Read) &&
                 api != static_cast<u32>(ApiKind::ReadState)) ||
                call.capacity < 1 || call.capacity > ORBIS_PAD_MAX_DATA_NUM ||
                call.count > call.capacity || call.count > ORBIS_PAD_MAX_DATA_NUM) {
                reason = "invalid call record";
                return false;
            }
            call.api = static_cast<ApiKind>(api);
            if (have_previous) {
                const bool valid_next =
                    (call.position.progression == previous.progression &&
                     call.position.ordinal == previous.ordinal + 1) ||
                    (call.position.progression > previous.progression && call.position.ordinal == 0);
                if (!valid_next) {
                    reason = "non-canonical call ordering";
                    return false;
                }
            } else if (call.position.progression != 0 || call.position.ordinal != 0) {
                reason = "first call is not at 0:0";
                return false;
            }
            for (u32 i = 0; i < call.count; ++i) {
                if (!Take(bytes, offset, call.samples[i])) {
                    reason = "truncated sample payload";
                    return false;
                }
            }
            previous = call.position;
            have_previous = true;
            calls.push_back(call);
        }
        if (!have_end) {
            reason = "missing end record";
            return false;
        }
        return true;
    }

    std::mutex mutex;
    std::atomic_bool toggle_requested{false};
    Mode mode{Mode::Off};
    std::filesystem::path path;
    std::vector<Call> calls;
    Position end{};
    size_t next_call{};
    size_t capture_bytes{};
    u32 base_progression{};
    u32 last_progression{};
    u32 ordinal{};
    s32 bound_handle{-1};
    std::optional<u32> boundary_progression;
    bool exit_after_replay{};
    bool self_test{};
    u32 observed_progression{};
    bool have_observed_progression{};
    bool ordered_match{};
    bool reported_ordered_divergence{};
};

Manager manager;

OrbisPadData Sample(u8 left_x, OrbisPadButtonDataOffset buttons) {
    OrbisPadData data{};
    data.buttons = buttons;
    data.leftStick = {left_x, 128};
    data.rightStick = {128, 128};
    data.orientation = {0.0f, 0.0f, 0.0f, 1.0f};
    data.connected = true;
    return data;
}

} // namespace

bool ConfigureRecord(const std::filesystem::path& path, bool exit_after_replay) {
    return manager.ConfigureRecord(path, exit_after_replay);
}

bool ConfigureReplay(const std::filesystem::path& path, bool exit_after_replay,
                     bool ordered_match) {
    return manager.ConfigureReplay(path, exit_after_replay, ordered_match);
}

void RequestToggle() {
    manager.RequestToggle();
}

bool IsEnabled() {
    return manager.IsEnabled();
}

int RejectUnsupported(const char* api_name, u32 progression) {
    return manager.RejectUnsupported(api_name, progression);
}

int Dispatch(ApiKind api, s32 handle, OrbisPadData* data, s32 capacity, u32 progression,
             const LiveRead& live_read) {
    return manager.Dispatch(api, handle, data, capacity, progression, live_read);
}

int RunSelfTest(const std::filesystem::path& directory) {
    std::error_code error;
    std::filesystem::create_directories(directory, error);
    if (error) {
        std::cerr << "input replay self-test: cannot create directory: " << error.message() << '\n';
        return 1;
    }
    const auto nonce = std::chrono::steady_clock::now().time_since_epoch().count();
    const auto replay_path = directory / ("self-test-" + std::to_string(nonce) + ".bbpad");
    const auto invalid_path = directory / ("self-test-invalid-" + std::to_string(nonce) + ".bbpad");
    manager.SetSelfTest(true);

    std::vector<std::vector<OrbisPadData>> outputs{
        {Sample(128, OrbisPadButtonDataOffset::None)},
        {Sample(128, OrbisPadButtonDataOffset::None)},
        {Sample(220, OrbisPadButtonDataOffset::None)},
        {},
        {Sample(128, OrbisPadButtonDataOffset::None),
         Sample(128, OrbisPadButtonDataOffset::Cross)},
        {Sample(128, OrbisPadButtonDataOffset::None)},
        {Sample(128, OrbisPadButtonDataOffset::None)},
    };
    size_t live_index = 0;
    auto live = [&](OrbisPadData* data, s32 capacity) {
        const auto& output = outputs.at(live_index++);
        if (static_cast<s32>(output.size()) > capacity) {
            return -1;
        }
        if (!output.empty()) {
            std::memcpy(data, output.data(), output.size() * sizeof(*data));
        }
        return static_cast<int>(output.size());
    };
    OrbisPadData buffer[4];
    std::memset(buffer, 0xa5, sizeof(buffer));
    if (!ConfigureRecord(replay_path, false)) {
        return 1;
    }
    RequestToggle();
    Dispatch(ApiKind::Read, 7, buffer, 1, 99, live);
    const std::array<std::tuple<u32, ApiKind, s32>, 5> shape{{
        {100, ApiKind::Read, 1}, {100, ApiKind::ReadState, 1}, {101, ApiKind::Read, 2},
        {102, ApiKind::Read, 2}, {103, ApiKind::ReadState, 1},
    }};
    for (const auto& [progression, api, capacity] : shape) {
        std::memset(buffer, 0xa5, sizeof(buffer));
        const auto trailing = buffer[capacity];
        const int result = Dispatch(api, 7, buffer, capacity, progression, live);
        if (result < 0 || std::memcmp(&buffer[capacity], &trailing, sizeof(trailing)) != 0) {
            std::cerr << "input replay self-test: record copy semantics failed\n";
            return 1;
        }
    }
    RequestToggle();
    Dispatch(ApiKind::Read, 7, buffer, 1, 104, live);

    auto replay_once = [&](std::optional<size_t> mutation, const char* expected_failure) {
        if (!ConfigureReplay(replay_path, false)) {
            return false;
        }
        RequestToggle();
        int live_calls = 0;
        auto forbidden_live = [&](OrbisPadData*, s32) {
            ++live_calls;
            return 1;
        };
        Dispatch(ApiKind::Read, 7, buffer, 1, 199, forbidden_live);
        for (size_t i = 0; i < shape.size(); ++i) {
            auto [progression, api, capacity] = shape[i];
            progression += 100;
            if (mutation == i) {
                if (std::string_view(expected_failure) == "progression") {
                    ++progression;
                } else if (std::string_view(expected_failure) == "api") {
                    api = api == ApiKind::Read ? ApiKind::ReadState : ApiKind::Read;
                } else if (std::string_view(expected_failure) == "capacity") {
                    capacity = capacity == 1 ? 2 : 1;
                }
            }
            std::memset(buffer, 0xa5, sizeof(buffer));
            std::array<OrbisPadData, 4> before;
            std::memcpy(before.data(), buffer, sizeof(buffer));
            const int result = Dispatch(api, 7, buffer, capacity, progression, forbidden_live);
            if (mutation) {
                if (i == *mutation) {
                    return result == ORBIS_PAD_ERROR_FATAL && live_calls == 1;
                }
            } else {
                const auto& expected_output = outputs[i + 1];
                if (result != static_cast<int>(expected_output.size()) ||
                    (!expected_output.empty() &&
                     std::memcmp(buffer, expected_output.data(),
                                 expected_output.size() * sizeof(*buffer)) != 0) ||
                    std::memcmp(buffer + expected_output.size(),
                                before.data() + expected_output.size(),
                                (std::size(buffer) - expected_output.size()) * sizeof(*buffer)) != 0) {
                    return false;
                }
            }
        }
        const int complete = Dispatch(ApiKind::Read, 7, buffer, 1, 204, forbidden_live);
        return complete == 1 && live_calls == 1;
    };

    if (!replay_once(std::nullopt, "") || !replay_once(2, "progression") ||
        !replay_once(0, "api") || !replay_once(0, "capacity")) {
        std::cerr << "input replay self-test: replay or strict mismatch control failed\n";
        return 1;
    }

    if (!ConfigureReplay(replay_path, false, true)) {
        return 1;
    }
    RequestToggle();
    int ordered_live_calls = 0;
    auto ordered_forbidden_live = [&](OrbisPadData*, s32) {
        ++ordered_live_calls;
        return 1;
    };
    Dispatch(ApiKind::Read, 7, buffer, 1, 499, ordered_forbidden_live);
    const std::array<u32, 5> ordered_progression{{500, 501, 503, 504, 506}};
    for (size_t i = 0; i < shape.size(); ++i) {
        const auto [unused_progression, api, capacity] = shape[i];
        std::memset(buffer, 0xa5, sizeof(buffer));
        const auto& expected_output = outputs[i + 1];
        const int result =
            Dispatch(api, 7, buffer, capacity, ordered_progression[i], ordered_forbidden_live);
        if (result != static_cast<int>(expected_output.size()) ||
            (!expected_output.empty() &&
             std::memcmp(buffer, expected_output.data(),
                         expected_output.size() * sizeof(*buffer)) != 0)) {
            std::cerr << "input replay self-test: ordered-call replay failed\n";
            return 1;
        }
    }
    if (Dispatch(ApiKind::Read, 7, buffer, 1, 507, ordered_forbidden_live) != 1 ||
        ordered_live_calls != 1) {
        std::cerr << "input replay self-test: ordered-call completion/fallback failed\n";
        return 1;
    }

    if (!ConfigureReplay(replay_path, false)) {
        return 1;
    }
    RequestToggle();
    int ordinal_live_calls = 0;
    auto ordinal_live = [&](OrbisPadData*, s32) {
        ++ordinal_live_calls;
        return 1;
    };
    Dispatch(ApiKind::Read, 7, buffer, 1, 399, ordinal_live);
    Dispatch(ApiKind::Read, 7, buffer, 1, 400, ordinal_live);
    Dispatch(ApiKind::ReadState, 7, buffer, 1, 400, ordinal_live);
    const int ordinal_result = Dispatch(ApiKind::Read, 7, buffer, 2, 400, ordinal_live);
    if (ordinal_result != ORBIS_PAD_ERROR_FATAL || ordinal_live_calls != 1) {
        std::cerr << "input replay self-test: ordinal control failed\n";
        return 1;
    }

    std::ifstream valid(replay_path, std::ios::binary);
    std::vector<char> truncated((std::istreambuf_iterator<char>(valid)), {});
    truncated.resize(truncated.size() - 1);
    std::ofstream invalid(invalid_path, std::ios::binary);
    invalid.write(truncated.data(), truncated.size());
    invalid.close();
    if (ConfigureReplay(invalid_path, false)) {
        std::cerr << "input replay self-test: truncated replay was accepted\n";
        return 1;
    }

    std::cout << "INPUT_REPLAY_SELF_TEST PASS record serialize strict/ordered replay complete; "
                 "wrong-progression wrong-ordinal API capacity truncated controls PASS\n";
    return 0;
}

} // namespace Libraries::Pad::InputReplay
