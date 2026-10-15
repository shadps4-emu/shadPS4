// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
#include <queue>

#include <SDL3/SDL.h>
#include <minimp3_ex.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libavutil/mathematics.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
}
#include "common/support/avdec.h"

#define DR_FLAC_IMPLEMENTATION
#include <dr_flac.h>

#include "common/logging/log.h"
#include "common/singleton.h"
#include "core/emulator_settings.h"
#include "core/file_sys/fs.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/libs.h"
#include "music_player_service.h"

namespace Libraries::MusicPlayerService {

class MusicPlayerManager {
public:
    static MusicPlayerManager& Instance() {
        static MusicPlayerManager s_instance;
        return s_instance;
    }

    void SetTrackList(const std::vector<std::string>& tracks) {
        std::scoped_lock lock(m_mutex);
        m_tracks = tracks;
        m_tracklists[0] = tracks;
        m_tracklist_version++;
    }

    void ClearTracks() {
        StopPlayback();
        std::scoped_lock lock(m_mutex);
        m_tracks.clear();
        m_tracklists.clear();
        m_tracklist_version++;
    }

    s32 BeginTransaction() {
        std::scoped_lock lock(m_mutex);
        if (m_in_transaction.load()) {
            LOG_ERROR(Lib_AudioOut, "BeginTransaction: already in transaction");
            return static_cast<s32>(0x812f0101);
        }
        m_in_transaction = true;
        return ORBIS_OK;
    }

    s32 EndTransaction() {
        std::scoped_lock lock(m_mutex);
        if (!m_in_transaction.load()) {
            LOG_ERROR(Lib_AudioOut, "EndTransaction: not in transaction");
            return static_cast<s32>(0x812f0102);
        }
        m_in_transaction = false;
        m_tracklist_version++;
        auto it = m_tracklists.find(0);
        if (it != m_tracklists.end()) {
            m_tracks = it->second;
        }
        return ORBIS_OK;
    }

    s32 CreateTrackList(u32 tracklist_id) {
        if (tracklist_id > 99) {
            LOG_ERROR(Lib_AudioOut, "CreateTrackList: invalid tracklist_id={}", tracklist_id);
            return static_cast<s32>(0x812f0004);
        }
        std::scoped_lock lock(m_mutex);
        if (!m_in_transaction.load()) {
            LOG_ERROR(Lib_AudioOut, "CreateTrackList: not in transaction");
            return static_cast<s32>(0x812f0102);
        }
        m_tracklists[tracklist_id].clear();
        return ORBIS_OK;
    }

    s32 RemoveTrackList(u32 tracklist_id) {
        if (tracklist_id > 99) {
            LOG_ERROR(Lib_AudioOut, "RemoveTrackList: invalid tracklist_id={}", tracklist_id);
            return static_cast<s32>(0x812f0004);
        }
        std::scoped_lock lock(m_mutex);
        if (!m_in_transaction.load()) {
            LOG_ERROR(Lib_AudioOut, "RemoveTrackList: not in transaction");
            return static_cast<s32>(0x812f0102);
        }
        m_tracklists.erase(tracklist_id);
        if (tracklist_id == 0) {
            m_tracks.clear();
        }
        return ORBIS_OK;
    }

    s32 InsertTrackEntry(u32 tracklist_id, u32 track_index, const std::string& uri) {
        if (tracklist_id > 99 || track_index > 3999) {
            LOG_ERROR(Lib_AudioOut, "InsertTrackEntry: invalid tracklist_id={} or track_index={}",
                      tracklist_id, track_index);
            return static_cast<s32>(0x812f0004);
        }
        std::scoped_lock lock(m_mutex);
        if (!m_in_transaction.load()) {
            LOG_ERROR(Lib_AudioOut, "InsertTrackEntry: not in transaction");
            return static_cast<s32>(0x812f0102);
        }
        auto& list = m_tracklists[tracklist_id];
        if (track_index < list.size()) {
            list.insert(list.begin() + track_index, uri);
        } else {
            list.push_back(uri);
        }
        return ORBIS_OK;
    }

    s32 RemoveTrackEntry(u32 tracklist_id, u32 track_index) {
        if (tracklist_id > 99 || track_index > 3999) {
            LOG_ERROR(Lib_AudioOut, "RemoveTrackEntry: invalid tracklist_id={} or track_index={}",
                      tracklist_id, track_index);
            return static_cast<s32>(0x812f0004);
        }
        std::scoped_lock lock(m_mutex);
        if (!m_in_transaction.load()) {
            LOG_ERROR(Lib_AudioOut, "RemoveTrackEntry: not in transaction");
            return static_cast<s32>(0x812f0102);
        }
        auto it = m_tracklists.find(tracklist_id);
        if (it != m_tracklists.end() && track_index < it->second.size()) {
            it->second.erase(it->second.begin() + track_index);
        }
        return ORBIS_OK;
    }

    u64 GetTrackListVersion() const {
        return m_tracklist_version.load();
    }

    u32 GetTrackCount() {
        std::scoped_lock lock(m_mutex);
        return static_cast<u32>(m_tracks.size());
    }

    struct MusicEvent {
        u32 event_id;
        u32 track_index;
    };

    void PushEvent(u32 event_id, u32 track_index) {
        std::scoped_lock lock(m_event_mutex);
        m_event_queue.push({event_id, track_index});
        m_event_cv.notify_one();
    }

    bool PopEvent(MusicEvent& ev, u32 timeout_ms = 50) {
        std::unique_lock lock(m_event_mutex);
        if (m_event_cv.wait_for(lock, std::chrono::milliseconds(timeout_ms),
                                [this] { return !m_event_queue.empty(); })) {
            ev = m_event_queue.front();
            m_event_queue.pop();
            return true;
        }
        return false;
    }

