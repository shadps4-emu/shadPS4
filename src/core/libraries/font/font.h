// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Font {

struct OrbisFontTextCharacter {
    // Other fields...
    struct OrbisFontTextCharacter* next; // Pointer to the next node 0x00
    struct OrbisFontTextCharacter* prev; // Pointer to the next node 0x08
    void* textOrder;                     // Field at offset 0x10 (pointer to text order info)
    u32 characterCode;                   // Field assumed at offset 0x28
    u8 unkn_0x31;                        // Offset 0x31
    u8 unkn_0x33;                        // Offset 0x33
    u8 charType;                         // Field assumed at offset 0x39
    u8 bidiLevel;                        // Field assumed at offset 0x3B stores the Bidi level
    u8 formatFlags;                      // Field at offset 0x3D (stores format-related flags)
};

struct OrbisFontRenderSurface {
    void* buffer;
    s32 widthByte;
    s8 pixelSizeByte;
    u8 unkn_0xd;
    u8 styleFlag;
    u8 unkn_0xf;
    s32 width, height;
    u32 sc_x0;
    u32 sc_y0;
    u32 sc_x1;
    u32 sc_y1;
    void* unkn_28[3];
};

struct OrbisFontStyleFrame {
    /*0x00*/ u16 magic; // Expected to be 0xF09
    /*0x02*/ u16 flags;
    /*0x04*/ s32 dpiX;          // DPI scaling factor for width
    /*0x08*/ s32 dpiY;          // DPI scaling factor for height
    /*0x0c*/ s32 scalingFlag;   // Indicates whether scaling is enabled
                                /*0x10*/
    /*0x14*/ float scaleWidth;  // Width scaling factor
    /*0x18*/ float scaleHeight; // Height scaling factor
    /*0x1c*/ float weightXScale;
    /*0x20*/ float weightYScale;
    /*0x24*/ float slantRatio;
};

s32 PS4_SYSV_ABI sceFontAttachDeviceCacheBuffer();
s32 PS4_SYSV_ABI sceFontBindRenderer();
s32 PS4_SYSV_ABI sceFontCharacterGetBidiLevel(OrbisFontTextCharacter* textCharacter,
                                              int* bidiLevel);
s32 PS4_SYSV_ABI sceFontCharacterGetSyllableStringState();
s32 PS4_SYSV_ABI sceFontCharacterGetTextFontCode();
s32 PS4_SYSV_ABI sceFontCharacterGetTextOrder(OrbisFontTextCharacter* textCharacter,
                                              void** pTextOrder);
