// SPDX-FileCopyrightText: Copyright 2025 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/assert.h"
#include "common/types.h"

namespace AmdGpu {
enum class TileMode : u32;
}

namespace VideoCore {

// clang-format off
// The table of macro tiles parameters for given tiling index (row) and bpp (column)
/* Calculation:
 * - Inputs:
 *     TileMode, BytesPerPixel, NumFragments
 * - Constants:
 *     MicroTileWidth = 8, MicroTileHeight = 8,
 *     Tile Mode LUTs: IsDepth(), IsPrt(), TileThickness(), TileSplit(), SampleSplit(), NumPipes()
 *     Macro Tile Mode LUTs: BankWidth(), BankHeight(), NumBanks(), MacroTileAspect()
 * - Determine the macro tile mode:
 *     TileBytes = MicroTileWidth * MicroTileHeight * TileThickness(TileMode) * BytesPerPixel
 *     TileSplit = min(IsDepth(TileMode) ? TileSplit(TileMode) : max(TileBytes * SampleSplit(TileMode), 256), NumFragments * TileBytes, 1024)
 *     MacroTileModeIndex = log2(TileSplit / 64)
 *     MacroTileMode = IsPrt(TileMode) ? MacroTileModeIndex + 8 : MacroTileModeIndex
 * - Calculate macro tile width and height:
 *     Width = NumPipes(TileMode) * BankWidth(MacroTileMode) * MicroTileWidth * MacroTileAspect(MacroTileMode, AltTileMode)
 *     Height = NumBanks(MacroTileMode, AltTileMode) * BankHeight(MacroTileMode, AltTileMode) * MicroTileHeight / MacroTileAspect(MacroTileMode, AltTileMode)
 */

constexpr std::array macro_tile_extents_x1{
    std::pair{256u, 128u}, std::pair{256u, 128u}, std::pair{256u, 128u}, std::pair{256u, 128u}, std::pair{256u, 128u}, // 00
    std::pair{256u, 128u}, std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 128u}, // 01
    std::pair{256u, 128u}, std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{128u, 64u},  std::pair{128u, 64u},  // 02
    std::pair{256u, 128u}, std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{128u, 64u},  std::pair{128u, 64u},  // 03
    std::pair{256u, 128u}, std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{128u, 64u},  std::pair{64u, 64u},   // 04
    std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     // 05
    std::pair{256u, 256u}, std::pair{256u, 128u}, std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 128u}, // 06
    std::pair{256u, 256u}, std::pair{256u, 128u}, std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{64u, 64u},   // 07
    std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     // 08
    std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     // 09
    std::pair{256u, 128u}, std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{128u, 64u},  std::pair{64u, 64u},   // 0A
    std::pair{256u, 256u}, std::pair{256u, 128u}, std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{64u, 64u},   // 0B
    std::pair{256u, 256u}, std::pair{256u, 128u}, std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{64u, 64u},   // 0C
    std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     // 0D
    std::pair{256u, 128u}, std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{128u, 64u},  std::pair{64u, 64u},   // 0E
    std::pair{256u, 128u}, std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{128u, 64u},  std::pair{64u, 64u},   // 0F
    std::pair{256u, 256u}, std::pair{256u, 128u}, std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{64u, 64u},   // 10
    std::pair{256u, 256u}, std::pair{256u, 128u}, std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{64u, 64u},   // 11
    std::pair{256u, 256u}, std::pair{256u, 128u}, std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{64u, 64u},   // 12
    std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     // 13
    std::pair{128u, 64u},  std::pair{128u, 64u},  std::pair{64u, 64u},   std::pair{64u, 64u},   std::pair{64u, 64u},   // 14
    std::pair{128u, 64u},  std::pair{128u, 64u},  std::pair{64u, 64u},   std::pair{64u, 64u},   std::pair{64u, 64u},   // 15
    std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{64u, 64u},   std::pair{64u, 64u},   std::pair{64u, 64u},   // 16
    std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{64u, 64u},   std::pair{64u, 64u},   std::pair{64u, 64u},   // 17
    std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{64u, 64u},   std::pair{64u, 64u},   std::pair{64u, 64u},   // 18
    std::pair{128u, 64u},  std::pair{64u, 64u},   std::pair{64u, 64u},   std::pair{64u, 64u},   std::pair{64u, 64u},   // 19
    std::pair{128u, 64u},  std::pair{64u, 64u},   std::pair{64u, 64u},   std::pair{64u, 64u},   std::pair{64u, 64u},   // 1A
};