    void Play(u32 track_index, u64 start_time_ms) {
        std::scoped_lock control_lock(m_control_mutex);
        PlayLocked(track_index, start_time_ms);
    }

    void PlayLocked(u32 track_index, u64 start_time_ms) {
        StopPlaybackInternal();

        {
            std::scoped_lock lock(m_mutex);
            if (track_index >= m_tracks.size()) {
                return;
            }
            m_current_track_idx = track_index;
        }

        m_stop_requested = false;
        m_pause_requested = false;
        m_seek_offset_ms = -1;
        m_current_time_ms = start_time_ms;
        m_total_duration_ms = 0;
        m_play_state = 1;

        m_worker_thread =
            std::thread(&MusicPlayerManager::PlaybackWorker, this, track_index, start_time_ms);
    }

    void StopPlayback() {
        std::scoped_lock control_lock(m_control_mutex);
        StopPlaybackInternal();
    }

    void StopPlaybackInternal() {
        m_stop_requested = true;
        m_pause_requested = false;
        m_play_state = 0;
        m_current_time_ms = 0;
        m_total_duration_ms = 0;

        if (m_worker_thread.joinable()) {
            m_worker_thread.join();
        }
    }

    void Pause() {
        m_pause_requested = true;
        m_play_state = 2;
    }

    void Unpause() {
        m_pause_requested = false;
        m_play_state = 1;
    }

    void Seek(u64 time_ms) {
        m_seek_offset_ms = static_cast<s64>(time_ms);
        m_current_time_ms = time_ms;
    }

    bool IsTrackActive(u32 track_index) {
        return (m_current_track_idx.load() == track_index) && (m_play_state.load() != 0) &&
               m_worker_thread.joinable() && !m_stop_requested.load();
    }

    void PlayOrSeek(u32 track_index, u64 start_time_ms) {
        std::scoped_lock control_lock(m_control_mutex);
        if (IsTrackActive(track_index)) {
            if (start_time_ms > 0) {
                Seek(start_time_ms);
            }
            Unpause();
        } else {
            PlayLocked(track_index, start_time_ms);
        }
    }

    void SeekTrack(u32 track_index, u64 time_ms) {
        std::scoped_lock control_lock(m_control_mutex);
        if (IsTrackActive(track_index)) {
            Seek(time_ms);
        } else {
            PlayLocked(track_index, time_ms);
        }
    }

    void NextTrack() {
        std::scoped_lock control_lock(m_control_mutex);
        u32 count = GetTrackCount();
        if (count > 0) {
            u32 cur = m_current_track_idx.load();
            u32 next_idx = (cur + 1) % count;
            if (m_shuffle_mode.load() == 1 && count > 1) {
                for (int retry = 0; retry < 10 && next_idx == cur; ++retry) {
                    next_idx = static_cast<u32>(rand() % count);
                }
            }
            PlayLocked(next_idx, 0);
        }
    }

    void PreviousTrack() {
        std::scoped_lock control_lock(m_control_mutex);
        u32 count = GetTrackCount();
        if (count > 0) {
            u32 cur = m_current_track_idx.load();
            u32 prev_idx = (cur > 0) ? (cur - 1) : 0;
            PlayLocked(prev_idx, 0);
        }
    }

    void SetVolume(int vol) {
        m_volume = std::clamp(vol, 0, 25);
    }

    int GetVolume() const {
        return m_volume.load();
    }

    void SetRepeatMode(u32 mode) {
        m_repeat_mode = mode;
    }

    void SetShuffle(u32 mode) {
        m_shuffle_mode = mode;
    }

    u32 GetPlayState() const {
        return m_play_state.load();
    }

    u32 GetCurrentTrackIndex() const {
        return m_current_track_idx.load();
    }

    u64 GetCurrentTimeMs() const {
        return m_current_time_ms.load();
    }

    u64 GetTotalDurationMs() const {
        return m_total_duration_ms.load();
    }

    u32 GetRepeatMode() const {
        return m_repeat_mode.load();
    }

    u32 GetShuffleMode() const {
        return m_shuffle_mode.load();
    }

private:
    MusicPlayerManager() = default;
    ~MusicPlayerManager() {
        StopPlayback();
    }

