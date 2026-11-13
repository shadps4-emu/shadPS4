// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <array>
#include <memory>
#include <mutex>
#include <vector>
#include <fmt/format.h>

#include "common/logging/log.h"
#include "core/libraries/libs.h"
#include "core/libraries/piglet/piglet_gles.h"
#include "core/libraries/piglet/piglet_internal.h"

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
constexpr u32 EGL_BAD_CONFIG = 0x3005;
constexpr u32 EGL_BAD_DISPLAY = 0x3008;
constexpr u32 EGL_BAD_ALLOC = 0x3003;
constexpr u32 EGL_BAD_CONTEXT = 0x3006;
constexpr u32 EGL_BAD_MATCH = 0x3009;
constexpr u32 EGL_BAD_NATIVE_PIXMAP = 0x300A;
constexpr u32 EGL_BAD_NATIVE_WINDOW = 0x300B;
constexpr u32 EGL_BAD_PARAMETER = 0x300C;
constexpr u32 EGL_BAD_SURFACE = 0x300D;

constexpr s32 EGL_VENDOR = 0x3053;
constexpr s32 EGL_VERSION = 0x3054;
constexpr s32 EGL_EXTENSIONS = 0x3055;
constexpr s32 EGL_CLIENT_APIS = 0x308D;

constexpr u32 EGL_OPENGL_ES_API = 0x30A0;

constexpr s32 EGL_HEIGHT = 0x3056;
constexpr s32 EGL_WIDTH = 0x3057;
constexpr s32 EGL_LARGEST_PBUFFER = 0x3058;
constexpr s32 EGL_CORE_NATIVE_ENGINE = 0x305B;
constexpr s32 EGL_TEXTURE_FORMAT = 0x3080;
constexpr s32 EGL_TEXTURE_TARGET = 0x3081;
constexpr s32 EGL_MIPMAP_TEXTURE = 0x3082;
constexpr s32 EGL_MIPMAP_LEVEL = 0x3083;
constexpr s32 EGL_BACK_BUFFER = 0x3084;
constexpr s32 EGL_RENDER_BUFFER = 0x3086;
constexpr s32 EGL_HORIZONTAL_RESOLUTION = 0x3090;
constexpr s32 EGL_VERTICAL_RESOLUTION = 0x3091;
constexpr s32 EGL_PIXEL_ASPECT_RATIO = 0x3092;
constexpr s32 EGL_SWAP_BEHAVIOR = 0x3093;
constexpr s32 EGL_BUFFER_DESTROYED = 0x3095;
constexpr s32 EGL_CONTEXT_CLIENT_TYPE = 0x3097;
constexpr s32 EGL_CONTEXT_CLIENT_VERSION = 0x3098;
constexpr s32 EGL_CONTEXT_MINOR_VERSION = 0x30FB;
constexpr s32 EGL_NO_TEXTURE = 0x305C;
constexpr s32 EGL_UNKNOWN = -1;
constexpr s32 EGL_PIGLET_WINDOW_ATTRIB = 0x32AA; // Accepted and ignored in window surfaces.

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

struct Surface {
    const Config* config = nullptr;
    bool is_window = false;
    void* native_window = nullptr;
    s32 width = 0;
    s32 height = 0;
    s32 swap_interval = 1;
    bool delete_pending = false;
};

struct Context {
    const Config* config = nullptr;
    s32 version = 1;
    std::unique_ptr<GlContext> gl;
    bool delete_pending = false;
};

struct Display {
    s32 ref_count = 0;
    bool valid = false;
    std::array<Config, NUM_CONFIGS> configs{};
    std::vector<std::unique_ptr<Surface>> surfaces;
    std::vector<std::unique_ptr<Context>> contexts;
};

struct EglState {
    std::mutex mutex;
    u32 error = EGL_SUCCESS;
    u32 bound_api = EGL_OPENGL_ES_API;
    // Piglet has a single display. Once created it is reused by every eglGetDisplay call until it
    // is terminated.
    Display display;
    bool display_is_default = false;
    // The current context and surfaces are global, not per thread.
    Context* current_context = nullptr;
    Surface* current_draw = nullptr;
    Surface* current_read = nullptr;
};

EglState g_egl;

// Default size reported for window surfaces. How Piglet derives the real size from the native
// window object is not known yet.
constexpr s32 DEFAULT_WINDOW_WIDTH = 1920;
constexpr s32 DEFAULT_WINDOW_HEIGHT = 1080;

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