u32 PS4_SYSV_ABI sceFontCharacterLooksFormatCharacters(OrbisFontTextCharacter* textCharacter);
u32 PS4_SYSV_ABI sceFontCharacterLooksWhiteSpace(OrbisFontTextCharacter* textCharacter);
OrbisFontTextCharacter* PS4_SYSV_ABI
sceFontCharacterRefersTextBack(OrbisFontTextCharacter* textCharacter);
OrbisFontTextCharacter* PS4_SYSV_ABI
sceFontCharacterRefersTextNext(OrbisFontTextCharacter* textCharacter);
s32 PS4_SYSV_ABI sceFontCharactersRefersTextCodes();
s32 PS4_SYSV_ABI sceFontClearDeviceCache();
s32 PS4_SYSV_ABI sceFontCloseFont();
s32 PS4_SYSV_ABI sceFontControl();
s32 PS4_SYSV_ABI sceFontCreateGraphicsDevice();
s32 PS4_SYSV_ABI sceFontCreateGraphicsService();
s32 PS4_SYSV_ABI sceFontCreateGraphicsServiceWithEdition();
s32 PS4_SYSV_ABI sceFontCreateLibrary();
s32 PS4_SYSV_ABI sceFontCreateLibraryWithEdition();
s32 PS4_SYSV_ABI sceFontCreateRenderer();
s32 PS4_SYSV_ABI sceFontCreateRendererWithEdition();
s32 PS4_SYSV_ABI sceFontCreateString();
s32 PS4_SYSV_ABI sceFontCreateWords();
s32 PS4_SYSV_ABI sceFontCreateWritingLine();
s32 PS4_SYSV_ABI sceFontDefineAttribute();
s32 PS4_SYSV_ABI sceFontDeleteGlyph();
s32 PS4_SYSV_ABI sceFontDestroyGraphicsDevice();
s32 PS4_SYSV_ABI sceFontDestroyGraphicsService();
s32 PS4_SYSV_ABI sceFontDestroyLibrary();
s32 PS4_SYSV_ABI sceFontDestroyRenderer();
s32 PS4_SYSV_ABI sceFontDestroyString();
s32 PS4_SYSV_ABI sceFontDestroyWords();
s32 PS4_SYSV_ABI sceFontDestroyWritingLine();
s32 PS4_SYSV_ABI sceFontDettachDeviceCacheBuffer();
s32 PS4_SYSV_ABI sceFontGenerateCharGlyph();
s32 PS4_SYSV_ABI sceFontGetAttribute();
s32 PS4_SYSV_ABI sceFontGetCharGlyphCode();
s32 PS4_SYSV_ABI sceFontGetCharGlyphMetrics();
s32 PS4_SYSV_ABI sceFontGetEffectSlant();
s32 PS4_SYSV_ABI sceFontGetEffectWeight();
s32 PS4_SYSV_ABI sceFontGetFontGlyphsCount();
s32 PS4_SYSV_ABI sceFontGetFontGlyphsOutlineProfile();
s32 PS4_SYSV_ABI sceFontGetFontMetrics();
s32 PS4_SYSV_ABI sceFontGetFontResolution();
s32 PS4_SYSV_ABI sceFontGetFontStyleInformation();
s32 PS4_SYSV_ABI sceFontGetGlyphExpandBufferState();
s32 PS4_SYSV_ABI sceFontGetHorizontalLayout();
s32 PS4_SYSV_ABI sceFontGetKerning();
s32 PS4_SYSV_ABI sceFontGetLibrary();
s32 PS4_SYSV_ABI sceFontGetPixelResolution();
s32 PS4_SYSV_ABI sceFontGetRenderCharGlyphMetrics();
s32 PS4_SYSV_ABI sceFontGetRenderEffectSlant();
s32 PS4_SYSV_ABI sceFontGetRenderEffectWeight();
s32 PS4_SYSV_ABI sceFontGetRenderScaledKerning();
s32 PS4_SYSV_ABI sceFontGetRenderScalePixel();
s32 PS4_SYSV_ABI sceFontGetRenderScalePoint();
s32 PS4_SYSV_ABI sceFontGetResolutionDpi();
s32 PS4_SYSV_ABI sceFontGetScalePixel();
s32 PS4_SYSV_ABI sceFontGetScalePoint();
s32 PS4_SYSV_ABI sceFontGetScriptLanguage();
s32 PS4_SYSV_ABI sceFontGetTypographicDesign();
s32 PS4_SYSV_ABI sceFontGetVerticalLayout();
s32 PS4_SYSV_ABI sceFontGlyphDefineAttribute();
s32 PS4_SYSV_ABI sceFontGlyphGetAttribute();
s32 PS4_SYSV_ABI sceFontGlyphGetGlyphForm();
s32 PS4_SYSV_ABI sceFontGlyphGetMetricsForm();
s32 PS4_SYSV_ABI sceFontGlyphGetScalePixel();
s32 PS4_SYSV_ABI sceFontGlyphRefersMetrics();
s32 PS4_SYSV_ABI sceFontGlyphRefersMetricsHorizontal();
s32 PS4_SYSV_ABI sceFontGlyphRefersMetricsHorizontalAdvance();
s32 PS4_SYSV_ABI sceFontGlyphRefersMetricsHorizontalX();
s32 PS4_SYSV_ABI sceFontGlyphRefersOutline();
s32 PS4_SYSV_ABI sceFontGlyphRenderImage();
s32 PS4_SYSV_ABI sceFontGlyphRenderImageHorizontal();
s32 PS4_SYSV_ABI sceFontGlyphRenderImageVertical();
s32 PS4_SYSV_ABI sceFontGraphicsBeginFrame();
s32 PS4_SYSV_ABI sceFontGraphicsDrawingCancel();
s32 PS4_SYSV_ABI sceFontGraphicsDrawingFinish();
s32 PS4_SYSV_ABI sceFontGraphicsEndFrame();
s32 PS4_SYSV_ABI sceFontGraphicsExchangeResource();
s32 PS4_SYSV_ABI sceFontGraphicsFillMethodInit();
s32 PS4_SYSV_ABI sceFontGraphicsFillPlotInit();
s32 PS4_SYSV_ABI sceFontGraphicsFillPlotSetLayout();
s32 PS4_SYSV_ABI sceFontGraphicsFillPlotSetMapping();
s32 PS4_SYSV_ABI sceFontGraphicsFillRatesInit();
s32 PS4_SYSV_ABI sceFontGraphicsFillRatesSetFillEffect();
s32 PS4_SYSV_ABI sceFontGraphicsFillRatesSetLayout();
s32 PS4_SYSV_ABI sceFontGraphicsFillRatesSetMapping();
s32 PS4_SYSV_ABI sceFontGraphicsGetDeviceUsage();
s32 PS4_SYSV_ABI sceFontGraphicsRegionInit();
s32 PS4_SYSV_ABI sceFontGraphicsRegionInitCircular();
s32 PS4_SYSV_ABI sceFontGraphicsRegionInitRoundish();
s32 PS4_SYSV_ABI sceFontGraphicsRelease();
s32 PS4_SYSV_ABI sceFontGraphicsRenderResource();
s32 PS4_SYSV_ABI sceFontGraphicsSetFramePolicy();
s32 PS4_SYSV_ABI sceFontGraphicsSetupClipping();
s32 PS4_SYSV_ABI sceFontGraphicsSetupColorRates();
s32 PS4_SYSV_ABI sceFontGraphicsSetupFillMethod();
s32 PS4_SYSV_ABI sceFontGraphicsSetupFillRates();
s32 PS4_SYSV_ABI sceFontGraphicsSetupGlyphFill();
s32 PS4_SYSV_ABI sceFontGraphicsSetupGlyphFillPlot();
s32 PS4_SYSV_ABI sceFontGraphicsSetupHandleDefault();
s32 PS4_SYSV_ABI sceFontGraphicsSetupLocation();
s32 PS4_SYSV_ABI sceFontGraphicsSetupPositioning();
s32 PS4_SYSV_ABI sceFontGraphicsSetupRotation();
s32 PS4_SYSV_ABI sceFontGraphicsSetupScaling();
s32 PS4_SYSV_ABI sceFontGraphicsSetupShapeFill();
s32 PS4_SYSV_ABI sceFontGraphicsSetupShapeFillPlot();
s32 PS4_SYSV_ABI sceFontGraphicsStructureCanvas();
s32 PS4_SYSV_ABI sceFontGraphicsStructureCanvasSequence();
s32 PS4_SYSV_ABI sceFontGraphicsStructureDesign();
s32 PS4_SYSV_ABI sceFontGraphicsStructureDesignResource();
s32 PS4_SYSV_ABI sceFontGraphicsStructureSurfaceTexture();
s32 PS4_SYSV_ABI sceFontGraphicsUpdateClipping();
s32 PS4_SYSV_ABI sceFontGraphicsUpdateColorRates();
s32 PS4_SYSV_ABI sceFontGraphicsUpdateFillMethod();
s32 PS4_SYSV_ABI sceFontGraphicsUpdateFillRates();
s32 PS4_SYSV_ABI sceFontGraphicsUpdateGlyphFill();
s32 PS4_SYSV_ABI sceFontGraphicsUpdateGlyphFillPlot();
s32 PS4_SYSV_ABI sceFontGraphicsUpdateLocation();
s32 PS4_SYSV_ABI sceFontGraphicsUpdatePositioning();
s32 PS4_SYSV_ABI sceFontGraphicsUpdateRotation();
s32 PS4_SYSV_ABI sceFontGraphicsUpdateScaling();
s32 PS4_SYSV_ABI sceFontGraphicsUpdateShapeFill();
s32 PS4_SYSV_ABI sceFontGraphicsUpdateShapeFillPlot();
s32 PS4_SYSV_ABI sceFontMemoryInit();
s32 PS4_SYSV_ABI sceFontMemoryTerm();
s32 PS4_SYSV_ABI sceFontOpenFontFile();
s32 PS4_SYSV_ABI sceFontOpenFontInstance();
s32 PS4_SYSV_ABI sceFontOpenFontMemory();
s32 PS4_SYSV_ABI sceFontOpenFontSet();
s32 PS4_SYSV_ABI sceFontRebindRenderer();
s32 PS4_SYSV_ABI sceFontRenderCharGlyphImage();
s32 PS4_SYSV_ABI sceFontRenderCharGlyphImageHorizontal();
s32 PS4_SYSV_ABI sceFontRenderCharGlyphImageVertical();
s32 PS4_SYSV_ABI sceFontRendererGetOutlineBufferSize();
s32 PS4_SYSV_ABI sceFontRendererResetOutlineBuffer();
s32 PS4_SYSV_ABI sceFontRendererSetOutlineBufferPolicy();
void PS4_SYSV_ABI sceFontRenderSurfaceInit(OrbisFontRenderSurface* renderSurface, void* buffer,
                                           int bufWidthByte, int pixelSizeByte, int widthPixel,
                                           int heightPixel);
