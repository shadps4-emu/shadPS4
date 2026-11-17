// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <array>
#include <mutex>

#include "common/types.h"

namespace Libraries::AudioOut {

class MasteringProcessor {
public:
    static constexpr u32 MaxChannels = 8;
    static constexpr u32 MaxEqBands = 4;
    static constexpr u32 MaxCompressorBands = 3;
    static constexpr double SampleRate = 48000.0;

    /// Measurements of the most recently processed buffer.
    struct Meters {
        bool valid = false;
        float input_rms[MaxCompressorBands][MaxChannels]{};         // linear
        float compression_coeff[MaxCompressorBands][MaxChannels]{}; // linear gain
        float input_peak[MaxChannels]{};                            // absolute linear
        float output_peak[MaxChannels]{};                           // absolute linear
        float gain_peak[MaxChannels]{};                             // linear gain
    };

    void SetParams(const void* params, u32 params_id, bool swap_eq_compressor);
    void Disable();
    bool IsActive() const {
        return active;
    }
    void Process(void* buffer, u32 frames, u32 num_channels, bool is_float, bool is_std,
                 float pre_gain);

    Meters GetMeters() const;

private:
    struct Biquad {
        double b0 = 1.0, b1 = 0.0, b2 = 0.0, a1 = 0.0, a2 = 0.0;
    };
    struct BiquadState {
        double z1 = 0.0, z2 = 0.0;
    };
    /// 4th order Linkwitz-Riley filter: two identical 2nd order sections.
    struct Lr4State {
        BiquadState s[2];
    };

    struct CompressorBand {
        bool bypass = false;
        double threshold_db = 0.0;
        double slope = 0.0;          // 1/ratio - 1, <= 0
        double knee_db = 0.0;        // knee width, 0 = hard knee
        double knee_start_pow = 0.0; // mean square below which no reduction happens
        double attack_coeff = 0.0;
        double release_coeff = 0.0;
        double makeup = 1.0; // linear
    };

    static Biquad DesignEqBand(u32 mode, double frequency, double q, double gain_db);
    static Biquad DesignButterworth(bool highpass, double frequency);
    static double Tick(const Biquad& c, BiquadState& s, double x);
    static double TickLr4(const Biquad& c, Lr4State& s, double x);

    void ProcessEq(double* x, u32 num_channels);
    void ProcessCompressor(double* x, u32 num_channels);
    void FlushDenormals();

    bool active = false;
    bool eq_on = false;
    bool compressor_on = false;
    bool volume_on = false;
    bool limiter_on = false;
    bool swap_eq_compressor = false;

    // Parametric EQ
    u32 eq_num_bands = 0;
    std::array<bool, MaxEqBands> eq_band_on{};
    std::array<Biquad, MaxEqBands> eq_coeffs{};
    BiquadState eq_state[MaxEqBands][MaxChannels]{};

    // Multiband compressor
    u32 comp_num_bands = 1;
    u32 comp_link_mode = 0;
    double comp_link_strength = 0.0; // 0..1
    bool comp_link_stereo_pairs = false;
    std::array<CompressorBand, MaxCompressorBands> comp_bands{};
    Biquad xover_lp[2], xover_hp[2];
    Lr4State xo1_lp[MaxChannels]{}, xo1_hp[MaxChannels]{};
    Lr4State xo2_lp[MaxChannels]{}, xo2_hp[MaxChannels]{};
    Lr4State xo2_ap_lp[MaxChannels]{}, xo2_ap_hp[MaxChannels]{};
    double comp_mean_square[MaxCompressorBands][MaxChannels]{};
    double comp_envelope_db[MaxCompressorBands][MaxChannels]{}; // smoothed gain reduction
    double rms_coeff = 0.0;

    // Master volume, linear, indexed by ORBIS_AUDIO_OUT_CHANNEL_*
    std::array<double, MaxChannels> volume_gain{};

    // Limiter
    bool limiter_hard = false;
    double limiter_threshold = 1.0; // linear
    double limiter_attack_coeff = 0.0;
    double limiter_release_coeff = 0.0;
    std::array<double, MaxChannels> limiter_gain{};

    mutable std::mutex meter_mutex;
    Meters meters;
};

} // namespace Libraries::AudioOut