    static bool DecodeAudioWithFFmpeg(const std::string& path, mp3dec_file_info_t* file_info) {
        AVFormatContext* format_ctx = nullptr;
        if (avformat_open_input(&format_ctx, path.c_str(), nullptr, nullptr) != 0) {
            return false;
        }
        if (avformat_find_stream_info(format_ctx, nullptr) < 0) {
            avformat_close_input(&format_ctx);
            return false;
        }
        int audio_stream_idx =
            av_find_best_stream(format_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
        if (audio_stream_idx < 0) {
            avformat_close_input(&format_ctx);
            return false;
        }
        AVStream* stream = format_ctx->streams[audio_stream_idx];
        const AVCodec* decoder = avcodec_find_decoder(stream->codecpar->codec_id);
        if (!decoder) {
            avformat_close_input(&format_ctx);
            return false;
        }
        AVCodecContext* codec_ctx = avcodec_alloc_context3(decoder);
        if (!codec_ctx) {
            avformat_close_input(&format_ctx);
            return false;
        }
        if (avcodec_parameters_to_context(codec_ctx, stream->codecpar) < 0 ||
            avcodec_open2(codec_ctx, decoder, nullptr) < 0) {
            avcodec_free_context(&codec_ctx);
            avformat_close_input(&format_ctx);
            return false;
        }

        int out_channels =
            codec_ctx->ch_layout.nb_channels > 0 ? codec_ctx->ch_layout.nb_channels : 2;
        int out_sample_rate = codec_ctx->sample_rate > 0 ? codec_ctx->sample_rate : 44100;
        AVChannelLayout out_ch_layout;
        av_channel_layout_default(&out_ch_layout, out_channels);

        SwrContext* swr = nullptr;
        int swr_res = swr_alloc_set_opts2(&swr, &out_ch_layout, AV_SAMPLE_FMT_S16, out_sample_rate,
                                          &codec_ctx->ch_layout, codec_ctx->sample_fmt,
                                          codec_ctx->sample_rate, 0, nullptr);
        if (swr_res < 0 || !swr || swr_init(swr) < 0) {
            if (swr)
                swr_free(&swr);
            av_channel_layout_uninit(&out_ch_layout);
            avcodec_free_context(&codec_ctx);
            avformat_close_input(&format_ctx);
            return false;
        }

        AVPacket* pkt = av_packet_alloc();
        AVFrame* frame = av_frame_alloc();
        std::vector<int16_t> pcm_data;

        while (av_read_frame(format_ctx, pkt) >= 0) {
            if (pkt->stream_index == audio_stream_idx) {
                if (avcodec_send_packet(codec_ctx, pkt) >= 0) {
                    while (avcodec_receive_frame(codec_ctx, frame) >= 0) {
                        int max_out_samples =
                            av_rescale_rnd(swr_get_delay(swr, out_sample_rate) + frame->nb_samples,
                                           out_sample_rate, codec_ctx->sample_rate, AV_ROUND_UP);
                        size_t cur_size = pcm_data.size();
                        pcm_data.resize(cur_size + max_out_samples * out_channels);
                        uint8_t* out_ptr = reinterpret_cast<uint8_t*>(pcm_data.data() + cur_size);
                        int converted =
                            swr_convert(swr, &out_ptr, max_out_samples,
                                        (const uint8_t**)frame->extended_data, frame->nb_samples);
                        if (converted > 0) {
                            pcm_data.resize(cur_size + converted * out_channels);
                        } else {
                            pcm_data.resize(cur_size);
                        }
                    }
                }
            }
            av_packet_unref(pkt);
        }

        avcodec_send_packet(codec_ctx, nullptr);
        while (avcodec_receive_frame(codec_ctx, frame) >= 0) {
            int max_out_samples =
                av_rescale_rnd(swr_get_delay(swr, out_sample_rate) + frame->nb_samples,
                               out_sample_rate, codec_ctx->sample_rate, AV_ROUND_UP);
            size_t cur_size = pcm_data.size();
            pcm_data.resize(cur_size + max_out_samples * out_channels);
            uint8_t* out_ptr = reinterpret_cast<uint8_t*>(pcm_data.data() + cur_size);
            int converted = swr_convert(swr, &out_ptr, max_out_samples,
                                        (const uint8_t**)frame->extended_data, frame->nb_samples);
            if (converted > 0) {
                pcm_data.resize(cur_size + converted * out_channels);
            } else {
                pcm_data.resize(cur_size);
            }
        }

        int max_out_samples = swr_get_delay(swr, out_sample_rate);
        if (max_out_samples > 0) {
            size_t cur_size = pcm_data.size();
            pcm_data.resize(cur_size + max_out_samples * out_channels);
            uint8_t* out_ptr = reinterpret_cast<uint8_t*>(pcm_data.data() + cur_size);
            int converted = swr_convert(swr, &out_ptr, max_out_samples, nullptr, 0);
            if (converted > 0) {
                pcm_data.resize(cur_size + converted * out_channels);
            } else {
                pcm_data.resize(cur_size);
            }
        }

        av_frame_free(&frame);
        av_packet_free(&pkt);
        swr_free(&swr);
        av_channel_layout_uninit(&out_ch_layout);
        avcodec_free_context(&codec_ctx);
        avformat_close_input(&format_ctx);

        if (pcm_data.empty()) {
            return false;
        }

        file_info->channels = out_channels;
        file_info->hz = out_sample_rate;
        file_info->samples = pcm_data.size();
        file_info->buffer = static_cast<mp3d_sample_t*>(malloc(pcm_data.size() * sizeof(int16_t)));
        if (!file_info->buffer) {
            return false;
        }
        std::memcpy(file_info->buffer, pcm_data.data(), pcm_data.size() * sizeof(int16_t));
        return true;
    }

    void PlaybackWorker(u32 start_track_index, u64 initial_start_time_ms) {
        u32 track_idx = start_track_index;
        u64 start_time_ms = initial_start_time_ms;

        while (!m_stop_requested.load()) {
            std::string guest_path;
            {
                std::scoped_lock lock(m_mutex);
                if (track_idx < m_tracks.size()) {
                    guest_path = m_tracks[track_idx];
                    m_current_track_idx = track_idx;
                } else {
                    break;
                }
            }

            auto scheme_pos = guest_path.find("://");
            if (scheme_pos != std::string::npos) {
                guest_path = guest_path.substr(scheme_pos + 3);
            }
            if (!guest_path.starts_with("/")) {
                guest_path = "/" + guest_path;
            }

            std::string decoded_path;
            decoded_path.reserve(guest_path.size());
            for (size_t i = 0; i < guest_path.size(); ++i) {
                if (guest_path[i] == '%' && i + 2 < guest_path.size()) {
                    if (std::isxdigit(guest_path[i + 1]) && std::isxdigit(guest_path[i + 2])) {
                        char hex_str[3] = {guest_path[i + 1], guest_path[i + 2], '\0'};
                        int hex_val = static_cast<int>(std::strtol(hex_str, nullptr, 16));
                        decoded_path.push_back(static_cast<char>(hex_val));
                        i += 2;
                        continue;
                    }
                }
                decoded_path.push_back(guest_path[i]);
            }
            guest_path = std::move(decoded_path);

            auto* mnt = Common::Singleton<Core::FileSys::MntPoints>::Instance();
            std::filesystem::path host_path;
            if (mnt) {
                host_path = mnt->GetHostPath(guest_path);
            }
            if (host_path.empty() || !std::filesystem::exists(host_path)) {
                host_path = guest_path;
            }

            if (!std::filesystem::exists(host_path)) {
                LOG_ERROR(Lib_AudioOut, "Music file does not exist: {}", host_path.string());
                break;
            }

            mp3dec_file_info_t file_info{};
            bool decoded = false;

            const auto ext = host_path.extension().string();
            if (ext == ".flac" || ext == ".FLAC") {
                unsigned int flac_channels = 0;
                unsigned int flac_hz = 0;
                drflac_uint64 flac_total_frames = 0;
                drflac_int16* flac_buf = drflac_open_file_and_read_pcm_frames_s16(
                    host_path.string().c_str(), &flac_channels, &flac_hz, &flac_total_frames,
                    nullptr);
                if (flac_buf && flac_total_frames > 0 && flac_channels > 0 && flac_hz > 0) {
                    file_info.channels = static_cast<int>(flac_channels);
                    file_info.hz = static_cast<int>(flac_hz);
                    file_info.samples = static_cast<size_t>(flac_total_frames * flac_channels);
                    file_info.buffer = reinterpret_cast<mp3d_sample_t*>(flac_buf);
                    decoded = true;
                }
            }

            if (!decoded && (ext == ".mp3" || ext == ".MP3")) {
                mp3dec_t dec;
                mp3dec_init(&dec);
                int res =
                    mp3dec_load(&dec, host_path.string().c_str(), &file_info, nullptr, nullptr);
                if (res == 0 && file_info.samples > 0 && file_info.channels > 0 &&
                    file_info.hz > 0) {
                    decoded = true;
                }
            }

            if (!decoded && (ext == ".m4a" || ext == ".M4A" || ext == ".aac" || ext == ".AAC" ||
                             ext == ".mp4" || ext == ".MP4")) {
                decoded = DecodeAudioWithFFmpeg(host_path.string(), &file_info);
            }

            if (!decoded) {
                LOG_ERROR(Lib_AudioOut, "Failed to decode audio file: {}", host_path.string());
                break;
            }

            u64 total_duration =
                (static_cast<u64>(file_info.samples) / file_info.channels) * 1000 / file_info.hz;
            m_total_duration_ms = total_duration;

            if (m_stop_requested.load()) {
                free(file_info.buffer);
                break;
            }

            const SDL_AudioSpec spec = {
                .format = SDL_AUDIO_S16LE,
                .channels = static_cast<u8>(file_info.channels),
                .freq = static_cast<int>(file_info.hz),
            };

            SDL_AudioStream* stream = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK,
                                                                &spec, nullptr, nullptr);
            if (!stream) {
                LOG_ERROR(Lib_AudioOut, "Failed to open SDL audio stream: {}", SDL_GetError());
                free(file_info.buffer);
                break;
            }

            SDL_ResumeAudioStreamDevice(stream);

            size_t sample_offset = 0;
            if (start_time_ms > 0 && total_duration > 0) {
                sample_offset = (start_time_ms * file_info.hz / 1000) * file_info.channels;
                if (sample_offset > file_info.samples) {
                    sample_offset = file_info.samples;
                }
            }

            constexpr size_t chunk_frames = 2048;
            const size_t chunk_samples = chunk_frames * file_info.channels;
            bool flushed = false;

            while (!m_stop_requested.load()) {
                s64 seek_target = m_seek_offset_ms.exchange(-1);
                if (seek_target >= 0) {
                    sample_offset =
                        (static_cast<u64>(seek_target) * file_info.hz / 1000) * file_info.channels;
                    if (sample_offset > file_info.samples) {
                        sample_offset = file_info.samples;
                    }
                    m_current_time_ms = static_cast<u64>(seek_target);
                    SDL_ClearAudioStream(stream);
                    flushed = false;
                }

                if (m_pause_requested.load()) {
                    SDL_PauseAudioStreamDevice(stream);
                    std::this_thread::sleep_for(std::chrono::milliseconds(20));
                    continue;
                }

                SDL_ResumeAudioStreamDevice(stream);

                float vol =
                    (m_volume.load() / 25.0f) * (EmulatorSettings.GetVolumeSlider() * 0.01f);
                SDL_SetAudioStreamGain(stream, vol);

                int queued = SDL_GetAudioStreamQueued(stream);
                if (sample_offset < file_info.samples) {
                    if (queued <
                        static_cast<int>(chunk_frames * file_info.channels * sizeof(int16_t) * 4)) {
                        size_t to_write =
                            std::min(chunk_samples, file_info.samples - sample_offset);
                        SDL_PutAudioStreamData(stream, &file_info.buffer[sample_offset],
                                               static_cast<int>(to_write * sizeof(int16_t)));
                        sample_offset += to_write;
                    }
                } else {
                    if (!flushed) {
                        SDL_FlushAudioStream(stream);
                        flushed = true;
                    }
                    if (queued <= 0) {
                        break;
                    }
                }

                size_t queued_samples =
                    (queued > 0)
                        ? (static_cast<size_t>(queued) / (file_info.channels * sizeof(int16_t)))
                        : 0;
                size_t played_samples =
                    (sample_offset > queued_samples) ? (sample_offset - queued_samples) : 0;
                u64 cur_ms = (played_samples / file_info.channels) * 1000 / file_info.hz;
                m_current_time_ms = cur_ms;

                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }

            SDL_DestroyAudioStream(stream);
            free(file_info.buffer);

            if (m_stop_requested.load()) {
                break;
            }

            u32 count = GetTrackCount();
            if (count == 0) {
                break;
            }

            u32 rep = m_repeat_mode.load();
            u32 shuf = m_shuffle_mode.load();

            u32 next_track = track_idx;
            if (rep == 2) {
                // Repeat one
                next_track = track_idx;
            } else if (shuf == 1) {
                // Shuffle
                if (count > 1) {
                    for (int retry = 0; retry < 10 && next_track == track_idx; ++retry) {
                        next_track = static_cast<u32>(rand() % count);
                    }
                }
            } else if (rep == 1) {
                // Repeat all
                next_track = (track_idx + 1) % count;
            } else {
                // Repeat off
                if (track_idx + 1 < count) {
                    next_track = track_idx + 1;
                } else {
                    LOG_INFO(Lib_AudioOut, "Playback reached end of playlist");
                    break;
                }
            }

            track_idx = next_track;
            start_time_ms = 0;
            m_current_track_idx = track_idx;
            m_current_time_ms = 0;
            LOG_INFO(Lib_AudioOut, "Auto-advancing to track_index={}", track_idx);
        }

        if (!m_stop_requested.load()) {
            m_play_state = 0;
            m_current_time_ms = 0;
            LOG_INFO(Lib_AudioOut, "Playback finished");
        }
    }