void PS4_SYSV_ABI sceFontRenderSurfaceSetScissor(OrbisFontRenderSurface* renderSurface, int x0,
                                                 int y0, int w, int h);
s32 PS4_SYSV_ABI sceFontRenderSurfaceSetStyleFrame(OrbisFontRenderSurface* renderSurface,
                                                   OrbisFontStyleFrame* styleFrame);
s32 PS4_SYSV_ABI sceFontSetEffectSlant();
s32 PS4_SYSV_ABI sceFontSetEffectWeight();
s32 PS4_SYSV_ABI sceFontSetFontsOpenMode();
s32 PS4_SYSV_ABI sceFontSetResolutionDpi();
s32 PS4_SYSV_ABI sceFontSetScalePixel();
s32 PS4_SYSV_ABI sceFontSetScalePoint();
s32 PS4_SYSV_ABI sceFontSetScriptLanguage();
s32 PS4_SYSV_ABI sceFontSetTypographicDesign();
s32 PS4_SYSV_ABI sceFontSetupRenderEffectSlant();
s32 PS4_SYSV_ABI sceFontSetupRenderEffectWeight();
s32 PS4_SYSV_ABI sceFontSetupRenderScalePixel();
s32 PS4_SYSV_ABI sceFontSetupRenderScalePoint();
s32 PS4_SYSV_ABI sceFontStringGetTerminateCode();
s32 PS4_SYSV_ABI sceFontStringGetTerminateOrder();
s32 PS4_SYSV_ABI sceFontStringGetWritingForm();
s32 PS4_SYSV_ABI sceFontStringRefersRenderCharacters();
s32 PS4_SYSV_ABI sceFontStringRefersTextCharacters();
s32 PS4_SYSV_ABI sceFontStyleFrameGetEffectSlant(OrbisFontStyleFrame* styleFrame,
                                                 float* slantRatio);
