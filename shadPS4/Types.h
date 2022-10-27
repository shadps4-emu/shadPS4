#pragma once
#include <immintrin.h>

using S08 = char;
using S16 = short;
using S32 = int;
using S64 = long long;

using U08 = unsigned char;
using U16 = unsigned short;
using U32 = unsigned int;
using U64 = unsigned long long;

using F32 = float;
using F64 = double;


template< typename T > T inline LoadBE(T* src) { return *src; };
template< typename T > inline void StoreBE(T* dst, T src) { *dst = src; };

inline S16 LoadBE(S16* src) { return _loadbe_i16(src); };
inline S32 LoadBE(S32* src) { return _loadbe_i32(src); };
inline S64 LoadBE(S64* src) { return _loadbe_i64(src); };

inline U16 LoadBE(U16* src) { return _load_be_u16(src); };
inline U32 LoadBE(U32* src) { return _load_be_u32(src); };
inline U64 LoadBE(U64* src) { return _load_be_u64(src); };

inline void StoreBE(S16* dst, S16 const src) { _storebe_i16(dst, src); };
inline void StoreBE(S32* dst, S32 const src) { _storebe_i32(dst, src); };
inline void StoreBE(S64* dst, S64 const src) { _storebe_i64(dst, src); };

inline void StoreBE(U16* dst, U16 const src) { _store_be_u16(dst, src); };
inline void StoreBE(U32* dst, U32 const src) { _store_be_u32(dst, src); };
inline void StoreBE(U64* dst, U64 const src) { _store_be_u64(dst, src); };


template< typename T > inline void ReadBE(T& val)
{
    val = LoadBE(&val); // swap inplace
}

template< typename T > inline void WriteBE(T& val)
{
    StoreBE(&val, val); // swap inplace
}