const Config* ToConfig(const Display& display, const void* handle) {
    for (const Config& config : display.configs) {
        if (handle == &config) {
            return &config;
        }
    }
    return nullptr;
}

template <typename T>
T* FindObject(std::vector<std::unique_ptr<T>>& objects, const void* handle) {
    for (auto& object : objects) {
        if (object.get() == handle) {
            return object.get();
        }
    }
    return nullptr;
}

template <typename T>
void EraseObject(std::vector<std::unique_ptr<T>>& objects, const T* object) {
    std::erase_if(objects, [object](const std::unique_ptr<T>& o) { return o.get() == object; });
}

// Makes no surface and no context current, freeing the objects that were waiting for it.
void ReleaseCurrent(Display& display) {
    Context* context = g_egl.current_context;
    Surface* draw = g_egl.current_draw;
    Surface* read = g_egl.current_read;
    g_egl.current_context = nullptr;
    g_egl.current_draw = nullptr;
    g_egl.current_read = nullptr;
    if (context != nullptr && context->delete_pending) {
        EraseObject(display.contexts, context);
    }
    for (Surface* surface : {draw, read}) {
        if (surface != nullptr && surface->delete_pending) {
            EraseObject(display.surfaces, surface);
        }
    }
}

void DestroyDisplayObjects(Display& display) {
    g_egl.current_context = nullptr;
    g_egl.current_draw = nullptr;
    g_egl.current_read = nullptr;
    display.contexts.clear();
    display.surfaces.clear();
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

u32 CopyConfigs(Display& display, void** configs, s32 config_size, s32* num_config) {
    if (configs == nullptr) {
        *num_config = static_cast<s32>(NUM_CONFIGS);
        return EGL_TRUE;
    }
    const s32 count = std::clamp(config_size, 0, static_cast<s32>(NUM_CONFIGS));
    *num_config = count;
    for (s32 i = 0; i < count; i++) {
        configs[i] = &display.configs[i];
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
    MarkPigletInitialized();
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
    if (--display->ref_count == 0) {
        DestroyDisplayObjects(*display);
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

    std::array<Config*, NUM_CONFIGS> matches;
    u32 match_count = 0;
    for (Config& config : display->configs) {
        if (MatchesCriteria(config, criteria)) {
            matches[match_count++] = &config;
        }
    }
    std::stable_sort(matches.begin(), matches.begin() + match_count,
                     [&](const Config* a, const Config* b) { return ConfigLess(a, b, criteria); });

    const s32 count = std::min(static_cast<s32>(match_count), std::max(config_size, 0));
    for (s32 i = 0; i < count; i++) {
        configs[i] = matches[i];
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
        SetError(EGL_BAD_CONFIG);
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

void* PS4_SYSV_ABI eglCreateWindowSurface(void* dpy, void* config, void* native_window,
                                          const s32* attrib_list) {
    std::scoped_lock lk{g_egl.mutex};
    SetError(EGL_SUCCESS);
    Display* display = ValidateDisplay(dpy);
    if (display == nullptr) {
        LOG_ERROR(Lib_Piglet, "Invalid display {}", fmt::ptr(dpy));
        return nullptr;
    }
    const Config* cfg = ToConfig(*display, config);
    if (cfg == nullptr) {
        SetError(EGL_BAD_CONFIG);
        LOG_ERROR(Lib_Piglet, "Invalid config {}", fmt::ptr(config));
        return nullptr;
    }
    for (const s32* attrib = attrib_list; attrib != nullptr && *attrib != EGL_NONE; attrib += 2) {
        if (*attrib != EGL_RENDER_BUFFER && *attrib != EGL_PIGLET_WINDOW_ATTRIB) {
            SetError(EGL_BAD_MATCH);
            LOG_ERROR(Lib_Piglet, "Invalid attribute {:#x}", *attrib);
            return nullptr;
        }
    }
    if ((cfg->Get(EGL_SURFACE_TYPE) & EGL_WINDOW_BIT) == 0) {
        SetError(EGL_BAD_MATCH);
        LOG_ERROR(Lib_Piglet, "Config does not support window surfaces");
        return nullptr;
    }
    if (native_window == nullptr) {
        SetError(EGL_BAD_NATIVE_WINDOW);
        LOG_ERROR(Lib_Piglet, "Native window is null");
        return nullptr;
    }
    for (const auto& surface : display->surfaces) {
        if (surface->native_window == native_window) {
            SetError(EGL_BAD_ALLOC);
            LOG_ERROR(Lib_Piglet, "Native window already has a surface");
            return nullptr;
        }
    }
    auto surface = std::make_unique<Surface>();
    surface->config = cfg;
    surface->is_window = true;
    surface->native_window = native_window;
    surface->width = DEFAULT_WINDOW_WIDTH;
    surface->height = DEFAULT_WINDOW_HEIGHT;
    return display->surfaces.emplace_back(std::move(surface)).get();
}

void* PS4_SYSV_ABI eglCreatePbufferSurface(void* dpy, void* config, const s32* attrib_list) {
    std::scoped_lock lk{g_egl.mutex};
    SetError(EGL_SUCCESS);
    Display* display = ValidateDisplay(dpy);
    if (display == nullptr) {
        LOG_ERROR(Lib_Piglet, "Invalid display {}", fmt::ptr(dpy));
        return nullptr;
    }
    const Config* cfg = ToConfig(*display, config);
    if (cfg == nullptr) {
        SetError(EGL_BAD_CONFIG);
        LOG_ERROR(Lib_Piglet, "Invalid config {}", fmt::ptr(config));
        return nullptr;
    }
    s32 width = 0;
    s32 height = 0;
    for (const s32* attrib = attrib_list; attrib != nullptr && *attrib != EGL_NONE; attrib += 2) {
        switch (*attrib) {
        case EGL_WIDTH:
            width = attrib[1];
            break;
        case EGL_HEIGHT:
            height = attrib[1];
            break;
        case EGL_LARGEST_PBUFFER:
        case EGL_TEXTURE_FORMAT:
        case EGL_TEXTURE_TARGET:
        case EGL_MIPMAP_TEXTURE:
            break;
        default:
            SetError(EGL_BAD_ATTRIBUTE);
            LOG_ERROR(Lib_Piglet, "Invalid attribute {:#x}", *attrib);
            return nullptr;
        }
    }
    if ((cfg->Get(EGL_SURFACE_TYPE) & EGL_PBUFFER_BIT) == 0) {
        SetError(EGL_BAD_MATCH);
        LOG_ERROR(Lib_Piglet, "Config does not support pbuffer surfaces");
        return nullptr;
    }
    if (width < 0 || height < 0) {
        SetError(EGL_BAD_PARAMETER);
        LOG_ERROR(Lib_Piglet, "Invalid pbuffer size {}x{}", width, height);
        return nullptr;
    }
    if (width > cfg->Get(EGL_MAX_PBUFFER_WIDTH) || height > cfg->Get(EGL_MAX_PBUFFER_HEIGHT)) {
        SetError(EGL_BAD_ALLOC);
        LOG_ERROR(Lib_Piglet, "Pbuffer size {}x{} is too large", width, height);
        return nullptr;
    }
    auto surface = std::make_unique<Surface>();
    surface->config = cfg;
    surface->width = width;
    surface->height = height;
    return display->surfaces.emplace_back(std::move(surface)).get();
}

void* PS4_SYSV_ABI eglCreatePixmapSurface(void* dpy, void* config,
                                          [[maybe_unused]] void* native_pixmap,
                                          [[maybe_unused]] const s32* attrib_list) {
    std::scoped_lock lk{g_egl.mutex};
    SetError(EGL_SUCCESS);
    Display* display = ValidateDisplay(dpy);
    if (display == nullptr) {
        LOG_ERROR(Lib_Piglet, "Invalid display {}", fmt::ptr(dpy));
        return nullptr;
    }
    const Config* cfg = ToConfig(*display, config);
    if (cfg == nullptr) {
        SetError(EGL_BAD_CONFIG);
        LOG_ERROR(Lib_Piglet, "Invalid config {}", fmt::ptr(config));
        return nullptr;
    }
    // No config supports pixmaps.
    SetError(EGL_BAD_MATCH);
    LOG_ERROR(Lib_Piglet, "Config does not support pixmap surfaces");
    return nullptr;
}

void* PS4_SYSV_ABI eglCreatePbufferFromClientBuffer([[maybe_unused]] void* dpy,
                                                    [[maybe_unused]] u32 buftype,
                                                    [[maybe_unused]] void* buffer,
                                                    [[maybe_unused]] void* config,
                                                    [[maybe_unused]] const s32* attrib_list) {
    std::scoped_lock lk{g_egl.mutex};
    SetError(EGL_SUCCESS);
    LOG_ERROR(Lib_Piglet, "Pbuffers from client buffers are not supported");
    return nullptr;
}

u32 PS4_SYSV_ABI eglDestroySurface(void* dpy, void* surface) {
    std::scoped_lock lk{g_egl.mutex};
    SetError(EGL_SUCCESS);
    Display* display = ValidateDisplay(dpy);
    if (display == nullptr) {
        LOG_ERROR(Lib_Piglet, "Invalid display {}", fmt::ptr(dpy));
        return EGL_FALSE;
    }
    Surface* obj = FindObject(display->surfaces, surface);
    if (obj == nullptr) {
        SetError(EGL_BAD_SURFACE);
        LOG_ERROR(Lib_Piglet, "Invalid surface {}", fmt::ptr(surface));
        return EGL_FALSE;
    }
    // A surface that is current is destroyed once it stops being current.
    if (obj == g_egl.current_draw || obj == g_egl.current_read) {
        obj->delete_pending = true;
    } else {
        EraseObject(display->surfaces, obj);
    }
    return EGL_TRUE;
}

u32 PS4_SYSV_ABI eglQuerySurface(void* dpy, void* surface, s32 attribute, s32* value) {
    std::scoped_lock lk{g_egl.mutex};
    SetError(EGL_SUCCESS);
    Display* display = ValidateDisplay(dpy);
    if (display == nullptr) {
        LOG_ERROR(Lib_Piglet, "Invalid display {}", fmt::ptr(dpy));
        return EGL_FALSE;
    }
    const Surface* obj = FindObject(display->surfaces, surface);
    if (obj == nullptr) {
        SetError(EGL_BAD_SURFACE);
        LOG_ERROR(Lib_Piglet, "Invalid surface {}", fmt::ptr(surface));
        return EGL_FALSE;
    }
    if (value == nullptr) {
        SetError(EGL_BAD_PARAMETER);
        LOG_ERROR(Lib_Piglet, "value is null");
        return EGL_FALSE;
    }
    switch (attribute) {
    case EGL_CONFIG_ID:
        *value = obj->config->Get(EGL_CONFIG_ID);
        break;
    case EGL_WIDTH:
        *value = obj->width;
        break;
    case EGL_HEIGHT:
        *value = obj->height;
        break;
    case EGL_LARGEST_PBUFFER:
        *value = 0;
        break;
    case EGL_TEXTURE_FORMAT:
    case EGL_TEXTURE_TARGET:
        *value = EGL_NO_TEXTURE;
        break;
    case EGL_MIPMAP_TEXTURE:
    case EGL_MIPMAP_LEVEL:
        *value = 0;
        break;
    case EGL_RENDER_BUFFER:
        *value = EGL_BACK_BUFFER;
        break;
    case EGL_SWAP_BEHAVIOR:
        *value = EGL_BUFFER_DESTROYED;
        break;
    case EGL_HORIZONTAL_RESOLUTION:
    case EGL_VERTICAL_RESOLUTION:
    case EGL_PIXEL_ASPECT_RATIO:
        *value = EGL_UNKNOWN;
        break;
    default:
        SetError(EGL_BAD_PARAMETER);
        LOG_ERROR(Lib_Piglet, "Invalid attribute {:#x}", attribute);
        return EGL_FALSE;
    }
    return EGL_TRUE;
}

u32 PS4_SYSV_ABI eglSurfaceAttrib(void* dpy, void* surface, s32 attribute,
                                  [[maybe_unused]] s32 value) {
    std::scoped_lock lk{g_egl.mutex};
    SetError(EGL_SUCCESS);
    Display* display = ValidateDisplay(dpy);
    if (display == nullptr) {
        LOG_ERROR(Lib_Piglet, "Invalid display {}", fmt::ptr(dpy));
        return EGL_FALSE;
    }
    if (FindObject(display->surfaces, surface) == nullptr) {
        SetError(EGL_BAD_SURFACE);
        LOG_ERROR(Lib_Piglet, "Invalid surface {}", fmt::ptr(surface));
        return EGL_FALSE;
    }
    switch (attribute) {
    case EGL_MIPMAP_LEVEL:
    case EGL_SWAP_BEHAVIOR:
        // Accepted without effect: no surface has a texture or a preserved back buffer.
        return EGL_TRUE;
    default:
        SetError(EGL_BAD_PARAMETER);
        LOG_ERROR(Lib_Piglet, "Invalid attribute {:#x}", attribute);
        return EGL_FALSE;
    }
}

u32 PS4_SYSV_ABI eglBindTexImage(void* dpy, void* surface, s32 buffer) {
    std::scoped_lock lk{g_egl.mutex};
    SetError(EGL_SUCCESS);
    Display* display = ValidateDisplay(dpy);
    if (display == nullptr) {
        LOG_ERROR(Lib_Piglet, "Invalid display {}", fmt::ptr(dpy));
        return EGL_FALSE;
    }
    if (FindObject(display->surfaces, surface) == nullptr) {
        SetError(EGL_BAD_SURFACE);
        LOG_ERROR(Lib_Piglet, "Invalid surface {}", fmt::ptr(surface));
        return EGL_FALSE;
    }
    if (buffer != EGL_BACK_BUFFER) {
        SetError(EGL_BAD_PARAMETER);
        LOG_ERROR(Lib_Piglet, "Invalid buffer {:#x}", buffer);
        return EGL_FALSE;
    }
    // Surfaces never have a texture format, so none can be bound to a texture.
    SetError(EGL_BAD_MATCH);
    LOG_ERROR(Lib_Piglet, "Surface has no texture format");
    return EGL_FALSE;
}

u32 PS4_SYSV_ABI eglReleaseTexImage(void* dpy, void* surface, s32 buffer) {
    std::scoped_lock lk{g_egl.mutex};
    SetError(EGL_SUCCESS);
    Display* display = ValidateDisplay(dpy);
    if (display == nullptr) {
        LOG_ERROR(Lib_Piglet, "Invalid display {}", fmt::ptr(dpy));
        return EGL_FALSE;
    }
    if (FindObject(display->surfaces, surface) == nullptr) {
        SetError(EGL_BAD_SURFACE);
        LOG_ERROR(Lib_Piglet, "Invalid surface {}", fmt::ptr(surface));
        return EGL_FALSE;
    }
    if (buffer != EGL_BACK_BUFFER) {
        SetError(EGL_BAD_PARAMETER);
        LOG_ERROR(Lib_Piglet, "Invalid buffer {:#x}", buffer);
        return EGL_FALSE;
    }
    SetError(EGL_BAD_MATCH);
    LOG_ERROR(Lib_Piglet, "Surface has no texture format");
    return EGL_FALSE;
}

u32 PS4_SYSV_ABI eglSwapInterval(void* dpy, s32 interval) {
    std::scoped_lock lk{g_egl.mutex};
    SetError(EGL_SUCCESS);
    if (ValidateDisplay(dpy) == nullptr) {
        LOG_ERROR(Lib_Piglet, "Invalid display {}", fmt::ptr(dpy));
        return EGL_FALSE;
    }
    if (g_egl.current_draw != nullptr) {
        const Config* cfg = g_egl.current_draw->config;
        g_egl.current_draw->swap_interval =
            std::clamp(interval, cfg->Get(EGL_MIN_SWAP_INTERVAL), cfg->Get(EGL_MAX_SWAP_INTERVAL));
    }
    return EGL_TRUE;
}

void* PS4_SYSV_ABI eglCreateContext(void* dpy, void* config, void* share_context,
                                    const s32* attrib_list) {
    std::scoped_lock lk{g_egl.mutex};
    SetError(EGL_SUCCESS);
    Display* display = ValidateDisplay(dpy);
    if (display == nullptr) {
        LOG_ERROR(Lib_Piglet, "Invalid display {}", fmt::ptr(dpy));
        return nullptr;
    }
    const Config* cfg = ToConfig(*display, config);
    if (cfg == nullptr) {
        SetError(EGL_BAD_CONFIG);
        LOG_ERROR(Lib_Piglet, "Invalid config {}", fmt::ptr(config));
        return nullptr;
    }
    if (share_context != nullptr && FindObject(display->contexts, share_context) == nullptr) {
        SetError(EGL_BAD_CONTEXT);
        LOG_ERROR(Lib_Piglet, "Invalid share context {}", fmt::ptr(share_context));
        return nullptr;
    }
    s32 version = 1;
    for (const s32* attrib = attrib_list; attrib != nullptr && *attrib != EGL_NONE; attrib += 2) {
        if (*attrib == EGL_CONTEXT_CLIENT_VERSION) {
            version = attrib[1];
        } else if (*attrib != EGL_CONTEXT_MINOR_VERSION) {
            SetError(EGL_BAD_MATCH);
            LOG_ERROR(Lib_Piglet, "Invalid attribute {:#x}", *attrib);
            return nullptr;
        }
    }
    s32 required_bit = 0;
    switch (version) {
    case 1:
        required_bit = 1; // EGL_OPENGL_ES_BIT
        break;
    case 2:
        required_bit = EGL_OPENGL_ES2_BIT;
        break;
    case 3:
        required_bit = EGL_OPENGL_ES3_BIT;
        break;
    default:
        break;
    }
    if (required_bit == 0 || (cfg->Get(EGL_RENDERABLE_TYPE) & required_bit) == 0) {
        SetError(EGL_BAD_CONFIG);
        LOG_ERROR(Lib_Piglet, "Config does not support OpenGL ES {}", version);
        return nullptr;
    }
    // TODO: Objects are not shared with share_context yet.
    auto context = std::make_unique<Context>();
    context->config = cfg;
    context->version = version;
    context->gl = std::make_unique<GlContext>(version);
    return display->contexts.emplace_back(std::move(context)).get();
}

u32 PS4_SYSV_ABI eglDestroyContext(void* dpy, void* ctx) {
    std::scoped_lock lk{g_egl.mutex};
    SetError(EGL_SUCCESS);
    Display* display = ValidateDisplay(dpy);
    if (display == nullptr) {
        LOG_ERROR(Lib_Piglet, "Invalid display {}", fmt::ptr(dpy));
        return EGL_FALSE;
    }
    Context* context = FindObject(display->contexts, ctx);
    if (context == nullptr) {
        SetError(EGL_BAD_CONTEXT);
        LOG_ERROR(Lib_Piglet, "Invalid context {}", fmt::ptr(ctx));
        return EGL_FALSE;
    }
    // A context that is current is destroyed once it stops being current.
    if (context == g_egl.current_context) {
        context->delete_pending = true;
    } else {
        EraseObject(display->contexts, context);
    }
    return EGL_TRUE;
}

u32 PS4_SYSV_ABI eglMakeCurrent(void* dpy, void* draw, void* read, void* ctx) {
    std::scoped_lock lk{g_egl.mutex};
    SetError(EGL_SUCCESS);
    Display* display = ValidateDisplay(dpy);
    if (display == nullptr) {
        LOG_ERROR(Lib_Piglet, "Invalid display {}", fmt::ptr(dpy));
        return EGL_FALSE;
    }
    if (draw == nullptr && read == nullptr && ctx == nullptr) {
        ReleaseCurrent(*display);
        return EGL_TRUE;
    }
    if (ctx == nullptr || draw == nullptr || read == nullptr) {
        SetError(EGL_BAD_MATCH);
        LOG_ERROR(Lib_Piglet, "Context and surfaces must be all set or all null");
        return EGL_FALSE;
    }
    // Objects that are waiting to be destroyed cannot be made current again.
    Context* context = FindObject(display->contexts, ctx);
    if (context == nullptr || context->delete_pending) {
        SetError(EGL_BAD_CONTEXT);
        LOG_ERROR(Lib_Piglet, "Invalid context {}", fmt::ptr(ctx));
        return EGL_FALSE;
    }
    Surface* draw_surface = FindObject(display->surfaces, draw);
    Surface* read_surface = FindObject(display->surfaces, read);
    if (draw_surface == nullptr || read_surface == nullptr || draw_surface->delete_pending ||
        read_surface->delete_pending) {
        SetError(EGL_BAD_SURFACE);
        LOG_ERROR(Lib_Piglet, "Invalid surface {}",
                  fmt::ptr(draw_surface == nullptr ? draw : read));
        return EGL_FALSE;
    }
    ReleaseCurrent(*display);
    g_egl.current_context = context;
    g_egl.current_draw = draw_surface;
    g_egl.current_read = read_surface;
    GlContext& gl = *context->gl;
    if (!gl.viewport_initialized) {
        // The first time a context is made current its viewport and scissor cover the surface.
        gl.viewport = {0, 0, draw_surface->width, draw_surface->height};
        gl.scissor = gl.viewport;
        gl.viewport_initialized = true;
    }
    return EGL_TRUE;
}

void* PS4_SYSV_ABI eglGetCurrentContext() {
    std::scoped_lock lk{g_egl.mutex};
    SetError(EGL_SUCCESS);
    return g_egl.current_context;
}

void* PS4_SYSV_ABI eglGetCurrentSurface(s32 readdraw) {
    std::scoped_lock lk{g_egl.mutex};
    SetError(EGL_SUCCESS);
    switch (readdraw) {
    case 0x3059: // EGL_DRAW
        return g_egl.current_draw;
    case 0x305A: // EGL_READ
        return g_egl.current_read;
    default:
        SetError(EGL_BAD_PARAMETER);
        LOG_ERROR(Lib_Piglet, "Invalid surface type {:#x}", readdraw);
        return nullptr;
    }
}

void* PS4_SYSV_ABI eglGetCurrentDisplay() {
    std::scoped_lock lk{g_egl.mutex};
    SetError(EGL_SUCCESS);
    return g_egl.current_context != nullptr ? &g_egl.display : nullptr;
}

u32 PS4_SYSV_ABI eglQueryContext(void* dpy, void* ctx, s32 attribute, s32* value) {
    std::scoped_lock lk{g_egl.mutex};
    SetError(EGL_SUCCESS);
    Display* display = ValidateDisplay(dpy);
    if (display == nullptr) {
        LOG_ERROR(Lib_Piglet, "Invalid display {}", fmt::ptr(dpy));
        return EGL_FALSE;
    }
    const Context* context = FindObject(display->contexts, ctx);
    if (context == nullptr) {
        SetError(EGL_BAD_CONTEXT);
        LOG_ERROR(Lib_Piglet, "Invalid context {}", fmt::ptr(ctx));
        return EGL_FALSE;
    }
    if (value == nullptr) {
        SetError(EGL_BAD_PARAMETER);
        LOG_ERROR(Lib_Piglet, "value is null");
        return EGL_FALSE;
    }
    switch (attribute) {
    case EGL_CONFIG_ID:
        *value = context->config->Get(EGL_CONFIG_ID);
        break;
    case EGL_CONTEXT_CLIENT_TYPE:
        *value = static_cast<s32>(EGL_OPENGL_ES_API);
        break;
    case EGL_CONTEXT_CLIENT_VERSION:
        *value = context->version;
        break;
    case EGL_RENDER_BUFFER:
        *value = EGL_BACK_BUFFER;
        break;
    default:
        SetError(EGL_BAD_PARAMETER);
        LOG_ERROR(Lib_Piglet, "Invalid attribute {:#x}", attribute);
        return EGL_FALSE;
    }
    return EGL_TRUE;
}

u32 PS4_SYSV_ABI eglWaitClient() {
    std::scoped_lock lk{g_egl.mutex};
    SetError(EGL_SUCCESS);
    // TODO: Wait for the GPU once there is a rendering backend.
    return EGL_TRUE;
}

u32 PS4_SYSV_ABI eglWaitGL() {
    return eglWaitClient();
}

u32 PS4_SYSV_ABI eglWaitNative(s32 engine) {
    std::scoped_lock lk{g_egl.mutex};
    SetError(EGL_SUCCESS);
    if (engine != EGL_CORE_NATIVE_ENGINE) {
        SetError(EGL_BAD_PARAMETER);
        LOG_ERROR(Lib_Piglet, "Invalid engine {:#x}", engine);
        return EGL_FALSE;
    }
    return EGL_TRUE;
}

u32 PS4_SYSV_ABI eglReleaseThread() {
    std::scoped_lock lk{g_egl.mutex};
    SetError(EGL_SUCCESS);
    return EGL_TRUE;
}

u32 PS4_SYSV_ABI eglSwapBuffers(void* dpy, void* surface) {
    std::scoped_lock lk{g_egl.mutex};
    SetError(EGL_SUCCESS);
    Display* display = ValidateDisplay(dpy);
    if (display == nullptr) {
        LOG_ERROR(Lib_Piglet, "Invalid display {}", fmt::ptr(dpy));
        return EGL_FALSE;
    }
    if (FindObject(display->surfaces, surface) == nullptr) {
        SetError(EGL_BAD_SURFACE);
        LOG_ERROR(Lib_Piglet, "Invalid surface {}", fmt::ptr(surface));
        return EGL_FALSE;
    }
    // TODO: Present the frame once there is a rendering backend.
    return EGL_TRUE;
}

u32 PS4_SYSV_ABI eglCopyBuffers(void* dpy, void* surface, [[maybe_unused]] void* native_pixmap) {
    std::scoped_lock lk{g_egl.mutex};
    SetError(EGL_SUCCESS);
    Display* display = ValidateDisplay(dpy);
    if (display == nullptr) {
        LOG_ERROR(Lib_Piglet, "Invalid display {}", fmt::ptr(dpy));
        return EGL_FALSE;
    }
    if (FindObject(display->surfaces, surface) == nullptr) {
        SetError(EGL_BAD_SURFACE);
        LOG_ERROR(Lib_Piglet, "Invalid surface {}", fmt::ptr(surface));
        return EGL_FALSE;
    }
    SetError(EGL_BAD_NATIVE_PIXMAP);
    LOG_ERROR(Lib_Piglet, "Native pixmaps are not supported");
    return EGL_FALSE;
}

void* PS4_SYSV_ABI eglGetProcAddress(const char* procname) {
    std::scoped_lock lk{g_egl.mutex};
    SetError(EGL_SUCCESS);
    if (procname == nullptr) {
        return nullptr;
    }
    return LookupProcAddress(procname);
}

bool IsDisplayCreated() {
    std::scoped_lock lk{g_egl.mutex};
    return g_egl.display_is_default;
}

GlContext* GetCurrentGlContext() {
    std::scoped_lock lk{g_egl.mutex};
    return g_egl.current_context != nullptr ? g_egl.current_context->gl.get() : nullptr;
}

void RegisterEgl(Core::Loader::SymbolsResolver* sym) {
    PIGLET_EXPORT("Pu5Yd9+FY9Y", eglGetError);
    PIGLET_EXPORT("Z7pNCsK2dAs", eglGetDisplay);
    PIGLET_EXPORT("KJmeNzmA6UM", eglInitialize);
    PIGLET_EXPORT("ConU-nXswn8", eglTerminate);
    PIGLET_EXPORT("mwSvDt0dHF8", eglQueryString);
    PIGLET_EXPORT("xg5Kn1Zto4k", eglGetConfigs);
    PIGLET_EXPORT("fesoNJCZpSA", eglChooseConfig);
    PIGLET_EXPORT("5O92iRi6Hk4", eglGetConfigAttrib);
    PIGLET_EXPORT("PSqt1KLK7o0", eglCreateWindowSurface);
    PIGLET_EXPORT("GGIG9nj2zXk", eglCreatePbufferSurface);
    PIGLET_EXPORT("tcVQcBMJato", eglCreatePixmapSurface);
    PIGLET_EXPORT("9TB8RLYXg+g", eglDestroySurface);
    PIGLET_EXPORT("SR9U4-+eibc", eglQuerySurface);
    PIGLET_EXPORT("yCo3HzKQSts", eglBindAPI);
    PIGLET_EXPORT("GpgGEmVl9Ck", eglQueryAPI);
    PIGLET_EXPORT("Q6OKS0BRf0A", eglWaitClient);
    PIGLET_EXPORT("mNDMTLdRYR8", eglReleaseThread);
    PIGLET_EXPORT("RqcWVDCXZcY", eglCreatePbufferFromClientBuffer);
    PIGLET_EXPORT("0cZxCTXvyPs", eglSurfaceAttrib);
    PIGLET_EXPORT("EdefUaKYEPE", eglBindTexImage);
    PIGLET_EXPORT("UEiVB1s+wBw", eglReleaseTexImage);
    PIGLET_EXPORT("1h8m1KuIR9o", eglSwapInterval);
    PIGLET_EXPORT("0oZKK6QmW-k", eglCreateContext);
    PIGLET_EXPORT("vrBLBaiUr5g", eglDestroyContext);
    PIGLET_EXPORT("D947DWfj9tY", eglMakeCurrent);
    PIGLET_EXPORT("7Nm1o42CbJ8", eglGetCurrentContext);
    PIGLET_EXPORT("LP5RvgCb3AA", eglGetCurrentSurface);
    PIGLET_EXPORT("13Y3oJ7jRHQ", eglGetCurrentDisplay);
    PIGLET_EXPORT("6CQ7RFmBjnQ", eglQueryContext);
    PIGLET_EXPORT("xf-XRFu56-8", eglWaitGL);
    PIGLET_EXPORT("hGmiwXj3cKU", eglWaitNative);
    PIGLET_EXPORT("51YpII41z34", eglSwapBuffers);
    PIGLET_EXPORT("rP9VyAtSJ08", eglCopyBuffers);
    PIGLET_EXPORT("1Ml5nNkeImU", eglGetProcAddress);
}

} // namespace Libraries::Piglet
