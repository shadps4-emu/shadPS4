// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include "common/singleton.h"
#include "core/libraries/audiodec_cpu/audiodec_cpu_backend.h"
#include "core/linker.h"

namespace Libraries::AudiodecCpu {

const CodecOps* ResolveLleCodecOps(const char* module, const char* nid) {
    const Core::Loader::SymbolResolver symbol{
        .name = nid,
        .library = module,
        .library_version = 1,
        .module = module,
        .type = Core::Loader::SymbolType::Object,
    };
    // Callers must finish loading the decoder through sysmodule before using audiodec.
    // Only inspect loaded LLE exports here.
    const auto address = Common::Singleton<Core::Linker>::Instance()->FindExport(symbol);
    return reinterpret_cast<const CodecOps*>(address);
}

} // namespace Libraries::AudiodecCpu
