// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <filesystem>
#include <Cocoa/Cocoa.h>

#include "common/apple.h"

namespace Common {

void SetAppIcon(const std::filesystem::path& path) {
    @autoreleasepool {
        NSString* path_str = [NSString stringWithUTF8String:path.native().c_str()];
        NSImage* base_icon = [[[NSImage alloc] initWithContentsOfFile:path_str] autorelease];

        // Transform the icon to match native look-and-feel.
        constexpr double ScaleFactor = 13.0 / 16.0;
        constexpr double CornerRadiusFactor = 22.0 / 100.0;

        const double base_icon_width = base_icon.size.width;
        const double base_icon_height = base_icon.size.height;
        const double icon_width = base_icon_width * ScaleFactor;
        const double icon_height = base_icon_height * ScaleFactor;
        const double icon_x = (base_icon_width - icon_width) / 2.0;
        const double icon_y = (base_icon_height - icon_height) / 2.0;
        const double corner_radius_x = icon_width * CornerRadiusFactor;
        const double corner_radius_y = icon_height * CornerRadiusFactor;

        NSRect bounds = NSMakeRect(icon_x, icon_y, icon_width, icon_height);
        NSBezierPath* mask_path = [NSBezierPath bezierPathWithRoundedRect:bounds
                                                                  xRadius:corner_radius_x
                                                                  yRadius:corner_radius_y];

        NSImage* rounded_icon = [[[NSImage alloc] initWithSize:base_icon.size] autorelease];
        [rounded_icon lockFocus];
        [mask_path addClip];
        [base_icon drawInRect:bounds
                     fromRect:NSZeroRect
                    operation:NSCompositingOperationSourceOver
                     fraction:1.0f];
        [rounded_icon unlockFocus];

        [NSApp setApplicationIconImage:rounded_icon];
    }
}

} // namespace Common
