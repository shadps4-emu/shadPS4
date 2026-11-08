// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <array>
#include <cstring>
#include <mutex>

#include "common/logging/log.h"
#include "core/libraries/libs.h"
#include "core/libraries/piglet/piglet.h"

namespace Libraries::Piglet {

namespace {

// EGL 1.4 enumerants used by libScePigletv2VSH.
constexpr u32 EGL_FALSE = 0;
constexpr u32 EGL_TRUE = 1;
constexpr s32 EGL_DONT_CARE = -1;
constexpr s32 EGL_NONE = 0x3038;

constexpr u32 EGL_SUCCESS = 0x3000;
constexpr u32 EGL_NOT_INITIALIZED = 0x3001;
constexpr u32 EGL_BAD_ATTRIBUTE = 0x3004;
constexpr u32 EGL_BAD_DISPLAY = 0x3008;
constexpr u32 EGL_BAD_PARAMETER = 0x300C;

constexpr s32 EGL_VENDOR = 0x3053;
constexpr s32 EGL_VERSION = 0x3054;
constexpr s32 EGL_EXTENSIONS = 0x3055;
constexpr s32 EGL_CLIENT_APIS = 0x308D;

constexpr u32 EGL_OPENGL_ES_API = 0x30A0;

constexpr s32 EGL_CONFIG_CAVEAT = 0x3027;
constexpr s32 EGL_CONFIG_ID = 0x3028;
constexpr s32 EGL_LEVEL = 0x3029;
constexpr s32 EGL_MAX_PBUFFER_HEIGHT = 0x302A;
constexpr s32 EGL_MAX_PBUFFER_WIDTH = 0x302C;
constexpr s32 EGL_SAMPLES = 0x3031;
constexpr s32 EGL_SAMPLE_BUFFERS = 0x3032;
constexpr s32 EGL_SURFACE_TYPE = 0x3033;
constexpr s32 EGL_MIN_SWAP_INTERVAL = 0x303B;
constexpr s32 EGL_MAX_SWAP_INTERVAL = 0x303C;
constexpr s32 EGL_COLOR_BUFFER_TYPE = 0x303F;
constexpr s32 EGL_RENDERABLE_TYPE = 0x3040;
constexpr s32 EGL_MATCH_NATIVE_PIXMAP = 0x3041;
constexpr s32 EGL_CONFORMANT = 0x3042;
constexpr s32 EGL_RGB_BUFFER = 0x308E;

constexpr s32 EGL_ALPHA_SIZE = 0x3021;
constexpr s32 EGL_BLUE_SIZE = 0x3022;
constexpr s32 EGL_GREEN_SIZE = 0x3023;
constexpr s32 EGL_RED_SIZE = 0x3024;
constexpr s32 EGL_DEPTH_SIZE = 0x3025;
constexpr s32 EGL_STENCIL_SIZE = 0x3026;

constexpr s32 EGL_PBUFFER_BIT = 0x0001;
constexpr s32 EGL_WINDOW_BIT = 0x0004;
constexpr s32 EGL_OPENGL_ES2_BIT = 0x0004;
constexpr s32 EGL_OPENGL_ES3_BIT = 0x0040;

// Config attributes live in a table indexed by (attribute - EGL_BUFFER_SIZE). Piglet stores
// 0x23 entries for the standard attributes (0x3020..0x3042) plus one extra slot that is exposed
// as attribute 0x309D.
constexpr s32 EGL_BUFFER_SIZE = 0x3020;
constexpr u32 NUM_STANDARD_ATTRIBS = 0x23;
constexpr s32 EGL_PIGLET_EXTRA_ATTRIB = 0x309D;
constexpr u32 NUM_CONFIG_ATTRIBS = NUM_STANDARD_ATTRIBS + 1;

// Piglet exposes 30 configs: 10 pixel formats for each of 3 multisample modes.
constexpr u32 NUM_FORMATS = 10;
constexpr u32 NUM_MSAA_MODES = 3;
constexpr u32 NUM_CONFIGS = NUM_FORMATS * NUM_MSAA_MODES;

struct ConfigFormat {
    u8 red;
    u8 green;
    u8 blue;
    u8 alpha;
    u8 depth;
    u8 stencil;
    u16 surface_type;
};

// Pixel formats, in the order Piglet registers them.
constexpr std::array<ConfigFormat, NUM_FORMATS> CONFIG_FORMATS = {{
    {8, 8, 8, 8, 16, 8, EGL_WINDOW_BIT | EGL_PBUFFER_BIT},
    {8, 8, 8, 8, 32, 8, EGL_WINDOW_BIT | EGL_PBUFFER_BIT},
    {8, 8, 8, 8, 16, 0, EGL_WINDOW_BIT | EGL_PBUFFER_BIT},
    {8, 8, 8, 8, 32, 0, EGL_WINDOW_BIT | EGL_PBUFFER_BIT},
    {8, 8, 8, 8, 0, 0, EGL_WINDOW_BIT | EGL_PBUFFER_BIT},
    {10, 10, 10, 2, 16, 8, EGL_WINDOW_BIT | EGL_PBUFFER_BIT},
    {10, 10, 10, 2, 32, 8, EGL_WINDOW_BIT | EGL_PBUFFER_BIT},
    {10, 10, 10, 2, 16, 0, EGL_WINDOW_BIT | EGL_PBUFFER_BIT},
    {10, 10, 10, 2, 32, 0, EGL_WINDOW_BIT | EGL_PBUFFER_BIT},
    {10, 10, 10, 2, 0, 0, EGL_WINDOW_BIT | EGL_PBUFFER_BIT},
}};

// Sample counts for each multisample mode.
constexpr std::array<s32, NUM_MSAA_MODES> CONFIG_SAMPLES = {0, 4, 8};

// How eglChooseConfig compares each attribute against the requested criterion.
enum class Match : u8 {
    Ignore = 0,
    AtLeast = 1,
    Exact = 2,
    Mask = 3,
    Special = 4, // EGL_MATCH_NATIVE_PIXMAP, only EGL_NONE can match.
};

constexpr std::array<Match, NUM_CONFIG_ATTRIBS> MATCH_KINDS = {
    Match::AtLeast, Match::AtLeast, Match::AtLeast, Match::AtLeast, Match::AtLeast, // 0x3020-24
    Match::AtLeast, Match::AtLeast, Match::Exact,   Match::Exact,   Match::Exact,   // 0x3025-29
    Match::Ignore,  Match::Ignore,  Match::Ignore,  Match::Exact,   Match::Ignore,  // 0x302A-2E
    Match::Exact,   Match::Ignore,  Match::AtLeast, Match::AtLeast, Match::Mask,    // 0x302F-33
    Match::Exact,   Match::Exact,   Match::Exact,   Match::Exact,   Match::Ignore,  // 0x3034-38
    Match::Exact,   Match::Exact,   Match::Exact,   Match::Exact,   Match::AtLeast, // 0x3039-3D
    Match::AtLeast, Match::Exact,   Match::Mask,    Match::Special, Match::Mask,    // 0x303E-42
    Match::Ignore,                                                                  // 0x309D
};

// Attributes accepted in an eglChooseConfig attribute list. Everything else is
// EGL_BAD_ATTRIBUTE. The last few are surface/texture attributes that Piglet accepts but ignores.
constexpr bool IsValidChooseAttrib(s32 attrib) {
    return (attrib >= EGL_BUFFER_SIZE && attrib <= EGL_CONFORMANT && attrib != 0x3030 &&
            attrib != EGL_NONE) ||
           (attrib >= 0x3056 && attrib <= 0x3058) || (attrib >= 0x3080 && attrib <= 0x3083) ||
           attrib == 0x3086;
}

using ConfigValues = std::array<s32, NUM_CONFIG_ATTRIBS>;

constexpr u32 AttribIndex(s32 attrib) {
    return static_cast<u32>(attrib - EGL_BUFFER_SIZE);
}

// Default value of every attribute. Used both for configs (attributes the config table does not
// set keep these values) and as the default eglChooseConfig criteria.
ConfigValues MakeDefaultValues() {
    ConfigValues v{};
    v[AttribIndex(EGL_CONFIG_CAVEAT)] = EGL_DONT_CARE;
    v[AttribIndex(EGL_CONFIG_ID)] = EGL_DONT_CARE;
    v[AttribIndex(0x302D)] = EGL_DONT_CARE; // EGL_NATIVE_RENDERABLE
    v[AttribIndex(0x302F)] = EGL_DONT_CARE; // EGL_NATIVE_VISUAL_TYPE
    v[AttribIndex(EGL_SURFACE_TYPE)] = EGL_WINDOW_BIT;
    v[AttribIndex(0x3034)] = EGL_NONE; // EGL_TRANSPARENT_TYPE
    v[AttribIndex(0x3035)] = EGL_DONT_CARE;
    v[AttribIndex(0x3036)] = EGL_DONT_CARE;
    v[AttribIndex(0x3037)] = EGL_DONT_CARE;
    v[AttribIndex(0x3039)] = EGL_DONT_CARE; // EGL_BIND_TO_TEXTURE_RGB
    v[AttribIndex(0x303A)] = EGL_DONT_CARE; // EGL_BIND_TO_TEXTURE_RGBA
    v[AttribIndex(EGL_MIN_SWAP_INTERVAL)] = EGL_DONT_CARE;
    v[AttribIndex(EGL_MAX_SWAP_INTERVAL)] = EGL_DONT_CARE;
    v[AttribIndex(EGL_COLOR_BUFFER_TYPE)] = EGL_RGB_BUFFER;
    v[AttribIndex(EGL_RENDERABLE_TYPE)] = 1; // EGL_OPENGL_ES_BIT
    v[AttribIndex(EGL_MATCH_NATIVE_PIXMAP)] = EGL_NONE;
    return v;
}

struct Config {
    ConfigValues values{};

