// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <limits>
#include <mutex>
#include <new>
#include <string>
#include <system_error>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <fmt/format.h>
#include "common/config.h"
#include "common/logging/log.h"
#include "common/singleton.h"
#include "core/file_sys/fs.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/font/font.h"
#include "core/libraries/libs.h"
#include "font_error.h"

#define STBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION
#include "externals/dear_imgui/imstb_truetype.h"

namespace Libraries::Font {
struct FontLibOpaque;
}

namespace {
Core::FileSys::MntPoints* g_mnt = Common::Singleton<Core::FileSys::MntPoints>::Instance();

struct GlyphEntry {
    std::vector<std::uint8_t> bitmap;
    int w = 0;
    int h = 0;
    int x0 = 0;
    int y0 = 0;
    int x1 = 0;
    int y1 = 0;
    float advance = 0.0f;
    float bearingX = 0.0f;
};

struct FontState {
    float scale_w = 16.0f;
    float scale_h = 16.0f;
    float scale_point_w = 12.0f;
    float scale_point_h = 12.0f;
    bool scale_point_active = false;
    u32 dpi_x = 72;
    u32 dpi_y = 72;
    Libraries::Font::OrbisFontLib library = nullptr;
    bool ext_face_ready = false;
    std::vector<unsigned char> ext_face_data;
    stbtt_fontinfo ext_face{};
    float ext_scale_for_height = 0.0f;
    int ext_ascent = 0, ext_descent = 0, ext_lineGap = 0;
    std::unordered_map<std::uint64_t, GlyphEntry> ext_cache;
    std::vector<std::uint8_t> scratch;
    bool logged_ext_use = false;
    // Cached layout (PS4-style) for GetHorizontalLayout
    float cached_baseline_y = 0.0f;
    bool layout_cached = false;
    // Renderer binding state (PS4 requires a bound renderer for render-scale queries)
    Libraries::Font::OrbisFontRenderer bound_renderer = nullptr;
    // Mark when the game asked for PS4 internal/system fonts (via OpenFontSet)
    bool system_requested = false;
};

static std::unordered_map<Libraries::Font::OrbisFontHandle, FontState> g_font_state;

struct LibraryState {
    bool support_system = false;
    bool support_external = false;
    u32 external_formats = 0;
    u32 external_fontMax = 0;
    const Libraries::Font::OrbisFontMem* backing_memory = nullptr;
    Libraries::Font::OrbisFontLibCreateParams create_params = nullptr;
    u64 edition = 0;
    Libraries::Font::FontLibOpaque* native = nullptr;
};
static std::unordered_map<Libraries::Font::OrbisFontLib, LibraryState> g_library_state;

static std::unordered_map<Libraries::Font::OrbisFontRenderSurface*,
                          const Libraries::Font::OrbisFontStyleFrame*>
    g_style_for_surface;

static FontState& GetState(Libraries::Font::OrbisFontHandle h) {
    return g_font_state[h];
}

static FontState* TryGetState(Libraries::Font::OrbisFontHandle h) {
    if (!h)
        return nullptr;
    auto it = g_font_state.find(h);
    if (it == g_font_state.end())
        return nullptr;
    return &it->second;
}

static LibraryState& GetLibState(Libraries::Font::OrbisFontLib lib) {
    return g_library_state[lib];
}

static void RemoveLibState(Libraries::Font::OrbisFontLib lib) {
    g_library_state.erase(lib);
}

static void LogExternalFormatSupport(u32 formats_mask) {
    LOG_INFO(Lib_Font, "ExternalFormatsMask=0x{:X}", formats_mask);
}

static std::filesystem::path ResolveGuestPath(const char* guest_path) {
    if (!guest_path) {
        return {};
    }
    if (guest_path[0] != '/') {
        return std::filesystem::path(guest_path);
    }
    if (!g_mnt) {
        return {};
    }
    return g_mnt->GetHostPath(guest_path);
}

static bool LoadGuestFileBytes(const std::filesystem::path& host_path,
                               std::vector<unsigned char>& out_bytes) {
    std::ifstream file(host_path, std::ios::binary | std::ios::ate);
    if (!file) {
        return false;
    }
    const std::streamoff size = file.tellg();
    if (size < 0) {
        return false;
    }
    if (size == 0) {
        out_bytes.clear();
        return true;
    }
    if (static_cast<std::uint64_t>(size) > std::numeric_limits<std::size_t>::max()) {
        return false;
    }
    out_bytes.resize(static_cast<std::size_t>(size));
    file.seekg(0, std::ios::beg);
    if (!file.read(reinterpret_cast<char*>(out_bytes.data()),
                   static_cast<std::streamsize>(out_bytes.size()))) {
        out_bytes.clear();
        return false;
    }
    return true;
}

enum class ScaleMode {
    AscenderHeight,
    EmSquare,
};

static ScaleMode GetScaleMode() {
    static int inited = 0;
    static ScaleMode mode = ScaleMode::AscenderHeight;
    if (!inited) {
        inited = 1;
        if (const char* env = std::getenv("SHADPS4_FONT_SCALE_MODE")) {
            std::string v = env;
            for (auto& c : v)
                c = static_cast<char>(std::tolower(c));
            if (v == "em" || v == "emsquare" || v == "em_square") {
                mode = ScaleMode::EmSquare;
            }
        }
        LOG_INFO(Lib_Font, "FontScaleMode: {}",
                 mode == ScaleMode::EmSquare ? "em-square" : "ascender-height");
    }
    return mode;
}

static float GetScaleBias() {
    static int inited = 0;
    static float bias = 1.0f;
    if (!inited) {
        inited = 1;
        if (const char* env = std::getenv("SHADPS4_FONT_SCALE_BIAS")) {
            try {
                bias = std::max(0.01f, std::min(10.0f, std::stof(env)));
            } catch (...) {
                bias = 1.0f;
            }
        }
        if (bias != 1.0f) {
            LOG_INFO(Lib_Font, "FontScaleBias: {}", bias);
        }
    }
    return bias;
}

static float ComputeScale(const stbtt_fontinfo* face, float pixel_h) {
    if (!face)
        return 0.0f;
    float s = 0.0f;
    if (GetScaleMode() == ScaleMode::EmSquare)
        s = stbtt_ScaleForMappingEmToPixels(face, pixel_h);
    else
        s = stbtt_ScaleForPixelHeight(face, pixel_h);
    return s * GetScaleBias();
}

// External face scale: keep legacy ascender-height mapping to avoid regressions
// in titles that ship their own fonts (e.g., CUSA47038). No EM toggle, no bias.
static float ComputeScaleExt(const stbtt_fontinfo* face, float pixel_h) {
    if (!face)
        return 0.0f;
    return stbtt_ScaleForPixelHeight(face, pixel_h);
}

static constexpr float kPointsPerInch = 72.0f;
static constexpr float kScaleEpsilon = 1e-4f;

static float SafeDpiToFloat(u32 dpi) {
    return dpi == 0 ? kPointsPerInch : static_cast<float>(dpi);
}

static float PointsToPixels(float pt, u32 dpi) {
    return pt * SafeDpiToFloat(dpi) / kPointsPerInch;
}

static float PixelsToPoints(float px, u32 dpi) {
    const float dpi_f = SafeDpiToFloat(dpi);
    return px * kPointsPerInch / dpi_f;
}

static void UpdateCachedLayout(FontState& st) {
    if (!st.ext_face_ready) {
        return;
    }
    if (st.ext_scale_for_height == 0.0f)
        st.ext_scale_for_height = ComputeScaleExt(&st.ext_face, st.scale_h);
    int asc = 0, desc = 0, gap = 0;
    stbtt_GetFontVMetrics(&st.ext_face, &asc, &desc, &gap);
    const float scale = st.ext_scale_for_height;
    st.cached_baseline_y = static_cast<float>(asc) * scale;
    st.layout_cached = true;
}

static std::unordered_set<u32> g_logged_pua;

static inline std::uint64_t MakeGlyphKey(u32 code, int pixel_h) {
    return (static_cast<std::uint64_t>(code) << 32) | static_cast<std::uint32_t>(pixel_h);
}
static std::unordered_set<const void*> g_stride_logged;
static inline void LogStrideOnce(const Libraries::Font::OrbisFontRenderSurface* surf) {
    if (!surf)
        return;
    const void* key = static_cast<const void*>(surf);
    if (g_stride_logged.insert(key).second) {
        const int bpp = std::max(1, static_cast<int>(surf->pixelSizeByte));
        const long expected = static_cast<long>(surf->width) * bpp;
        const bool match = (expected == surf->widthByte);
        LOG_INFO(Lib_Font,
                 "StrideCheck: surf={} buf={} width={} height={} pixelSizeByte={} widthByte={} "
                 "expected={} match={}",
                 key, surf->buffer, surf->width, surf->height, bpp, surf->widthByte, expected,
                 match);
    }
}

static std::once_flag g_system_font_once;
static bool g_system_font_available = false;
static std::vector<unsigned char> g_system_font_bytes;
static std::filesystem::path g_system_font_path;

static bool LoadFontFromPath(const std::filesystem::path& path) {
    if (path.empty()) {
        return false;
    }
    if (!LoadGuestFileBytes(path, g_system_font_bytes) || g_system_font_bytes.empty()) {
        return false;
    }
    g_system_font_available = true;
    g_system_font_path = path;
    LOG_INFO(Lib_Font, "SystemFace: fallback font '{}' loaded ({} bytes)",
             g_system_font_path.string(), g_system_font_bytes.size());
    return true;
}

static void LoadSystemFontBlob() {
    const auto configured_path = Config::getSysFontPath();
    if (!configured_path.empty()) {
        const auto resolved = configured_path.is_absolute()
                                  ? configured_path
                                  : std::filesystem::current_path() / configured_path;
        if (LoadFontFromPath(resolved)) {
            return;
        }
    }
    g_system_font_bytes.clear();
    g_system_font_available = false;
    LOG_ERROR(Lib_Font, "SystemFace: configured font '{}' missing; no fallback available",
              configured_path.string());
}

static bool EnsureSystemFontBlob() {
    std::call_once(g_system_font_once, LoadSystemFontBlob);
    return g_system_font_available;
}

static bool AttachSystemFont(FontState& st, Libraries::Font::OrbisFontHandle handle) {
    if (st.ext_face_ready) {
        return true;
    }
    if (!EnsureSystemFontBlob()) {
        return false;
    }
    st.ext_face_data = g_system_font_bytes;
    st.ext_cache.clear();
    st.ext_scale_for_height = 0.0f;
    st.layout_cached = false;
    st.logged_ext_use = true; // skip "external (game font)" log for built-in fallback
    const int offset = stbtt_GetFontOffsetForIndex(st.ext_face_data.data(), 0);
    if (offset < 0) {
        LOG_ERROR(Lib_Font, "SystemFace: invalid font offset in '{}'", g_system_font_path.string());
        st.ext_face_data.clear();
        st.ext_face_ready = false;
        return false;
    }
    if (stbtt_InitFont(&st.ext_face, st.ext_face_data.data(), offset) == 0) {
        LOG_ERROR(Lib_Font, "SystemFace: stbtt_InitFont failed for '{}'",
                  g_system_font_path.string());
        st.ext_face_data.clear();
        st.ext_face_ready = false;
        return false;
    }
    st.ext_face_ready = true;
    LOG_INFO(Lib_Font, "SystemFace: handle={} now uses '{}'", static_cast<const void*>(handle),
             g_system_font_path.string());
    return true;
}

static std::string ReportSystemFaceRequest(FontState& st, Libraries::Font::OrbisFontHandle handle,
                                           bool& attached_out) {
    attached_out = AttachSystemFont(st, handle);
    if (attached_out) {
        st.system_requested = true;
        return {};
    }
    if (!st.system_requested) {
        st.system_requested = true;
        const auto configured = Config::getSysFontPath();
        return fmt::format(
            "SystemFace: handle={} requested internal font but sysFontPath ('{}') could not be "
            "loaded",
            static_cast<const void*>(handle), configured.string());
    }
    return {};
}
} // namespace