constexpr std::array macro_tile_extents_x2{
    std::pair{256u, 128u}, std::pair{256u, 128u}, std::pair{256u, 128u}, std::pair{256u, 128u}, std::pair{256u, 128u}, // 00
    std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 128u}, // 01
    std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{128u, 64u},  std::pair{128u, 64u},  std::pair{128u, 64u},  // 02
    std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{128u, 64u},  std::pair{128u, 64u},  std::pair{128u, 64u},  // 03
    std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{128u, 64u},  std::pair{64u, 64u},   std::pair{64u, 64u},   // 04
    std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     // 05
    std::pair{256u, 128u}, std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 128u}, // 06
    std::pair{256u, 128u}, std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{64u, 64u},   std::pair{64u, 64u},   // 07
    std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     // 08
    std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     // 09
    std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{128u, 64u},  std::pair{64u, 64u},   std::pair{64u, 64u},   // 0A
    std::pair{256u, 128u}, std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{64u, 64u},   std::pair{64u, 64u},   // 0B
    std::pair{256u, 128u}, std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{64u, 64u},   std::pair{64u, 64u},   // 0C
    std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     // 0D
    std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{128u, 64u},  std::pair{64u, 64u},   std::pair{64u, 64u},   // 0E
    std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{128u, 64u},  std::pair{64u, 64u},   std::pair{64u, 64u},   // 0F
    std::pair{256u, 128u}, std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{64u, 64u},   std::pair{64u, 64u},   // 10
    std::pair{256u, 128u}, std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{64u, 64u},   std::pair{64u, 64u},   // 11
    std::pair{256u, 128u}, std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{64u, 64u},   std::pair{64u, 64u},   // 12
    std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     // 13
    std::pair{128u, 64u},  std::pair{128u, 64u},  std::pair{64u, 64u},   std::pair{64u, 64u},   std::pair{64u, 64u},   // 14
    std::pair{128u, 64u},  std::pair{128u, 64u},  std::pair{64u, 64u},   std::pair{64u, 64u},   std::pair{64u, 64u},   // 15
    std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{64u, 64u},   std::pair{64u, 64u},   std::pair{64u, 64u},   // 16
    std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{64u, 64u},   std::pair{64u, 64u},   std::pair{64u, 64u},   // 17
    std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{64u, 64u},   std::pair{64u, 64u},   std::pair{64u, 64u},   // 18
    std::pair{128u, 64u},  std::pair{64u, 64u},   std::pair{64u, 64u},   std::pair{64u, 64u},   std::pair{64u, 64u},   // 19
    std::pair{128u, 64u},  std::pair{64u, 64u},   std::pair{64u, 64u},   std::pair{64u, 64u},   std::pair{64u, 64u},   // 1A
};