    s32 Get(s32 attrib) const {
        return values[AttribIndex(attrib)];
    }
};

struct Display {
    s32 ref_count = 0;
    bool valid = false;
    std::array<Config, NUM_CONFIGS> configs{};
};

struct EglState {
    std::mutex mutex;
    u32 error = EGL_SUCCESS;
    u32 bound_api = EGL_OPENGL_ES_API;
    // Piglet has a single display. Once created it is reused by every eglGetDisplay call until it
    // is terminated.
    Display display;
    bool display_is_default = false;
};

EglState g_egl;

struct PigletConfigState {
    OrbisPigletConfigVSH config{};
    bool configured = false;
    // Piglet initializes its platform layer together with the first display.
    bool initialized = false;
};

PigletConfigState g_piglet;
std::mutex g_piglet_mutex;

void BuildConfigs(Display& display) {
    const ConfigValues defaults = MakeDefaultValues();
    for (u32 mode = 0; mode < NUM_MSAA_MODES; mode++) {
        for (u32 i = 0; i < NUM_FORMATS; i++) {
            const ConfigFormat& format = CONFIG_FORMATS[i];
            Config& config = display.configs[mode * NUM_FORMATS + i];
            config.values = defaults;
            auto set = [&](s32 attrib, s32 value) { config.values[AttribIndex(attrib)] = value; };
            set(EGL_CONFIG_ID, static_cast<s32>(mode * NUM_FORMATS + i + 1));
            set(EGL_ALPHA_SIZE, format.alpha);
            set(EGL_BLUE_SIZE, format.blue);
            set(EGL_GREEN_SIZE, format.green);
            set(EGL_RED_SIZE, format.red);
            set(EGL_DEPTH_SIZE, format.depth);
            set(EGL_STENCIL_SIZE, format.stencil);
            set(EGL_SAMPLE_BUFFERS, mode != 0 ? 1 : 0);
            set(EGL_SAMPLES, CONFIG_SAMPLES[mode]);
            set(EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT | EGL_OPENGL_ES3_BIT);
            set(EGL_CONFORMANT, EGL_OPENGL_ES2_BIT);
            set(EGL_SURFACE_TYPE, format.surface_type);
            set(EGL_MAX_PBUFFER_WIDTH, 0x1000);
            set(EGL_MAX_PBUFFER_HEIGHT, 0x1000);
            set(EGL_MIN_SWAP_INTERVAL, 0);
            set(EGL_MAX_SWAP_INTERVAL, 3);
        }
    }
}

void SetError(u32 error) {
    g_egl.error = error;
}

// Returns the display if the handle refers to a valid, initialized display. Otherwise sets the
// matching EGL error and returns nullptr.
Display* ValidateDisplay(void* dpy) {
    if (dpy == nullptr || dpy != &g_egl.display) {
        SetError(EGL_BAD_DISPLAY);
        return nullptr;
    }
    if (!g_egl.display.valid) {
        SetError(EGL_NOT_INITIALIZED);
        return nullptr;
    }
    return &g_egl.display;
}

const Config* ToConfig(const Display& display, void* handle) {
    for (const Config& config : display.configs) {
        if (handle == &config) {
            return &config;
        }
    }
    return nullptr;
}

bool MatchesCriteria(const Config& config, const ConfigValues& criteria) {
    for (u32 i = 0; i < NUM_CONFIG_ATTRIBS; i++) {
        const s32 attrib = EGL_BUFFER_SIZE + static_cast<s32>(i);
        const s32 wanted = criteria[i];
        const s32 actual = config.values[i];
        if (MATCH_KINDS[i] == Match::Ignore) {
            continue;
        }
        if (wanted == EGL_DONT_CARE && attrib != EGL_MATCH_NATIVE_PIXMAP && attrib != EGL_LEVEL) {
            continue;
        }
        switch (MATCH_KINDS[i]) {
        case Match::AtLeast:
            if (actual < wanted) {
                return false;
            }
            break;
        case Match::Exact:
            if (actual != wanted) {
                return false;
            }
            break;
        case Match::Mask:
            if ((actual & wanted) != wanted) {
                return false;
            }
            break;
        case Match::Special:
            if (wanted != EGL_NONE) {
                return false;
            }
            break;
        default:
            break;
        }
    }
    return true;
}

// Sort order from the EGL 1.4 specification (section 3.4.1), which is what the attribute
// priorities stored in Piglet's config tables describe.
bool ConfigLess(const Config* a, const Config* b, const ConfigValues& criteria) {
    auto caveat_rank = [](const Config* c) {
        const s32 caveat = c->Get(EGL_CONFIG_CAVEAT);
        const bool has_caveat = caveat != EGL_DONT_CARE && caveat != EGL_NONE;
        return has_caveat ? 1 : 0;
    };
    if (caveat_rank(a) != caveat_rank(b)) {
        return caveat_rank(a) < caveat_rank(b);
    }
    // Only RGB buffers exist, so the color buffer type never separates two configs.
    auto color_bits = [&](const Config* c) {
        s32 sum = 0;
        for (const s32 attrib : {EGL_RED_SIZE, EGL_GREEN_SIZE, EGL_BLUE_SIZE, EGL_ALPHA_SIZE}) {
            if (criteria[AttribIndex(attrib)] != 0) {
                sum += c->Get(attrib);
            }
        }
        return sum;
    };
    if (color_bits(a) != color_bits(b)) {
        return color_bits(a) > color_bits(b);
    }
    for (const s32 attrib : {EGL_BUFFER_SIZE, EGL_SAMPLE_BUFFERS, EGL_SAMPLES, EGL_DEPTH_SIZE,
                             EGL_STENCIL_SIZE, 0x303E /* EGL_ALPHA_MASK_SIZE */}) {
        if (a->Get(attrib) != b->Get(attrib)) {
            return a->Get(attrib) < b->Get(attrib);
        }
    }
    return a->Get(EGL_CONFIG_ID) < b->Get(EGL_CONFIG_ID);
}

// Copies up to config_size config handles to the output array, as eglGetConfigs does.
u32 CopyConfigs(const Display& display, void** configs, s32 config_size, s32* num_config) {
    if (configs == nullptr) {
        *num_config = static_cast<s32>(NUM_CONFIGS);
        return EGL_TRUE;
    }
    const s32 count = std::clamp(config_size, 0, static_cast<s32>(NUM_CONFIGS));
    *num_config = count;
    for (s32 i = 0; i < count; i++) {
        configs[i] = const_cast<Config*>(&display.configs[i]);
    }
    return EGL_TRUE;
}

} // namespace

u32 PS4_SYSV_ABI eglGetError() {
    // Piglet returns the last error without resetting it. Every entry point resets it to
    // EGL_SUCCESS before it starts.
    std::scoped_lock lk{g_egl.mutex};
    return g_egl.error;
}

void* PS4_SYSV_ABI eglGetDisplay(void* native_display) {
    std::scoped_lock lk{g_egl.mutex};
    SetError(EGL_SUCCESS);
    if (native_display != nullptr) {
        LOG_ERROR(Lib_Piglet, "Unsupported display id {}", fmt::ptr(native_display));
        return nullptr;
    }
    if (g_egl.display_is_default) {
        return &g_egl.display;
    }
    // First display creation also brings up the Piglet platform layer.
    {
        std::scoped_lock piglet_lk{g_piglet_mutex};
        g_piglet.initialized = true;
    }
    BuildConfigs(g_egl.display);
    g_egl.display.valid = true;
    g_egl.display.ref_count = 0;
    g_egl.display_is_default = true;
    return &g_egl.display;
}

u32 PS4_SYSV_ABI eglInitialize(void* dpy, s32* major, s32* minor) {
    std::scoped_lock lk{g_egl.mutex};
    SetError(EGL_SUCCESS);
    if (dpy == nullptr || dpy != &g_egl.display) {
        SetError(EGL_BAD_DISPLAY);
        LOG_ERROR(Lib_Piglet, "Invalid display {}", fmt::ptr(dpy));
        return EGL_FALSE;
    }
    if (!g_egl.display.valid) {
        // A terminated display is brought back up.
        BuildConfigs(g_egl.display);
        g_egl.display.valid = true;
        g_egl.display.ref_count = 0;
        g_egl.display_is_default = true;
    }
    if (major != nullptr) {
        *major = 1;
    }
    if (minor != nullptr) {
        *minor = 4;
    }
    g_egl.display.ref_count++;
    return EGL_TRUE;
}

u32 PS4_SYSV_ABI eglTerminate(void* dpy) {
    std::scoped_lock lk{g_egl.mutex};
    SetError(EGL_SUCCESS);
    Display* display = ValidateDisplay(dpy);
    if (display == nullptr) {
        LOG_ERROR(Lib_Piglet, "Invalid display {}", fmt::ptr(dpy));
        return EGL_FALSE;
    }
    // The display goes away when the last eglInitialize reference is released.
    if (--display->ref_count == 0) {
        display->valid = false;
        g_egl.display_is_default = false;
    }
    return EGL_TRUE;
}

const char* PS4_SYSV_ABI eglQueryString(void* dpy, s32 name) {
    std::scoped_lock lk{g_egl.mutex};
    SetError(EGL_SUCCESS);
    if (ValidateDisplay(dpy) == nullptr) {
        LOG_ERROR(Lib_Piglet, "Invalid display {}", fmt::ptr(dpy));
        return nullptr;
    }
    switch (name) {
    case EGL_VENDOR:
        return "Sony Interactive Entertainment Inc.";
    case EGL_VERSION:
        return "1.4";
    case EGL_EXTENSIONS:
        // Piglet appends a space after every extension it registers.
        return "EGL_SCE_piglet_sync EGL_SCE_piglet_memory_info ";
    case EGL_CLIENT_APIS:
        // Only OpenGL ES is supported.
        return "OpenGL_ES";
    default:
        SetError(EGL_BAD_PARAMETER);
        LOG_ERROR(Lib_Piglet, "Invalid name {:#x}", name);
        return nullptr;
    }
}

u32 PS4_SYSV_ABI eglGetConfigs(void* dpy, void** configs, s32 config_size, s32* num_config) {
    std::scoped_lock lk{g_egl.mutex};
    SetError(EGL_SUCCESS);
    Display* display = ValidateDisplay(dpy);
    if (display == nullptr) {
        LOG_ERROR(Lib_Piglet, "Invalid display {}", fmt::ptr(dpy));
        return EGL_FALSE;
    }
    if (num_config == nullptr) {
        SetError(EGL_BAD_PARAMETER);
        LOG_ERROR(Lib_Piglet, "num_config is null");
        return EGL_FALSE;
    }
    *num_config = 0;
    return CopyConfigs(*display, configs, config_size, num_config);
}

u32 PS4_SYSV_ABI eglChooseConfig(void* dpy, const s32* attrib_list, void** configs, s32 config_size,
                                 s32* num_config) {
    std::scoped_lock lk{g_egl.mutex};
    SetError(EGL_SUCCESS);
    Display* display = ValidateDisplay(dpy);
    if (display == nullptr) {
        LOG_ERROR(Lib_Piglet, "Invalid display {}", fmt::ptr(dpy));
        return EGL_FALSE;
    }
    if (num_config == nullptr) {
        SetError(EGL_BAD_PARAMETER);
        LOG_ERROR(Lib_Piglet, "num_config is null");
        return EGL_FALSE;
    }
    if (configs == nullptr) {
        // Piglet reports the total number of configs here, not the number of matches.
        *num_config = static_cast<s32>(NUM_CONFIGS);
        return EGL_TRUE;
    }

    ConfigValues criteria = MakeDefaultValues();
    for (const s32* attrib = attrib_list; attrib != nullptr && *attrib != EGL_NONE; attrib += 2) {
        if (!IsValidChooseAttrib(*attrib)) {
            SetError(EGL_BAD_ATTRIBUTE);
            LOG_ERROR(Lib_Piglet, "Invalid attribute {:#x}", *attrib);
            return EGL_FALSE;
        }
        if (*attrib <= EGL_CONFORMANT) {
            criteria[AttribIndex(*attrib)] = attrib[1];
        }
    }

    std::array<const Config*, NUM_CONFIGS> matches;
    u32 match_count = 0;
    for (const Config& config : display->configs) {
        if (MatchesCriteria(config, criteria)) {
            matches[match_count++] = &config;
        }
    }
    std::stable_sort(matches.begin(), matches.begin() + match_count,
                     [&](const Config* a, const Config* b) { return ConfigLess(a, b, criteria); });

    const s32 count = std::min(static_cast<s32>(match_count), std::max(config_size, 0));
    for (s32 i = 0; i < count; i++) {
        configs[i] = const_cast<Config*>(matches[i]);
    }
    *num_config = count;
    return EGL_TRUE;
}

u32 PS4_SYSV_ABI eglGetConfigAttrib(void* dpy, void* config, s32 attribute, s32* value) {
    std::scoped_lock lk{g_egl.mutex};
    SetError(EGL_SUCCESS);
    Display* display = ValidateDisplay(dpy);
    if (display == nullptr) {
        LOG_ERROR(Lib_Piglet, "Invalid display {}", fmt::ptr(dpy));
        return EGL_FALSE;
    }
    if (config == nullptr || value == nullptr) {
        SetError(EGL_BAD_PARAMETER);
        LOG_ERROR(Lib_Piglet, "config or value is null");
        return EGL_FALSE;
    }
    if (attribute == EGL_MATCH_NATIVE_PIXMAP) {
        SetError(EGL_BAD_ATTRIBUTE);
        LOG_ERROR(Lib_Piglet, "EGL_MATCH_NATIVE_PIXMAP cannot be queried");
        return EGL_FALSE;
    }
    const Config* cfg = ToConfig(*display, config);
    if (cfg == nullptr) {
        SetError(EGL_BAD_PARAMETER);
        LOG_ERROR(Lib_Piglet, "Invalid config {}", fmt::ptr(config));
        return EGL_FALSE;
    }
    if (attribute >= EGL_BUFFER_SIZE &&
        attribute < EGL_BUFFER_SIZE + static_cast<s32>(NUM_STANDARD_ATTRIBS)) {
        *value = cfg->values[AttribIndex(attribute)];
        return EGL_TRUE;
    }
    if (attribute == EGL_PIGLET_EXTRA_ATTRIB) {
        *value = cfg->values[NUM_STANDARD_ATTRIBS];
        return EGL_TRUE;
    }
    SetError(EGL_BAD_ATTRIBUTE);
    return EGL_FALSE;
}

u32 PS4_SYSV_ABI eglBindAPI(u32 api) {
    std::scoped_lock lk{g_egl.mutex};
    SetError(EGL_SUCCESS);
    if (api != EGL_OPENGL_ES_API) {
        SetError(EGL_BAD_PARAMETER);
        LOG_ERROR(Lib_Piglet, "Unsupported API {:#x}", api);
        return EGL_FALSE;
    }
    g_egl.bound_api = api;
    return EGL_TRUE;
}

u32 PS4_SYSV_ABI eglQueryAPI() {
    std::scoped_lock lk{g_egl.mutex};
    SetError(EGL_SUCCESS);
    return g_egl.bound_api;
}

bool PS4_SYSV_ABI scePigletSetConfigurationVSH(const OrbisPigletConfigVSH* config) {
    if (config == nullptr) {
        return false;
    }
    std::scoped_lock lk{g_piglet_mutex};
    if (g_piglet.initialized) {
        LOG_ERROR(Lib_Piglet, "Setting Piglet configuration after initialization!");
    }
    std::memset(&g_piglet.config, 0, sizeof(g_piglet.config));
    g_piglet.configured = true;
    u32 copy_size = PIGLET_CONFIG_VSH_SIZE;
    if (config->size < PIGLET_CONFIG_VSH_SIZE) {
        LOG_WARNING(Lib_Piglet, "Configuration smaller than expected; continuing anyway");
        copy_size = config->size;
    }
    std::memcpy(&g_piglet.config, config, copy_size);
    g_piglet.config.size = PIGLET_CONFIG_VSH_SIZE;
    return true;
}

bool PS4_SYSV_ABI scePigletGetConfigurationVSH(OrbisPigletConfigVSH* config) {
    if (config == nullptr) {
        return false;
    }
    const u32 size = config->size;
    if (size == 0) {
        return false;
    }
    std::scoped_lock lk{g_piglet_mutex};
    std::memset(config, 0, size);
    std::memcpy(config, &g_piglet.config, std::min(size, PIGLET_CONFIG_VSH_SIZE));
    config->size = size;
    return true;
}

void RegisterLib(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("Pu5Yd9+FY9Y", "libScePigletv2VSH", 1, "libScePigletv2VSH", eglGetError);
    LIB_FUNCTION("Z7pNCsK2dAs", "libScePigletv2VSH", 1, "libScePigletv2VSH", eglGetDisplay);
    LIB_FUNCTION("KJmeNzmA6UM", "libScePigletv2VSH", 1, "libScePigletv2VSH", eglInitialize);
    LIB_FUNCTION("ConU-nXswn8", "libScePigletv2VSH", 1, "libScePigletv2VSH", eglTerminate);
    LIB_FUNCTION("mwSvDt0dHF8", "libScePigletv2VSH", 1, "libScePigletv2VSH", eglQueryString);
    LIB_FUNCTION("xg5Kn1Zto4k", "libScePigletv2VSH", 1, "libScePigletv2VSH", eglGetConfigs);
    LIB_FUNCTION("fesoNJCZpSA", "libScePigletv2VSH", 1, "libScePigletv2VSH", eglChooseConfig);
    LIB_FUNCTION("5O92iRi6Hk4", "libScePigletv2VSH", 1, "libScePigletv2VSH", eglGetConfigAttrib);
    LIB_FUNCTION("yCo3HzKQSts", "libScePigletv2VSH", 1, "libScePigletv2VSH", eglBindAPI);
    LIB_FUNCTION("GpgGEmVl9Ck", "libScePigletv2VSH", 1, "libScePigletv2VSH", eglQueryAPI);
    LIB_FUNCTION("M9RtXpjSYtE", "libScePigletv2VSH", 1, "libScePigletv2VSH",
                 scePigletSetConfigurationVSH);
    LIB_FUNCTION("3gtNzvkq-XY", "libScePigletv2VSH", 1, "libScePigletv2VSH",
                 scePigletGetConfigurationVSH);
}

} // namespace Libraries::Piglet
