#include <types.h>

namespace Emulator::HLE::Libraries::LibC {

// https://stackoverflow.com/questions/4958384/what-is-the-format-of-the-x86-64-va-list-structure

struct VaList {
    u32 gp_offset;
    u32 fp_offset;
    void* overflow_arg_area;
    void* reg_save_area;
};

template <class T, uint32_t Size>
T vaArgRegSaveAreaGp(VaList* l) {
    auto* addr = reinterpret_cast<T*>(static_cast<uint8_t*>(l->reg_save_area) + l->gp_offset);
    l->gp_offset += Size;
    return *addr;
}
template <class T, uint64_t Align, uint64_t Size>
T vaArgOverflowArgArea(VaList* l) {
    auto ptr = ((reinterpret_cast<uint64_t>(l->overflow_arg_area) + (Align - 1)) & ~(Align - 1));
    auto* addr = reinterpret_cast<T*>(ptr);
    l->overflow_arg_area = reinterpret_cast<void*>(ptr + Size);
    return *addr;
}

template <class T, uint32_t Size>
T vaArgRegSaveAreaFp(VaList* l) {
    auto* addr = reinterpret_cast<T*>(static_cast<uint8_t*>(l->reg_save_area) + l->fp_offset);
    l->fp_offset += Size;
    return *addr;
}

inline int vaArgInteger(VaList* l) {
    if (l->gp_offset <= 40) {
        return vaArgRegSaveAreaGp<int, 8>(l);
    }
    return vaArgOverflowArgArea<int, 1, 8>(l);
}

inline long long vaArgLongLong(VaList* l) {
    if (l->gp_offset <= 40) {
        return vaArgRegSaveAreaGp<long long, 8>(l);
    }
    return vaArgOverflowArgArea<long long, 1, 8>(l);
}
inline long vaArgLong(VaList* l) {
    if (l->gp_offset <= 40) {
        return vaArgRegSaveAreaGp<long, 8>(l);
    }
    return vaArgOverflowArgArea<long, 1, 8>(l);
}

inline double vaArgDouble(VaList* l) {
    if (l->fp_offset <= 160) {
        return vaArgRegSaveAreaFp<double, 16>(l);
    }
    return vaArgOverflowArgArea<double, 1, 8>(l);
}

template <class T>
T* vaArgPtr(VaList* l) {
    if (l->gp_offset <= 40) {
        return vaArgRegSaveAreaGp<T*, 8>(l);
    }
    return vaArgOverflowArgArea<T*, 1, 8>(l);
}

}  // namespace Emulator::HLE::Libraries::LibC