constexpr std::array macro_tile_extents_x4{
    std::pair{256u, 128u}, std::pair{256u, 128u}, std::pair{256u, 128u}, std::pair{256u, 128u}, std::pair{256u, 128u}, // 00
    std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 128u}, // 01
    std::pair{128u, 64u},  std::pair{128u, 64u},  std::pair{128u, 64u},  std::pair{128u, 64u},  std::pair{128u, 64u},  // 02
    std::pair{128u, 64u},  std::pair{128u, 64u},  std::pair{128u, 64u},  std::pair{128u, 64u},  std::pair{128u, 64u},  // 03
    std::pair{128u, 64u},  std::pair{128u, 64u},  std::pair{64u, 64u},   std::pair{64u, 64u},   std::pair{64u, 64u},   // 04
    std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     // 05
    std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 128u}, // 06
    std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{64u, 64u},   std::pair{64u, 64u},   std::pair{64u, 64u},   // 07
    std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     // 08
    std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     // 09
    std::pair{128u, 64u},  std::pair{128u, 64u},  std::pair{128u, 64u},  std::pair{64u, 64u},   std::pair{64u, 64u},   // 0A
    std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{64u, 64u},   std::pair{64u, 64u},   // 0B
    std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{64u, 64u},   std::pair{64u, 64u},   // 0C
    std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     // 0D
    std::pair{128u, 64u},  std::pair{128u, 64u},  std::pair{128u, 64u},  std::pair{64u, 64u},   std::pair{64u, 64u},   // 0E
    std::pair{128u, 64u},  std::pair{128u, 64u},  std::pair{128u, 64u},  std::pair{64u, 64u},   std::pair{64u, 64u},   // 0F
    std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{64u, 64u},   std::pair{64u, 64u},   // 10
    std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{64u, 64u},   std::pair{64u, 64u},   // 11
    std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{64u, 64u},   std::pair{64u, 64u},   // 12
    std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     // 13
    std::pair{128u, 64u},  std::pair{128u, 64u},  std::pair{64u, 64u},   std::pair{64u, 64u},   std::pair{64u, 64u},   // 14
    std::pair{128u, 64u},  std::pair{128u, 64u},  std::pair{64u, 64u},   std::pair{64u, 64u},   std::pair{64u, 64u},   // 15
    std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{64u, 64u},   std::pair{64u, 64u},   std::pair{64u, 64u},   // 16
    std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{64u, 64u},   std::pair{64u, 64u},   std::pair{64u, 64u},   // 17
    std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{64u, 64u},   std::pair{64u, 64u},   std::pair{64u, 64u},   // 18
    std::pair{128u, 64u},  std::pair{64u, 64u},   std::pair{64u, 64u},   std::pair{64u, 64u},   std::pair{64u, 64u},   // 19
    std::pair{128u, 64u},  std::pair{64u, 64u},   std::pair{64u, 64u},   std::pair{64u, 64u},   std::pair{64u, 64u},   // 1A
};

constexpr std::array macro_tile_extents_x8{
    std::pair{256u, 128u}, std::pair{256u, 128u}, std::pair{256u, 128u}, std::pair{256u, 128u}, std::pair{256u, 128u}, // 00
    std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 128u}, // 01
    std::pair{128u, 64u},  std::pair{128u, 64u},  std::pair{128u, 64u},  std::pair{128u, 64u},  std::pair{128u, 64u},  // 02
    std::pair{128u, 64u},  std::pair{128u, 64u},  std::pair{128u, 64u},  std::pair{128u, 64u},  std::pair{128u, 64u},  // 03
    std::pair{128u, 64u},  std::pair{64u, 64u},   std::pair{64u, 64u},   std::pair{64u, 64u},   std::pair{64u, 64u},   // 04
    std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     // 05
    std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 128u}, // 06
    std::pair{128u, 64u},  std::pair{64u, 64u},   std::pair{64u, 64u},   std::pair{64u, 64u},   std::pair{64u, 64u},   // 07
    std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     // 08
    std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     // 09
    std::pair{128u, 64u},  std::pair{128u, 64u},  std::pair{128u, 64u},  std::pair{64u, 64u},   std::pair{64u, 64u},   // 0A
    std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{64u, 64u},   std::pair{64u, 64u},   // 0B
    std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{64u, 64u},   std::pair{64u, 64u},   // 0C
    std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     // 0D
    std::pair{128u, 64u},  std::pair{128u, 64u},  std::pair{128u, 64u},  std::pair{64u, 64u},   std::pair{64u, 64u},   // 0E
    std::pair{128u, 64u},  std::pair{128u, 64u},  std::pair{128u, 64u},  std::pair{64u, 64u},   std::pair{64u, 64u},   // 0F
    std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{64u, 64u},   std::pair{64u, 64u},   // 10
    std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{64u, 64u},   std::pair{64u, 64u},   // 11
    std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{64u, 64u},   std::pair{64u, 64u},   // 12
    std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     // 13
    std::pair{128u, 64u},  std::pair{128u, 64u},  std::pair{64u, 64u},   std::pair{64u, 64u},   std::pair{64u, 64u},   // 14
    std::pair{128u, 64u},  std::pair{128u, 64u},  std::pair{64u, 64u},   std::pair{64u, 64u},   std::pair{64u, 64u},   // 15
    std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{64u, 64u},   std::pair{64u, 64u},   std::pair{64u, 64u},   // 16
    std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{64u, 64u},   std::pair{64u, 64u},   std::pair{64u, 64u},   // 17
    std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{64u, 64u},   std::pair{64u, 64u},   std::pair{64u, 64u},   // 18
    std::pair{128u, 64u},  std::pair{64u, 64u},   std::pair{64u, 64u},   std::pair{64u, 64u},   std::pair{64u, 64u},   // 19
    std::pair{128u, 64u},  std::pair{64u, 64u},   std::pair{64u, 64u},   std::pair{64u, 64u},   std::pair{64u, 64u},   // 1A
};

