// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

// Based on imgui_impl_sdlrenderer3.cpp from Dear ImGui repository

#include "imgui.h"
#ifndef IMGUI_DISABLE
#include <stdint.h> // intptr_t
#include "imgui_impl_sdlrenderer3.h"

// Clang warnings with -Weverything
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored                                                                   \
    "-Wsign-conversion" // warning: implicit conversion changes signedness
#endif

// SDL
#include <SDL3/SDL.h>
#if !SDL_VERSION_ATLEAST(3, 0, 0)
#error This backend requires SDL 3.0.0+
#endif

// SDL_Renderer data
struct ImGui_ImplSDLRenderer3_Data {
    SDL_Renderer* Renderer; // Main viewport's renderer
    ImVector<SDL_FColor> ColorBuffer;

    ImGui_ImplSDLRenderer3_Data() {
        memset((void*)this, 0, sizeof(*this));
    }
};

// Backend data stored in io.BackendRendererUserData to allow support for multiple Dear ImGui
// contexts It is STRONGLY preferred that you use docking branch with multi-viewports (==single Dear
// ImGui context + multiple windows) instead of multiple Dear ImGui contexts.
static ImGui_ImplSDLRenderer3_Data* ImGui_ImplSDLRenderer3_GetBackendData() {
    return ImGui::GetCurrentContext()
               ? (ImGui_ImplSDLRenderer3_Data*)ImGui::GetIO().BackendRendererUserData
               : nullptr;
}

// Functions
bool ImGui_ImplSDLRenderer3_Init(SDL_Renderer* renderer) {
    ImGuiIO& io = ImGui::GetIO();
    IMGUI_CHECKVERSION();
    IM_ASSERT(io.BackendRendererUserData == nullptr && "Already initialized a renderer backend!");
    IM_ASSERT(renderer != nullptr && "SDL_Renderer not initialized!");

    // Setup backend capabilities flags
    ImGui_ImplSDLRenderer3_Data* bd = IM_NEW(ImGui_ImplSDLRenderer3_Data)();
    io.BackendRendererUserData = (void*)bd;
    io.BackendRendererName = "imgui_impl_sdlrenderer3";
    io.BackendFlags |=
        ImGuiBackendFlags_RendererHasVtxOffset; // We can honor the ImDrawCmd::VtxOffset
                                                // field, allowing for large meshes.
    io.BackendFlags |= ImGuiBackendFlags_RendererHasTextures; // Dynamic font atlas

    bd->Renderer = renderer;

    return true;
}

void ImGui_ImplSDLRenderer3_Shutdown() {
    ImGui_ImplSDLRenderer3_Data* bd = ImGui_ImplSDLRenderer3_GetBackendData();
    IM_ASSERT(bd != nullptr && "No renderer backend to shutdown, or already shutdown?");
    ImGuiIO& io = ImGui::GetIO();

    ImGui_ImplSDLRenderer3_DestroyDeviceObjects();

    io.BackendRendererName = nullptr;
    io.BackendRendererUserData = nullptr;
    io.BackendFlags &=
        ~(ImGuiBackendFlags_RendererHasVtxOffset | ImGuiBackendFlags_RendererHasTextures);
    IM_DELETE(bd);
}

static void ImGui_ImplSDLRenderer3_SetupRenderState(SDL_Renderer* renderer) {
    // Clear out any viewports and cliprect set by the user
    // FIXME: Technically speaking there are lots of other things we could backup/setup/restore
    // during our render process.
    SDL_SetRenderViewport(renderer, nullptr);
    SDL_SetRenderClipRect(renderer, nullptr);
}

void ImGui_ImplSDLRenderer3_NewFrame() {
    ImGui_ImplSDLRenderer3_Data* bd = ImGui_ImplSDLRenderer3_GetBackendData();
    IM_ASSERT(bd != nullptr &&
              "Context or backend not initialized! Did you call ImGui_ImplSDLRenderer3_Init()?");
    IM_UNUSED(bd);
}

// https://github.com/libsdl-org/SDL/issues/9009
static int SDL_RenderGeometryRaw8BitColor(SDL_Renderer* renderer, ImVector<SDL_FColor>& colors_out,
                                          SDL_Texture* texture, const float* xy, int xy_stride,
                                          const SDL_Color* color, int color_stride, const float* uv,
                                          int uv_stride, int num_vertices, const void* indices,
                                          int num_indices, int size_indices) {
    const Uint8* color2 = (const Uint8*)color;
    colors_out.resize(num_vertices);
    SDL_FColor* color3 = colors_out.Data;
    for (int i = 0; i < num_vertices; i++) {
        color3[i].r = color->r / 255.0f;
        color3[i].g = color->g / 255.0f;
        color3[i].b = color->b / 255.0f;
        color3[i].a = color->a / 255.0f;
        color2 += color_stride;
        color = (const SDL_Color*)color2;
    }
    return SDL_RenderGeometryRaw(renderer, texture, xy, xy_stride, color3, sizeof(*color3), uv,
                                 uv_stride, num_vertices, indices, num_indices, size_indices);
}