    std::mutex m_control_mutex;
    std::mutex m_mutex;
    std::vector<std::string> m_tracks;
    std::atomic<u32> m_play_state{0};
    std::atomic<u32> m_current_track_idx{0};
    std::atomic<u64> m_current_time_ms{0};
    std::atomic<u64> m_total_duration_ms{0};
    std::atomic<int> m_volume{25};
    std::atomic<u32> m_repeat_mode{0};
    std::atomic<u32> m_shuffle_mode{0};
    std::atomic<bool> m_stop_requested{false};
    std::atomic<bool> m_pause_requested{false};
    std::atomic<s64> m_seek_offset_ms{-1};
    std::thread m_worker_thread;
    std::mutex m_event_mutex;
    std::condition_variable m_event_cv;
    std::queue<MusicEvent> m_event_queue;
    std::unordered_map<u32, std::vector<std::string>> m_tracklists;
    std::atomic<bool> m_in_transaction{false};
    std::atomic<u64> m_tracklist_version{1};
};

static s32 PS4_SYSV_ABI sceMusicPlayerServiceInitialize(size_t pool_size, void* pool_addr) {
    LOG_ERROR(Lib_AudioOut, "(STUBBED) called");
    return ORBIS_OK;
}

static s32 PS4_SYSV_ABI sceMusicPlayerServiceInitialize2(size_t pool_size, void* pool_addr) {
    LOG_ERROR(Lib_AudioOut, "(STUBBED) called");
    return ORBIS_OK;
}

static s32 PS4_SYSV_ABI sceMusicPlayerServiceInitialize3(size_t pool_size, void* pool_addr,
                                                         u64 unk1, u64 unk2) {
    LOG_INFO(Lib_AudioOut, "pool_size={:#x}, pool_addr={}", pool_size, pool_addr);
    if (pool_size < 0x200000 || !pool_addr) {
        LOG_ERROR(Lib_AudioOut, "invalid arguments: pool_size={:#x}, pool_addr={}", pool_size,
                  pool_addr);
        return static_cast<s32>(0x812f0004);
    }
    return ORBIS_OK;
}

static s32 PS4_SYSV_ABI sceMusicPlayerServiceTerminate() {
    LOG_INFO(Lib_AudioOut, "called");
    MusicPlayerManager::Instance().StopPlayback();
    return ORBIS_OK;
}

static s32 PS4_SYSV_ABI sceMusicPlayerServiceSetUsbStorageDeviceInfo(void* a, void* b) {
    LOG_ERROR(Lib_AudioOut, "(STUBBED) called");
    return ORBIS_OK;
}

static s32 PS4_SYSV_ABI sceMusicPlayerServiceRemoveAllData(s32 player_id) {
    LOG_INFO(Lib_AudioOut, "player_id={}", player_id);
    MusicPlayerManager::Instance().ClearTracks();
    return ORBIS_OK;
}

static s32 SetTrackListInternal(s32 player_id, const void* tracks, u32 count, u32 flags,
                                size_t track_entry_size) {
    if (!tracks || count == 0) {
        LOG_ERROR(Lib_AudioOut, "invalid arguments: tracks={}, count={}", tracks, count);
        return static_cast<s32>(0x812f0004);
    }

    const auto* base = reinterpret_cast<const u8*>(tracks);
    std::vector<std::string> track_uris;
    track_uris.reserve(count);

    LOG_INFO(Lib_AudioOut, "player_id={}, count={}, flags={}, stride={:#x}", player_id, count,
             flags, track_entry_size);

    for (u32 i = 0; i < count; i++) {
        const char* uri_str = reinterpret_cast<const char*>(base + i * track_entry_size);
        track_uris.emplace_back(uri_str);
        LOG_DEBUG(Lib_AudioOut, "  track[{}]: {}", i, uri_str);
    }

    MusicPlayerManager::Instance().SetTrackList(track_uris);
    return ORBIS_OK;
}

static s32 PS4_SYSV_ABI sceMusicPlayerServiceSetTrackList(s32 player_id, const void* tracks,
                                                          u32 count) {
    return SetTrackListInternal(player_id, tracks, count, 0, 0xc2f);
}

static s32 PS4_SYSV_ABI sceMusicPlayerServiceSetTrackList2(s32 player_id, const void* tracks,
                                                           u32 count, u32 flags) {
    return SetTrackListInternal(player_id, tracks, count, flags, 0xc48);
}

static s32 PS4_SYSV_ABI sceMusicPlayerServicePlay(s32 player_id, u32 track_index, s64 start_time_s,
                                                  s32 flags) {
    LOG_INFO(Lib_AudioOut, "player_id={}, track_index={}, start_time_s={}, flags={}", player_id,
             track_index, start_time_s, flags);
    auto& mgr = MusicPlayerManager::Instance();
    u64 start_time_ms = start_time_s > 0 ? static_cast<u64>(start_time_s * 1000) : 0;
    mgr.PlayOrSeek(track_index, start_time_ms);
    return ORBIS_OK;
}

static s32 PS4_SYSV_ABI sceMusicPlayerServicePlayStartByTime(s32 player_id, u32 track_index,
                                                             s64 start_time_s) {
    LOG_INFO(Lib_AudioOut, "player_id={}, track_index={}, start_time_s={}", player_id, track_index,
             start_time_s);
    auto& mgr = MusicPlayerManager::Instance();
    u64 start_time_ms = start_time_s > 0 ? static_cast<u64>(start_time_s * 1000) : 0;
    mgr.PlayOrSeek(track_index, start_time_ms);
    return ORBIS_OK;
}

static s32 PS4_SYSV_ABI sceMusicPlayerServiceStop() {
    LOG_INFO(Lib_AudioOut, "called");
    MusicPlayerManager::Instance().StopPlayback();
    return ORBIS_OK;
}

static s32 PS4_SYSV_ABI sceMusicPlayerServicePause() {
    LOG_INFO(Lib_AudioOut, "called");
    MusicPlayerManager::Instance().Pause();
    return ORBIS_OK;
}

static s32 PS4_SYSV_ABI sceMusicPlayerServiceUnpause() {
    LOG_INFO(Lib_AudioOut, "called");
    MusicPlayerManager::Instance().Unpause();
    return ORBIS_OK;
}

static s32 PS4_SYSV_ABI sceMusicPlayerServiceNextTrack() {
    LOG_INFO(Lib_AudioOut, "called");
    MusicPlayerManager::Instance().NextTrack();
    return ORBIS_OK;
}

static s32 PS4_SYSV_ABI sceMusicPlayerServicePreviousTrack() {
    LOG_INFO(Lib_AudioOut, "called");
    MusicPlayerManager::Instance().PreviousTrack();
    return ORBIS_OK;
}

static s32 PS4_SYSV_ABI sceMusicPlayerServiceSeek(s32 player_id, u32 track_index, s64 time_s) {
    LOG_INFO(Lib_AudioOut, "player_id={}, track_index={}, time_s={}", player_id, track_index,
             time_s);
    auto& mgr = MusicPlayerManager::Instance();
    u64 time_ms = time_s > 0 ? static_cast<u64>(time_s * 1000) : 0;
    mgr.SeekTrack(track_index, time_ms);
    return ORBIS_OK;
}

static s32 PS4_SYSV_ABI sceMusicPlayerServiceSetAudioVolume(s32 volume) {
    LOG_INFO(Lib_AudioOut, "volume={}", volume);
    if (volume < 0 || volume > 25) {
        LOG_ERROR(Lib_AudioOut, "invalid volume={}", volume);
        return static_cast<s32>(0x812f0004);
    }
    MusicPlayerManager::Instance().SetVolume(volume);
    return ORBIS_OK;
}

static s32 PS4_SYSV_ABI sceMusicPlayerServiceGetAudioVolume(s32* volume, s32* mute) {
    if (volume) {
        *volume = MusicPlayerManager::Instance().GetVolume();
    }
    if (mute) {
        *mute = 0;
    }
    return ORBIS_OK;
}

static s32 PS4_SYSV_ABI sceMusicPlayerServiceSetShuffle(u32 mode) {
    LOG_INFO(Lib_AudioOut, "mode={}", mode);
    if (mode > 1) {
        LOG_ERROR(Lib_AudioOut, "invalid mode={}", mode);
        return static_cast<s32>(0x812f0004);
    }
    MusicPlayerManager::Instance().SetShuffle(mode);
    return ORBIS_OK;
}

static s32 PS4_SYSV_ABI sceMusicPlayerServiceSetRepeatMode(u32 mode) {
    LOG_INFO(Lib_AudioOut, "mode={}", mode);
    if (mode > 2) {
        LOG_ERROR(Lib_AudioOut, "invalid mode={}", mode);
        return static_cast<s32>(0x812f0004);
    }
    MusicPlayerManager::Instance().SetRepeatMode(mode);
    return ORBIS_OK;
}

static s32 PS4_SYSV_ABI
sceMusicPlayerServiceGetPlayStatusExtension(OrbisMusicPlayerPlayStatusExtension* status) {
    if (!status || status->size != 0x48) {
        return static_cast<s32>(0x812f0004);
    }

    auto& mgr = MusicPlayerManager::Instance();
    const u32 track_count = mgr.GetTrackCount();
    const u32 cur_track = mgr.GetCurrentTrackIndex();
    const u32 rep_mode = mgr.GetRepeatMode();
    const u32 shuf_mode = mgr.GetShuffleMode();

    u32 play_state = 2; // 2: stopped
    if (mgr.GetPlayState() == 1) {
        play_state = 0; // 0: playing
    } else if (mgr.GetPlayState() == 2) {
        play_state = 1; // 1: paused
    }

    status->error_code = 0;
    status->play_state = play_state;
    status->total_duration_s = mgr.GetTotalDurationMs() / 1000;
    status->current_time_s = mgr.GetCurrentTimeMs() / 1000;
    status->tracklist_id = 0;
    status->track_index = cur_track;
    status->play_index = cur_track;
    status->total_track_count = track_count;
    status->volume = static_cast<u32>(mgr.GetVolume());
    status->repeat_mode = rep_mode;
    status->shuffle_mode = shuf_mode;
    const bool can_next =
        (track_count > 0) && ((cur_track + 1 < track_count) || (rep_mode != 0) || (shuf_mode != 0));
    const bool can_prev =
        (track_count > 0) && ((cur_track > 0) || (rep_mode != 0) || (shuf_mode != 0));

    // PS4 OrbisMusicPlayerPlayStatusExtension specification:
    // 0 = skip allowed, 1 = skip disabled (playlist boundary reached with repeat/shuffle off)
    status->can_skip_next = can_next ? 0 : 1;
    status->can_skip_previous = can_prev ? 0 : 1;
    status->is_loading = 0;
    return ORBIS_OK;
}

static s32 PS4_SYSV_ABI
sceMusicPlayerServiceGetCurrentPlayStatus(SceMusicPlayerServicePlayStatus* status) {
    if (!status) {
        return static_cast<s32>(0x812f0004);
    }
    auto& mgr = MusicPlayerManager::Instance();

    u32 play_status = 2; // 2: stopped
    if (mgr.GetPlayState() == 1) {
        play_status = 0; // 0: playing
    } else if (mgr.GetPlayState() == 2) {
        play_status = 1; // 1: paused
    }

    status->result = 0;
    status->play_status = play_status;
    status->track_index = mgr.GetCurrentTrackIndex();
    status->tracklist_id = 0;
    status->playback_time = mgr.GetCurrentTimeMs() / 1000;
    status->shuffle = mgr.GetShuffleMode();
    status->repeat = mgr.GetRepeatMode();
    return ORBIS_OK;
}

static s32 PS4_SYSV_ABI sceMusicPlayerServiceReceiveEvent(void* buffer) {
    if (!buffer) {
        LOG_ERROR(Lib_AudioOut, "invalid buffer");
        return static_cast<s32>(0x812f0004);
    }
    u8* buf = static_cast<u8*>(buffer);
    auto& mgr = MusicPlayerManager::Instance();
    MusicPlayerManager::MusicEvent ev{};
    if (mgr.PopEvent(ev, 50)) {
        LOG_DEBUG(Lib_AudioOut, "delivering event_id={}, track_idx={}", ev.event_id,
                  ev.track_index);
        *reinterpret_cast<u32*>(buf + 0x08) = 1;
        *reinterpret_cast<u32*>(buf + 0x20) = 1;
        *reinterpret_cast<u32*>(buf + 0x24) = ev.event_id;
        *reinterpret_cast<u32*>(buf + 0x28) = ev.track_index;
        return ORBIS_OK;
    }
    return static_cast<s32>(0x812f010a);
}

static s32 PS4_SYSV_ABI sceMusicPlayerServiceGetPlaybackResults(void* a, void* b) {
    LOG_ERROR(Lib_AudioOut, "(STUBBED) called");
    return ORBIS_OK;
}

static s32 PS4_SYSV_ABI sceMusicPlayerServiceGetTrackInfo(void* a, void* b) {
    LOG_ERROR(Lib_AudioOut, "(STUBBED) called");
    return ORBIS_OK;
}

static s32 PS4_SYSV_ABI sceMusicPlayerServiceBeginTransaction(s32 a) {
    LOG_INFO(Lib_AudioOut, "called: a={}", a);
    return MusicPlayerManager::Instance().BeginTransaction();
}

static s32 PS4_SYSV_ABI sceMusicPlayerServiceEndTransaction(s32 a) {
    LOG_INFO(Lib_AudioOut, "called: a={}", a);
    return MusicPlayerManager::Instance().EndTransaction();
}

static s32 PS4_SYSV_ABI sceMusicPlayerServiceCreateTrackList(u32 tracklist_id) {
    LOG_INFO(Lib_AudioOut, "called: tracklist_id={}", tracklist_id);
    return MusicPlayerManager::Instance().CreateTrackList(tracklist_id);
}

static s32 PS4_SYSV_ABI sceMusicPlayerServiceRemoveTrackList(u32 tracklist_id) {
    LOG_INFO(Lib_AudioOut, "called: tracklist_id={}", tracklist_id);
    return MusicPlayerManager::Instance().RemoveTrackList(tracklist_id);
}

static s32 PS4_SYSV_ABI sceMusicPlayerServiceInsertTrackEntry(u32 tracklist_id, u32 track_index,
                                                              const void* track_entry) {
    if (!track_entry) {
        LOG_ERROR(Lib_AudioOut, "InsertTrackEntry: null track_entry");
        return static_cast<s32>(0x812f0004);
    }
    const char* uri_str = reinterpret_cast<const char*>(track_entry);
    LOG_INFO(Lib_AudioOut, "called: tracklist_id={}, track_index={}, uri={}", tracklist_id,
             track_index, uri_str);
    return MusicPlayerManager::Instance().InsertTrackEntry(tracklist_id, track_index, uri_str);
}

static s32 PS4_SYSV_ABI sceMusicPlayerServiceRemoveTrackEntry(u32 tracklist_id, u32 track_index) {
    LOG_INFO(Lib_AudioOut, "called: tracklist_id={}, track_index={}", tracklist_id, track_index);
    return MusicPlayerManager::Instance().RemoveTrackEntry(tracklist_id, track_index);
}

static s32 PS4_SYSV_ABI sceMusicPlayerServiceGetTrackListVersion(s32 player_id, u64* version) {
    if (!version) {
        LOG_ERROR(Lib_AudioOut, "GetTrackListVersion: null version pointer");
        return static_cast<s32>(0x812f0004);
    }
    *version = MusicPlayerManager::Instance().GetTrackListVersion();
    return ORBIS_OK;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("WypjBw-l+pE", "libSceMusicPlayerService", 1, "libSceMusicPlayerService",
                 sceMusicPlayerServiceInitialize);
    LIB_FUNCTION("xT55MQDY9lg", "libSceMusicPlayerService", 1, "libSceMusicPlayerService",
                 sceMusicPlayerServiceInitialize2);
    LIB_FUNCTION("sY8beqtqGv0", "libSceMusicPlayerService", 1, "libSceMusicPlayerService",
                 sceMusicPlayerServiceInitialize3);
    LIB_FUNCTION("uZXjTVsS-hU", "libSceMusicPlayerService", 1, "libSceMusicPlayerService",
                 sceMusicPlayerServiceTerminate);
    LIB_FUNCTION("JlbWOi1Webo", "libSceMusicPlayerService", 1, "libSceMusicPlayerService",
                 sceMusicPlayerServiceSetUsbStorageDeviceInfo);
    LIB_FUNCTION("-JE2KLnsg9I", "libSceMusicPlayerService", 1, "libSceMusicPlayerService",
                 sceMusicPlayerServiceRemoveAllData);
    LIB_FUNCTION("4cuKd4S83xw", "libSceMusicPlayerService", 1, "libSceMusicPlayerService",
                 sceMusicPlayerServiceSetTrackList);
    LIB_FUNCTION("iawJpYIQM7s", "libSceMusicPlayerService", 1, "libSceMusicPlayerService",
                 sceMusicPlayerServiceSetTrackList2);
    LIB_FUNCTION("KblnYfZ5AGM", "libSceMusicPlayerService", 1, "libSceMusicPlayerService",
                 sceMusicPlayerServicePlay);
    LIB_FUNCTION("kMb+qpZTQ18", "libSceMusicPlayerService", 1, "libSceMusicPlayerService",
                 sceMusicPlayerServicePlayStartByTime);
    LIB_FUNCTION("7Ayp3diNYnk", "libSceMusicPlayerService", 1, "libSceMusicPlayerService",
                 sceMusicPlayerServiceStop);
    LIB_FUNCTION("fIii6-0Adxc", "libSceMusicPlayerService", 1, "libSceMusicPlayerService",
                 sceMusicPlayerServicePause);
    LIB_FUNCTION("DLXiDjC1D1A", "libSceMusicPlayerService", 1, "libSceMusicPlayerService",
                 sceMusicPlayerServiceUnpause);
    LIB_FUNCTION("M76dl-GnCs8", "libSceMusicPlayerService", 1, "libSceMusicPlayerService",
                 sceMusicPlayerServiceNextTrack);
    LIB_FUNCTION("NuJT-nmTqrI", "libSceMusicPlayerService", 1, "libSceMusicPlayerService",
                 sceMusicPlayerServicePreviousTrack);
    LIB_FUNCTION("5LhFvqzrcug", "libSceMusicPlayerService", 1, "libSceMusicPlayerService",
                 sceMusicPlayerServiceSeek);
    LIB_FUNCTION("tMgpmzMA4Zc", "libSceMusicPlayerService", 1, "libSceMusicPlayerService",
                 sceMusicPlayerServiceSetAudioVolume);
    LIB_FUNCTION("3KChwrxsVPg", "libSceMusicPlayerService", 1, "libSceMusicPlayerService",
                 sceMusicPlayerServiceGetAudioVolume);
    LIB_FUNCTION("DMvAm4HFKg0", "libSceMusicPlayerService", 1, "libSceMusicPlayerService",
                 sceMusicPlayerServiceSetShuffle);
    LIB_FUNCTION("+uCwAp6KYtQ", "libSceMusicPlayerService", 1, "libSceMusicPlayerService",
                 sceMusicPlayerServiceSetRepeatMode);
    LIB_FUNCTION("TVe6T-UKcoc", "libSceMusicPlayerService", 1, "libSceMusicPlayerService",
                 sceMusicPlayerServiceGetPlayStatusExtension);
    LIB_FUNCTION("CNsRsR5a+qc", "libSceMusicPlayerService", 1, "libSceMusicPlayerService",
                 sceMusicPlayerServiceGetCurrentPlayStatus);
    LIB_FUNCTION("Py0XtCoi5IU", "libSceMusicPlayerService", 1, "libSceMusicPlayerService",
                 sceMusicPlayerServiceReceiveEvent);
    LIB_FUNCTION("1+KpQYgv1zk", "libSceMusicPlayerService", 1, "libSceMusicPlayerService",
                 sceMusicPlayerServiceGetPlaybackResults);
    LIB_FUNCTION("wHKiDjNLQwI", "libSceMusicPlayerService", 1, "libSceMusicPlayerService",
                 sceMusicPlayerServiceGetTrackInfo);
    LIB_FUNCTION("AbiIaBA50I0", "libSceMusicPlayerService", 1, "libSceMusicPlayerService",
                 sceMusicPlayerServiceBeginTransaction);
    LIB_FUNCTION("7SOVShNUDXo", "libSceMusicPlayerService", 1, "libSceMusicPlayerService",
                 sceMusicPlayerServiceEndTransaction);
    LIB_FUNCTION("mW3EHwAVHPA", "libSceMusicPlayerService", 1, "libSceMusicPlayerService",
                 sceMusicPlayerServiceCreateTrackList);
    LIB_FUNCTION("EF1lpApBn4s", "libSceMusicPlayerService", 1, "libSceMusicPlayerService",
                 sceMusicPlayerServiceRemoveTrackList);
    LIB_FUNCTION("R1SLegxzCGU", "libSceMusicPlayerService", 1, "libSceMusicPlayerService",
                 sceMusicPlayerServiceInsertTrackEntry);
    LIB_FUNCTION("QEIJRZErmxQ", "libSceMusicPlayerService", 1, "libSceMusicPlayerService",
                 sceMusicPlayerServiceRemoveTrackEntry);
    LIB_FUNCTION("qYbmAxRGuq8", "libSceMusicPlayerService", 1, "libSceMusicPlayerService",
                 sceMusicPlayerServiceGetTrackListVersion);
}

bool IsBgmPlaying() {
    return MusicPlayerManager::Instance().GetPlayState() != 0;
}

} // namespace Libraries::MusicPlayerService
