#pragma once

namespace Core::Loader {
class SymbolsResolver;
}

namespace Core::Libraries::LibC {

void libcSymbolsRegister(Loader::SymbolsResolver* sym);

} // namespace Core::Libraries::LibC
