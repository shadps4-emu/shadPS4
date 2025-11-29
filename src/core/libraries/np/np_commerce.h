#pragma once

#include "common/types.h"

namespace Core::Loader {
class SymbolsResolver;
}

namespace Libraries::Np::NpCommerce {

void RegisterLib(Core::Loader::SymbolsResolver* sym);

} // namespace Libraries::Np::NpCommerce