void ImGui_ImplSDLRenderer3_RenderDrawData(ImDrawData* draw_data, SDL_Renderer* renderer) {
    ImGui_ImplSDLRenderer3_Data* bd = ImGui_ImplSDLRenderer3_GetBackendData();

    // If there's a scale factor set by the user, use that instead
    // If the user has specified a scale factor to SDL_Renderer already via SDL_RenderSetScale(),
    // SDL will scale whatever we pass to SDL_RenderGeometryRaw() by that scale factor. In that case
    // we don't want to be also scaling it ourselves here.
    float rsx = 1.0f;
    float rsy = 1.0f;
    SDL_GetRenderScale(renderer, &rsx, &rsy);
    ImVec2 render_scale;
    render_scale.x = (rsx == 1.0f) ? draw_data->FramebufferScale.x : 1.0f;
    render_scale.y = (rsy == 1.0f) ? draw_data->FramebufferScale.y : 1.0f;

    // Avoid rendering when minimized, scale coordinates for retina displays (screen coordinates !=
    // framebuffer coordinates)
    int fb_width = (int)(draw_data->DisplaySize.x * render_scale.x);
    int fb_height = (int)(draw_data->DisplaySize.y * render_scale.y);
    if (fb_width == 0 || fb_height == 0)
        return;

    // Catch up with texture updates. Usually, the list has 1 element with an OK status.
    if (draw_data->Textures != nullptr)
        for (ImTextureData* tex : *draw_data->Textures)
            if (tex->Status != ImTextureStatus_OK)
                ImGui_ImplSDLRenderer3_UpdateTexture(tex);

    // Backup SDL_Renderer state that will be modified to restore it afterwards
    struct BackupSDLRendererState {
        SDL_Rect Viewport;
        bool ViewportEnabled;
        bool ClipEnabled;
        SDL_Rect ClipRect;
    };
    BackupSDLRendererState old = {};
    old.ViewportEnabled = SDL_RenderViewportSet(renderer);
    old.ClipEnabled = SDL_RenderClipEnabled(renderer);
    SDL_GetRenderViewport(renderer, &old.Viewport);
    SDL_GetRenderClipRect(renderer, &old.ClipRect);

    // Setup desired state
    ImGui_ImplSDLRenderer3_SetupRenderState(renderer);

    // Setup render state structure (for callbacks and custom texture bindings)
    ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
    ImGui_ImplSDLRenderer3_RenderState render_state;
    render_state.Renderer = renderer;
    platform_io.Renderer_RenderState = &render_state;

    // Will project scissor/clipping rectangles into framebuffer space
    ImVec2 clip_off = draw_data->DisplayPos; // (0,0) unless using multi-viewports
    ImVec2 clip_scale = render_scale;

    // Render command lists
    for (int n = 0; n < draw_data->CmdListsCount; n++) {
        const ImDrawList* draw_list = draw_data->CmdLists[n];
        const ImDrawVert* vtx_buffer = draw_list->VtxBuffer.Data;
        const ImDrawIdx* idx_buffer = draw_list->IdxBuffer.Data;

        for (int cmd_i = 0; cmd_i < draw_list->CmdBuffer.Size; cmd_i++) {
            const ImDrawCmd* pcmd = &draw_list->CmdBuffer[cmd_i];
            if (pcmd->UserCallback) {
                // User callback, registered via ImDrawList::AddCallback()
                // (ImDrawCallback_ResetRenderState is a special callback value used by the user to
                // request the renderer to reset render state.)
                if (pcmd->UserCallback == ImDrawCallback_ResetRenderState)
                    ImGui_ImplSDLRenderer3_SetupRenderState(renderer);
                else
                    pcmd->UserCallback(draw_list, pcmd);
            } else {
                // Project scissor/clipping rectangles into framebuffer space
                ImVec2 clip_min((pcmd->ClipRect.x - clip_off.x) * clip_scale.x,
                                (pcmd->ClipRect.y - clip_off.y) * clip_scale.y);
                ImVec2 clip_max((pcmd->ClipRect.z - clip_off.x) * clip_scale.x,
                                (pcmd->ClipRect.w - clip_off.y) * clip_scale.y);
                if (clip_min.x < 0.0f) {
                    clip_min.x = 0.0f;
                }
                if (clip_min.y < 0.0f) {
                    clip_min.y = 0.0f;
                }
                if (clip_max.x > (float)fb_width) {
                    clip_max.x = (float)fb_width;
                }
                if (clip_max.y > (float)fb_height) {
                    clip_max.y = (float)fb_height;
                }
                if (clip_max.x <= clip_min.x || clip_max.y <= clip_min.y)
                    continue;

                SDL_Rect r = {(int)(clip_min.x), (int)(clip_min.y), (int)(clip_max.x - clip_min.x),
                              (int)(clip_max.y - clip_min.y)};
                SDL_SetRenderClipRect(renderer, &r);

                const float* xy =
                    (const float*)(const void*)((const char*)(vtx_buffer + pcmd->VtxOffset) +
                                                offsetof(ImDrawVert, pos));
                const float* uv =
                    (const float*)(const void*)((const char*)(vtx_buffer + pcmd->VtxOffset) +
                                                offsetof(ImDrawVert, uv));
                const SDL_Color* color =
                    (const SDL_Color*)(const void*)((const char*)(vtx_buffer + pcmd->VtxOffset) +
                                                    offsetof(ImDrawVert, col)); // SDL 2.0.19+

                // Bind texture, Draw
                SDL_Texture* tex = (SDL_Texture*)pcmd->GetTexID();
                SDL_RenderGeometryRaw8BitColor(
                    renderer, bd->ColorBuffer, tex, xy, (int)sizeof(ImDrawVert), color,
                    (int)sizeof(ImDrawVert), uv, (int)sizeof(ImDrawVert),
                    draw_list->VtxBuffer.Size - pcmd->VtxOffset, idx_buffer + pcmd->IdxOffset,
                    pcmd->ElemCount, sizeof(ImDrawIdx));
            }
        }
    }
    platform_io.Renderer_RenderState = nullptr;

    // Restore modified SDL_Renderer state
    SDL_SetRenderViewport(renderer, old.ViewportEnabled ? &old.Viewport : nullptr);
    SDL_SetRenderClipRect(renderer, old.ClipEnabled ? &old.ClipRect : nullptr);
}

