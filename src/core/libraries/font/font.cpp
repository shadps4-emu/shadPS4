// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/logging/log.h"
#include "core/libraries/error_codes.h"
#include "core/libraries/font/font.h"
#include "core/libraries/libs.h"
#include "font_error.h"

namespace Libraries::Font {

s32 PS4_SYSV_ABI sceFontAttachDeviceCacheBuffer() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontBindRenderer() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
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

    // Retrieve text order
    *pTextOrder = textCharacter->textOrder;
    return ORBIS_OK;
}

u32 PS4_SYSV_ABI sceFontCharacterLooksFormatCharacters(OrbisFontTextCharacter* textCharacter) {
    if (!textCharacter) {
        return 0;
    }

    // Check if the format flag (bit 2) is set
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
        return NULL; // Check if input is NULL

    OrbisFontTextCharacter* current = textCharacter->prev; // Move backward instead of forward
    while (current) {
        if (current->unkn_0x31 == 0 && current->unkn_0x33 == 0) {
            return current; // Return the first matching node
        }
        current = current->prev; // Move to the previous node
    }

    return NULL; // No valid node found
}

OrbisFontTextCharacter* PS4_SYSV_ABI
sceFontCharacterRefersTextNext(OrbisFontTextCharacter* textCharacter) {
    if (!textCharacter)
        return NULL; // Null check

    OrbisFontTextCharacter* current = textCharacter->next;
    while (current) {
        if (current->unkn_0x31 == 0 && current->unkn_0x33 == 0) {
            return current; // Found a match
        }
        current = current->next; // Move to the next node
    }

    return NULL; // No matching node found
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

s32 PS4_SYSV_ABI sceFontCreateLibrary() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontCreateLibraryWithEdition() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontCreateRenderer() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontCreateRendererWithEdition() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
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

s32 PS4_SYSV_ABI sceFontDestroyLibrary() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontDestroyRenderer() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
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

s32 PS4_SYSV_ABI sceFontGetCharGlyphMetrics() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
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

s32 PS4_SYSV_ABI sceFontGetHorizontalLayout() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGetKerning() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGetLibrary() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGetPixelResolution() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGetRenderCharGlyphMetrics() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
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

s32 PS4_SYSV_ABI sceFontGetRenderScalePixel() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGetRenderScalePoint() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGetResolutionDpi() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGetScalePixel() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontGetScalePoint() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
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

s32 PS4_SYSV_ABI sceFontGetVerticalLayout() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
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

s32 PS4_SYSV_ABI sceFontMemoryInit() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontMemoryTerm() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontOpenFontFile() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontOpenFontInstance() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontOpenFontMemory() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontOpenFontSet() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontRebindRenderer() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontRenderCharGlyphImage() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontRenderCharGlyphImageHorizontal() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
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
    if (renderSurface) { // Ensure surface is not NULL before modifying it
        renderSurface->buffer = buffer;
        renderSurface->widthByte = bufWidthByte;
        renderSurface->pixelSizeByte = pixelSizeByte;

        // Initialize unknown fields (likely reserved or flags)
        renderSurface->unkn_0xd = 0;
        renderSurface->styleFlag = 0;
        renderSurface->unkn_0xf = 0;

        // Ensure width and height are non-negative
        renderSurface->width = (widthPixel < 0) ? 0 : widthPixel;
        renderSurface->height = (heightPixel < 0) ? 0 : heightPixel;

        // Set the clipping/scaling rectangle
        renderSurface->sc_x0 = 0;
        renderSurface->sc_y0 = 0;
        renderSurface->sc_x1 = renderSurface->width;
        renderSurface->sc_y1 = renderSurface->height;
    }
}

void PS4_SYSV_ABI sceFontRenderSurfaceSetScissor(OrbisFontRenderSurface* renderSurface, int x0,
                                                 int y0, int w, int h) {
    if (!renderSurface)
        return; // Null check

    // Handle horizontal clipping
    int surfaceWidth = renderSurface->width;
    int clip_x0, clip_x1;

    if (surfaceWidth != 0) {
        if (x0 < 0) { // Adjust for negative x0
            clip_x0 = 0;
            clip_x1 = (w + x0 > surfaceWidth) ? surfaceWidth : w + x0;
            if (w <= -x0)
                clip_x1 = 0; // Entire width is clipped
        } else {
            clip_x0 = (x0 > surfaceWidth) ? surfaceWidth : x0;
            clip_x1 = (w + x0 > surfaceWidth) ? surfaceWidth : w + x0;
        }
        renderSurface->sc_x0 = clip_x0;
        renderSurface->sc_x1 = clip_x1;
    }

    // Handle vertical clipping
    int surfaceHeight = renderSurface->height;
    int clip_y0, clip_y1;

    if (surfaceHeight != 0) {
        if (y0 < 0) { // Adjust for negative y0
            clip_y0 = 0;
            clip_y1 = (h + y0 > surfaceHeight) ? surfaceHeight : h + y0;
            if (h <= -y0)
                clip_y1 = 0; // Entire height is clipped
        } else {
            clip_y0 = (y0 > surfaceHeight) ? surfaceHeight : y0;
            clip_y1 = (h + y0 > surfaceHeight) ? surfaceHeight : h + y0;
        }
        renderSurface->sc_y0 = clip_y0;
        renderSurface->sc_y1 = clip_y1;
    }
}

