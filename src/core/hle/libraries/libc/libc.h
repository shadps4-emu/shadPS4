#pragma once

#include <types.h>
#include "core/PS4/Loader/SymbolsResolver.h"

namespace Core::Libraries::LibC {

void libcSymbolsRegister(SymbolsResolver* sym);

}  // namespace Core::Libraries::LibC