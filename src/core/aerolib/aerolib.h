#pragma once

#include <cstdint>

namespace Core::AeroLib {

struct NidEntry {
    const char* nid;
    const char* name;
};

const NidEntry* FindByNid(const char* nid);

} // namespace Core::AeroLib