constexpr std::array macro_tile_extents_alt_x1{
    std::pair{256u, 128u}, std::pair{256u, 128u}, std::pair{256u, 128u}, std::pair{256u, 128u}, std::pair{256u, 128u}, // 00
    std::pair{256u, 128u}, std::pair{256u, 128u}, std::pair{256u, 128u}, std::pair{256u, 128u}, std::pair{256u, 128u}, // 01
    std::pair{256u, 128u}, std::pair{256u, 128u}, std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 128u}, // 02
    std::pair{256u, 128u}, std::pair{256u, 128u}, std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 128u}, // 03
    std::pair{256u, 128u}, std::pair{256u, 128u}, std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 64u},  // 04
    std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     // 05
    std::pair{256u, 256u}, std::pair{256u, 128u}, std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 128u}, // 06
    std::pair{256u, 256u}, std::pair{256u, 128u}, std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{128u, 32u},  // 07
    std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     // 08
    std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     // 09
    std::pair{256u, 128u}, std::pair{256u, 128u}, std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 64u},  // 0A
    std::pair{256u, 256u}, std::pair{256u, 128u}, std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{128u, 32u},  // 0B
    std::pair{256u, 256u}, std::pair{256u, 128u}, std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{128u, 32u},  // 0C
    std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     // 0D
    std::pair{256u, 128u}, std::pair{256u, 128u}, std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 64u},  // 0E
    std::pair{256u, 128u}, std::pair{256u, 128u}, std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 64u},  // 0F
    std::pair{256u, 256u}, std::pair{256u, 128u}, std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{128u, 32u},  // 10
    std::pair{256u, 256u}, std::pair{256u, 128u}, std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{128u, 32u},  // 11
    std::pair{256u, 256u}, std::pair{256u, 128u}, std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{128u, 32u},  // 12
    std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     // 13
    std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{128u, 64u},  std::pair{128u, 64u},  // 14
    std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{128u, 64u},  std::pair{128u, 64u},  // 15
    std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{128u, 32u},  std::pair{128u, 32u},  std::pair{128u, 32u},  // 16
    std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{128u, 32u},  std::pair{128u, 32u},  std::pair{128u, 32u},  // 17
    std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{128u, 32u},  std::pair{128u, 32u},  std::pair{128u, 32u},  // 18
    std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{128u, 64u},  std::pair{128u, 64u},  std::pair{128u, 64u},  // 19
    std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{128u, 64u},  std::pair{128u, 64u},  std::pair{128u, 64u},  // 1A
};

constexpr std::array macro_tile_extents_alt_x2{
    std::pair{256u, 128u}, std::pair{256u, 128u}, std::pair{256u, 128u}, std::pair{256u, 128u}, std::pair{256u, 128u}, // 00
    std::pair{256u, 128u}, std::pair{256u, 128u}, std::pair{256u, 128u}, std::pair{256u, 128u}, std::pair{256u, 128u}, // 01
    std::pair{256u, 128u}, std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 128u}, // 02
    std::pair{256u, 128u}, std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 128u}, // 03
    std::pair{256u, 128u}, std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{128u, 64u},  // 04
    std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     // 05
    std::pair{256u, 128u}, std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 128u}, // 06
    std::pair{256u, 128u}, std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{128u, 32u},  std::pair{128u, 32u},  // 07
    std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     // 08
    std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     // 09
    std::pair{256u, 128u}, std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{128u, 64u},  // 0A
    std::pair{256u, 128u}, std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{128u, 32u},  std::pair{128u, 32u},  // 0B
    std::pair{256u, 128u}, std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{128u, 32u},  std::pair{128u, 32u},  // 0C
    std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     // 0D
    std::pair{256u, 128u}, std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{128u, 64u},  // 0E
    std::pair{256u, 128u}, std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{128u, 64u},  // 0F
    std::pair{256u, 128u}, std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{128u, 32u},  std::pair{128u, 32u},  // 10
    std::pair{256u, 128u}, std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{128u, 32u},  std::pair{128u, 32u},  // 11
    std::pair{256u, 128u}, std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{128u, 32u},  std::pair{128u, 32u},  // 12
    std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     // 13
    std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{128u, 64u},  std::pair{128u, 64u},  // 14
    std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{128u, 64u},  std::pair{128u, 64u},  // 15
    std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{128u, 32u},  std::pair{128u, 32u},  std::pair{128u, 32u},  // 16
    std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{128u, 32u},  std::pair{128u, 32u},  std::pair{128u, 32u},  // 17
    std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{128u, 32u},  std::pair{128u, 32u},  std::pair{128u, 32u},  // 18
    std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{128u, 64u},  std::pair{128u, 64u},  std::pair{128u, 64u},  // 19
    std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{128u, 64u},  std::pair{128u, 64u},  std::pair{128u, 64u},  // 1A
};