void ImGui_ImplSDLRenderer3_UpdateTexture(ImTextureData* tex) {
    ImGui_ImplSDLRenderer3_Data* bd = ImGui_ImplSDLRenderer3_GetBackendData();

    if (tex->Status == ImTextureStatus_WantCreate) {
        // Create and upload new texture to graphics system
        IM_ASSERT(tex->TexID == ImTextureID_Invalid && tex->BackendUserData == nullptr);
        IM_ASSERT(tex->Format == ImTextureFormat_RGBA32);

        // Create texture (bilinear sampling is required)
        SDL_Texture* sdl_texture =
            SDL_CreateTexture(bd->Renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STATIC,
                              tex->Width, tex->Height);
        IM_ASSERT(sdl_texture != nullptr && "Backend failed to create texture!");
        SDL_UpdateTexture(sdl_texture, nullptr, tex->GetPixels(), tex->GetPitch());
        SDL_SetTextureBlendMode(sdl_texture, SDL_BLENDMODE_BLEND);
        SDL_SetTextureScaleMode(sdl_texture, SDL_SCALEMODE_LINEAR);

        // Store identifiers
        tex->SetTexID((ImTextureID)(intptr_t)sdl_texture);
        tex->SetStatus(ImTextureStatus_OK);
    } else if (tex->Status == ImTextureStatus_WantUpdates) {
        // Update selected blocks. Write to textures regions never used before.
        SDL_Texture* sdl_texture = (SDL_Texture*)(intptr_t)tex->TexID;
        for (ImTextureRect& r : tex->Updates) {
            SDL_Rect sdl_r = {r.x, r.y, r.w, r.h};
            SDL_UpdateTexture(sdl_texture, &sdl_r, tex->GetPixelsAt(r.x, r.y), tex->GetPitch());
        }
        tex->SetStatus(ImTextureStatus_OK);
    } else if (tex->Status == ImTextureStatus_WantDestroy) {
        if (tex->TexID != ImTextureID_Invalid)
            if (SDL_Texture* sdl_texture = (SDL_Texture*)(intptr_t)tex->TexID)
                SDL_DestroyTexture(sdl_texture);

        // Clear identifiers and mark as destroyed
        tex->SetTexID(ImTextureID_Invalid);
        tex->SetStatus(ImTextureStatus_Destroyed);
    }
}

// Called by Init/NewFrame/Shutdown
bool ImGui_ImplSDLRenderer3_CreateDeviceObjects() {
    return true;
}

void ImGui_ImplSDLRenderer3_DestroyDeviceObjects() {
    // Destroy all textures
    for (ImTextureData* tex : ImGui::GetPlatformIO().Textures)
        if (tex->RefCount == 1) {
            tex->SetStatus(ImTextureStatus_WantDestroy);
            ImGui_ImplSDLRenderer3_UpdateTexture(tex);
        }
}

//-----------------------------------------------------------------------------

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#endif // #ifndef IMGUI_DISABLE