s32 PS4_SYSV_ABI sceFontStyleFrameGetEffectWeight(OrbisFontStyleFrame* fontStyleFrame,
                                                  float* weightXScale, float* weightYScale,
                                                  uint32_t* mode);
s32 PS4_SYSV_ABI sceFontStyleFrameGetResolutionDpi();
s32 PS4_SYSV_ABI sceFontStyleFrameGetScalePixel(OrbisFontStyleFrame* styleFrame, float* w,
                                                float* h);
s32 PS4_SYSV_ABI sceFontStyleFrameGetScalePoint();
s32 PS4_SYSV_ABI sceFontStyleFrameInit();
s32 PS4_SYSV_ABI sceFontStyleFrameSetEffectSlant();
s32 PS4_SYSV_ABI sceFontStyleFrameSetEffectWeight();
s32 PS4_SYSV_ABI sceFontStyleFrameSetResolutionDpi();
s32 PS4_SYSV_ABI sceFontStyleFrameSetScalePixel();
s32 PS4_SYSV_ABI sceFontStyleFrameSetScalePoint();
s32 PS4_SYSV_ABI sceFontStyleFrameUnsetEffectSlant();
s32 PS4_SYSV_ABI sceFontStyleFrameUnsetEffectWeight();
s32 PS4_SYSV_ABI sceFontStyleFrameUnsetScale();
s32 PS4_SYSV_ABI sceFontSupportExternalFonts();
s32 PS4_SYSV_ABI sceFontSupportGlyphs();
s32 PS4_SYSV_ABI sceFontSupportSystemFonts();
s32 PS4_SYSV_ABI sceFontTextCodesStepBack();
s32 PS4_SYSV_ABI sceFontTextCodesStepNext();
s32 PS4_SYSV_ABI sceFontTextSourceInit();
s32 PS4_SYSV_ABI sceFontTextSourceRewind();
s32 PS4_SYSV_ABI sceFontTextSourceSetDefaultFont();
s32 PS4_SYSV_ABI sceFontTextSourceSetWritingForm();
s32 PS4_SYSV_ABI sceFontUnbindRenderer();
s32 PS4_SYSV_ABI sceFontWordsFindWordCharacters();
s32 PS4_SYSV_ABI sceFontWritingGetRenderMetrics();
s32 PS4_SYSV_ABI sceFontWritingInit();
s32 PS4_SYSV_ABI sceFontWritingLineClear();
s32 PS4_SYSV_ABI sceFontWritingLineGetOrderingSpace();
s32 PS4_SYSV_ABI sceFontWritingLineGetRenderMetrics();
s32 PS4_SYSV_ABI sceFontWritingLineRefersRenderStep();
s32 PS4_SYSV_ABI sceFontWritingLineWritesOrder();
s32 PS4_SYSV_ABI sceFontWritingRefersRenderStep();
s32 PS4_SYSV_ABI sceFontWritingRefersRenderStepCharacter();
s32 PS4_SYSV_ABI sceFontWritingSetMaskInvisible();
s32 PS4_SYSV_ABI Func_00F4D778F1C88CB3();
s32 PS4_SYSV_ABI Func_03C650025FBB0DE7();
s32 PS4_SYSV_ABI Func_07EAB8A163B27E1A();
s32 PS4_SYSV_ABI Func_09408E88E4F97CE3();
s32 PS4_SYSV_ABI Func_09F92905ED82A814();
s32 PS4_SYSV_ABI Func_0D142CEE1AB21ABE();
s32 PS4_SYSV_ABI Func_14BD2E9E119C16F2();
s32 PS4_SYSV_ABI Func_1AC53C9EDEAE8D75();
s32 PS4_SYSV_ABI Func_1D401185D5E24C3D();
s32 PS4_SYSV_ABI Func_1E83CD20C2CC996F();
s32 PS4_SYSV_ABI Func_314B1F765B9FE78A();
s32 PS4_SYSV_ABI Func_350E6725FEDE29E1();
s32 PS4_SYSV_ABI Func_3DB773F0A604BF39();
s32 PS4_SYSV_ABI Func_4FF49DD21E311B1C();
s32 PS4_SYSV_ABI Func_526287664A493981();
s32 PS4_SYSV_ABI Func_55CA718DBC84A6E9();
s32 PS4_SYSV_ABI Func_563FC5F0706A8B4D();
s32 PS4_SYSV_ABI Func_569E2ECD34290F45();
s32 PS4_SYSV_ABI Func_5A04775B6BE47685();
s32 PS4_SYSV_ABI Func_5FD93BCAB6F79750();
s32 PS4_SYSV_ABI Func_62B5398F864BD3B4();
s32 PS4_SYSV_ABI Func_6F9010294D822367();
s32 PS4_SYSV_ABI Func_7757E947423A7A67();
s32 PS4_SYSV_ABI Func_7E06BA52077F54FA();
s32 PS4_SYSV_ABI Func_93B36DEA021311D6();
s32 PS4_SYSV_ABI Func_94B0891E7111598A();
s32 PS4_SYSV_ABI Func_9785C9128C2FE7CD();
s32 PS4_SYSV_ABI Func_97DFBC9B65FBC0E1();
s32 PS4_SYSV_ABI Func_ACD9717405D7D3CA();
s32 PS4_SYSV_ABI Func_B19A8AEC3FD4F16F();
s32 PS4_SYSV_ABI Func_C10F488AD7CF103D();
s32 PS4_SYSV_ABI Func_D0C8B5FF4A6826C7();
s32 PS4_SYSV_ABI Func_E48D3CD01C342A33();
s32 PS4_SYSV_ABI Func_EAC96B2186B71E14();
s32 PS4_SYSV_ABI Func_FE4788A96EF46256();
s32 PS4_SYSV_ABI Func_FE7E5AE95D3058F5();

void RegisterlibSceFont(Core::Loader::SymbolsResolver* sym);
} // namespace Libraries::Font