constexpr std::array macro_tile_extents_alt_x4{
    std::pair{256u, 128u}, std::pair{256u, 128u}, std::pair{256u, 128u}, std::pair{256u, 128u}, std::pair{256u, 128u}, // 00
    std::pair{256u, 128u}, std::pair{256u, 128u}, std::pair{256u, 128u}, std::pair{256u, 128u}, std::pair{256u, 128u}, // 01
    std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 128u}, // 02
    std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 128u}, // 03
    std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{128u, 64u},  std::pair{128u, 64u},  // 04
    std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     // 05
    std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 128u}, // 06
    std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{128u, 32u},  std::pair{128u, 32u},  std::pair{128u, 32u},  // 07
    std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     // 08
    std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     // 09
    std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{128u, 64u},  // 0A
    std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{128u, 32u},  std::pair{128u, 32u},  // 0B
    std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{128u, 32u},  std::pair{128u, 32u},  // 0C
    std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     // 0D
    std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{128u, 64u},  // 0E
    std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{128u, 64u},  // 0F
    std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{128u, 32u},  std::pair{128u, 32u},  // 10
    std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{128u, 32u},  std::pair{128u, 32u},  // 11
    std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{128u, 32u},  std::pair{128u, 32u},  // 12
    std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     // 13
    std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{128u, 64u},  std::pair{128u, 64u},  // 14
    std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{128u, 64u},  std::pair{128u, 64u},  // 15
    std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{128u, 32u},  std::pair{128u, 32u},  std::pair{128u, 32u},  // 16
    std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{128u, 32u},  std::pair{128u, 32u},  std::pair{128u, 32u},  // 17
    std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{128u, 32u},  std::pair{128u, 32u},  std::pair{128u, 32u},  // 18
    std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{128u, 64u},  std::pair{128u, 64u},  std::pair{128u, 64u},  // 19
    std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{128u, 64u},  std::pair{128u, 64u},  std::pair{128u, 64u},  // 1A
};

constexpr std::array macro_tile_extents_alt_x8{
    std::pair{256u, 128u}, std::pair{256u, 128u}, std::pair{256u, 128u}, std::pair{256u, 128u}, std::pair{256u, 128u}, // 00
    std::pair{256u, 128u}, std::pair{256u, 128u}, std::pair{256u, 128u}, std::pair{256u, 128u}, std::pair{256u, 128u}, // 01
    std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 128u}, // 02
    std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 128u}, // 03
    std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{128u, 64u},  std::pair{128u, 64u},  std::pair{128u, 64u},  // 04
    std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     // 05
    std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 128u}, // 06
    std::pair{128u, 64u},  std::pair{128u, 32u},  std::pair{128u, 32u},  std::pair{128u, 32u},  std::pair{128u, 32u},  // 07
    std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     // 08
    std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     // 09
    std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{128u, 64u},  // 0A
    std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{128u, 32u},  std::pair{128u, 32u},  // 0B
    std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{128u, 32u},  std::pair{128u, 32u},  // 0C
    std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     // 0D
    std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{128u, 64u},  // 0E
    std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{128u, 64u},  // 0F
    std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{128u, 32u},  std::pair{128u, 32u},  // 10
    std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{128u, 32u},  std::pair{128u, 32u},  // 11
    std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{128u, 32u},  std::pair{128u, 32u},  // 12
    std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     std::pair{0u, 0u},     // 13
    std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{128u, 64u},  std::pair{128u, 64u},  // 14
    std::pair{128u, 128u}, std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{128u, 64u},  std::pair{128u, 64u},  // 15
    std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{128u, 32u},  std::pair{128u, 32u},  std::pair{128u, 32u},  // 16
    std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{128u, 32u},  std::pair{128u, 32u},  std::pair{128u, 32u},  // 17
    std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{128u, 32u},  std::pair{128u, 32u},  std::pair{128u, 32u},  // 18
    std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{128u, 64u},  std::pair{128u, 64u},  std::pair{128u, 64u},  // 19
    std::pair{128u, 128u}, std::pair{128u, 64u},  std::pair{128u, 64u},  std::pair{128u, 64u},  std::pair{128u, 64u},  // 1A
};