s32 PS4_SYSV_ABI sceFontRenderSurfaceSetStyleFrame(OrbisFontRenderSurface* renderSurface,
                                                   OrbisFontStyleFrame* styleFrame) {
    if (!renderSurface) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }

    if (!styleFrame) {
        renderSurface->styleFlag &= 0xFE; // Clear style flag
    } else {
        // Validate magic number
        if (styleFrame->magic != 0xF09) {
            return ORBIS_FONT_ERROR_INVALID_PARAMETER;
        }

        renderSurface->styleFlag |= 1; // Set style flag
    }

    // Assign style frame pointer
    renderSurface->unkn_28[0] = styleFrame;
    *(uint32_t*)(renderSurface->unkn_28 + 1) = 0; // Reset related field
}

s32 PS4_SYSV_ABI sceFontSetEffectSlant() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontSetEffectWeight() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontSetFontsOpenMode() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontSetResolutionDpi() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontSetScalePixel() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontSetScalePoint() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
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

s32 PS4_SYSV_ABI sceFontSetupRenderScalePixel() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontSetupRenderScalePoint() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
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
    if (!styleFrame) {
        printf("[ERROR] Style frame is NULL.\n");
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }

    // Validate the magic number
    if (styleFrame->magic != 0xF09) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }

    // Check if the slant effect is enabled (bit 1 in flags)
    if (!(styleFrame->flags & 0x02)) {
        return ORBIS_FONT_ERROR_UNSET_PARAMETER;
    }

    if (!slantRatio) {
        return ORBIS_FONT_ERROR_INVALID_PARAMETER;
    }

    // Retrieve slant ratio
    *slantRatio = styleFrame->slantRatio;
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontStyleFrameGetEffectWeight() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontStyleFrameGetResolutionDpi() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontStyleFrameGetScalePixel() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
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

s32 PS4_SYSV_ABI sceFontSupportExternalFonts() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontSupportGlyphs() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI sceFontSupportSystemFonts() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
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

s32 PS4_SYSV_ABI sceFontUnbindRenderer() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
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

s32 PS4_SYSV_ABI module_start() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

s32 PS4_SYSV_ABI module_stop() {
    LOG_ERROR(Lib_Font, "(STUBBED) called");
    return ORBIS_OK;
}

