// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <cmath>
#include <numbers>

#include "core/libraries/audio/audioout.h"
#include "core/libraries/audio/audioout_mastering.h"

namespace Libraries::AudioOut {

namespace {

constexpr double Pi = std::numbers::pi;
constexpr double Ln10Over20 = std::numbers::ln10 / 20.0;
constexpr double MaxFilterFrequency = MasteringProcessor::SampleRate * 0.49;
constexpr double RmsWindowSeconds = 0.010;

double DbToLinear(double db) {
    return std::exp(db * Ln10Over20);
}

double TimeCoeff(double seconds) {
    if (!(seconds > 0.0)) {
        return 0.0;
    }
    return std::exp(-1.0 / (seconds * MasteringProcessor::SampleRate));
}

double ClampFinite(double value, double lo, double hi, double fallback) {
    if (!std::isfinite(value)) {
        return fallback;
    }
    return std::clamp(value, lo, hi);
}

} // namespace

double MasteringProcessor::Tick(const Biquad& c, BiquadState& s, double x) {
    // Transposed direct form II
    const double y = c.b0 * x + s.z1;
    s.z1 = c.b1 * x - c.a1 * y + s.z2;
    s.z2 = c.b2 * x - c.a2 * y;
    return y;
}

double MasteringProcessor::TickLr4(const Biquad& c, Lr4State& s, double x) {
    return Tick(c, s.s[1], Tick(c, s.s[0], x));
}

MasteringProcessor::Biquad MasteringProcessor::DesignEqBand(u32 mode, double frequency, double q,
                                                            double gain_db) {
    using Mode = OrbisAudioOutMasteringFilterMode;

    frequency = ClampFinite(frequency, 10.0, MaxFilterFrequency, 1000.0);
    q = ClampFinite(q, 0.1, 12.0, 0.7071);
    gain_db = ClampFinite(gain_db, -20.0, 20.0, 0.0);

    const double w0 = 2.0 * Pi * frequency / SampleRate;
    const double cs = std::cos(w0);
    const double sn = std::sin(w0);
    const double alpha = sn / (2.0 * q);

    double b0 = 1.0, b1 = 0.0, b2 = 0.0, a0 = 1.0, a1 = 0.0, a2 = 0.0;

    switch (static_cast<Mode>(mode)) {
    case Mode::LowpassResonant:
        // Gain at the cut-off frequency equals Q: -3 dB at 0.7, 0 dB at 1.0, +6 dB at 2.0.
        b0 = (1.0 - cs) / 2.0;
        b1 = 1.0 - cs;
        b2 = (1.0 - cs) / 2.0;
        a0 = 1.0 + alpha;
        a1 = -2.0 * cs;
        a2 = 1.0 - alpha;
        break;
    case Mode::HighpassResonant:
        b0 = (1.0 + cs) / 2.0;
        b1 = -(1.0 + cs);
        b2 = (1.0 + cs) / 2.0;
        a0 = 1.0 + alpha;
        a1 = -2.0 * cs;
        a2 = 1.0 - alpha;
        break;
    case Mode::Peak: {
        const double a = std::exp(gain_db * Ln10Over20 / 2.0); // 10^(dB/40)
        b0 = 1.0 + alpha * a;
        b1 = -2.0 * cs;
        b2 = 1.0 - alpha * a;
        a0 = 1.0 + alpha / a;
        a1 = -2.0 * cs;
        a2 = 1.0 - alpha / a;
        break;
    }
    case Mode::HighShelf:
    case Mode::LowShelf: {
        // Shelf slope 1: half of the gain (in dB) is reached at the cut-off frequency.
        const double a = std::exp(gain_db * Ln10Over20 / 2.0);
        const double beta = 2.0 * std::sqrt(a) * (sn / 2.0 * std::numbers::sqrt2);
        if (static_cast<Mode>(mode) == Mode::LowShelf) {
            b0 = a * ((a + 1.0) - (a - 1.0) * cs + beta);
            b1 = 2.0 * a * ((a - 1.0) - (a + 1.0) * cs);
            b2 = a * ((a + 1.0) - (a - 1.0) * cs - beta);
            a0 = (a + 1.0) + (a - 1.0) * cs + beta;
            a1 = -2.0 * ((a - 1.0) + (a + 1.0) * cs);
            a2 = (a + 1.0) + (a - 1.0) * cs - beta;
        } else {
            b0 = a * ((a + 1.0) + (a - 1.0) * cs + beta);
            b1 = -2.0 * a * ((a - 1.0) + (a + 1.0) * cs);
            b2 = a * ((a + 1.0) + (a - 1.0) * cs - beta);
            a0 = (a + 1.0) - (a - 1.0) * cs + beta;
            a1 = 2.0 * ((a - 1.0) - (a + 1.0) * cs);
            a2 = (a + 1.0) - (a - 1.0) * cs - beta;
        }
        break;
    }
    case Mode::LowpassOnePole:
    case Mode::HighpassOnePole: {
        // First order: -3 dB at the cut-off frequency, 6 dB per octave.
        const double k = std::tan(w0 / 2.0);
        a0 = 1.0 + k;
        a1 = k - 1.0;
        a2 = 0.0;
        b2 = 0.0;
        if (static_cast<Mode>(mode) == Mode::LowpassOnePole) {
            b0 = k;
            b1 = k;
        } else {
            b0 = 1.0;
            b1 = -1.0;
        }
        break;
    }
    case Mode::Off:
    default:
        return Biquad{};
    }

    return Biquad{b0 / a0, b1 / a0, b2 / a0, a1 / a0, a2 / a0};
}

MasteringProcessor::Biquad MasteringProcessor::DesignButterworth(bool highpass, double frequency) {
    return DesignEqBand(static_cast<u32>(highpass
                                             ? OrbisAudioOutMasteringFilterMode::HighpassResonant
                                             : OrbisAudioOutMasteringFilterMode::LowpassResonant),
                        frequency, std::numbers::sqrt2 / 2.0, 0.0);
}

void MasteringProcessor::Disable() {
    active = false;
    eq_on = compressor_on = volume_on = limiter_on = false;
    std::scoped_lock lock{meter_mutex};
    meters = {};
}

void MasteringProcessor::SetParams(const void* params, u32 params_id, bool swap) {
    const auto& common = *static_cast<const OrbisAudioOutMasteringParams*>(params);
    const bool is_v2 = params_id == ORBIS_AUDIO_OUT_MASTERING_PARAMS_ID_V2;
    const auto& v2 = *static_cast<const OrbisAudioOutMasteringParamsV2*>(params);
    const OrbisAudioOutMasteringLimiterParams& limiter =
        is_v2 ? v2.limiterParams : common.limiterParams;

    const u32 bypass = common.moduleBypassFlags;
    swap_eq_compressor = swap;

    // --- Parametric EQ
    const auto& eq = common.paramEqParams;
    eq_num_bands = std::min<u32>(eq.numBands, MaxEqBands);
    bool any_eq_band = false;
    for (u32 band = 0; band < MaxEqBands; band++) {
        const auto& filter = eq.filterParams[band];
        const u32 mode = static_cast<u32>(filter.filterMode);
        eq_band_on[band] =
            band < eq_num_bands &&
            mode != static_cast<u32>(OrbisAudioOutMasteringFilterMode::Off) &&
            mode <= static_cast<u32>(OrbisAudioOutMasteringFilterMode::HighpassOnePole);
        eq_coeffs[band] = eq_band_on[band]
                              ? DesignEqBand(mode, filter.frequency, filter.resonance, filter.gain)
                              : Biquad{};
        any_eq_band |= eq_band_on[band];
    }
    eq_on = (bypass & ORBIS_AUDIO_OUT_MASTERING_BYPASSED_PARAM_EQ) == 0 && any_eq_band;

    // --- Multiband compressor
    const auto& comp = common.compressorParams;
    comp_num_bands = std::clamp<u32>(comp.numBands, 1, MaxCompressorBands);
    comp_link_mode = static_cast<u32>(comp.linkMode);
    comp_link_strength = ClampFinite(comp.linkStrength, 0.0, 100.0, 0.0) / 100.0;
    comp_link_stereo_pairs = comp.linkStereoPairs != 0;
    rms_coeff = TimeCoeff(RmsWindowSeconds);

    double f0 = ClampFinite(comp.crossoverFrequencies[0], 10.0, MaxFilterFrequency, 200.0);
    double f1 = ClampFinite(comp.crossoverFrequencies[1], 10.0, MaxFilterFrequency, 2000.0);
    f1 = std::max(f1, f0);
    xover_lp[0] = DesignButterworth(false, f0);
    xover_hp[0] = DesignButterworth(true, f0);
    xover_lp[1] = DesignButterworth(false, f1);
    xover_hp[1] = DesignButterworth(true, f1);

    bool any_comp_band = false;
    for (u32 band = 0; band < MaxCompressorBands; band++) {
        const auto& in = comp.bandParams[band];
        auto& out = comp_bands[band];
        out.bypass = band >= comp_num_bands || (comp.bandsBypassFlags & (1u << band)) != 0;
        out.threshold_db = ClampFinite(in.threshold, -60.0, 0.0, 0.0);
        const double ratio = ClampFinite(in.ratio, 1.0, 30.0, 1.0);
        out.slope = 1.0 / ratio - 1.0;
        out.knee_db = ClampFinite(in.knee, 0.0, 20.0, 0.0);
        out.knee_start_pow = std::exp((out.threshold_db - out.knee_db / 2.0) * Ln10Over20 * 2.0);
        out.attack_coeff = TimeCoeff(ClampFinite(in.attack, 0.0, 0.5, 0.0));
        out.release_coeff = TimeCoeff(ClampFinite(in.release, 0.0, 5.0, 0.0));
        out.makeup = DbToLinear(ClampFinite(in.makeupGain, -18.0, 18.0, 0.0));
        any_comp_band |= !out.bypass;
    }
    compressor_on = (bypass & ORBIS_AUDIO_OUT_MASTERING_BYPASSED_COMPRESSOR) == 0 && any_comp_band;

    for (u32 ch = 0; ch < MaxChannels; ch++) {
        const float db =
            is_v2 ? v2.masterVolumeParams.volume[ch] : common.masterVolumeParams.volume;
        volume_gain[ch] = DbToLinear(ClampFinite(db, -100.0, 12.0, 0.0));
    }
    volume_on = (bypass & ORBIS_AUDIO_OUT_MASTERING_BYPASSED_MASTER_VOLUME) == 0;

    limiter_hard = limiter.mode == OrbisAudioOutMasteringLimiterMode::Hard;
    limiter_threshold = DbToLinear(ClampFinite(limiter.threshold, -30.0, 0.0, 0.0));
    limiter_attack_coeff = TimeCoeff(ClampFinite(limiter.attack, 0.0, 2.6, 0.0));
    limiter_release_coeff = TimeCoeff(ClampFinite(limiter.release, 0.0, 5.0, 0.0));
    limiter_on = (bypass & ORBIS_AUDIO_OUT_MASTERING_BYPASSED_LIMITER) == 0;

    const bool was_active = active;
    active = eq_on || compressor_on || volume_on || limiter_on;
    if (active && !was_active) {
        for (auto& band : eq_state) {
            std::fill(std::begin(band), std::end(band), BiquadState{});
        }
        for (u32 ch = 0; ch < MaxChannels; ch++) {
            xo1_lp[ch] = xo1_hp[ch] = xo2_lp[ch] = xo2_hp[ch] = xo2_ap_lp[ch] = xo2_ap_hp[ch] = {};
            limiter_gain[ch] = 1.0;
            for (u32 band = 0; band < MaxCompressorBands; band++) {
                comp_mean_square[band][ch] = 0.0;
                comp_envelope_db[band][ch] = 0.0;
            }
        }
    }
    if (!active) {
        std::scoped_lock lock{meter_mutex};
        meters = {};
    }
}

void MasteringProcessor::ProcessEq(double* x, u32 num_channels) {
    for (u32 band = 0; band < eq_num_bands; band++) {
        if (!eq_band_on[band]) {
            continue;
        }
        for (u32 ch = 0; ch < num_channels; ch++) {
            x[ch] = Tick(eq_coeffs[band], eq_state[band][ch], x[ch]);
        }
    }
}

void MasteringProcessor::ProcessCompressor(double* x, u32 num_channels) {
    double bands[MaxCompressorBands][MaxChannels];
    for (u32 ch = 0; ch < num_channels; ch++) {
        if (comp_num_bands == 1) {
            bands[0][ch] = x[ch];
        } else {
            const double low = TickLr4(xover_lp[0], xo1_lp[ch], x[ch]);
            const double high = TickLr4(xover_hp[0], xo1_hp[ch], x[ch]);
            if (comp_num_bands == 2) {
                bands[0][ch] = low;
                bands[1][ch] = high;
            } else {
                bands[0][ch] = TickLr4(xover_lp[1], xo2_ap_lp[ch], low) +
                               TickLr4(xover_hp[1], xo2_ap_hp[ch], low);
                bands[1][ch] = TickLr4(xover_lp[1], xo2_lp[ch], high);
                bands[2][ch] = TickLr4(xover_hp[1], xo2_hp[ch], high);
            }
        }
    }

    for (u32 band = 0; band < comp_num_bands; band++) {
        const CompressorBand& cfg = comp_bands[band];
        double target_db[MaxChannels];

        for (u32 ch = 0; ch < num_channels; ch++) {
            const double sample = bands[band][ch];
            double& mean_square = comp_mean_square[band][ch];
            mean_square = rms_coeff * mean_square + (1.0 - rms_coeff) * sample * sample;

            double reduction = 0.0;
            if (!cfg.bypass && mean_square > cfg.knee_start_pow) {
                const double over = 10.0 * std::log10(mean_square) - cfg.threshold_db;
                if (cfg.knee_db > 0.0 && 2.0 * over < cfg.knee_db) {
                    const double t = over + cfg.knee_db / 2.0;
                    reduction = cfg.slope * t * t / (2.0 * cfg.knee_db);
                } else {
                    reduction = cfg.slope * over;
                }
            }
            target_db[ch] = reduction;
        }

        if (!cfg.bypass && num_channels > 1) {
            u32 lead = 0;
            for (u32 ch = 1; ch < num_channels; ch++) {
                if (target_db[ch] < target_db[lead]) {
                    lead = ch;
                }
            }
            const double lead_db = target_db[lead];

            using Link = OrbisAudioOutMasteringCompressorLinkMode;
            if (comp_link_mode == static_cast<u32>(Link::AllChannels)) {
                std::fill(target_db, target_db + num_channels, lead_db);
            } else if (comp_link_mode == static_cast<u32>(Link::PartialLink)) {
                for (u32 ch = 0; ch < num_channels; ch++) {
                    target_db[ch] += (lead_db - target_db[ch]) * comp_link_strength;
                }
            }

            if (comp_link_stereo_pairs) {
                const bool paired = lead < 2 || lead >= 4;
                const u32 partner = lead ^ 1;
                if (paired && partner < num_channels) {
                    target_db[partner] = lead_db;
                }
            }
        }

        for (u32 ch = 0; ch < num_channels; ch++) {
            if (cfg.bypass) {
                continue;
            }
            double& envelope = comp_envelope_db[band][ch];
            const double coeff = target_db[ch] < envelope ? cfg.attack_coeff : cfg.release_coeff;
            envelope = coeff * envelope + (1.0 - coeff) * target_db[ch];
            const double gain = envelope < -1.0e-6 ? std::exp(envelope * Ln10Over20) : 1.0;
            bands[band][ch] *= gain * cfg.makeup;
        }
    }

    for (u32 ch = 0; ch < num_channels; ch++) {
        double sum = bands[0][ch];
        for (u32 band = 1; band < comp_num_bands; band++) {
            sum += bands[band][ch];
        }
        x[ch] = sum;
    }
}

void MasteringProcessor::FlushDenormals() {
    constexpr double Tiny = 1.0e-30;
    const auto flush = [](double& v) {
        if (std::abs(v) < Tiny) {
            v = 0.0;
        }
    };
    const auto flush_lr4 = [&](Lr4State& s) {
        flush(s.s[0].z1);
        flush(s.s[0].z2);
        flush(s.s[1].z1);
        flush(s.s[1].z2);
    };
    for (u32 ch = 0; ch < MaxChannels; ch++) {
        for (u32 band = 0; band < MaxEqBands; band++) {
            flush(eq_state[band][ch].z1);
            flush(eq_state[band][ch].z2);
        }
        flush_lr4(xo1_lp[ch]);
        flush_lr4(xo1_hp[ch]);
        flush_lr4(xo2_lp[ch]);
        flush_lr4(xo2_hp[ch]);
        flush_lr4(xo2_ap_lp[ch]);
        flush_lr4(xo2_ap_hp[ch]);
        for (u32 band = 0; band < MaxCompressorBands; band++) {
            flush(comp_mean_square[band][ch]);
            flush(comp_envelope_db[band][ch]);
        }
    }
}

void MasteringProcessor::Process(void* buffer, u32 frames, u32 num_channels, bool is_float,
                                 bool is_std, float pre_gain) {
    if (!active || buffer == nullptr || frames == 0 || num_channels == 0 ||
        num_channels > MaxChannels || !(pre_gain > 0.0f)) {
        return;
    }

    constexpr double S16Scale = 32768.0;
    auto* samples_f32 = static_cast<float*>(buffer);
    auto* samples_s16 = static_cast<s16*>(buffer);
    const double in_gain = pre_gain;
    const double out_gain = 1.0 / pre_gain;

    double position_volume[MaxChannels];
    for (u32 pos = 0; pos < num_channels; pos++) {
        const u32 channel = (is_std && pos >= ORBIS_AUDIO_OUT_CHANNEL_LS) ? (pos ^ 2) : pos;
        position_volume[pos] = volume_gain[channel];
    }

    double input_peak[MaxChannels]{};
    double output_peak[MaxChannels]{};
    double gain_peak[MaxChannels];
    std::fill(std::begin(gain_peak), std::end(gain_peak), 1.0);

    double x[MaxChannels];
    for (u32 frame = 0; frame < frames; frame++) {
        const u32 offset = frame * num_channels;
        for (u32 ch = 0; ch < num_channels; ch++) {
            const double sample =
                is_float ? samples_f32[offset + ch] : samples_s16[offset + ch] / S16Scale;
            x[ch] = sample * in_gain;
        }

        if (swap_eq_compressor) {
            if (compressor_on) {
                ProcessCompressor(x, num_channels);
            }
            if (eq_on) {
                ProcessEq(x, num_channels);
            }
        } else {
            if (eq_on) {
                ProcessEq(x, num_channels);
            }
            if (compressor_on) {
                ProcessCompressor(x, num_channels);
            }
        }

        if (volume_on) {
            for (u32 ch = 0; ch < num_channels; ch++) {
                x[ch] *= position_volume[ch];
            }
        }

        if (limiter_on) {
            for (u32 ch = 0; ch < num_channels; ch++) {
                const double level = std::abs(x[ch]);
                input_peak[ch] = std::max(input_peak[ch], level);
                double out;
                if (limiter_hard) {
                    out = std::clamp(x[ch], -limiter_threshold, limiter_threshold);
                } else {
                    const double target =
                        level > limiter_threshold ? limiter_threshold / level : 1.0;
                    double& gain = limiter_gain[ch];
                    const double coeff =
                        target < gain ? limiter_attack_coeff : limiter_release_coeff;
                    gain = coeff * gain + (1.0 - coeff) * target;
                    // Whatever still overshoots is cut off at full scale.
                    out = std::clamp(x[ch] * gain, -1.0, 1.0);
                }
                if (level > 0.0) {
                    gain_peak[ch] = std::min(gain_peak[ch], std::abs(out) / level);
                }
                output_peak[ch] = std::max(output_peak[ch], std::abs(out));
                x[ch] = out;
            }
        }

        for (u32 ch = 0; ch < num_channels; ch++) {
            const double sample = x[ch] * out_gain;
            if (is_float) {
                samples_f32[offset + ch] = static_cast<float>(sample);
            } else {
                samples_s16[offset + ch] =
                    static_cast<s16>(std::clamp(std::round(sample * S16Scale), -32768.0, 32767.0));
            }
        }
    }

    FlushDenormals();

    std::scoped_lock lock{meter_mutex};
    meters = {};
    meters.valid = true;
    for (u32 band = 0; band < MaxCompressorBands; band++) {
        for (u32 ch = 0; ch < MaxChannels; ch++) {
            const bool used = compressor_on && band < comp_num_bands && ch < num_channels;
            meters.input_rms[band][ch] =
                used ? static_cast<float>(std::sqrt(comp_mean_square[band][ch])) : 0.0f;
            meters.compression_coeff[band][ch] =
                used ? static_cast<float>(std::exp(comp_envelope_db[band][ch] * Ln10Over20)) : 1.0f;
        }
    }
    for (u32 ch = 0; ch < MaxChannels; ch++) {
        const bool used = limiter_on && ch < num_channels;
        meters.input_peak[ch] = used ? static_cast<float>(input_peak[ch]) : 0.0f;
        meters.output_peak[ch] = used ? static_cast<float>(output_peak[ch]) : 0.0f;
        meters.gain_peak[ch] = used ? static_cast<float>(gain_peak[ch]) : 1.0f;
    }
}

MasteringProcessor::Meters MasteringProcessor::GetMeters() const {
    std::scoped_lock lock{meter_mutex};
    return meters;
}

} // namespace Libraries::AudioOut