namespace Libraries::Font {

struct FontLibOpaque {
    u16 magic;
    u16 reserved0;
    u32 lock_word;
    u32 flags;
    u8 reserved1[0x14];
    void* alloc_ctx;
    void** alloc_vtbl;
    u8 reserved2[0x50];
    void* sys_driver;
    void* fontset_registry;
    u8 reserved3[0x10];
    void* sysfonts_ctx;
    void* external_fonts_ctx;
    u32* device_cache_buf;
};
static_assert(sizeof(FontLibOpaque) == 0xB8, "FontLibOpaque size");
static_assert(offsetof(FontLibOpaque, alloc_ctx) == 0x20, "FontLibOpaque alloc_ctx offset");
static_assert(offsetof(FontLibOpaque, alloc_vtbl) == 0x28, "FontLibOpaque alloc_vtbl offset");
static_assert(offsetof(FontLibOpaque, sys_driver) == 0x80, "FontLibOpaque sys_driver offset");
static_assert(offsetof(FontLibOpaque, fontset_registry) == 0x88,
              "FontLibOpaque fontset_registry offset");
static_assert(offsetof(FontLibOpaque, sysfonts_ctx) == 0xA0, "FontLibOpaque sysfonts_ctx offset");
static_assert(offsetof(FontLibOpaque, external_fonts_ctx) == 0xA8,
              "FontLibOpaque external_fonts_ctx offset");
static_assert(offsetof(FontLibOpaque, device_cache_buf) == 0xB0,
              "FontLibOpaque device_cache_buf offset");
struct OrbisFontRenderer_ {};

static void* g_allocator_vtbl_stub[4] = {};
static std::uint8_t g_sys_driver_stub{};
static std::uint8_t g_fontset_registry_stub{};
static std::uint8_t g_sysfonts_ctx_stub{};
static std::uint8_t g_external_fonts_ctx_stub{};
static u32 g_device_cache_stub{};

s32 PS4_SYSV_ABI sceFontAttachDeviceCacheBuffer(OrbisFontLib library, void* buffer, u32 size) {
    LOG_INFO(Lib_Font, "AttachDeviceCacheBuffer(begin): library={} buffer={} size={}",
             static_cast<const void*>(library), static_cast<const void*>(buffer), size);
    LOG_ERROR(Lib_Font, "(STUBBED) called library={} buffer={} size={}",
              static_cast<const void*>(library), static_cast<const void*>(buffer), size);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontBindRenderer(OrbisFontHandle fontHandle, OrbisFontRenderer renderer) {
    if (!fontHandle || !renderer) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }
    auto& st = GetState(fontHandle);
    st.bound_renderer = renderer;
    LOG_INFO(Lib_Font, "BindRenderer: handle={} renderer={}", static_cast<const void*>(fontHandle),
             static_cast<const void*>(renderer));
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontCharacterGetBidiLevel(OrbisFontTextCharacter* textCharacter,
                                              int* bidiLevel) {
    if (!textCharacter || !bidiLevel) {
        LOG_DEBUG(Lib_Font, "Invalid parameter");
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }

    *bidiLevel = textCharacter->bidiLevel;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontCharacterGetSyllableStringState() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontCharacterGetTextFontCode() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontCharacterGetTextOrder(OrbisFontTextCharacter* textCharacter,
                                              void** pTextOrder) {
    if (!pTextOrder) {
        LOG_DEBUG(Lib_Font, "Invalid parameter");
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }

    if (!textCharacter) {
        LOG_DEBUG(Lib_Font, "Invalid parameter");
        *pTextOrder = NULL;
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }

    *pTextOrder = textCharacter->textOrder;
    return ORBIS_OK;
}

u32 PS4_SYSV_ABI sceFontCharacterLooksFormatCharacters(OrbisFontTextCharacter* textCharacter) {
    if (!textCharacter) {
        return 0;
    }

    return (textCharacter->formatFlags & 0x04) ? textCharacter->characterCode : 0;
}

u32 PS4_SYSV_ABI sceFontCharacterLooksWhiteSpace(OrbisFontTextCharacter* textCharacter) {
    if (!textCharacter) {
        return 0;
    }

    return (textCharacter->charType == 0x0E) ? textCharacter->characterCode : 0;
}

OrbisFontTextCharacter* PS4_SYSV_ABI
sceFontCharacterRefersTextBack(OrbisFontTextCharacter* textCharacter) {
    if (!textCharacter)
        return NULL;

    OrbisFontTextCharacter* current = textCharacter->prev;
    while (current) {
        if (current->unkn_0x31 == 0 && current->unkn_0x33 == 0) {
            return current;
        }
        current = current->prev;
    }

    return NULL;
}

OrbisFontTextCharacter* PS4_SYSV_ABI
sceFontCharacterRefersTextNext(OrbisFontTextCharacter* textCharacter) {
    if (!textCharacter)
        return NULL;

    OrbisFontTextCharacter* current = textCharacter->next;
    while (current) {
        if (current->unkn_0x31 == 0 && current->unkn_0x33 == 0) {
            return current;
        }
        current = current->next;
    }

    return NULL;
}

s32 PS4_SYSV_ABI sceFontCharactersRefersTextCodes() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontClearDeviceCache() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontCloseFont() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontControl() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontCreateGraphicsDevice() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontCreateGraphicsService() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontCreateGraphicsServiceWithEdition() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontCreateLibrary(const OrbisFontMem* memory,
                                      OrbisFontLibCreateParams create_params,
                                      OrbisFontLib* pLibrary) {
    return sceFontCreateLibraryWithEdition(memory, create_params, 0, pLibrary);
}

s32 PS4_SYSV_ABI sceFontCreateLibraryWithEdition(const OrbisFontMem* memory,
                                                 OrbisFontLibCreateParams create_params,
                                                 u64 edition, OrbisFontLib* pLibrary) {
    if (!pLibrary) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }
    if (!memory) {
        return ORBIS_FONT_ERROR_INVALID_MEMORY;
    }
    auto* lib = new (std::nothrow) FontLibOpaque{};
    if (!lib) {
        return ORBIS_FONT_ERROR_ALLOCATION_FAILED;
    }
    lib->magic = 0x0F01;
    lib->lock_word = 0;
    lib->flags = 0;
    lib->alloc_ctx = const_cast<OrbisFontMem*>(memory);
    lib->alloc_vtbl = g_allocator_vtbl_stub;
    lib->sys_driver = &g_sys_driver_stub;
    lib->fontset_registry = &g_fontset_registry_stub;
    lib->sysfonts_ctx = &g_sysfonts_ctx_stub;
    lib->external_fonts_ctx = &g_external_fonts_ctx_stub;
    lib->device_cache_buf = &g_device_cache_stub;
    *pLibrary = lib;
    auto& state = GetLibState(lib);
    state = LibraryState{};
    state.backing_memory = memory;
    state.create_params = create_params;
    state.edition = edition;
    state.native = lib;
    LOG_INFO(Lib_Font, "CreateLibrary: memory={} selection={} edition={} -> lib={}",
             static_cast<const void*>(memory), static_cast<const void*>(create_params), edition,
             static_cast<const void*>(*pLibrary));
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontDestroyLibrary(OrbisFontLib* pLibrary) {
    if (!pLibrary || !*pLibrary) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }
    auto lib = *pLibrary;
    RemoveLibState(lib);
    delete static_cast<FontLibOpaque*>(lib);
    *pLibrary = nullptr;
    LOG_INFO(Lib_Font, "DestroyLibrary: lib={} destroyed", static_cast<const void*>(lib));
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontCreateRenderer(const OrbisFontMem* memory,
                                       OrbisFontRendererCreateParams create_params,
                                       OrbisFontRenderer* pRenderer) {
    return sceFontCreateRendererWithEdition(memory, create_params, 0, pRenderer);
}

s32 PS4_SYSV_ABI sceFontCreateRendererWithEdition(const OrbisFontMem* memory,
                                                  OrbisFontRendererCreateParams create_params,
                                                  u64 edition, OrbisFontRenderer* pRenderer) {
    if (!pRenderer) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }
    if (!memory) {
        return ORBIS_FONT_ERROR_INVALID_MEMORY;
    }
    auto* renderer = new (std::nothrow) OrbisFontRenderer_{};
    if (!renderer) {
        return ORBIS_FONT_ERROR_ALLOCATION_FAILED;
    }
    *pRenderer = renderer;
    LOG_INFO(Lib_Font, "CreateRenderer: memory={} selection={} edition={} renderer={}",
             static_cast<const void*>(memory), static_cast<const void*>(create_params), edition,
             static_cast<const void*>(renderer));
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontCreateString() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontCreateWords() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontCreateWritingLine() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontDefineAttribute() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontDeleteGlyph() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontDestroyGraphicsDevice() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontDestroyGraphicsService() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontDestroyRenderer(OrbisFontRenderer* pRenderer) {
    if (!pRenderer || !*pRenderer) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }
    auto renderer = *pRenderer;
    delete static_cast<OrbisFontRenderer_*>(renderer);
    *pRenderer = nullptr;
    LOG_INFO(Lib_Font, "DestroyRenderer: renderer={} destroyed",
             static_cast<const void*>(renderer));
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontDestroyString() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontDestroyWords() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontDestroyWritingLine() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontDettachDeviceCacheBuffer() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGenerateCharGlyph() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGetAttribute() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGetCharGlyphCode() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGetCharGlyphMetrics(OrbisFontHandle fontHandle, u32 code,
                                            OrbisFontGlyphMetrics* metrics) {
    if (!metrics) {
        LOG_DEBUG(Lib_Font, "sceFontGetCharGlyphMetrics: invalid params");
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }
    auto& st = GetState(fontHandle);
    const stbtt_fontinfo* face = nullptr;
    float scale = 0.0f;

    if (st.ext_face_ready) {
        if (st.ext_scale_for_height == 0.0f)
            st.ext_scale_for_height = stbtt_ScaleForPixelHeight(&st.ext_face, st.scale_h);
        const int glyph = stbtt_FindGlyphIndex(&st.ext_face, static_cast<int>(code));
        if (glyph > 0) {
            face = &st.ext_face;
            scale = st.ext_scale_for_height;
            if (!st.logged_ext_use) {
                LOG_INFO(Lib_Font, "RenderFace: handle={} source=external (game font)",
                         static_cast<const void*>(fontHandle));
                st.logged_ext_use = true;
            }
        }
    }

    if (!face) {
        bool system_attached = false;
        const std::string sys_log = ReportSystemFaceRequest(st, fontHandle, system_attached);
        if (!sys_log.empty()) {
            LOG_ERROR(Lib_Font, "{}", sys_log);
        }
        if (system_attached) {
            face = &st.ext_face;
            if (st.ext_scale_for_height == 0.0f)
                st.ext_scale_for_height = ComputeScaleExt(&st.ext_face, st.scale_h);
            scale = st.ext_scale_for_height;
        }
    }

    if (face) {
        const int pixel_h = std::max(1, (int)std::lround(st.scale_h));
        const std::uint64_t key = MakeGlyphKey(code, pixel_h);
        GlyphEntry* ge = nullptr;
        if (auto it = st.ext_cache.find(key); it != st.ext_cache.end()) {
            ge = &it->second;
        }
        if (!ge) {
            GlyphEntry entry{};
            int aw = 0, lsb = 0;
            stbtt_GetCodepointHMetrics(face, static_cast<int>(code), &aw, &lsb);
            stbtt_GetCodepointBitmapBox(face, static_cast<int>(code), scale, scale, &entry.x0,
                                        &entry.y0, &entry.x1, &entry.y1);
            entry.w = std::max(0, entry.x1 - entry.x0);
            entry.h = std::max(0, entry.y1 - entry.y0);
            entry.advance = static_cast<float>(aw) * scale;
            entry.bearingX = static_cast<float>(lsb) * scale;
            ge = &st.ext_cache.emplace(key, std::move(entry)).first->second;
        }
        metrics->w = ge->w > 0 ? static_cast<float>(ge->w) : st.scale_w;
        metrics->h = ge->h > 0 ? static_cast<float>(ge->h) : st.scale_h;
        metrics->h_bearing_x = ge->bearingX;
        metrics->h_bearing_y = static_cast<float>(-ge->y0);
        metrics->h_adv = ge->advance > 0.0f ? ge->advance : st.scale_w;
        metrics->v_bearing_x = 0.0f;
        metrics->v_bearing_y = 0.0f;
        metrics->v_adv = 0.0f;
        LOG_TRACE(Lib_Font,
                  "GetCharGlyphMetrics: code=U+{:04X} src=external size=({}, {}) adv={} "
                  "bearing=({}, {}) box={}x{}",
                  code, st.scale_w, st.scale_h, metrics->h_adv, metrics->h_bearing_x,
                  metrics->h_bearing_y, metrics->w, metrics->h);
        return ORBIS_OK;
    }
    metrics->w = st.scale_w;
    metrics->h = st.scale_h;
    metrics->h_bearing_x = 0.0f;
    metrics->h_bearing_y = st.scale_h;
    metrics->h_adv = st.scale_w;
    metrics->v_bearing_x = 0.0f;
    metrics->v_bearing_y = 0.0f;
    metrics->v_adv = 0.0f;
    LOG_TRACE(Lib_Font, "GetCharGlyphMetrics(fallback): code=U+{:04X} size=({}, {}) box={}x{}",
              code, st.scale_w, st.scale_h, metrics->w, metrics->h);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGetEffectSlant() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGetEffectWeight() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGetFontGlyphsCount() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGetFontGlyphsOutlineProfile() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGetFontMetrics() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGetFontResolution() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGetFontStyleInformation() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGetGlyphExpandBufferState() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGetHorizontalLayout(OrbisFontHandle fontHandle,
                                            OrbisFontHorizontalLayout* layout) {
    if (!fontHandle || !layout) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }
    auto* st_ptr = TryGetState(fontHandle);
    if (!st_ptr) {
        return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    }
    auto& st = *st_ptr;
    if (!st.ext_face_ready) {
        bool system_attached = false;
        const std::string sys_log = ReportSystemFaceRequest(st, fontHandle, system_attached);
        if (!sys_log.empty()) {
            LOG_ERROR(Lib_Font, "{}", sys_log);
        }
    }
    if (st.ext_face_ready) {
        if (st.ext_scale_for_height == 0.0f)
            st.ext_scale_for_height = stbtt_ScaleForPixelHeight(&st.ext_face, st.scale_h);
        int asc = 0, desc = 0, gap = 0;
        stbtt_GetFontVMetrics(&st.ext_face, &asc, &desc, &gap);
        const float scale = st.ext_scale_for_height;
        layout->baselineOffset = static_cast<float>(asc) * scale;
        layout->lineAdvance = static_cast<float>(asc - desc + gap) * scale;
        layout->decorationExtent = 0.0f;
        LOG_INFO(
            Lib_Font,
            "GetHorizontalLayout: handle={} ext_ready={} baselineOffset={} lineAdvance={} scale={}",
            static_cast<const void*>(fontHandle), st.ext_face_ready, layout->baselineOffset,
            layout->lineAdvance, scale);
        return ORBIS_OK;
    }
    layout->baselineOffset = st.scale_h * 0.8f;
    layout->lineAdvance = st.scale_h;
    layout->decorationExtent = 0.0f;
    LOG_INFO(
        Lib_Font,
        "GetHorizontalLayout: fallback handle={} ext_ready={} baselineOffset={} lineAdvance={}",
        static_cast<const void*>(fontHandle), st.ext_face_ready, layout->baselineOffset,
        layout->lineAdvance);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGetKerning(OrbisFontHandle fontHandle, u32 preCode, u32 code,
                                   OrbisFontKerning* kerning) {
    if (!kerning) {
        LOG_DEBUG(Lib_Font, "sceFontGetKerning: invalid params");
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }
    auto& st = GetState(fontHandle);
    if (!st.ext_face_ready) {
        bool system_attached = false;
        const std::string sys_log = ReportSystemFaceRequest(st, fontHandle, system_attached);
        if (!sys_log.empty()) {
            LOG_ERROR(Lib_Font, "{}", sys_log);
        }
    }
    if (st.ext_face_ready) {
        if (st.ext_scale_for_height == 0.0f)
            st.ext_scale_for_height = stbtt_ScaleForPixelHeight(&st.ext_face, st.scale_h);
        int g1 = stbtt_FindGlyphIndex(&st.ext_face, static_cast<int>(preCode));
        int g2 = stbtt_FindGlyphIndex(&st.ext_face, static_cast<int>(code));
        if (g1 > 0 && g2 > 0) {
            const int kern = stbtt_GetCodepointKernAdvance(&st.ext_face, static_cast<int>(preCode),
                                                           static_cast<int>(code));
            const float kx = static_cast<float>(kern) * st.ext_scale_for_height;
            kerning->dx = kx;
            kerning->dy = 0.0f;
            kerning->px = 0.0f;
            kerning->py = 0.0f;
            LOG_TRACE(Lib_Font, "GetKerning: pre=U+{:04X} code=U+{:04X} dx={}", preCode, code, kx);
            return ORBIS_OK;
        }
    }
    kerning->dx = 0.0f;
    kerning->dy = 0.0f;
    kerning->px = 0.0f;
    kerning->py = 0.0f;
    LOG_TRACE(Lib_Font, "GetKerning: pre=U+{:04X} code=U+{:04X} dx=0 (no face)", preCode, code);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGetLibrary(OrbisFontHandle fontHandle, OrbisFontLib* pLibrary) {
    if (!pLibrary) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }
    const auto& st = GetState(fontHandle);
    *pLibrary = st.library;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGetPixelResolution() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGetRenderCharGlyphMetrics(OrbisFontHandle fontHandle, u32 codepoint,
                                                  OrbisFontGlyphMetrics* out_metrics) {
    if (!fontHandle || !out_metrics) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }
    auto& st = GetState(fontHandle);
    if (!st.bound_renderer) {
        LOG_DEBUG(Lib_Font, "GetRenderCharGlyphMetrics: renderer not bound for handle={}",
                  static_cast<const void*>(fontHandle));
        return ORBIS_FONT_ERROR_NOT_BOUND_RENDERER;
    }
    return sceFontGetCharGlyphMetrics(fontHandle, codepoint, out_metrics);
}

s32 PS4_SYSV_ABI sceFontGetRenderEffectSlant() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGetRenderEffectWeight() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGetRenderScaledKerning() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGetRenderScalePixel(OrbisFontHandle fontHandle, float* out_w,
                                            float* out_h) {
    if (!fontHandle || (!out_w && !out_h)) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }
    auto* st = TryGetState(fontHandle);
    if (!st) {
        return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    }
    if (out_w)
        *out_w = st->scale_w;
    if (out_h)
        *out_h = st->scale_h;
    if (!st->bound_renderer) {
        LOG_DEBUG(Lib_Font,
                  "GetRenderScalePixel: no renderer bound; returning configured scale w={} h={}",
                  out_w ? *out_w : -1.0f, out_h ? *out_h : -1.0f);
    } else {
        LOG_DEBUG(Lib_Font, "GetRenderScalePixel: handle={} -> w={}, h={}",
                  static_cast<const void*>(fontHandle), out_w ? *out_w : -1.0f,
                  out_h ? *out_h : -1.0f);
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGetRenderScalePoint(OrbisFontHandle fontHandle, float* out_w,
                                            float* out_h) {
    return sceFontGetScalePoint(fontHandle, out_w, out_h);
}

s32 PS4_SYSV_ABI sceFontGetResolutionDpi() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGetScalePixel(OrbisFontHandle fontHandle, float* out_w, float* out_h) {
    if (!fontHandle || (!out_w && !out_h)) {
        LOG_DEBUG(Lib_Font, "sceFontGetScalePixel: invalid params");
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }
    auto* st = TryGetState(fontHandle);
    if (!st) {
        return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    }
    if (out_w)
        *out_w = st->scale_w;
    if (out_h)
        *out_h = st->scale_h;
    LOG_DEBUG(Lib_Font, "GetScalePixel: handle={} -> w={}, h={}",
              static_cast<const void*>(fontHandle), out_w ? *out_w : -1.0f, out_h ? *out_h : -1.0f);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGetScalePoint(OrbisFontHandle fontHandle, float* out_w, float* out_h) {
    if (!fontHandle || (!out_w && !out_h)) {
        LOG_DEBUG(Lib_Font, "sceFontGetScalePoint: invalid params");
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }
    auto* st = TryGetState(fontHandle);
    if (!st) {
        return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    }
    const float default_w = PixelsToPoints(st->scale_w, st->dpi_x);
    const float default_h = PixelsToPoints(st->scale_h, st->dpi_y);
    const float point_w = st->scale_point_active ? st->scale_point_w : default_w;
    const float point_h = st->scale_point_active ? st->scale_point_h : default_h;
    if (out_w)
        *out_w = point_w;
    if (out_h)
        *out_h = point_h;
    LOG_DEBUG(Lib_Font, "GetScalePoint: handle={} -> w={}pt h={}pt (active={})",
              static_cast<const void*>(fontHandle), point_w, point_h, st->scale_point_active);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGetScriptLanguage() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGetTypographicDesign() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGetVerticalLayout(OrbisFontHandle fontHandle,
                                          OrbisFontVerticalLayout* layout) {
    if (!layout) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }
    auto& st = GetState(fontHandle);
    if (!st.ext_face_ready) {
        bool system_attached = false;
        const std::string sys_log = ReportSystemFaceRequest(st, fontHandle, system_attached);
        if (!sys_log.empty()) {
            LOG_ERROR(Lib_Font, "{}", sys_log);
        }
    }
    if (st.ext_face_ready) {
        if (st.ext_scale_for_height == 0.0f)
            st.ext_scale_for_height = ComputeScaleExt(&st.ext_face, st.scale_h);
        int asc = 0, desc = 0, gap = 0;
        stbtt_GetFontVMetrics(&st.ext_face, &asc, &desc, &gap);
        layout->baselineOffsetX = 0.0f;
        layout->columnAdvance = static_cast<float>(asc - desc + gap) * st.ext_scale_for_height;
        layout->decorationSpan = 0.0f;
        return ORBIS_OK;
    }
    layout->baselineOffsetX = 0.0f;
    layout->columnAdvance = st.scale_h;
    layout->decorationSpan = 0.0f;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGlyphDefineAttribute() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGlyphGetAttribute() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGlyphGetGlyphForm() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGlyphGetMetricsForm() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGlyphGetScalePixel() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGlyphRefersMetrics() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGlyphRefersMetricsHorizontal() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGlyphRefersMetricsHorizontalAdvance() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGlyphRefersMetricsHorizontalX() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGlyphRefersOutline() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGlyphRenderImage() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGlyphRenderImageHorizontal() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGlyphRenderImageVertical() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsBeginFrame() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsDrawingCancel() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsDrawingFinish() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsEndFrame() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsExchangeResource() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsFillMethodInit() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsFillPlotInit() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsFillPlotSetLayout() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsFillPlotSetMapping() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsFillRatesInit() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsFillRatesSetFillEffect() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsFillRatesSetLayout() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsFillRatesSetMapping() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsGetDeviceUsage() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsRegionInit() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsRegionInitCircular() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsRegionInitRoundish() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsRelease() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsRenderResource() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsSetFramePolicy() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsSetupClipping() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsSetupColorRates() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsSetupFillMethod() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsSetupFillRates() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsSetupGlyphFill() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsSetupGlyphFillPlot() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsSetupHandleDefault() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsSetupLocation() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsSetupPositioning() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsSetupRotation() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsSetupScaling() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsSetupShapeFill() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsSetupShapeFillPlot() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsStructureCanvas() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsStructureCanvasSequence() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsStructureDesign() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsStructureDesignResource() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsStructureSurfaceTexture() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsUpdateClipping() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsUpdateColorRates() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsUpdateFillMethod() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsUpdateFillRates() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsUpdateGlyphFill() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsUpdateGlyphFillPlot() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsUpdateLocation() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsUpdatePositioning() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsUpdateRotation() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsUpdateScaling() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsUpdateShapeFill() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGraphicsUpdateShapeFillPlot() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontMemoryInit(OrbisFontMem* mem_desc, void* region_addr, u32 region_size,
                                   const OrbisFontMemInterface* iface, void* mspace_obj,
                                   OrbisFontMemDestroyCb destroy_cb, void* destroy_ctx) {
    if (!mem_desc) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }

    mem_desc->mem_kind = 0;
    mem_desc->attr_bits = 0;
    mem_desc->region_base = region_addr;
    mem_desc->region_size = region_size;
    mem_desc->iface = iface;
    mem_desc->mspace_handle = mspace_obj;
    mem_desc->on_destroy = destroy_cb;
    mem_desc->destroy_ctx = destroy_ctx;
    mem_desc->some_ctx1 = nullptr;
    mem_desc->some_ctx2 = nullptr;

    LOG_INFO(
        Lib_Font,
        "FontMemoryInit: font_mem={} region_base={} size={} mspace={} mem_if_set={} destroy_cb={}",
        static_cast<const void*>(mem_desc), static_cast<const void*>(region_addr), region_size,
        static_cast<const void*>(mem_desc->mspace_handle), iface != nullptr,
        reinterpret_cast<const void*>(destroy_cb));
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontMemoryTerm(OrbisFontMem* mem_desc) {
    if (!mem_desc) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }
    if (mem_desc->on_destroy) {
        mem_desc->on_destroy(mem_desc, mem_desc->destroy_ctx, mem_desc->some_ctx1);
    }
    std::memset(mem_desc, 0, sizeof(*mem_desc));
    LOG_INFO(Lib_Font, "FontMemoryTerm: font_mem={} cleaned", static_cast<const void*>(mem_desc));
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontOpenFontFile(OrbisFontLib library, const char* guest_path, u32 open_mode,
                                     const OrbisFontOpenParams* open_detail,
                                     OrbisFontHandle* out_handle) {
    if (!library || !guest_path || !out_handle) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }
    const auto host_path = ResolveGuestPath(guest_path);
    const std::filesystem::path path_to_open =
        host_path.empty() ? std::filesystem::path(guest_path) : host_path;

    std::vector<unsigned char> file_bytes;
    if (!LoadGuestFileBytes(path_to_open, file_bytes)) {
        LOG_WARNING(Lib_Font, "OpenFontFile: failed to open '{}'", path_to_open.string());
        return ORBIS_FONT_ERROR_FS_OPEN_FAILED;
    }
    if (file_bytes.size() > std::numeric_limits<u32>::max()) {
        LOG_WARNING(Lib_Font, "OpenFontFile: '{}' exceeds libSceFont size limit ({} bytes)",
                    path_to_open.string(), file_bytes.size());
        return ORBIS_FONT_ERROR_FS_OPEN_FAILED;
    }
    LOG_INFO(Lib_Font, "OpenFontFile: path='{}' size={} openMode={}", path_to_open.string(),
             static_cast<u32>(file_bytes.size()), open_mode);
    return sceFontOpenFontMemory(library, file_bytes.data(), static_cast<u32>(file_bytes.size()),
                                 open_detail, out_handle);
}

s32 PS4_SYSV_ABI sceFontOpenFontInstance(OrbisFontHandle fontHandle, OrbisFontHandle templateFont,
                                         OrbisFontHandle* pFontHandle) {
    if (!templateFont || !pFontHandle) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }
    auto* src_state = TryGetState(templateFont);
    if (!src_state) {
        return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    }
    FontState* base_state = TryGetState(fontHandle);
    if (!base_state && fontHandle) {
        LOG_WARNING(Lib_Font, "OpenFontInstance: base handle={} unknown, falling back to template",
                    static_cast<const void*>(fontHandle));
    }
    auto* new_handle = new (std::nothrow) OrbisFontHandleOpaque{};
    if (!new_handle) {
        LOG_ERROR(Lib_Font, "OpenFontInstance: allocation failed");
        return ORBIS_FONT_ERROR_ALLOCATION_FAILED;
    }
    auto& dst = GetState(new_handle);
    dst = *src_state;
    dst.library = base_state ? base_state->library : src_state->library;
    dst.bound_renderer = base_state ? base_state->bound_renderer : src_state->bound_renderer;
    dst.logged_ext_use = false;
    if (dst.ext_face_ready && !dst.ext_face_data.empty()) {
        const int font_offset = src_state->ext_face.fontstart;
        if (stbtt_InitFont(&dst.ext_face, dst.ext_face_data.data(), font_offset) == 0) {
            LOG_WARNING(Lib_Font,
                        "OpenFontInstance: stbtt_InitFont failed when cloning handle={} -> new={}",
                        static_cast<const void*>(templateFont),
                        static_cast<const void*>(new_handle));
            dst.ext_face_ready = false;
            dst.ext_cache.clear();
        } else {
            dst.ext_scale_for_height = ComputeScaleExt(&dst.ext_face, dst.scale_h);
        }
    }
    dst.layout_cached = false;
    *pFontHandle = new_handle;
    LOG_INFO(Lib_Font,
             "OpenFontInstance: base={} template={} -> new={} ext_ready={} renderer_inherited={}",
             static_cast<const void*>(fontHandle), static_cast<const void*>(templateFont),
             static_cast<const void*>(new_handle), dst.ext_face_ready,
             dst.bound_renderer != nullptr);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontOpenFontMemory(OrbisFontLib library, const void* fontAddress, u32 fontSize,
                                       const OrbisFontOpenParams* open_params,
                                       OrbisFontHandle* pFontHandle) {
    if (!library || !fontAddress || fontSize == 0 || !pFontHandle) {
        LOG_DEBUG(Lib_Font, "sceFontOpenFontMemory: invalid params");
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }
    (void)open_params;
    auto* f = new OrbisFontHandleOpaque{};
    *pFontHandle = f;
    auto& st = GetState(f);
    st.library = library;
    const unsigned char* p = reinterpret_cast<const unsigned char*>(fontAddress);
    auto& ls = GetLibState(library);
    LOG_INFO(Lib_Font,
             "OpenFontMemory: lib={} size={} open_params={} handle={} sig='{}{}{}{}' "
             "ext_supported={} formats=0x{:X}",
             static_cast<const void*>(library), fontSize, static_cast<const void*>(open_params),
             static_cast<const void*>(*pFontHandle), (fontSize >= 1 ? (char)p[0] : '?'),
             (fontSize >= 2 ? (char)p[1] : '?'), (fontSize >= 3 ? (char)p[2] : '?'),
             (fontSize >= 4 ? (char)p[3] : '?'), ls.support_external, ls.external_formats);
    st.ext_face_data.assign(reinterpret_cast<const unsigned char*>(fontAddress),
                            reinterpret_cast<const unsigned char*>(fontAddress) + fontSize);
    int font_count = stbtt_GetNumberOfFonts(st.ext_face_data.data());
    int chosen_index = 0;
    if (font_count > 1) {
        chosen_index = 0;
        if (open_params) {
            chosen_index =
                static_cast<int>(open_params->subfont_index % static_cast<u32>(font_count));
        }
    }
    int offset = stbtt_GetFontOffsetForIndex(st.ext_face_data.data(), chosen_index);
    const unsigned char* d = st.ext_face_data.data();
    const u32 sig32 = (fontSize >= 4)
                          ? (static_cast<u32>(d[0]) << 24) | (static_cast<u32>(d[1]) << 16) |
                                (static_cast<u32>(d[2]) << 8) | static_cast<u32>(d[3])
                          : 0u;
    const bool is_ttc = (font_count > 1);
    const bool is_otf_cff = (sig32 == 0x4F54544Fu);
    const bool is_ttf_sfnt = (sig32 == 0x00010000u) || (sig32 == 0x74727565u);
    const bool is_sfnt_typ1 = (sig32 == 0x74797031u);
    if (is_otf_cff) {
        LOG_WARNING(Lib_Font,
                    "ExternalFace: OTF/CFF detected (OTTO). CFF outlines are not supported;"
                    " handle={} fonts={} requested_index={} -> fallback may occur",
                    static_cast<const void*>(*pFontHandle), font_count, chosen_index);
    }
    if (stbtt_InitFont(&st.ext_face, st.ext_face_data.data(), offset)) {
        st.ext_face_ready = true;
        stbtt_GetFontVMetrics(&st.ext_face, &st.ext_ascent, &st.ext_descent, &st.ext_lineGap);
        st.ext_scale_for_height = stbtt_ScaleForPixelHeight(&st.ext_face, st.scale_h);
        LOG_INFO(Lib_Font,
                 "ExternalFace: handle={} ascent={} descent={} lineGap={} scale={} (data={} bytes)"
                 " fonts={} chosen_index={} ttc={} sig=0x{:08X} kind={}",
                 static_cast<const void*>(*pFontHandle), st.ext_ascent, st.ext_descent,
                 st.ext_lineGap, st.ext_scale_for_height, (int)st.ext_face_data.size(), font_count,
                 chosen_index, is_ttc, sig32,
                 is_otf_cff ? "OTF/CFF (unsupported)"
                            : (is_ttf_sfnt ? "TTF (ready)"
                                           : (is_sfnt_typ1 ? "Type1(sfnt) (stub)" : "unknown")));
        if (is_ttf_sfnt) {
            LOG_INFO(Lib_Font, "ExternalFormat: OpenType-TT (glyf) -> ready");
        } else if (is_otf_cff) {
            LOG_WARNING(Lib_Font, "ExternalFormat: OpenType-CFF -> stub (CFF unsupported)");
        } else if (is_sfnt_typ1) {
            LOG_WARNING(Lib_Font, "ExternalFormat: Type 1 (sfnt wrapper) -> stub");
        }
    } else {
        LOG_WARNING(Lib_Font,
                    "ExternalFace: stbtt_InitFont failed for handle={} size={} fonts={}"
                    " chosen_index={} sig='{}{}{}{}' (OTF/CFF unsupported={})",
                    static_cast<const void*>(*pFontHandle), fontSize, font_count, chosen_index,
                    (fontSize >= 1 ? (char)p[0] : '?'), (fontSize >= 2 ? (char)p[1] : '?'),
                    (fontSize >= 3 ? (char)p[2] : '?'), (fontSize >= 4 ? (char)p[3] : '?'),
                    is_otf_cff);
        if (is_otf_cff) {
            LOG_WARNING(Lib_Font, "Stubbed: CFF outlines not implemented; system font unavailable");
        }
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontOpenFontSet(OrbisFontLib library, u32 fontSetType, u32 openMode,
                                    const OrbisFontOpenParams* open_params,
                                    OrbisFontHandle* pFontHandle) {
    (void)fontSetType;
    (void)openMode;
    (void)open_params;
    if (!pFontHandle) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }
    auto* f = new OrbisFontHandleOpaque{};
    *pFontHandle = f;
    auto& st = GetState(f);
    st.library = library;
    bool system_ok = false;
    const std::string sys_log = ReportSystemFaceRequest(st, f, system_ok);
    if (!sys_log.empty()) {
        LOG_ERROR(Lib_Font, "{}", sys_log);
    }

    LOG_INFO(Lib_Font,
             "OpenFontSet: lib={} fontSetType={} openMode={} open_params={} handle={} (system "
             "available={})",
             static_cast<const void*>(library), fontSetType, openMode,
             static_cast<const void*>(open_params), static_cast<const void*>(*pFontHandle),
             system_ok);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontRebindRenderer(OrbisFontHandle fontHandle) {
    if (!fontHandle) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }
    auto& st = GetState(fontHandle);
    if (!st.bound_renderer) {
        return ORBIS_FONT_ERROR_NOT_BOUND_RENDERER;
    }
    LOG_INFO(Lib_Font, "RebindRenderer: handle={} renderer={}",
             static_cast<const void*>(fontHandle), static_cast<const void*>(st.bound_renderer));
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontRenderCharGlyphImage(OrbisFontHandle fontHandle, u32 code,
                                             OrbisFontRenderSurface* surf, float x, float y,
                                             OrbisFontGlyphMetrics* metrics,
                                             OrbisFontRenderOutput* result) {
    return sceFontRenderCharGlyphImageHorizontal(fontHandle, code, surf, x, y, metrics, result);
}

s32 PS4_SYSV_ABI sceFontRenderCharGlyphImageHorizontal(OrbisFontHandle fontHandle, u32 code,
                                                       OrbisFontRenderSurface* surf, float x,
                                                       float y, OrbisFontGlyphMetrics* metrics,
                                                       OrbisFontRenderOutput* result) {
    LOG_INFO(Lib_Font,
             "RenderGlyph(H): handle={} code=U+{:04X} x={} y={} metrics={} result={} surf={}"
             " buf={} widthByte={} pixelSizeByte={} size={}x{} sc=[{},{}-{}:{}] styleFlag={}",
             static_cast<const void*>(fontHandle), code, x, y, static_cast<const void*>(metrics),
             static_cast<const void*>(result), static_cast<const void*>(surf),
             surf ? static_cast<const void*>(surf->buffer) : nullptr, surf ? surf->widthByte : -1,
             surf ? (int)surf->pixelSizeByte : -1, surf ? surf->width : -1,
             surf ? surf->height : -1, surf ? surf->sc_x0 : 0u, surf ? surf->sc_y0 : 0u,
             surf ? surf->sc_x1 : 0u, surf ? surf->sc_y1 : 0u, surf ? (int)surf->styleFlag : -1);
    if (!surf || !surf->buffer) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }
    LogStrideOnce(surf);
    LOG_DEBUG(Lib_Font,
              "RenderGlyph(H): handle={} code=U+{:04X} x={} y={} surf={} size={}x{} bpp={} "
              "sc=[{},{}-{}:{}]",
              static_cast<const void*>(fontHandle), code, x, y, static_cast<const void*>(surf),
              surf->width, surf->height, (int)surf->pixelSizeByte, surf->sc_x0, surf->sc_y0,
              surf->sc_x1, surf->sc_y1);
    if (result) {
        result->stage = nullptr;
        result->slot.maybe_addr = static_cast<u8*>(surf->buffer);
        result->slot.maybe_rowBytes = static_cast<u32>(std::max(0, surf->widthByte));
        result->slot.maybe_pixelSize = static_cast<u8>(std::max(1, (int)surf->pixelSizeByte));
        result->slot.maybe_pixelFmt = (result->slot.maybe_pixelSize == 4) ? 1 : 0;
    }
    auto& st = GetState(fontHandle);
    float fw = st.scale_w;
    float fh = st.scale_h;
    int g_x0 = 0, g_y0 = 0, g_x1 = 0, g_y1 = 0;
    const stbtt_fontinfo* face = nullptr;
    float scale = 0.0f;

    if (st.ext_face_ready) {
        if (st.ext_scale_for_height == 0.0f)
            st.ext_scale_for_height = stbtt_ScaleForPixelHeight(&st.ext_face, st.scale_h);
        const int glyph_index = stbtt_FindGlyphIndex(&st.ext_face, static_cast<int>(code));
        if (glyph_index > 0) {
            face = &st.ext_face;
            scale = st.ext_scale_for_height;
        }
    }

    if (!face) {
        bool system_attached = false;
        const std::string sys_log = ReportSystemFaceRequest(st, fontHandle, system_attached);
        if (!sys_log.empty()) {
            LOG_ERROR(Lib_Font, "{}", sys_log);
        }
        if (system_attached) {
            face = &st.ext_face;
            if (st.ext_scale_for_height == 0.0f)
                st.ext_scale_for_height = ComputeScaleExt(&st.ext_face, st.scale_h);
            scale = st.ext_scale_for_height;
        }
    }

    const float frac_x = x - std::floor(x);
    const float frac_y = y - std::floor(y);
    const bool use_subpixel = (frac_x != 0.0f) || (frac_y != 0.0f);

    if (face) {
        LOG_DEBUG(Lib_Font, "RenderGlyphSrc(H): handle={} code=U+{:04X} src=external",
                  static_cast<const void*>(fontHandle), code);
        const int pixel_h = std::max(1, (int)std::lround(st.scale_h));
        const std::uint64_t key = MakeGlyphKey(code, pixel_h);
        GlyphEntry* ge = nullptr;
        if (!use_subpixel) {
            if (auto it = st.ext_cache.find(key); it != st.ext_cache.end()) {
                ge = &it->second;
            }
        }
        if (!ge) {
            GlyphEntry entry{};
            int aw = 0, lsb = 0;
            stbtt_GetCodepointHMetrics(face, static_cast<int>(code), &aw, &lsb);
            if (use_subpixel) {
                stbtt_GetCodepointBitmapBoxSubpixel(face, static_cast<int>(code), scale, scale,
                                                    frac_x, frac_y, &entry.x0, &entry.y0, &entry.x1,
                                                    &entry.y1);
            } else {
                stbtt_GetCodepointBitmapBox(face, static_cast<int>(code), scale, scale, &entry.x0,
                                            &entry.y0, &entry.x1, &entry.y1);
            }
            entry.w = std::max(0, entry.x1 - entry.x0);
            entry.h = std::max(0, entry.y1 - entry.y0);
            entry.advance = static_cast<float>(aw) * scale;
            entry.bearingX = static_cast<float>(lsb) * scale;
            if (!use_subpixel && entry.w > 0 && entry.h > 0) {
                entry.bitmap.resize(static_cast<size_t>(entry.w * entry.h));
                stbtt_MakeCodepointBitmap(face, entry.bitmap.data(), entry.w, entry.h, entry.w,
                                          scale, scale, static_cast<int>(code));
                ge = &st.ext_cache.emplace(key, std::move(entry)).first->second;
            } else if (use_subpixel) {
                fw = static_cast<float>(entry.w);
                fh = static_cast<float>(entry.h);
                g_x0 = entry.x0;
                g_y0 = entry.y0;
                g_x1 = entry.x1;
                g_y1 = entry.y1;
            }
        }
        if (ge) {
            fw = static_cast<float>(ge->w);
            fh = static_cast<float>(ge->h);
            g_x0 = ge->x0;
            g_y0 = ge->y0;
            g_x1 = ge->x1;
            g_y1 = ge->y1;
        }
    }
    if (metrics) {
        if (face) {
            int aw = 0, lsb = 0;
            stbtt_GetCodepointHMetrics(face, static_cast<int>(code), &aw, &lsb);
            metrics->w = fw;
            metrics->h = fh;
            metrics->h_bearing_x = static_cast<float>(lsb) * scale;
            metrics->h_bearing_y = static_cast<float>(-g_y0);
            metrics->h_adv = static_cast<float>(aw) * scale;
        } else {
            metrics->w = fw;
            metrics->h = fh;
            metrics->h_bearing_x = 0.0f;
            metrics->h_bearing_y = fh;
            metrics->h_adv = fw;
        }
        metrics->v_bearing_x = 0.0f;
        metrics->v_bearing_y = 0.0f;
        metrics->v_adv = 0.0f;
    }

    if (code == 0x20) {
        if (result) {
            result->new_x = 0;
            result->new_y = 0;
            result->new_w = 0;
            result->new_h = 0;
            result->ImageMetrics = {};
        }
        return ORBIS_OK;
    }

    if (!st.layout_cached) {
        UpdateCachedLayout(st);
    }
    float layout_baseline = st.layout_cached ? st.cached_baseline_y : 0.0f;

    float adjusted_y = y;
    if (face && layout_baseline > 0.0f) {
        const float top_expect = adjusted_y + static_cast<float>(g_y0);
        const float bottom_expect = top_expect + fh;
        if (bottom_expect <= 0.0f || top_expect < 0.0f) {
            adjusted_y += layout_baseline;
        }
    }

    int ix = static_cast<int>(x);
    int iy = static_cast<int>(adjusted_y);
    int left = ix;
    int top = iy;
    if (face) {
        left = ix + g_x0;
        top = iy + g_y0;
    }
    int iw = std::max(0, static_cast<int>(std::round(fw)));
    int ih = std::max(0, static_cast<int>(std::round(fh)));

    int sw = surf->width;
    int sh = surf->height;
    if (sw <= 0 || sh <= 0 || iw <= 0 || ih <= 0) {
        return ORBIS_OK;
    }

    int sx0 = 0, sy0 = 0, sx1 = sw, sy1 = sh;
    if (surf->sc_x1 > 0 || surf->sc_y1 > 0 || surf->sc_x0 > 0 || surf->sc_y0 > 0) {
        sx0 = static_cast<int>(surf->sc_x0);
        sy0 = static_cast<int>(surf->sc_y0);
        sx1 = static_cast<int>(surf->sc_x1);
        sy1 = static_cast<int>(surf->sc_y1);
        sx0 = std::clamp(sx0, 0, sw);
        sy0 = std::clamp(sy0, 0, sh);
        sx1 = std::clamp(sx1, 0, sw);
        sy1 = std::clamp(sy1, 0, sh);
    }

    int rx0 = left;
    int ry0 = top;
    int rx1 = left + iw;
    int ry1 = top + ih;

    rx0 = std::clamp(rx0, sx0, sx1);
    ry0 = std::clamp(ry0, sy0, sy1);
    rx1 = std::clamp(rx1, sx0, sx1);
    ry1 = std::clamp(ry1, sy0, sy1);

    int cw = std::max(0, rx1 - rx0);
    int ch = std::max(0, ry1 - ry0);
    if (ch == 0 && face) {
        LOG_DEBUG(Lib_Font,
                  "RenderGlyph(H): zero-height glyph code=U+{:04X} top={} ih={} g_y0={} g_y1={}"
                  " y={}, g_adv={} scale={} src=external",
                  code, top, ih, g_y0, g_y1, y, metrics ? metrics->h_adv : -1.0f, scale);
    }

    if (cw > 0 && ch > 0) {
        const int bpp = std::max(1, static_cast<int>(surf->pixelSizeByte));
        bool is_pua = (code >= 0xE000 && code <= 0xF8FF) || (code >= 0xF0000 && code <= 0xFFFFD) ||
                      (code >= 0x100000 && code <= 0x10FFFD);
        bool is_placeholder_ascii = (code == 'h' || code == 'p' || code == 'x' || code == 'o');
        if ((is_pua || is_placeholder_ascii) && g_logged_pua.insert(code).second) {
            LOG_DEBUG(
                Lib_Font,
                "GlyphTrace: code=U+{:04X} pua={} placeholder_ascii={} dst=[{},{} {}x{}] bpp={}",
                code, is_pua, is_placeholder_ascii, rx0, ry0, cw, ch, bpp);
        }
        if (face && iw > 0 && ih > 0) {
            const int pixel_h = std::max(1, (int)std::lround(st.scale_h));
            const std::uint64_t key = MakeGlyphKey(code, pixel_h);
            const GlyphEntry* ge = nullptr;
            if (auto it = st.ext_cache.find(key); it != st.ext_cache.end()) {
                ge = &it->second;
            }
            const float frac_x = x - std::floor(x);
            const float frac_y = y - std::floor(y);
            const bool use_subpixel = (frac_x != 0.0f) || (frac_y != 0.0f);
            const std::uint8_t* src_bitmap = nullptr;
            if (!use_subpixel && ge && ge->w == iw && ge->h == ih && !ge->bitmap.empty()) {
                src_bitmap = ge->bitmap.data();
            } else {
                st.scratch.assign(static_cast<size_t>(iw * ih), 0);
                if (use_subpixel) {
                    stbtt_MakeCodepointBitmapSubpixel(face, st.scratch.data(), iw, ih, iw, scale,
                                                      scale, frac_x, frac_y,
                                                      static_cast<int>(code));
                } else {
                    stbtt_MakeCodepointBitmap(face, st.scratch.data(), iw, ih, iw, scale, scale,
                                              static_cast<int>(code));
                }
                src_bitmap = st.scratch.data();
            }

            const std::uint8_t* eff_src = src_bitmap;

            const int src_x0 = std::max(0, sx0 - rx0);
            const int src_y0 = std::max(0, sy0 - ry0);
            const int dst_x0 = std::max(rx0, sx0);
            const int dst_y0 = std::max(ry0, sy0);
            const int copy_w = std::min(rx1, sx1) - dst_x0;
            const int copy_h = std::min(ry1, sy1) - dst_y0;
            if (copy_w > 0 && copy_h > 0) {
                LOG_TRACE(Lib_Font,
                          "RenderGlyph(H): code=U+{:04X} baseline=({}, {}) box=[{},{}-{}:{}] "
                          "dst=[{},{} {}x{}] bpp={}",
                          code, ix, iy, g_x0, g_y0, g_x1, g_y1, dst_x0, dst_y0, copy_w, copy_h,
                          bpp);
                for (int row = 0; row < copy_h; ++row) {
                    const int src_y = src_y0 + row;
                    const std::uint8_t* src_row = eff_src + src_y * iw + src_x0;
                    int row_shift = 0;
                    int row_dst_x = dst_x0 + row_shift;
                    int row_copy_w = copy_w;
                    if (row_dst_x < sx0) {
                        int delta = sx0 - row_dst_x;
                        row_dst_x = sx0;
                        if (delta < row_copy_w) {
                            src_row += delta;
                            row_copy_w -= delta;
                        } else {
                            row_copy_w = 0;
                        }
                    }
                    if (row_dst_x + row_copy_w > sx1) {
                        row_copy_w = std::max(0, sx1 - row_dst_x);
                    }
                    std::uint8_t* dst_row = static_cast<std::uint8_t*>(surf->buffer) +
                                            (dst_y0 + row) * surf->widthByte + row_dst_x * bpp;
                    if (bpp == 1) {
                        for (int col = 0; col < row_copy_w; ++col) {
                            std::uint8_t cov = src_row[col];
                            std::uint8_t& d = dst_row[col];
                            d = std::max(d, cov);
                        }
                    } else if (bpp == 4) {
                        std::uint32_t* px = reinterpret_cast<std::uint32_t*>(dst_row);
                        for (int col = 0; col < row_copy_w; ++col) {
                            std::uint8_t a = src_row[col];
                            px[col] = (static_cast<std::uint32_t>(a) << 24) | 0x00FFFFFFu;
                        }
                    } else {
                        for (int col = 0; col < row_copy_w; ++col) {
                            std::uint8_t a = src_row[col];
                            std::uint8_t* p = dst_row + col * bpp;
                            std::memset(p, 0xFF, static_cast<size_t>(bpp));
                            p[0] = a;
                        }
                    }
                }
            }
        } else {
            if (result) {
                result->new_x = 0;
                result->new_y = 0;
                result->new_w = 0;
                result->new_h = 0;
                result->ImageMetrics.width = 0;
                result->ImageMetrics.height = 0;
            }
            LOG_DEBUG(Lib_Font, "RenderGlyph(H): skip draw (no face/zero size) code=U+{:04X}",
                      code);
            return ORBIS_OK;
        }
    }

    if (result) {
        result->new_x = static_cast<u32>(rx0);
        result->new_y = static_cast<u32>(ry0);
        result->new_w = static_cast<u32>(cw);
        result->new_h = static_cast<u32>(ch);
        const float stride_bytes = static_cast<float>(std::max(0, surf->widthByte));
        result->ImageMetrics.bearing_x = static_cast<float>(g_x0);
        result->ImageMetrics.bearing_y = static_cast<float>(-g_y0);
        float adv_px = 0.0f;
        if (face) {
            int aw_tmp = 0, lsb_tmp = 0;
            stbtt_GetCodepointHMetrics(face, static_cast<int>(code), &aw_tmp, &lsb_tmp);
            adv_px = static_cast<float>(aw_tmp) * scale;
        }
        result->ImageMetrics.dv = adv_px;
        result->ImageMetrics.stride = stride_bytes;
        result->ImageMetrics.width = static_cast<u32>(cw);
        result->ImageMetrics.height = static_cast<u32>(ch);
        LOG_DEBUG(Lib_Font, "RenderGlyph(H): UpdateRect=[{},{} {}x{}] stride={} adv={}",
                  result->new_x, result->new_y, result->new_w, result->new_h,
                  result->ImageMetrics.stride, result->ImageMetrics.dv);
    }
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontRenderCharGlyphImageVertical() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontRendererGetOutlineBufferSize() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontRendererResetOutlineBuffer() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontRendererSetOutlineBufferPolicy() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

void PS4_SYSV_ABI sceFontRenderSurfaceInit(OrbisFontRenderSurface* renderSurface, void* buffer,
                                           int bufWidthByte, int pixelSizeByte, int widthPixel,
                                           int heightPixel) {
    if (renderSurface) {
        renderSurface->buffer = buffer;
        renderSurface->widthByte = bufWidthByte;
        renderSurface->pixelSizeByte = static_cast<int8_t>(pixelSizeByte);
        renderSurface->pad0 = 0;
        renderSurface->styleFlag = 0;
        renderSurface->pad2 = 0;
        renderSurface->width = (widthPixel < 0) ? 0 : widthPixel;
        renderSurface->height = (heightPixel < 0) ? 0 : heightPixel;
        renderSurface->sc_x0 = 0;
        renderSurface->sc_y0 = 0;
        renderSurface->sc_x1 = static_cast<u32>(renderSurface->width);
        renderSurface->sc_y1 = static_cast<u32>(renderSurface->height);
        std::fill(std::begin(renderSurface->reserved_q), std::end(renderSurface->reserved_q), 0);
        LOG_INFO(Lib_Font,
                 "RenderSurfaceInit: buf={} widthByte={} pixelSizeByte={} size={}x{} "
                 "scissor=[0,0-{}:{}]",
                 static_cast<const void*>(buffer), bufWidthByte, pixelSizeByte,
                 renderSurface->width, renderSurface->height, renderSurface->sc_x1,
                 renderSurface->sc_y1);
    }
}

void PS4_SYSV_ABI sceFontRenderSurfaceSetScissor(OrbisFontRenderSurface* renderSurface, int x0,
                                                 int y0, int w, int h) {
    if (!renderSurface)
        return;

    int surfaceWidth = renderSurface->width;
    int clip_x0, clip_x1;

    if (surfaceWidth != 0) {
        if (x0 < 0) {
            clip_x0 = 0;
            clip_x1 = (w + x0 > surfaceWidth) ? surfaceWidth : w + x0;
            if (w <= -x0)
                clip_x1 = 0;
        } else {
            clip_x0 = (x0 > surfaceWidth) ? surfaceWidth : x0;
            clip_x1 = (w + x0 > surfaceWidth) ? surfaceWidth : w + x0;
        }
        renderSurface->sc_x0 = static_cast<u32>(clip_x0);
        renderSurface->sc_x1 = static_cast<u32>(clip_x1);
    }

    int surfaceHeight = renderSurface->height;
    int clip_y0, clip_y1;

    if (surfaceHeight != 0) {
        if (y0 < 0) {
            clip_y0 = 0;
            clip_y1 = (h + y0 > surfaceHeight) ? surfaceHeight : h + y0;
            if (h <= -y0)
                clip_y1 = 0;
        } else {
            clip_y0 = (y0 > surfaceHeight) ? surfaceHeight : y0;
            clip_y1 = (h + y0 > surfaceHeight) ? surfaceHeight : h + y0;
        }
        renderSurface->sc_y0 = static_cast<u32>(clip_y0);
        renderSurface->sc_y1 = static_cast<u32>(clip_y1);
    }
    LOG_INFO(Lib_Font, "RenderSurfaceSetScissor: [{},{}-{}:{}]", renderSurface->sc_x0,
             renderSurface->sc_y0, renderSurface->sc_x1, renderSurface->sc_y1);
}

s32 PS4_SYSV_ABI sceFontRenderSurfaceSetStyleFrame(OrbisFontRenderSurface* renderSurface,
                                                   OrbisFontStyleFrame* styleFrame) {
    if (!renderSurface)
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    g_style_for_surface[renderSurface] = styleFrame;
    renderSurface->styleFlag |= 0x1;
    renderSurface->reserved_q[0] = reinterpret_cast<u64>(styleFrame);
    LOG_INFO(Lib_Font, "RenderSurfaceSetStyleFrame: surf={} styleFrame={}",
             static_cast<const void*>(renderSurface), static_cast<const void*>(styleFrame));
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontSetEffectSlant() {
    LOG_DEBUG(Lib_Font, "SetEffectSlant: no-op (effects not implemented)");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontSetEffectWeight() {
    LOG_DEBUG(Lib_Font, "SetEffectWeight: no-op (effects not implemented)");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontSetFontsOpenMode() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontSetResolutionDpi(OrbisFontHandle fontHandle, u32 h_dpi, u32 v_dpi) {
    if (!fontHandle) {
        return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    }
    auto* st_ptr = TryGetState(fontHandle);
    if (!st_ptr) {
        return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    }
    auto& st = *st_ptr;
    const u32 new_h = h_dpi == 0 ? 0x48u : h_dpi;
    const u32 new_v = v_dpi == 0 ? 0x48u : v_dpi;
    if (st.dpi_x == new_h && st.dpi_y == new_v) {
        LOG_TRACE(Lib_Font, "SetResolutionDpi: handle={} unchanged h_dpi={} v_dpi={}",
                  static_cast<const void*>(fontHandle), new_h, new_v);
        return ORBIS_OK;
    }
    st.dpi_x = new_h;
    st.dpi_y = new_v;
    st.layout_cached = false; // PS4 clears cached metrics when the resolution changes.
    LOG_INFO(Lib_Font, "SetResolutionDpi: handle={} h_dpi={} v_dpi={}",
             static_cast<const void*>(fontHandle), new_h, new_v);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontSetScalePixel(OrbisFontHandle fontHandle, float w, float h) {
    if (!fontHandle || w <= 0.0f || h <= 0.0f) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }
    auto* st_ptr = TryGetState(fontHandle);
    if (!st_ptr) {
        return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    }
    auto& st = *st_ptr;
    st.scale_point_active = false;
    st.scale_w = w;
    st.scale_h = h;
    if (st.ext_face_ready)
        st.ext_scale_for_height = ComputeScaleExt(&st.ext_face, st.scale_h);
    bool system_attached = false;
    const std::string sys_log = ReportSystemFaceRequest(st, fontHandle, system_attached);
    if (!sys_log.empty()) {
        LOG_ERROR(Lib_Font, "{}", sys_log);
    }
    LOG_INFO(Lib_Font, "SetScalePixel: handle={} w={} h={} ext_scale={} ext_ready={}",
             static_cast<const void*>(fontHandle), w, h, st.ext_scale_for_height,
             st.ext_face_ready);
    st.layout_cached = false;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontSetScalePoint(OrbisFontHandle fontHandle, float w, float h) {
    if (!fontHandle || w <= 0.0f || h <= 0.0f) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }
    auto* st_ptr = TryGetState(fontHandle);
    if (!st_ptr) {
        return ORBIS_FONT_ERROR_INVALID_FONT_HANDLE;
    }
    auto& st = *st_ptr;
    const float pixel_w = std::max(0.01f, PointsToPixels(w, st.dpi_x));
    const float pixel_h = std::max(0.01f, PointsToPixels(h, st.dpi_y));
    const bool unchanged_point = st.scale_point_active &&
                                 std::abs(st.scale_point_w - w) < kScaleEpsilon &&
                                 std::abs(st.scale_point_h - h) < kScaleEpsilon;
    const bool unchanged_pixels = std::abs(st.scale_w - pixel_w) < kScaleEpsilon &&
                                  std::abs(st.scale_h - pixel_h) < kScaleEpsilon;
    if (unchanged_point && unchanged_pixels) {
        LOG_TRACE(Lib_Font, "SetScalePoint: handle={} unchanged point=({}, {})",
                  static_cast<const void*>(fontHandle), w, h);
        return ORBIS_OK;
    }
    st.scale_point_active = true;
    st.scale_point_w = w;
    st.scale_point_h = h;
    st.scale_w = pixel_w;
    st.scale_h = pixel_h;
    if (st.ext_face_ready)
        st.ext_scale_for_height = ComputeScaleExt(&st.ext_face, st.scale_h);
    bool system_attached = false;
    const std::string sys_log = ReportSystemFaceRequest(st, fontHandle, system_attached);
    if (!sys_log.empty()) {
        LOG_ERROR(Lib_Font, "{}", sys_log);
    }
    st.layout_cached = false;
    LOG_INFO(Lib_Font,
             "SetScalePoint: handle={} point=({}, {}) -> pixel=({}, {}) dpi=({}, {}) attached={}",
             static_cast<const void*>(fontHandle), w, h, st.scale_w, st.scale_h, st.dpi_x, st.dpi_y,
             system_attached);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontSetScriptLanguage() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontSetTypographicDesign() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontSetupRenderEffectSlant() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontSetupRenderEffectWeight() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontSetupRenderScalePixel(OrbisFontHandle fontHandle, float w, float h) {
    auto rc = sceFontSetScalePixel(fontHandle, w, h);
    LOG_INFO(Lib_Font, "SetupRenderScalePixel: handle={} w={} h={}",
             static_cast<const void*>(fontHandle), w, h);
    return rc;
}

s32 PS4_SYSV_ABI sceFontSetupRenderScalePoint(OrbisFontHandle fontHandle, float w, float h) {
    auto rc = sceFontSetScalePoint(fontHandle, w, h);
    LOG_INFO(Lib_Font, "SetupRenderScalePoint: handle={} w={} h={}",
             static_cast<const void*>(fontHandle), w, h);
    return rc;
}

s32 PS4_SYSV_ABI sceFontStringGetTerminateCode() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontStringGetTerminateOrder() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontStringGetWritingForm() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontStringRefersRenderCharacters() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontStringRefersTextCharacters() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontStyleFrameGetEffectSlant(OrbisFontStyleFrame* styleFrame,
                                                 float* slantRatio) {
    if (!styleFrame || !slantRatio)
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    *slantRatio = 0.0f;
    LOG_DEBUG(Lib_Font, "StyleFrameGetEffectSlant: frame={} slant={} (opaque)",
              static_cast<const void*>(styleFrame), *slantRatio);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontStyleFrameGetEffectWeight(OrbisFontStyleFrame* fontStyleFrame,
                                                  float* weightXScale, float* weightYScale,
                                                  uint32_t* mode) {
    if (!fontStyleFrame)
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    if (weightXScale)
        *weightXScale = 1.0f;
    if (weightYScale)
        *weightYScale = 1.0f;
    if (mode)
        *mode = 0;
    LOG_DEBUG(Lib_Font, "StyleFrameGetEffectWeight: frame={} weight=({}, {}) mode={} (opaque)",
              static_cast<const void*>(fontStyleFrame), weightXScale ? *weightXScale : -1.0f,
              weightYScale ? *weightYScale : -1.0f, mode ? *mode : 0u);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontStyleFrameGetResolutionDpi() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontStyleFrameGetScalePixel(OrbisFontStyleFrame* styleFrame, float* w,
                                                float* h) {
    if (!styleFrame)
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    if (w)
        *w = 0.0f;
    if (h)
        *h = 0.0f;
    LOG_DEBUG(Lib_Font, "StyleFrameGetScalePixel: frame={} -> w={}, h={} (opaque)",
              static_cast<const void*>(styleFrame), w ? *w : 0.0f, h ? *h : 0.0f);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontStyleFrameGetScalePoint() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontStyleFrameInit() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontStyleFrameSetEffectSlant() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontStyleFrameSetEffectWeight() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontStyleFrameSetResolutionDpi() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontStyleFrameSetScalePixel() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontStyleFrameSetScalePoint() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontStyleFrameUnsetEffectSlant() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontStyleFrameUnsetEffectWeight() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontStyleFrameUnsetScale() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontSupportExternalFonts(OrbisFontLib library, u32 fontMax, u32 formats) {
    LOG_INFO(Lib_Font, "SupportExternalFonts(begin): lib={} fontMax={} formats=0x{:X}",
             static_cast<const void*>(library), fontMax, formats);
    auto& ls = GetLibState(library);
    ls.support_external = true;
    ls.external_fontMax = fontMax;
    ls.external_formats = formats;
    LOG_INFO(Lib_Font, "SupportExternalFonts: lib={} fontMax={} formats=0x{:X}",
             static_cast<const void*>(library), fontMax, formats);
    LogExternalFormatSupport(formats);
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontSupportGlyphs() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontSupportSystemFonts(OrbisFontLib library) {
    LOG_INFO(Lib_Font, "SupportSystemFonts(begin): lib={}", static_cast<const void*>(library));
    auto& ls = GetLibState(library);
    ls.support_system = true;
    LOG_INFO(Lib_Font, "SupportSystemFonts: lib={} system=on", static_cast<const void*>(library));
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontTextCodesStepBack() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontTextCodesStepNext() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontTextSourceInit() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontTextSourceRewind() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontTextSourceSetDefaultFont() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontTextSourceSetWritingForm() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontUnbindRenderer(OrbisFontHandle fontHandle) {
    LOG_DEBUG(Lib_Font, "sceFontUnbindRenderer fontHandle={}",
              static_cast<const void*>(fontHandle));
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontWordsFindWordCharacters() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontWritingGetRenderMetrics() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontWritingInit() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontWritingLineClear() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontWritingLineGetOrderingSpace() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontWritingLineGetRenderMetrics() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontWritingLineRefersRenderStep() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontWritingLineWritesOrder() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontWritingRefersRenderStep() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontWritingRefersRenderStepCharacter() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontWritingSetMaskInvisible() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_00F4D778F1C88CB3() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_03C650025FBB0DE7() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_07EAB8A163B27E1A() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_09408E88E4F97CE3() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_09F92905ED82A814() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_0D142CEE1AB21ABE() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_14BD2E9E119C16F2() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_1AC53C9EDEAE8D75() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_1D401185D5E24C3D() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_1E83CD20C2CC996F() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_314B1F765B9FE78A() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_350E6725FEDE29E1() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_3DB773F0A604BF39() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_4FF49DD21E311B1C() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_526287664A493981() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_55CA718DBC84A6E9() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_563FC5F0706A8B4D() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_569E2ECD34290F45() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_5A04775B6BE47685() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_5FD93BCAB6F79750() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_62B5398F864BD3B4() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_6F9010294D822367() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_7757E947423A7A67() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_7E06BA52077F54FA() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_93B36DEA021311D6() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_94B0891E7111598A() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_9785C9128C2FE7CD() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_97DFBC9B65FBC0E1() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_ACD9717405D7D3CA() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_B19A8AEC3FD4F16F() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_C10F488AD7CF103D() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_D0C8B5FF4A6826C7() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_E48D3CD01C342A33() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_EAC96B2186B71E14() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_FE4788A96EF46256() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI Func_FE7E5AE95D3058F5() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

void RegisterlibSceFont(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("CUKn5pX-NVY", "libSceFont", 1, "libSceFont", sceFontAttachDeviceCacheBuffer);
    LIB_FUNCTION("3OdRkSjOcog", "libSceFont", 1, "libSceFont", sceFontBindRenderer);
    LIB_FUNCTION("6DFUkCwQLa8", "libSceFont", 1, "libSceFont", sceFontCharacterGetBidiLevel);
    LIB_FUNCTION("coCrV6IWplE", "libSceFont", 1, "libSceFont",
                 sceFontCharacterGetSyllableStringState);
    LIB_FUNCTION("zN3+nuA0SFQ", "libSceFont", 1, "libSceFont", sceFontCharacterGetTextFontCode);
    LIB_FUNCTION("mxgmMj-Mq-o", "libSceFont", 1, "libSceFont", sceFontCharacterGetTextOrder);
    LIB_FUNCTION("-P6X35Rq2-E", "libSceFont", 1, "libSceFont",
                 sceFontCharacterLooksFormatCharacters);
    LIB_FUNCTION("SaRlqtqaCew", "libSceFont", 1, "libSceFont", sceFontCharacterLooksWhiteSpace);
    LIB_FUNCTION("6Gqlv5KdTbU", "libSceFont", 1, "libSceFont", sceFontCharacterRefersTextBack);
    LIB_FUNCTION("BkjBP+YC19w", "libSceFont", 1, "libSceFont", sceFontCharacterRefersTextNext);
    LIB_FUNCTION("lVSR5ftvNag", "libSceFont", 1, "libSceFont", sceFontCharactersRefersTextCodes);
    LIB_FUNCTION("I9R5VC6eZWo", "libSceFont", 1, "libSceFont", sceFontClearDeviceCache);
    LIB_FUNCTION("vzHs3C8lWJk", "libSceFont", 1, "libSceFont", sceFontCloseFont);
    LIB_FUNCTION("MpKSBaYKluo", "libSceFont", 1, "libSceFont", sceFontControl);
    LIB_FUNCTION("WBNBaj9XiJU", "libSceFont", 1, "libSceFont", sceFontCreateGraphicsDevice);
    LIB_FUNCTION("4So0MC3oBIM", "libSceFont", 1, "libSceFont", sceFontCreateGraphicsService);
    LIB_FUNCTION("NlO5Qlhjkng", "libSceFont", 1, "libSceFont",
                 sceFontCreateGraphicsServiceWithEdition);
    LIB_FUNCTION("nWrfPI4Okmg", "libSceFont", 1, "libSceFont", sceFontCreateLibrary);
    LIB_FUNCTION("n590hj5Oe-k", "libSceFont", 1, "libSceFont", sceFontCreateLibraryWithEdition);
    LIB_FUNCTION("u5fZd3KZcs0", "libSceFont", 1, "libSceFont", sceFontCreateRenderer);
    LIB_FUNCTION("WaSFJoRWXaI", "libSceFont", 1, "libSceFont", sceFontCreateRendererWithEdition);
    LIB_FUNCTION("MO24vDhmS4E", "libSceFont", 1, "libSceFont", sceFontCreateString);
    LIB_FUNCTION("cYrMGk1wrMA", "libSceFont", 1, "libSceFont", sceFontCreateWords);
    LIB_FUNCTION("7rogx92EEyc", "libSceFont", 1, "libSceFont", sceFontCreateWritingLine);
    LIB_FUNCTION("8h-SOB-asgk", "libSceFont", 1, "libSceFont", sceFontDefineAttribute);
    LIB_FUNCTION("LHDoRWVFGqk", "libSceFont", 1, "libSceFont", sceFontDeleteGlyph);
    LIB_FUNCTION("5QG71IjgOpQ", "libSceFont", 1, "libSceFont", sceFontDestroyGraphicsDevice);
    LIB_FUNCTION("zZQD3EwJo3c", "libSceFont", 1, "libSceFont", sceFontDestroyGraphicsService);
    LIB_FUNCTION("FXP359ygujs", "libSceFont", 1, "libSceFont", sceFontDestroyLibrary);
    LIB_FUNCTION("exAxkyVLt0s", "libSceFont", 1, "libSceFont", sceFontDestroyRenderer);
    LIB_FUNCTION("SSCaczu2aMQ", "libSceFont", 1, "libSceFont", sceFontDestroyString);
    LIB_FUNCTION("hWE4AwNixqY", "libSceFont", 1, "libSceFont", sceFontDestroyWords);
    LIB_FUNCTION("PEjv7CVDRYs", "libSceFont", 1, "libSceFont", sceFontDestroyWritingLine);
    LIB_FUNCTION("UuY-OJF+f0k", "libSceFont", 1, "libSceFont", sceFontDettachDeviceCacheBuffer);
    LIB_FUNCTION("C-4Qw5Srlyw", "libSceFont", 1, "libSceFont", sceFontGenerateCharGlyph);
    LIB_FUNCTION("5kx49CAlO-M", "libSceFont", 1, "libSceFont", sceFontGetAttribute);
    LIB_FUNCTION("OINC0X9HGBY", "libSceFont", 1, "libSceFont", sceFontGetCharGlyphCode);
    LIB_FUNCTION("L97d+3OgMlE", "libSceFont", 1, "libSceFont", sceFontGetCharGlyphMetrics);
    LIB_FUNCTION("ynSqYL8VpoA", "libSceFont", 1, "libSceFont", sceFontGetEffectSlant);
    LIB_FUNCTION("d7dDgRY+Bzw", "libSceFont", 1, "libSceFont", sceFontGetEffectWeight);
    LIB_FUNCTION("ZB8xRemRRG8", "libSceFont", 1, "libSceFont", sceFontGetFontGlyphsCount);
    LIB_FUNCTION("4X14YSK4Ldk", "libSceFont", 1, "libSceFont", sceFontGetFontGlyphsOutlineProfile);
    LIB_FUNCTION("eb9S3zNlV5o", "libSceFont", 1, "libSceFont", sceFontGetFontMetrics);
    LIB_FUNCTION("tiIlroGki+g", "libSceFont", 1, "libSceFont", sceFontGetFontResolution);
    LIB_FUNCTION("3hVv3SNoL6E", "libSceFont", 1, "libSceFont", sceFontGetFontStyleInformation);
    LIB_FUNCTION("gVQpMBuB7fE", "libSceFont", 1, "libSceFont", sceFontGetGlyphExpandBufferState);
    LIB_FUNCTION("imxVx8lm+KM", "libSceFont", 1, "libSceFont", sceFontGetHorizontalLayout);
    LIB_FUNCTION("sDuhHGNhHvE", "libSceFont", 1, "libSceFont", sceFontGetKerning);
    LIB_FUNCTION("LzmHDnlcwfQ", "libSceFont", 1, "libSceFont", sceFontGetLibrary);
    LIB_FUNCTION("BozJej5T6fs", "libSceFont", 1, "libSceFont", sceFontGetPixelResolution);
    LIB_FUNCTION("IQtleGLL5pQ", "libSceFont", 1, "libSceFont", sceFontGetRenderCharGlyphMetrics);
    LIB_FUNCTION("Gqa5Pp7y4MU", "libSceFont", 1, "libSceFont", sceFontGetRenderEffectSlant);
    LIB_FUNCTION("woOjHrkjIYg", "libSceFont", 1, "libSceFont", sceFontGetRenderEffectWeight);
    LIB_FUNCTION("ryPlnDDI3rU", "libSceFont", 1, "libSceFont", sceFontGetRenderScaledKerning);
    LIB_FUNCTION("EY38A01lq2k", "libSceFont", 1, "libSceFont", sceFontGetRenderScalePixel);
    LIB_FUNCTION("FEafYUcxEGo", "libSceFont", 1, "libSceFont", sceFontGetRenderScalePoint);
    LIB_FUNCTION("8REoLjNGCpM", "libSceFont", 1, "libSceFont", sceFontGetResolutionDpi);
    LIB_FUNCTION("CkVmLoCNN-8", "libSceFont", 1, "libSceFont", sceFontGetScalePixel);
    LIB_FUNCTION("GoF2bhB7LYk", "libSceFont", 1, "libSceFont", sceFontGetScalePoint);
    LIB_FUNCTION("IrXeG0Lc6nA", "libSceFont", 1, "libSceFont", sceFontGetScriptLanguage);
    LIB_FUNCTION("7-miUT6pNQw", "libSceFont", 1, "libSceFont", sceFontGetTypographicDesign);
    LIB_FUNCTION("3BrWWFU+4ts", "libSceFont", 1, "libSceFont", sceFontGetVerticalLayout);
    LIB_FUNCTION("8-zmgsxkBek", "libSceFont", 1, "libSceFont", sceFontGlyphDefineAttribute);
    LIB_FUNCTION("oO33Uex4Ui0", "libSceFont", 1, "libSceFont", sceFontGlyphGetAttribute);
    LIB_FUNCTION("PXlA0M8ax40", "libSceFont", 1, "libSceFont", sceFontGlyphGetGlyphForm);
    LIB_FUNCTION("XUfSWpLhrUw", "libSceFont", 1, "libSceFont", sceFontGlyphGetMetricsForm);
    LIB_FUNCTION("lNnUqa1zA-M", "libSceFont", 1, "libSceFont", sceFontGlyphGetScalePixel);
    LIB_FUNCTION("ntrc3bEWlvQ", "libSceFont", 1, "libSceFont", sceFontGlyphRefersMetrics);
    LIB_FUNCTION("9kTbF59TjLs", "libSceFont", 1, "libSceFont", sceFontGlyphRefersMetricsHorizontal);
    LIB_FUNCTION("nJavPEdMDvM", "libSceFont", 1, "libSceFont",
                 sceFontGlyphRefersMetricsHorizontalAdvance);
    LIB_FUNCTION("JCnVgZgcucs", "libSceFont", 1, "libSceFont",
                 sceFontGlyphRefersMetricsHorizontalX);
    LIB_FUNCTION("R1T4i+DOhNY", "libSceFont", 1, "libSceFont", sceFontGlyphRefersOutline);
    LIB_FUNCTION("RmkXfBcZnrM", "libSceFont", 1, "libSceFont", sceFontGlyphRenderImage);
    LIB_FUNCTION("r4KEihtwxGs", "libSceFont", 1, "libSceFont", sceFontGlyphRenderImageHorizontal);
    LIB_FUNCTION("n22d-HIdmMg", "libSceFont", 1, "libSceFont", sceFontGlyphRenderImageVertical);
    LIB_FUNCTION("RL2cAQgyXR8", "libSceFont", 1, "libSceFont", sceFontGraphicsBeginFrame);
    LIB_FUNCTION("dUmIK6QjT7E", "libSceFont", 1, "libSceFont", sceFontGraphicsDrawingCancel);
    LIB_FUNCTION("X2Vl3yU19Zw", "libSceFont", 1, "libSceFont", sceFontGraphicsDrawingFinish);
    LIB_FUNCTION("DOmdOwV3Aqw", "libSceFont", 1, "libSceFont", sceFontGraphicsEndFrame);
    LIB_FUNCTION("zdYdKRQC3rw", "libSceFont", 1, "libSceFont", sceFontGraphicsExchangeResource);
    LIB_FUNCTION("UkMUIoj-e9s", "libSceFont", 1, "libSceFont", sceFontGraphicsFillMethodInit);
    LIB_FUNCTION("DJURdcnVUqo", "libSceFont", 1, "libSceFont", sceFontGraphicsFillPlotInit);
    LIB_FUNCTION("eQac6ftmBQQ", "libSceFont", 1, "libSceFont", sceFontGraphicsFillPlotSetLayout);
    LIB_FUNCTION("PEYQJa+MWnk", "libSceFont", 1, "libSceFont", sceFontGraphicsFillPlotSetMapping);
    LIB_FUNCTION("21g4m4kYF6g", "libSceFont", 1, "libSceFont", sceFontGraphicsFillRatesInit);
    LIB_FUNCTION("pJzji5FvdxU", "libSceFont", 1, "libSceFont",
                 sceFontGraphicsFillRatesSetFillEffect);
    LIB_FUNCTION("scaro-xEuUM", "libSceFont", 1, "libSceFont", sceFontGraphicsFillRatesSetLayout);
    LIB_FUNCTION("W66Kqtt0xU0", "libSceFont", 1, "libSceFont", sceFontGraphicsFillRatesSetMapping);
    LIB_FUNCTION("FzpLsBQEegQ", "libSceFont", 1, "libSceFont", sceFontGraphicsGetDeviceUsage);
    LIB_FUNCTION("W80hs0g5d+E", "libSceFont", 1, "libSceFont", sceFontGraphicsRegionInit);
    LIB_FUNCTION("S48+njg9p-o", "libSceFont", 1, "libSceFont", sceFontGraphicsRegionInitCircular);
    LIB_FUNCTION("wcOQ8Fz73+M", "libSceFont", 1, "libSceFont", sceFontGraphicsRegionInitRoundish);
    LIB_FUNCTION("YBaw2Yyfd5E", "libSceFont", 1, "libSceFont", sceFontGraphicsRelease);
    LIB_FUNCTION("qkySrQ4FGe0", "libSceFont", 1, "libSceFont", sceFontGraphicsRenderResource);
    LIB_FUNCTION("qzNjJYKVli0", "libSceFont", 1, "libSceFont", sceFontGraphicsSetFramePolicy);
    LIB_FUNCTION("9iRbHCtcx-o", "libSceFont", 1, "libSceFont", sceFontGraphicsSetupClipping);
    LIB_FUNCTION("KZ3qPyz5Opc", "libSceFont", 1, "libSceFont", sceFontGraphicsSetupColorRates);
    LIB_FUNCTION("LqclbpVzRvM", "libSceFont", 1, "libSceFont", sceFontGraphicsSetupFillMethod);
    LIB_FUNCTION("Wl4FiI4qKY0", "libSceFont", 1, "libSceFont", sceFontGraphicsSetupFillRates);
    LIB_FUNCTION("WC7s95TccVo", "libSceFont", 1, "libSceFont", sceFontGraphicsSetupGlyphFill);
    LIB_FUNCTION("zC6I4ty37NA", "libSceFont", 1, "libSceFont", sceFontGraphicsSetupGlyphFillPlot);
    LIB_FUNCTION("drZUF0XKTEI", "libSceFont", 1, "libSceFont", sceFontGraphicsSetupHandleDefault);
    LIB_FUNCTION("MEAmHMynQXE", "libSceFont", 1, "libSceFont", sceFontGraphicsSetupLocation);
    LIB_FUNCTION("XRUOmQhnYO4", "libSceFont", 1, "libSceFont", sceFontGraphicsSetupPositioning);
    LIB_FUNCTION("98XGr2Bkklg", "libSceFont", 1, "libSceFont", sceFontGraphicsSetupRotation);
    LIB_FUNCTION("Nj-ZUVOVAvc", "libSceFont", 1, "libSceFont", sceFontGraphicsSetupScaling);
    LIB_FUNCTION("p0avT2ggev0", "libSceFont", 1, "libSceFont", sceFontGraphicsSetupShapeFill);
    LIB_FUNCTION("0C5aKg9KghY", "libSceFont", 1, "libSceFont", sceFontGraphicsSetupShapeFillPlot);
    LIB_FUNCTION("4pA3qqAcYco", "libSceFont", 1, "libSceFont", sceFontGraphicsStructureCanvas);
    LIB_FUNCTION("cpjgdlMYdOM", "libSceFont", 1, "libSceFont",
                 sceFontGraphicsStructureCanvasSequence);
    LIB_FUNCTION("774Mee21wKk", "libSceFont", 1, "libSceFont", sceFontGraphicsStructureDesign);
    LIB_FUNCTION("Hp3NIFhUXvQ", "libSceFont", 1, "libSceFont",
                 sceFontGraphicsStructureDesignResource);
    LIB_FUNCTION("bhmZlml6NBs", "libSceFont", 1, "libSceFont",
                 sceFontGraphicsStructureSurfaceTexture);
    LIB_FUNCTION("5sAWgysOBfE", "libSceFont", 1, "libSceFont", sceFontGraphicsUpdateClipping);
    LIB_FUNCTION("W4e8obm+w6o", "libSceFont", 1, "libSceFont", sceFontGraphicsUpdateColorRates);
    LIB_FUNCTION("EgIn3QBajPs", "libSceFont", 1, "libSceFont", sceFontGraphicsUpdateFillMethod);
    LIB_FUNCTION("MnUYAs2jVuU", "libSceFont", 1, "libSceFont", sceFontGraphicsUpdateFillRates);
    LIB_FUNCTION("R-oVDMusYbc", "libSceFont", 1, "libSceFont", sceFontGraphicsUpdateGlyphFill);
    LIB_FUNCTION("b9R+HQuHSMI", "libSceFont", 1, "libSceFont", sceFontGraphicsUpdateGlyphFillPlot);
    LIB_FUNCTION("IN4P5pJADQY", "libSceFont", 1, "libSceFont", sceFontGraphicsUpdateLocation);
    LIB_FUNCTION("U+LLXdr2DxM", "libSceFont", 1, "libSceFont", sceFontGraphicsUpdatePositioning);
    LIB_FUNCTION("yStTYSeb4NM", "libSceFont", 1, "libSceFont", sceFontGraphicsUpdateRotation);
    LIB_FUNCTION("eDxmMoxE5xU", "libSceFont", 1, "libSceFont", sceFontGraphicsUpdateScaling);
    LIB_FUNCTION("Ax6LQJJq6HQ", "libSceFont", 1, "libSceFont", sceFontGraphicsUpdateShapeFill);
    LIB_FUNCTION("I5Rf2rXvBKQ", "libSceFont", 1, "libSceFont", sceFontGraphicsUpdateShapeFillPlot);
    LIB_FUNCTION("whrS4oksXc4", "libSceFont", 1, "libSceFont", sceFontMemoryInit);
    LIB_FUNCTION("h6hIgxXEiEc", "libSceFont", 1, "libSceFont", sceFontMemoryTerm);
    LIB_FUNCTION("RvXyHMUiLhE", "libSceFont", 1, "libSceFont", sceFontOpenFontFile);
    LIB_FUNCTION("JzCH3SCFnAU", "libSceFont", 1, "libSceFont", sceFontOpenFontInstance);
    LIB_FUNCTION("KXUpebrFk1U", "libSceFont", 1, "libSceFont", sceFontOpenFontMemory);
    LIB_FUNCTION("cKYtVmeSTcw", "libSceFont", 1, "libSceFont", sceFontOpenFontSet);
    LIB_FUNCTION("Z2cdsqJH+5k", "libSceFont", 1, "libSceFont", sceFontRebindRenderer);
    LIB_FUNCTION("3G4zhgKuxE8", "libSceFont", 1, "libSceFont", sceFontRenderCharGlyphImage);
    LIB_FUNCTION("kAenWy1Zw5o", "libSceFont", 1, "libSceFont",
                 sceFontRenderCharGlyphImageHorizontal);
    LIB_FUNCTION("i6UNdSig1uE", "libSceFont", 1, "libSceFont", sceFontRenderCharGlyphImageVertical);
    LIB_FUNCTION("amcmrY62BD4", "libSceFont", 1, "libSceFont", sceFontRendererGetOutlineBufferSize);
    LIB_FUNCTION("ai6AfGrBs4o", "libSceFont", 1, "libSceFont", sceFontRendererResetOutlineBuffer);
    LIB_FUNCTION("ydF+WuH0fAk", "libSceFont", 1, "libSceFont",
                 sceFontRendererSetOutlineBufferPolicy);
    LIB_FUNCTION("gdUCnU0gHdI", "libSceFont", 1, "libSceFont", sceFontRenderSurfaceInit);
    LIB_FUNCTION("vRxf4d0ulPs", "libSceFont", 1, "libSceFont", sceFontRenderSurfaceSetScissor);
    LIB_FUNCTION("0hr-w30SjiI", "libSceFont", 1, "libSceFont", sceFontRenderSurfaceSetStyleFrame);
    LIB_FUNCTION("TMtqoFQjjbA", "libSceFont", 1, "libSceFont", sceFontSetEffectSlant);
    LIB_FUNCTION("v0phZwa4R5o", "libSceFont", 1, "libSceFont", sceFontSetEffectWeight);
    LIB_FUNCTION("kihFGYJee7o", "libSceFont", 1, "libSceFont", sceFontSetFontsOpenMode);
    LIB_FUNCTION("I1acwR7Qp8E", "libSceFont", 1, "libSceFont", sceFontSetResolutionDpi);
    LIB_FUNCTION("N1EBMeGhf7E", "libSceFont", 1, "libSceFont", sceFontSetScalePixel);
    LIB_FUNCTION("sw65+7wXCKE", "libSceFont", 1, "libSceFont", sceFontSetScalePoint);
    LIB_FUNCTION("PxSR9UfJ+SQ", "libSceFont", 1, "libSceFont", sceFontSetScriptLanguage);
    LIB_FUNCTION("SnsZua35ngs", "libSceFont", 1, "libSceFont", sceFontSetTypographicDesign);
    LIB_FUNCTION("lz9y9UFO2UU", "libSceFont", 1, "libSceFont", sceFontSetupRenderEffectSlant);
    LIB_FUNCTION("XIGorvLusDQ", "libSceFont", 1, "libSceFont", sceFontSetupRenderEffectWeight);
    LIB_FUNCTION("6vGCkkQJOcI", "libSceFont", 1, "libSceFont", sceFontSetupRenderScalePixel);
    LIB_FUNCTION("nMZid4oDfi4", "libSceFont", 1, "libSceFont", sceFontSetupRenderScalePoint);
    LIB_FUNCTION("ObkDGDBsVtw", "libSceFont", 1, "libSceFont", sceFontStringGetTerminateCode);
    LIB_FUNCTION("+B-xlbiWDJ4", "libSceFont", 1, "libSceFont", sceFontStringGetTerminateOrder);
    LIB_FUNCTION("o1vIEHeb6tw", "libSceFont", 1, "libSceFont", sceFontStringGetWritingForm);
    LIB_FUNCTION("hq5LffQjz-s", "libSceFont", 1, "libSceFont", sceFontStringRefersRenderCharacters);
    LIB_FUNCTION("Avv7OApgCJk", "libSceFont", 1, "libSceFont", sceFontStringRefersTextCharacters);
    LIB_FUNCTION("lOfduYnjgbo", "libSceFont", 1, "libSceFont", sceFontStyleFrameGetEffectSlant);
    LIB_FUNCTION("HIUdjR-+Wl8", "libSceFont", 1, "libSceFont", sceFontStyleFrameGetEffectWeight);
    LIB_FUNCTION("VSw18Aqzl0U", "libSceFont", 1, "libSceFont", sceFontStyleFrameGetResolutionDpi);
    LIB_FUNCTION("2QfqfeLblbg", "libSceFont", 1, "libSceFont", sceFontStyleFrameGetScalePixel);
    LIB_FUNCTION("7x2xKiiB7MA", "libSceFont", 1, "libSceFont", sceFontStyleFrameGetScalePoint);
    LIB_FUNCTION("la2AOWnHEAc", "libSceFont", 1, "libSceFont", sceFontStyleFrameInit);
    LIB_FUNCTION("394sckksiCU", "libSceFont", 1, "libSceFont", sceFontStyleFrameSetEffectSlant);
    LIB_FUNCTION("faw77-pEBmU", "libSceFont", 1, "libSceFont", sceFontStyleFrameSetEffectWeight);
    LIB_FUNCTION("dB4-3Wdwls8", "libSceFont", 1, "libSceFont", sceFontStyleFrameSetResolutionDpi);
    LIB_FUNCTION("da4rQ4-+p-4", "libSceFont", 1, "libSceFont", sceFontStyleFrameSetScalePixel);
    LIB_FUNCTION("O997laxY-Ys", "libSceFont", 1, "libSceFont", sceFontStyleFrameSetScalePoint);
    LIB_FUNCTION("dUmABkAnVgk", "libSceFont", 1, "libSceFont", sceFontStyleFrameUnsetEffectSlant);
    LIB_FUNCTION("hwsuXgmKdaw", "libSceFont", 1, "libSceFont", sceFontStyleFrameUnsetEffectWeight);
    LIB_FUNCTION("bePC0L0vQWY", "libSceFont", 1, "libSceFont", sceFontStyleFrameUnsetScale);
    LIB_FUNCTION("mz2iTY0MK4A", "libSceFont", 1, "libSceFont", sceFontSupportExternalFonts);
    LIB_FUNCTION("71w5DzObuZI", "libSceFont", 1, "libSceFont", sceFontSupportGlyphs);
    LIB_FUNCTION("SsRbbCiWoGw", "libSceFont", 1, "libSceFont", sceFontSupportSystemFonts);
    LIB_FUNCTION("IPoYwwlMx-g", "libSceFont", 1, "libSceFont", sceFontTextCodesStepBack);
    LIB_FUNCTION("olSmXY+XP1E", "libSceFont", 1, "libSceFont", sceFontTextCodesStepNext);
    LIB_FUNCTION("oaJ1BpN2FQk", "libSceFont", 1, "libSceFont", sceFontTextSourceInit);
    LIB_FUNCTION("VRFd3diReec", "libSceFont", 1, "libSceFont", sceFontTextSourceRewind);
    LIB_FUNCTION("eCRMCSk96NU", "libSceFont", 1, "libSceFont", sceFontTextSourceSetDefaultFont);
    LIB_FUNCTION("OqQKX0h5COw", "libSceFont", 1, "libSceFont", sceFontTextSourceSetWritingForm);
    LIB_FUNCTION("1QjhKxrsOB8", "libSceFont", 1, "libSceFont", sceFontUnbindRenderer);
    LIB_FUNCTION("H-FNq8isKE0", "libSceFont", 1, "libSceFont", sceFontWordsFindWordCharacters);
    LIB_FUNCTION("fljdejMcG1c", "libSceFont", 1, "libSceFont", sceFontWritingGetRenderMetrics);
    LIB_FUNCTION("fD5rqhEXKYQ", "libSceFont", 1, "libSceFont", sceFontWritingInit);
    LIB_FUNCTION("1+DgKL0haWQ", "libSceFont", 1, "libSceFont", sceFontWritingLineClear);
    LIB_FUNCTION("JQKWIsS9joE", "libSceFont", 1, "libSceFont", sceFontWritingLineGetOrderingSpace);
    LIB_FUNCTION("nlU2VnfpqTM", "libSceFont", 1, "libSceFont", sceFontWritingLineGetRenderMetrics);
    LIB_FUNCTION("+FYcYefsVX0", "libSceFont", 1, "libSceFont", sceFontWritingLineRefersRenderStep);
    LIB_FUNCTION("wyKFUOWdu3Q", "libSceFont", 1, "libSceFont", sceFontWritingLineWritesOrder);
    LIB_FUNCTION("W-2WOXEHGck", "libSceFont", 1, "libSceFont", sceFontWritingRefersRenderStep);
    LIB_FUNCTION("f4Onl7efPEY", "libSceFont", 1, "libSceFont",
                 sceFontWritingRefersRenderStepCharacter);
    LIB_FUNCTION("BbCZjJizU4A", "libSceFont", 1, "libSceFont", sceFontWritingSetMaskInvisible);
    LIB_FUNCTION("APTXePHIjLM", "libSceFont", 1, "libSceFont", Func_00F4D778F1C88CB3);
    LIB_FUNCTION("A8ZQAl+7Dec", "libSceFont", 1, "libSceFont", Func_03C650025FBB0DE7);
    LIB_FUNCTION("B+q4oWOyfho", "libSceFont", 1, "libSceFont", Func_07EAB8A163B27E1A);
    LIB_FUNCTION("CUCOiOT5fOM", "libSceFont", 1, "libSceFont", Func_09408E88E4F97CE3);
    LIB_FUNCTION("CfkpBe2CqBQ", "libSceFont", 1, "libSceFont", Func_09F92905ED82A814);
    LIB_FUNCTION("DRQs7hqyGr4", "libSceFont", 1, "libSceFont", Func_0D142CEE1AB21ABE);
    LIB_FUNCTION("FL0unhGcFvI", "libSceFont", 1, "libSceFont", Func_14BD2E9E119C16F2);
    LIB_FUNCTION("GsU8nt6ujXU", "libSceFont", 1, "libSceFont", Func_1AC53C9EDEAE8D75);
    LIB_FUNCTION("HUARhdXiTD0", "libSceFont", 1, "libSceFont", Func_1D401185D5E24C3D);
    LIB_FUNCTION("HoPNIMLMmW8", "libSceFont", 1, "libSceFont", Func_1E83CD20C2CC996F);
    LIB_FUNCTION("MUsfdluf54o", "libSceFont", 1, "libSceFont", Func_314B1F765B9FE78A);
    LIB_FUNCTION("NQ5nJf7eKeE", "libSceFont", 1, "libSceFont", Func_350E6725FEDE29E1);
    LIB_FUNCTION("Pbdz8KYEvzk", "libSceFont", 1, "libSceFont", Func_3DB773F0A604BF39);
    LIB_FUNCTION("T-Sd0h4xGxw", "libSceFont", 1, "libSceFont", Func_4FF49DD21E311B1C);
    LIB_FUNCTION("UmKHZkpJOYE", "libSceFont", 1, "libSceFont", Func_526287664A493981);
    LIB_FUNCTION("VcpxjbyEpuk", "libSceFont", 1, "libSceFont", Func_55CA718DBC84A6E9);
    LIB_FUNCTION("Vj-F8HBqi00", "libSceFont", 1, "libSceFont", Func_563FC5F0706A8B4D);
    LIB_FUNCTION("Vp4uzTQpD0U", "libSceFont", 1, "libSceFont", Func_569E2ECD34290F45);
    LIB_FUNCTION("WgR3W2vkdoU", "libSceFont", 1, "libSceFont", Func_5A04775B6BE47685);
    LIB_FUNCTION("X9k7yrb3l1A", "libSceFont", 1, "libSceFont", Func_5FD93BCAB6F79750);
    LIB_FUNCTION("YrU5j4ZL07Q", "libSceFont", 1, "libSceFont", Func_62B5398F864BD3B4);
    LIB_FUNCTION("b5AQKU2CI2c", "libSceFont", 1, "libSceFont", Func_6F9010294D822367);
    LIB_FUNCTION("d1fpR0I6emc", "libSceFont", 1, "libSceFont", Func_7757E947423A7A67);
    LIB_FUNCTION("fga6Ugd-VPo", "libSceFont", 1, "libSceFont", Func_7E06BA52077F54FA);
    LIB_FUNCTION("k7Nt6gITEdY", "libSceFont", 1, "libSceFont", Func_93B36DEA021311D6);
    LIB_FUNCTION("lLCJHnERWYo", "libSceFont", 1, "libSceFont", Func_94B0891E7111598A);
    LIB_FUNCTION("l4XJEowv580", "libSceFont", 1, "libSceFont", Func_9785C9128C2FE7CD);
    LIB_FUNCTION("l9+8m2X7wOE", "libSceFont", 1, "libSceFont", Func_97DFBC9B65FBC0E1);
    LIB_FUNCTION("rNlxdAXX08o", "libSceFont", 1, "libSceFont", Func_ACD9717405D7D3CA);
    LIB_FUNCTION("sZqK7D-U8W8", "libSceFont", 1, "libSceFont", Func_B19A8AEC3FD4F16F);
    LIB_FUNCTION("wQ9IitfPED0", "libSceFont", 1, "libSceFont", Func_C10F488AD7CF103D);
    LIB_FUNCTION("0Mi1-0poJsc", "libSceFont", 1, "libSceFont", Func_D0C8B5FF4A6826C7);
    LIB_FUNCTION("5I080Bw0KjM", "libSceFont", 1, "libSceFont", Func_E48D3CD01C342A33);
    LIB_FUNCTION("6slrIYa3HhQ", "libSceFont", 1, "libSceFont", Func_EAC96B2186B71E14);
    LIB_FUNCTION("-keIqW70YlY", "libSceFont", 1, "libSceFont", Func_FE4788A96EF46256);
    LIB_FUNCTION("-n5a6V0wWPU", "libSceFont", 1, "libSceFont", Func_FE7E5AE95D3058F5);
};

} // namespace Libraries::Font