void RegisterlibSceFont(Core::Loader::SymbolsResolver* sym) {
    LIB_FUNCTION("CUKn5pX-NVY", "libSceFont", 1, "libSceFont", 1, 1,
                 sceFontAttachDeviceCacheBuffer);
    LIB_FUNCTION("3OdRkSjOcog", "libSceFont", 1, "libSceFont", 1, 1, sceFontBindRenderer);
    LIB_FUNCTION("6DFUkCwQLa8", "libSceFont", 1, "libSceFont", 1, 1, sceFontCharacterGetBidiLevel);
    LIB_FUNCTION("coCrV6IWplE", "libSceFont", 1, "libSceFont", 1, 1,
                 sceFontCharacterGetSyllableStringState);
    LIB_FUNCTION("zN3+nuA0SFQ", "libSceFont", 1, "libSceFont", 1, 1,
                 sceFontCharacterGetTextFontCode);
    LIB_FUNCTION("mxgmMj-Mq-o", "libSceFont", 1, "libSceFont", 1, 1, sceFontCharacterGetTextOrder);
    LIB_FUNCTION("-P6X35Rq2-E", "libSceFont", 1, "libSceFont", 1, 1,
                 sceFontCharacterLooksFormatCharacters);
    LIB_FUNCTION("SaRlqtqaCew", "libSceFont", 1, "libSceFont", 1, 1,
                 sceFontCharacterLooksWhiteSpace);
    LIB_FUNCTION("6Gqlv5KdTbU", "libSceFont", 1, "libSceFont", 1, 1,
                 sceFontCharacterRefersTextBack);
    LIB_FUNCTION("BkjBP+YC19w", "libSceFont", 1, "libSceFont", 1, 1,
                 sceFontCharacterRefersTextNext);
    LIB_FUNCTION("lVSR5ftvNag", "libSceFont", 1, "libSceFont", 1, 1,
                 sceFontCharactersRefersTextCodes);
    LIB_FUNCTION("I9R5VC6eZWo", "libSceFont", 1, "libSceFont", 1, 1, sceFontClearDeviceCache);
    LIB_FUNCTION("vzHs3C8lWJk", "libSceFont", 1, "libSceFont", 1, 1, sceFontCloseFont);
    LIB_FUNCTION("MpKSBaYKluo", "libSceFont", 1, "libSceFont", 1, 1, sceFontControl);
    LIB_FUNCTION("WBNBaj9XiJU", "libSceFont", 1, "libSceFont", 1, 1, sceFontCreateGraphicsDevice);
    LIB_FUNCTION("4So0MC3oBIM", "libSceFont", 1, "libSceFont", 1, 1, sceFontCreateGraphicsService);
    LIB_FUNCTION("NlO5Qlhjkng", "libSceFont", 1, "libSceFont", 1, 1,
                 sceFontCreateGraphicsServiceWithEdition);
    LIB_FUNCTION("nWrfPI4Okmg", "libSceFont", 1, "libSceFont", 1, 1, sceFontCreateLibrary);
    LIB_FUNCTION("n590hj5Oe-k", "libSceFont", 1, "libSceFont", 1, 1,
                 sceFontCreateLibraryWithEdition);
    LIB_FUNCTION("u5fZd3KZcs0", "libSceFont", 1, "libSceFont", 1, 1, sceFontCreateRenderer);
    LIB_FUNCTION("WaSFJoRWXaI", "libSceFont", 1, "libSceFont", 1, 1,
                 sceFontCreateRendererWithEdition);
    LIB_FUNCTION("MO24vDhmS4E", "libSceFont", 1, "libSceFont", 1, 1, sceFontCreateString);
    LIB_FUNCTION("cYrMGk1wrMA", "libSceFont", 1, "libSceFont", 1, 1, sceFontCreateWords);
    LIB_FUNCTION("7rogx92EEyc", "libSceFont", 1, "libSceFont", 1, 1, sceFontCreateWritingLine);
    LIB_FUNCTION("8h-SOB-asgk", "libSceFont", 1, "libSceFont", 1, 1, sceFontDefineAttribute);
    LIB_FUNCTION("LHDoRWVFGqk", "libSceFont", 1, "libSceFont", 1, 1, sceFontDeleteGlyph);
    LIB_FUNCTION("5QG71IjgOpQ", "libSceFont", 1, "libSceFont", 1, 1, sceFontDestroyGraphicsDevice);
    LIB_FUNCTION("zZQD3EwJo3c", "libSceFont", 1, "libSceFont", 1, 1, sceFontDestroyGraphicsService);
    LIB_FUNCTION("FXP359ygujs", "libSceFont", 1, "libSceFont", 1, 1, sceFontDestroyLibrary);
    LIB_FUNCTION("exAxkyVLt0s", "libSceFont", 1, "libSceFont", 1, 1, sceFontDestroyRenderer);
    LIB_FUNCTION("SSCaczu2aMQ", "libSceFont", 1, "libSceFont", 1, 1, sceFontDestroyString);
    LIB_FUNCTION("hWE4AwNixqY", "libSceFont", 1, "libSceFont", 1, 1, sceFontDestroyWords);
    LIB_FUNCTION("PEjv7CVDRYs", "libSceFont", 1, "libSceFont", 1, 1, sceFontDestroyWritingLine);
    LIB_FUNCTION("UuY-OJF+f0k", "libSceFont", 1, "libSceFont", 1, 1,
                 sceFontDettachDeviceCacheBuffer);
    LIB_FUNCTION("C-4Qw5Srlyw", "libSceFont", 1, "libSceFont", 1, 1, sceFontGenerateCharGlyph);
    LIB_FUNCTION("5kx49CAlO-M", "libSceFont", 1, "libSceFont", 1, 1, sceFontGetAttribute);
    LIB_FUNCTION("OINC0X9HGBY", "libSceFont", 1, "libSceFont", 1, 1, sceFontGetCharGlyphCode);
    LIB_FUNCTION("L97d+3OgMlE", "libSceFont", 1, "libSceFont", 1, 1, sceFontGetCharGlyphMetrics);
    LIB_FUNCTION("ynSqYL8VpoA", "libSceFont", 1, "libSceFont", 1, 1, sceFontGetEffectSlant);
    LIB_FUNCTION("d7dDgRY+Bzw", "libSceFont", 1, "libSceFont", 1, 1, sceFontGetEffectWeight);
    LIB_FUNCTION("ZB8xRemRRG8", "libSceFont", 1, "libSceFont", 1, 1, sceFontGetFontGlyphsCount);
    LIB_FUNCTION("4X14YSK4Ldk", "libSceFont", 1, "libSceFont", 1, 1,
                 sceFontGetFontGlyphsOutlineProfile);
    LIB_FUNCTION("eb9S3zNlV5o", "libSceFont", 1, "libSceFont", 1, 1, sceFontGetFontMetrics);
    LIB_FUNCTION("tiIlroGki+g", "libSceFont", 1, "libSceFont", 1, 1, sceFontGetFontResolution);
    LIB_FUNCTION("3hVv3SNoL6E", "libSceFont", 1, "libSceFont", 1, 1,
                 sceFontGetFontStyleInformation);
    LIB_FUNCTION("gVQpMBuB7fE", "libSceFont", 1, "libSceFont", 1, 1,
                 sceFontGetGlyphExpandBufferState);
    LIB_FUNCTION("imxVx8lm+KM", "libSceFont", 1, "libSceFont", 1, 1, sceFontGetHorizontalLayout);
    LIB_FUNCTION("sDuhHGNhHvE", "libSceFont", 1, "libSceFont", 1, 1, sceFontGetKerning);
    LIB_FUNCTION("LzmHDnlcwfQ", "libSceFont", 1, "libSceFont", 1, 1, sceFontGetLibrary);
    LIB_FUNCTION("BozJej5T6fs", "libSceFont", 1, "libSceFont", 1, 1, sceFontGetPixelResolution);
    LIB_FUNCTION("IQtleGLL5pQ", "libSceFont", 1, "libSceFont", 1, 1,
                 sceFontGetRenderCharGlyphMetrics);
    LIB_FUNCTION("Gqa5Pp7y4MU", "libSceFont", 1, "libSceFont", 1, 1, sceFontGetRenderEffectSlant);
    LIB_FUNCTION("woOjHrkjIYg", "libSceFont", 1, "libSceFont", 1, 1, sceFontGetRenderEffectWeight);
    LIB_FUNCTION("ryPlnDDI3rU", "libSceFont", 1, "libSceFont", 1, 1, sceFontGetRenderScaledKerning);
    LIB_FUNCTION("EY38A01lq2k", "libSceFont", 1, "libSceFont", 1, 1, sceFontGetRenderScalePixel);
    LIB_FUNCTION("FEafYUcxEGo", "libSceFont", 1, "libSceFont", 1, 1, sceFontGetRenderScalePoint);
    LIB_FUNCTION("8REoLjNGCpM", "libSceFont", 1, "libSceFont", 1, 1, sceFontGetResolutionDpi);
    LIB_FUNCTION("CkVmLoCNN-8", "libSceFont", 1, "libSceFont", 1, 1, sceFontGetScalePixel);
    LIB_FUNCTION("GoF2bhB7LYk", "libSceFont", 1, "libSceFont", 1, 1, sceFontGetScalePoint);
    LIB_FUNCTION("IrXeG0Lc6nA", "libSceFont", 1, "libSceFont", 1, 1, sceFontGetScriptLanguage);
    LIB_FUNCTION("7-miUT6pNQw", "libSceFont", 1, "libSceFont", 1, 1, sceFontGetTypographicDesign);
    LIB_FUNCTION("3BrWWFU+4ts", "libSceFont", 1, "libSceFont", 1, 1, sceFontGetVerticalLayout);
    LIB_FUNCTION("8-zmgsxkBek", "libSceFont", 1, "libSceFont", 1, 1, sceFontGlyphDefineAttribute);
    LIB_FUNCTION("oO33Uex4Ui0", "libSceFont", 1, "libSceFont", 1, 1, sceFontGlyphGetAttribute);
    LIB_FUNCTION("PXlA0M8ax40", "libSceFont", 1, "libSceFont", 1, 1, sceFontGlyphGetGlyphForm);
    LIB_FUNCTION("XUfSWpLhrUw", "libSceFont", 1, "libSceFont", 1, 1, sceFontGlyphGetMetricsForm);
    LIB_FUNCTION("lNnUqa1zA-M", "libSceFont", 1, "libSceFont", 1, 1, sceFontGlyphGetScalePixel);
    LIB_FUNCTION("ntrc3bEWlvQ", "libSceFont", 1, "libSceFont", 1, 1, sceFontGlyphRefersMetrics);
    LIB_FUNCTION("9kTbF59TjLs", "libSceFont", 1, "libSceFont", 1, 1,
                 sceFontGlyphRefersMetricsHorizontal);
    LIB_FUNCTION("nJavPEdMDvM", "libSceFont", 1, "libSceFont", 1, 1,
                 sceFontGlyphRefersMetricsHorizontalAdvance);
    LIB_FUNCTION("JCnVgZgcucs", "libSceFont", 1, "libSceFont", 1, 1,
                 sceFontGlyphRefersMetricsHorizontalX);
    LIB_FUNCTION("R1T4i+DOhNY", "libSceFont", 1, "libSceFont", 1, 1, sceFontGlyphRefersOutline);
    LIB_FUNCTION("RmkXfBcZnrM", "libSceFont", 1, "libSceFont", 1, 1, sceFontGlyphRenderImage);
    LIB_FUNCTION("r4KEihtwxGs", "libSceFont", 1, "libSceFont", 1, 1,
                 sceFontGlyphRenderImageHorizontal);
    LIB_FUNCTION("n22d-HIdmMg", "libSceFont", 1, "libSceFont", 1, 1,
                 sceFontGlyphRenderImageVertical);
    LIB_FUNCTION("RL2cAQgyXR8", "libSceFont", 1, "libSceFont", 1, 1, sceFontGraphicsBeginFrame);
    LIB_FUNCTION("dUmIK6QjT7E", "libSceFont", 1, "libSceFont", 1, 1, sceFontGraphicsDrawingCancel);
    LIB_FUNCTION("X2Vl3yU19Zw", "libSceFont", 1, "libSceFont", 1, 1, sceFontGraphicsDrawingFinish);
    LIB_FUNCTION("DOmdOwV3Aqw", "libSceFont", 1, "libSceFont", 1, 1, sceFontGraphicsEndFrame);
    LIB_FUNCTION("zdYdKRQC3rw", "libSceFont", 1, "libSceFont", 1, 1,
                 sceFontGraphicsExchangeResource);
    LIB_FUNCTION("UkMUIoj-e9s", "libSceFont", 1, "libSceFont", 1, 1, sceFontGraphicsFillMethodInit);
    LIB_FUNCTION("DJURdcnVUqo", "libSceFont", 1, "libSceFont", 1, 1, sceFontGraphicsFillPlotInit);
    LIB_FUNCTION("eQac6ftmBQQ", "libSceFont", 1, "libSceFont", 1, 1,
                 sceFontGraphicsFillPlotSetLayout);
    LIB_FUNCTION("PEYQJa+MWnk", "libSceFont", 1, "libSceFont", 1, 1,
                 sceFontGraphicsFillPlotSetMapping);
    LIB_FUNCTION("21g4m4kYF6g", "libSceFont", 1, "libSceFont", 1, 1, sceFontGraphicsFillRatesInit);
    LIB_FUNCTION("pJzji5FvdxU", "libSceFont", 1, "libSceFont", 1, 1,
                 sceFontGraphicsFillRatesSetFillEffect);
    LIB_FUNCTION("scaro-xEuUM", "libSceFont", 1, "libSceFont", 1, 1,
                 sceFontGraphicsFillRatesSetLayout);
    LIB_FUNCTION("W66Kqtt0xU0", "libSceFont", 1, "libSceFont", 1, 1,
                 sceFontGraphicsFillRatesSetMapping);
    LIB_FUNCTION("FzpLsBQEegQ", "libSceFont", 1, "libSceFont", 1, 1, sceFontGraphicsGetDeviceUsage);
    LIB_FUNCTION("W80hs0g5d+E", "libSceFont", 1, "libSceFont", 1, 1, sceFontGraphicsRegionInit);
    LIB_FUNCTION("S48+njg9p-o", "libSceFont", 1, "libSceFont", 1, 1,
                 sceFontGraphicsRegionInitCircular);
    LIB_FUNCTION("wcOQ8Fz73+M", "libSceFont", 1, "libSceFont", 1, 1,
                 sceFontGraphicsRegionInitRoundish);
    LIB_FUNCTION("YBaw2Yyfd5E", "libSceFont", 1, "libSceFont", 1, 1, sceFontGraphicsRelease);
    LIB_FUNCTION("qkySrQ4FGe0", "libSceFont", 1, "libSceFont", 1, 1, sceFontGraphicsRenderResource);
    LIB_FUNCTION("qzNjJYKVli0", "libSceFont", 1, "libSceFont", 1, 1, sceFontGraphicsSetFramePolicy);
    LIB_FUNCTION("9iRbHCtcx-o", "libSceFont", 1, "libSceFont", 1, 1, sceFontGraphicsSetupClipping);
    LIB_FUNCTION("KZ3qPyz5Opc", "libSceFont", 1, "libSceFont", 1, 1,
                 sceFontGraphicsSetupColorRates);
    LIB_FUNCTION("LqclbpVzRvM", "libSceFont", 1, "libSceFont", 1, 1,
                 sceFontGraphicsSetupFillMethod);
    LIB_FUNCTION("Wl4FiI4qKY0", "libSceFont", 1, "libSceFont", 1, 1, sceFontGraphicsSetupFillRates);
    LIB_FUNCTION("WC7s95TccVo", "libSceFont", 1, "libSceFont", 1, 1, sceFontGraphicsSetupGlyphFill);
    LIB_FUNCTION("zC6I4ty37NA", "libSceFont", 1, "libSceFont", 1, 1,
                 sceFontGraphicsSetupGlyphFillPlot);
    LIB_FUNCTION("drZUF0XKTEI", "libSceFont", 1, "libSceFont", 1, 1,
                 sceFontGraphicsSetupHandleDefault);
    LIB_FUNCTION("MEAmHMynQXE", "libSceFont", 1, "libSceFont", 1, 1, sceFontGraphicsSetupLocation);
    LIB_FUNCTION("XRUOmQhnYO4", "libSceFont", 1, "libSceFont", 1, 1,
                 sceFontGraphicsSetupPositioning);
    LIB_FUNCTION("98XGr2Bkklg", "libSceFont", 1, "libSceFont", 1, 1, sceFontGraphicsSetupRotation);
    LIB_FUNCTION("Nj-ZUVOVAvc", "libSceFont", 1, "libSceFont", 1, 1, sceFontGraphicsSetupScaling);
    LIB_FUNCTION("p0avT2ggev0", "libSceFont", 1, "libSceFont", 1, 1, sceFontGraphicsSetupShapeFill);
    LIB_FUNCTION("0C5aKg9KghY", "libSceFont", 1, "libSceFont", 1, 1,
                 sceFontGraphicsSetupShapeFillPlot);
    LIB_FUNCTION("4pA3qqAcYco", "libSceFont", 1, "libSceFont", 1, 1,
                 sceFontGraphicsStructureCanvas);
    LIB_FUNCTION("cpjgdlMYdOM", "libSceFont", 1, "libSceFont", 1, 1,
                 sceFontGraphicsStructureCanvasSequence);
    LIB_FUNCTION("774Mee21wKk", "libSceFont", 1, "libSceFont", 1, 1,
                 sceFontGraphicsStructureDesign);
    LIB_FUNCTION("Hp3NIFhUXvQ", "libSceFont", 1, "libSceFont", 1, 1,
                 sceFontGraphicsStructureDesignResource);
    LIB_FUNCTION("bhmZlml6NBs", "libSceFont", 1, "libSceFont", 1, 1,
                 sceFontGraphicsStructureSurfaceTexture);
    LIB_FUNCTION("5sAWgysOBfE", "libSceFont", 1, "libSceFont", 1, 1, sceFontGraphicsUpdateClipping);
    LIB_FUNCTION("W4e8obm+w6o", "libSceFont", 1, "libSceFont", 1, 1,
                 sceFontGraphicsUpdateColorRates);
    LIB_FUNCTION("EgIn3QBajPs", "libSceFont", 1, "libSceFont", 1, 1,
                 sceFontGraphicsUpdateFillMethod);
    LIB_FUNCTION("MnUYAs2jVuU", "libSceFont", 1, "libSceFont", 1, 1,
                 sceFontGraphicsUpdateFillRates);
    LIB_FUNCTION("R-oVDMusYbc", "libSceFont", 1, "libSceFont", 1, 1,
                 sceFontGraphicsUpdateGlyphFill);
    LIB_FUNCTION("b9R+HQuHSMI", "libSceFont", 1, "libSceFont", 1, 1,
                 sceFontGraphicsUpdateGlyphFillPlot);
    LIB_FUNCTION("IN4P5pJADQY", "libSceFont", 1, "libSceFont", 1, 1, sceFontGraphicsUpdateLocation);
    LIB_FUNCTION("U+LLXdr2DxM", "libSceFont", 1, "libSceFont", 1, 1,
                 sceFontGraphicsUpdatePositioning);
    LIB_FUNCTION("yStTYSeb4NM", "libSceFont", 1, "libSceFont", 1, 1, sceFontGraphicsUpdateRotation);
    LIB_FUNCTION("eDxmMoxE5xU", "libSceFont", 1, "libSceFont", 1, 1, sceFontGraphicsUpdateScaling);
    LIB_FUNCTION("Ax6LQJJq6HQ", "libSceFont", 1, "libSceFont", 1, 1,
                 sceFontGraphicsUpdateShapeFill);
    LIB_FUNCTION("I5Rf2rXvBKQ", "libSceFont", 1, "libSceFont", 1, 1,
                 sceFontGraphicsUpdateShapeFillPlot);
    LIB_FUNCTION("whrS4oksXc4", "libSceFont", 1, "libSceFont", 1, 1, sceFontMemoryInit);
    LIB_FUNCTION("h6hIgxXEiEc", "libSceFont", 1, "libSceFont", 1, 1, sceFontMemoryTerm);
    LIB_FUNCTION("RvXyHMUiLhE", "libSceFont", 1, "libSceFont", 1, 1, sceFontOpenFontFile);
    LIB_FUNCTION("JzCH3SCFnAU", "libSceFont", 1, "libSceFont", 1, 1, sceFontOpenFontInstance);
    LIB_FUNCTION("KXUpebrFk1U", "libSceFont", 1, "libSceFont", 1, 1, sceFontOpenFontMemory);
    LIB_FUNCTION("cKYtVmeSTcw", "libSceFont", 1, "libSceFont", 1, 1, sceFontOpenFontSet);
    LIB_FUNCTION("Z2cdsqJH+5k", "libSceFont", 1, "libSceFont", 1, 1, sceFontRebindRenderer);
    LIB_FUNCTION("3G4zhgKuxE8", "libSceFont", 1, "libSceFont", 1, 1, sceFontRenderCharGlyphImage);
    LIB_FUNCTION("kAenWy1Zw5o", "libSceFont", 1, "libSceFont", 1, 1,
                 sceFontRenderCharGlyphImageHorizontal);
    LIB_FUNCTION("i6UNdSig1uE", "libSceFont", 1, "libSceFont", 1, 1,
                 sceFontRenderCharGlyphImageVertical);
    LIB_FUNCTION("amcmrY62BD4", "libSceFont", 1, "libSceFont", 1, 1,
                 sceFontRendererGetOutlineBufferSize);
    LIB_FUNCTION("ai6AfGrBs4o", "libSceFont", 1, "libSceFont", 1, 1,
                 sceFontRendererResetOutlineBuffer);
    LIB_FUNCTION("ydF+WuH0fAk", "libSceFont", 1, "libSceFont", 1, 1,
                 sceFontRendererSetOutlineBufferPolicy);
    LIB_FUNCTION("gdUCnU0gHdI", "libSceFont", 1, "libSceFont", 1, 1, sceFontRenderSurfaceInit);
    LIB_FUNCTION("vRxf4d0ulPs", "libSceFont", 1, "libSceFont", 1, 1,
                 sceFontRenderSurfaceSetScissor);
    LIB_FUNCTION("0hr-w30SjiI", "libSceFont", 1, "libSceFont", 1, 1,
                 sceFontRenderSurfaceSetStyleFrame);
    LIB_FUNCTION("TMtqoFQjjbA", "libSceFont", 1, "libSceFont", 1, 1, sceFontSetEffectSlant);
    LIB_FUNCTION("v0phZwa4R5o", "libSceFont", 1, "libSceFont", 1, 1, sceFontSetEffectWeight);
    LIB_FUNCTION("kihFGYJee7o", "libSceFont", 1, "libSceFont", 1, 1, sceFontSetFontsOpenMode);
    LIB_FUNCTION("I1acwR7Qp8E", "libSceFont", 1, "libSceFont", 1, 1, sceFontSetResolutionDpi);
    LIB_FUNCTION("N1EBMeGhf7E", "libSceFont", 1, "libSceFont", 1, 1, sceFontSetScalePixel);
    LIB_FUNCTION("sw65+7wXCKE", "libSceFont", 1, "libSceFont", 1, 1, sceFontSetScalePoint);
    LIB_FUNCTION("PxSR9UfJ+SQ", "libSceFont", 1, "libSceFont", 1, 1, sceFontSetScriptLanguage);
    LIB_FUNCTION("SnsZua35ngs", "libSceFont", 1, "libSceFont", 1, 1, sceFontSetTypographicDesign);
    LIB_FUNCTION("lz9y9UFO2UU", "libSceFont", 1, "libSceFont", 1, 1, sceFontSetupRenderEffectSlant);
    LIB_FUNCTION("XIGorvLusDQ", "libSceFont", 1, "libSceFont", 1, 1,
                 sceFontSetupRenderEffectWeight);
    LIB_FUNCTION("6vGCkkQJOcI", "libSceFont", 1, "libSceFont", 1, 1, sceFontSetupRenderScalePixel);
    LIB_FUNCTION("nMZid4oDfi4", "libSceFont", 1, "libSceFont", 1, 1, sceFontSetupRenderScalePoint);
    LIB_FUNCTION("ObkDGDBsVtw", "libSceFont", 1, "libSceFont", 1, 1, sceFontStringGetTerminateCode);
    LIB_FUNCTION("+B-xlbiWDJ4", "libSceFont", 1, "libSceFont", 1, 1,
                 sceFontStringGetTerminateOrder);
    LIB_FUNCTION("o1vIEHeb6tw", "libSceFont", 1, "libSceFont", 1, 1, sceFontStringGetWritingForm);
    LIB_FUNCTION("hq5LffQjz-s", "libSceFont", 1, "libSceFont", 1, 1,
                 sceFontStringRefersRenderCharacters);
    LIB_FUNCTION("Avv7OApgCJk", "libSceFont", 1, "libSceFont", 1, 1,
                 sceFontStringRefersTextCharacters);
    LIB_FUNCTION("lOfduYnjgbo", "libSceFont", 1, "libSceFont", 1, 1,
                 sceFontStyleFrameGetEffectSlant);
    LIB_FUNCTION("HIUdjR-+Wl8", "libSceFont", 1, "libSceFont", 1, 1,
                 sceFontStyleFrameGetEffectWeight);
    LIB_FUNCTION("VSw18Aqzl0U", "libSceFont", 1, "libSceFont", 1, 1,
                 sceFontStyleFrameGetResolutionDpi);
    LIB_FUNCTION("2QfqfeLblbg", "libSceFont", 1, "libSceFont", 1, 1,
                 sceFontStyleFrameGetScalePixel);
    LIB_FUNCTION("7x2xKiiB7MA", "libSceFont", 1, "libSceFont", 1, 1,
                 sceFontStyleFrameGetScalePoint);
    LIB_FUNCTION("la2AOWnHEAc", "libSceFont", 1, "libSceFont", 1, 1, sceFontStyleFrameInit);
    LIB_FUNCTION("394sckksiCU", "libSceFont", 1, "libSceFont", 1, 1,
                 sceFontStyleFrameSetEffectSlant);
    LIB_FUNCTION("faw77-pEBmU", "libSceFont", 1, "libSceFont", 1, 1,
                 sceFontStyleFrameSetEffectWeight);
    LIB_FUNCTION("dB4-3Wdwls8", "libSceFont", 1, "libSceFont", 1, 1,
                 sceFontStyleFrameSetResolutionDpi);
    LIB_FUNCTION("da4rQ4-+p-4", "libSceFont", 1, "libSceFont", 1, 1,
                 sceFontStyleFrameSetScalePixel);
    LIB_FUNCTION("O997laxY-Ys", "libSceFont", 1, "libSceFont", 1, 1,
                 sceFontStyleFrameSetScalePoint);
    LIB_FUNCTION("dUmABkAnVgk", "libSceFont", 1, "libSceFont", 1, 1,
                 sceFontStyleFrameUnsetEffectSlant);
    LIB_FUNCTION("hwsuXgmKdaw", "libSceFont", 1, "libSceFont", 1, 1,
                 sceFontStyleFrameUnsetEffectWeight);
    LIB_FUNCTION("bePC0L0vQWY", "libSceFont", 1, "libSceFont", 1, 1, sceFontStyleFrameUnsetScale);
    LIB_FUNCTION("mz2iTY0MK4A", "libSceFont", 1, "libSceFont", 1, 1, sceFontSupportExternalFonts);
    LIB_FUNCTION("71w5DzObuZI", "libSceFont", 1, "libSceFont", 1, 1, sceFontSupportGlyphs);
    LIB_FUNCTION("SsRbbCiWoGw", "libSceFont", 1, "libSceFont", 1, 1, sceFontSupportSystemFonts);
    LIB_FUNCTION("IPoYwwlMx-g", "libSceFont", 1, "libSceFont", 1, 1, sceFontTextCodesStepBack);
    LIB_FUNCTION("olSmXY+XP1E", "libSceFont", 1, "libSceFont", 1, 1, sceFontTextCodesStepNext);
    LIB_FUNCTION("oaJ1BpN2FQk", "libSceFont", 1, "libSceFont", 1, 1, sceFontTextSourceInit);
    LIB_FUNCTION("VRFd3diReec", "libSceFont", 1, "libSceFont", 1, 1, sceFontTextSourceRewind);
    LIB_FUNCTION("eCRMCSk96NU", "libSceFont", 1, "libSceFont", 1, 1,
                 sceFontTextSourceSetDefaultFont);
    LIB_FUNCTION("OqQKX0h5COw", "libSceFont", 1, "libSceFont", 1, 1,
                 sceFontTextSourceSetWritingForm);
    LIB_FUNCTION("1QjhKxrsOB8", "libSceFont", 1, "libSceFont", 1, 1, sceFontUnbindRenderer);
    LIB_FUNCTION("H-FNq8isKE0", "libSceFont", 1, "libSceFont", 1, 1,
                 sceFontWordsFindWordCharacters);
    LIB_FUNCTION("fljdejMcG1c", "libSceFont", 1, "libSceFont", 1, 1,
                 sceFontWritingGetRenderMetrics);
    LIB_FUNCTION("fD5rqhEXKYQ", "libSceFont", 1, "libSceFont", 1, 1, sceFontWritingInit);
    LIB_FUNCTION("1+DgKL0haWQ", "libSceFont", 1, "libSceFont", 1, 1, sceFontWritingLineClear);
    LIB_FUNCTION("JQKWIsS9joE", "libSceFont", 1, "libSceFont", 1, 1,
                 sceFontWritingLineGetOrderingSpace);
    LIB_FUNCTION("nlU2VnfpqTM", "libSceFont", 1, "libSceFont", 1, 1,
                 sceFontWritingLineGetRenderMetrics);
    LIB_FUNCTION("+FYcYefsVX0", "libSceFont", 1, "libSceFont", 1, 1,
                 sceFontWritingLineRefersRenderStep);
    LIB_FUNCTION("wyKFUOWdu3Q", "libSceFont", 1, "libSceFont", 1, 1, sceFontWritingLineWritesOrder);
    LIB_FUNCTION("W-2WOXEHGck", "libSceFont", 1, "libSceFont", 1, 1,
                 sceFontWritingRefersRenderStep);
    LIB_FUNCTION("f4Onl7efPEY", "libSceFont", 1, "libSceFont", 1, 1,
                 sceFontWritingRefersRenderStepCharacter);
    LIB_FUNCTION("BbCZjJizU4A", "libSceFont", 1, "libSceFont", 1, 1,
                 sceFontWritingSetMaskInvisible);
    LIB_FUNCTION("APTXePHIjLM", "libSceFont", 1, "libSceFont", 1, 1, Func_00F4D778F1C88CB3);
    LIB_FUNCTION("A8ZQAl+7Dec", "libSceFont", 1, "libSceFont", 1, 1, Func_03C650025FBB0DE7);
    LIB_FUNCTION("B+q4oWOyfho", "libSceFont", 1, "libSceFont", 1, 1, Func_07EAB8A163B27E1A);
    LIB_FUNCTION("CUCOiOT5fOM", "libSceFont", 1, "libSceFont", 1, 1, Func_09408E88E4F97CE3);
    LIB_FUNCTION("CfkpBe2CqBQ", "libSceFont", 1, "libSceFont", 1, 1, Func_09F92905ED82A814);
    LIB_FUNCTION("DRQs7hqyGr4", "libSceFont", 1, "libSceFont", 1, 1, Func_0D142CEE1AB21ABE);
    LIB_FUNCTION("FL0unhGcFvI", "libSceFont", 1, "libSceFont", 1, 1, Func_14BD2E9E119C16F2);
    LIB_FUNCTION("GsU8nt6ujXU", "libSceFont", 1, "libSceFont", 1, 1, Func_1AC53C9EDEAE8D75);
    LIB_FUNCTION("HUARhdXiTD0", "libSceFont", 1, "libSceFont", 1, 1, Func_1D401185D5E24C3D);
    LIB_FUNCTION("HoPNIMLMmW8", "libSceFont", 1, "libSceFont", 1, 1, Func_1E83CD20C2CC996F);
    LIB_FUNCTION("MUsfdluf54o", "libSceFont", 1, "libSceFont", 1, 1, Func_314B1F765B9FE78A);
    LIB_FUNCTION("NQ5nJf7eKeE", "libSceFont", 1, "libSceFont", 1, 1, Func_350E6725FEDE29E1);
    LIB_FUNCTION("Pbdz8KYEvzk", "libSceFont", 1, "libSceFont", 1, 1, Func_3DB773F0A604BF39);
    LIB_FUNCTION("T-Sd0h4xGxw", "libSceFont", 1, "libSceFont", 1, 1, Func_4FF49DD21E311B1C);
    LIB_FUNCTION("UmKHZkpJOYE", "libSceFont", 1, "libSceFont", 1, 1, Func_526287664A493981);
    LIB_FUNCTION("VcpxjbyEpuk", "libSceFont", 1, "libSceFont", 1, 1, Func_55CA718DBC84A6E9);
    LIB_FUNCTION("Vj-F8HBqi00", "libSceFont", 1, "libSceFont", 1, 1, Func_563FC5F0706A8B4D);
    LIB_FUNCTION("Vp4uzTQpD0U", "libSceFont", 1, "libSceFont", 1, 1, Func_569E2ECD34290F45);
    LIB_FUNCTION("WgR3W2vkdoU", "libSceFont", 1, "libSceFont", 1, 1, Func_5A04775B6BE47685);
    LIB_FUNCTION("X9k7yrb3l1A", "libSceFont", 1, "libSceFont", 1, 1, Func_5FD93BCAB6F79750);
    LIB_FUNCTION("YrU5j4ZL07Q", "libSceFont", 1, "libSceFont", 1, 1, Func_62B5398F864BD3B4);
    LIB_FUNCTION("b5AQKU2CI2c", "libSceFont", 1, "libSceFont", 1, 1, Func_6F9010294D822367);
    LIB_FUNCTION("d1fpR0I6emc", "libSceFont", 1, "libSceFont", 1, 1, Func_7757E947423A7A67);
    LIB_FUNCTION("fga6Ugd-VPo", "libSceFont", 1, "libSceFont", 1, 1, Func_7E06BA52077F54FA);
    LIB_FUNCTION("k7Nt6gITEdY", "libSceFont", 1, "libSceFont", 1, 1, Func_93B36DEA021311D6);
    LIB_FUNCTION("lLCJHnERWYo", "libSceFont", 1, "libSceFont", 1, 1, Func_94B0891E7111598A);
    LIB_FUNCTION("l4XJEowv580", "libSceFont", 1, "libSceFont", 1, 1, Func_9785C9128C2FE7CD);
    LIB_FUNCTION("l9+8m2X7wOE", "libSceFont", 1, "libSceFont", 1, 1, Func_97DFBC9B65FBC0E1);
    LIB_FUNCTION("rNlxdAXX08o", "libSceFont", 1, "libSceFont", 1, 1, Func_ACD9717405D7D3CA);
    LIB_FUNCTION("sZqK7D-U8W8", "libSceFont", 1, "libSceFont", 1, 1, Func_B19A8AEC3FD4F16F);
    LIB_FUNCTION("wQ9IitfPED0", "libSceFont", 1, "libSceFont", 1, 1, Func_C10F488AD7CF103D);
    LIB_FUNCTION("0Mi1-0poJsc", "libSceFont", 1, "libSceFont", 1, 1, Func_D0C8B5FF4A6826C7);
    LIB_FUNCTION("5I080Bw0KjM", "libSceFont", 1, "libSceFont", 1, 1, Func_E48D3CD01C342A33);
    LIB_FUNCTION("6slrIYa3HhQ", "libSceFont", 1, "libSceFont", 1, 1, Func_EAC96B2186B71E14);
    LIB_FUNCTION("-keIqW70YlY", "libSceFont", 1, "libSceFont", 1, 1, Func_FE4788A96EF46256);
    LIB_FUNCTION("-n5a6V0wWPU", "libSceFont", 1, "libSceFont", 1, 1, Func_FE7E5AE95D3058F5);
};

} // namespace Libraries::Font