constexpr std::array macro_tile_extents{
    macro_tile_extents_x1,
    macro_tile_extents_x2,
    macro_tile_extents_x4,
    macro_tile_extents_x8,
};

constexpr std::array macro_tile_extents_alt{
    macro_tile_extents_alt_x1,
    macro_tile_extents_alt_x2,
    macro_tile_extents_alt_x4,
    macro_tile_extents_alt_x8,
};
// clang-format on

constexpr std::pair micro_tile_extent{8u, 8u};
constexpr auto hw_pipe_interleave = 256u;

constexpr std::pair<u32, u32> GetMacroTileExtents(AmdGpu::TileMode tile_mode, u32 bpp,
                                                  u32 num_samples, bool alt) {
    ASSERT(num_samples <= 8);
    const auto samples_log = static_cast<u32>(std::log2(num_samples));
    const auto row = u32(tile_mode) * 5;
    const auto column = std::bit_width(bpp) - 4; // bpps are 8, 16, 32, 64, 128
    return (alt ? macro_tile_extents_alt : macro_tile_extents)[samples_log][row + column];
}

constexpr std::tuple<u32, u32, size_t> ImageSizeLinearAligned(u32 pitch, u32 height, u32 bpp,
                                                              u32 num_samples) {
    const auto pitch_align = std::max(8u, 64u / ((bpp + 7) / 8));
    auto pitch_aligned = (pitch + pitch_align - 1) & ~(pitch_align - 1);
    const auto height_aligned = height;
    size_t log_sz = pitch_aligned * height_aligned * num_samples;
    const auto slice_align = std::max(64u, 256u / ((bpp + 7) / 8));
    while (log_sz % slice_align) {
        pitch_aligned += pitch_align;
        log_sz = pitch_aligned * height_aligned * num_samples;
    }
    return {pitch_aligned, height_aligned, (log_sz * bpp + 7) / 8};
}

constexpr std::tuple<u32, u32, size_t> ImageSizeMicroTiled(u32 pitch, u32 height, u32 thickness,
                                                           u32 bpp, u32 num_samples) {
    const auto& [pitch_align, height_align] = micro_tile_extent;
    auto pitch_aligned = (pitch + pitch_align - 1) & ~(pitch_align - 1);
    const auto height_aligned = (height + height_align - 1) & ~(height_align - 1);
    size_t log_sz = (pitch_aligned * height_aligned * bpp * num_samples + 7) / 8;
    while ((log_sz * thickness) % 256) {
        pitch_aligned += pitch_align;
        log_sz = (pitch_aligned * height_aligned * bpp * num_samples + 7) / 8;
    }
    return {pitch_aligned, height_aligned, log_sz};
}

constexpr std::tuple<u32, u32, size_t> ImageSizeMacroTiled(u32 pitch, u32 height, u32 thickness,
                                                           u32 bpp, u32 num_samples,
                                                           AmdGpu::TileMode tile_mode, u32 mip_n,
                                                           bool alt) {
    const auto [pitch_align, height_align] = GetMacroTileExtents(tile_mode, bpp, num_samples, alt);
    ASSERT(pitch_align != 0 && height_align != 0);
    bool downgrade_to_micro = false;
    if (mip_n > 0) {
        const bool is_less_than_tile = pitch < pitch_align || height < height_align;
        // TODO: threshold check
        downgrade_to_micro = is_less_than_tile;
    }

    if (downgrade_to_micro) {
        return ImageSizeMicroTiled(pitch, height, thickness, bpp, num_samples);
    }

    const auto pitch_aligned = (pitch + pitch_align - 1) & ~(pitch_align - 1);
    const auto height_aligned = (height + height_align - 1) & ~(height_align - 1);
    const auto log_sz = pitch_aligned * height_aligned * num_samples;
    return {pitch_aligned, height_aligned, (log_sz * bpp + 7) / 8};
}

} // namespace VideoCore
