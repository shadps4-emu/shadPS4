<!--
SPDX-FileCopyrightText: 2024-2026 shadPS4 Emulator Project
SPDX-License-Identifier: GPL-2.0-or-later
-->

# UFC 3 Fixes (loading hang + follow-ups)

**Game:** EA Sports UFC 3 (`CUSA06534`)  
**Base:** `origin/main` @ `23c41333` (includes upstream `#5358` null index-base `BindIndexBuffer`)  
**Patch:** [`ufc3-fixes.patch`](ufc3-fixes.patch) — apply with `git apply --check documents/ufc3-fixes.patch` then `git apply documents/ufc3-fixes.patch`

## Symptom (loading)

During boot, the game could hang or freeze around **28–30%** on the loading screen. Logs often showed Vulkan failures such as **`Device lost during submit`**, with validation noise around image copies and depth attachments.

## Root causes (loading)

1. **Wrong slice index in `SliceOf`** — used `guest_size` instead of `slice_size`.
2. **Invalid 2D ↔ 3D `vkCmdCopyImage` / unsafe `CopyMip`** — wrong `layerCount`, missing slice bounds.
3. **Incomplete depth writability / format matching** — depth views from `depth_write_enable` alone; no `exact_fmt`.
4. **CPU sync on promoted formats** — refresh/download when guest format ≠ Vulkan backing.
5. **Depth format / Color3D view edge cases** — missing `DepthValid`/`StencilValid`, `R16Uint` promote, Color3D layer clamp.

## Fixes in this patch (file map)

### Loading hang

| Area | File(s) | Change |
|------|---------|--------|
| Slice index | [`image_info.cpp`](../src/video_core/texture_cache/image_info.cpp) | `SliceOf` uses `slice_size`; depth formats via `DepthValid`/`StencilValid`. |
| Image copies | [`vk_runtime.cpp`](../src/video_core/renderer_vulkan/vk_runtime.cpp) | `CopyImage`/`CopyMip` 2D↔3D `layerCount = 1`; CopyMip bounds/clamp. |
| Depth view API | [`image_view.h`](../src/video_core/texture_cache/image_view.h)/[`.cpp`](../src/video_core/texture_cache/image_view.cpp), [`texture_cache.h`](../src/video_core/texture_cache/texture_cache.h) | Depth ctor takes `bool is_writable`; Color3D layer clamp. |
| Depth bind | [`vk_rasterizer.cpp`](../src/video_core/renderer_vulkan/vk_rasterizer.cpp) | `is_writable` from write/clear/stencil; `FindImage(desc, true)`. Keeps upstream null index-base handling. |
| Promoted format sync | [`texture_cache.cpp`](../src/video_core/texture_cache/texture_cache.cpp) | Skip refresh/download when formats differ. |
| R16 depth promote | [`liverpool_to_vk.h`](../src/video_core/renderer_vulkan/liverpool_to_vk.h) | `eR16Uint` depth-compatible → `eD16Unorm`. |

### Follow-ups (same patch; fight / CAP / readbacks)

| Area | File(s) | Change |
|------|---------|--------|
| Relaxed Track | [`region_manager.h`](../src/video_core/buffer_cache/region_manager.h), [`page_manager.cpp`](../src/video_core/page_manager.cpp) | Track GPU pages when not Disabled; u16 watchers / fault window. |
| Skinning-safe uploads | [`buffer_cache.cpp`](../src/video_core/buffer_cache/buffer_cache.cpp)/[`.h`](../src/video_core/buffer_cache/buffer_cache.h), [`memory_tracker.h`](../src/video_core/buffer_cache/memory_tracker.h) | Restore `sync_batch`/`FlushSyncBatch`; skip upload over `gpu_modified_ranges`; Disabled invalidate preserves GPU bits. |
| Flush hooks | [`vk_rasterizer.cpp`](../src/video_core/renderer_vulkan/vk_rasterizer.cpp), [`liverpool.cpp`](../src/video_core/amdgpu/liverpool.cpp) | Session/OnFence/`DumpConstRam` flush; `CopyBuffer` respects sync batch. |
| WAW barriers | [`image.cpp`](../src/video_core/texture_cache/image.cpp) | Emit barrier on same-layout write-after-write. |
| Tessellation | [`spirv_emit_context.cpp`](../src/shader_recompiler/backend/spirv/spirv_emit_context.cpp), [`emit_spirv_context_get_set.cpp`](../src/shader_recompiler/backend/spirv/emit_spirv_context_get_set.cpp), [`vk_pipeline_cache.cpp`](../src/video_core/renderer_vulkan/vk_pipeline_cache.cpp) | `[attr][vertex]` layout; prefer TCS `ls_stride`. |
| AJM CAP | [`ajm_mp3.cpp`](../src/core/libraries/ajm/ajm_mp3.cpp) | Soft-error invalid MP3 frames instead of assert. |

**Not in this patch (intentionally dropped for VRAM):** integer depth sample color aliases (`EnsureIntegerDepthSampleAlias`) — pinned DeviceLocal twins and OOMed on 6GB GPUs.

## Verification

- UFC3 boots past previous 28–30% loading hang (no `Device lost` on that path).
- With `readbacks_mode` = Relaxed (1) or Precise (2) + linear readbacks: fights are not black-world; skinning better than pre-Track.
- Upstream `#5358` `BindIndexBuffer` null `base_address` guard remains.
- Build: `cmake --build build -j` → `build/shadps4`.

## Suggested commit message (for later)

```
Fix UFC3 loading hang and related GPU/readback correctness.

Correct SliceOf/depth binding and promoted-format sync so mid-load
no longer device-loses; keep Relaxed Track, sync_batch skinning-safe
uploads, and related path fixes without regressing upstream BindIndexBuffer.
```

### Later commit commands (do not run until asked)

```bash
git apply --check documents/ufc3-fixes.patch
git apply documents/ufc3-fixes.patch   # if working tree is clean of these changes
# or: git add -u && git add documents/ufc3-loading-hang.md documents/ufc3-fixes.patch
git commit -m "$(cat <<'EOF'
Fix UFC3 loading hang and related GPU/readback correctness.

Correct SliceOf/depth binding and promoted-format sync so mid-load
no longer device-loses; keep Relaxed Track, sync_batch skinning-safe
uploads, and related path fixes without regressing upstream BindIndexBuffer.
EOF
)"
```

## Playable CUSA06534 config

- `vkvalidation_*` / `vkcrash_diagnostic_enabled` / `debug_dump` → off
- `pipeline_cache_enabled` → on
- `readbacks_mode` = Precise (2) or Relaxed (1); `readback_linear_images_enabled` = true
- Do **not** use Disabled (0) for fights until GPU→CPU feedback works without page faults
