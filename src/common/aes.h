// SPDX-FileCopyrightText: 2015 kkAyataka
// SPDX-License-Identifier: BSL-1.0

#pragma once

#include <algorithm>
#include <bitset>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <vector>

/** AES cipher APIs */
namespace aes {
namespace detail {

const int kWordSize = 4;
typedef unsigned int Word;

const int kBlockSize = 4;
/** @private */
struct State {
    Word w[4];
    Word& operator[](const int index) {
        return w[index];
    }
    const Word& operator[](const int index) const {
        return w[index];
    }
};

const int kStateSize = 16; // Word * BlockSize
typedef State RoundKey;
typedef std::vector<RoundKey> RoundKeys;

inline void add_round_key(const RoundKey& key, State& state) {
    for (int i = 0; i < kBlockSize; ++i) {
        state[i] ^= key[i];
    }
}

const unsigned char kSbox[] = {
    0x63, 0x7c, 0x77, 0x7b, 0xf2, 0x6b, 0x6f, 0xc5, 0x30, 0x01, 0x67, 0x2b, 0xfe, 0xd7, 0xab, 0x76,
    0xca, 0x82, 0xc9, 0x7d, 0xfa, 0x59, 0x47, 0xf0, 0xad, 0xd4, 0xa2, 0xaf, 0x9c, 0xa4, 0x72, 0xc0,
    0xb7, 0xfd, 0x93, 0x26, 0x36, 0x3f, 0xf7, 0xcc, 0x34, 0xa5, 0xe5, 0xf1, 0x71, 0xd8, 0x31, 0x15,
    0x04, 0xc7, 0x23, 0xc3, 0x18, 0x96, 0x05, 0x9a, 0x07, 0x12, 0x80, 0xe2, 0xeb, 0x27, 0xb2, 0x75,
    0x09, 0x83, 0x2c, 0x1a, 0x1b, 0x6e, 0x5a, 0xa0, 0x52, 0x3b, 0xd6, 0xb3, 0x29, 0xe3, 0x2f, 0x84,
    0x53, 0xd1, 0x00, 0xed, 0x20, 0xfc, 0xb1, 0x5b, 0x6a, 0xcb, 0xbe, 0x39, 0x4a, 0x4c, 0x58, 0xcf,
    0xd0, 0xef, 0xaa, 0xfb, 0x43, 0x4d, 0x33, 0x85, 0x45, 0xf9, 0x02, 0x7f, 0x50, 0x3c, 0x9f, 0xa8,
    0x51, 0xa3, 0x40, 0x8f, 0x92, 0x9d, 0x38, 0xf5, 0xbc, 0xb6, 0xda, 0x21, 0x10, 0xff, 0xf3, 0xd2,
    0xcd, 0x0c, 0x13, 0xec, 0x5f, 0x97, 0x44, 0x17, 0xc4, 0xa7, 0x7e, 0x3d, 0x64, 0x5d, 0x19, 0x73,
    0x60, 0x81, 0x4f, 0xdc, 0x22, 0x2a, 0x90, 0x88, 0x46, 0xee, 0xb8, 0x14, 0xde, 0x5e, 0x0b, 0xdb,
    0xe0, 0x32, 0x3a, 0x0a, 0x49, 0x06, 0x24, 0x5c, 0xc2, 0xd3, 0xac, 0x62, 0x91, 0x95, 0xe4, 0x79,
    0xe7, 0xc8, 0x37, 0x6d, 0x8d, 0xd5, 0x4e, 0xa9, 0x6c, 0x56, 0xf4, 0xea, 0x65, 0x7a, 0xae, 0x08,
    0xba, 0x78, 0x25, 0x2e, 0x1c, 0xa6, 0xb4, 0xc6, 0xe8, 0xdd, 0x74, 0x1f, 0x4b, 0xbd, 0x8b, 0x8a,
    0x70, 0x3e, 0xb5, 0x66, 0x48, 0x03, 0xf6, 0x0e, 0x61, 0x35, 0x57, 0xb9, 0x86, 0xc1, 0x1d, 0x9e,
    0xe1, 0xf8, 0x98, 0x11, 0x69, 0xd9, 0x8e, 0x94, 0x9b, 0x1e, 0x87, 0xe9, 0xce, 0x55, 0x28, 0xdf,
    0x8c, 0xa1, 0x89, 0x0d, 0xbf, 0xe6, 0x42, 0x68, 0x41, 0x99, 0x2d, 0x0f, 0xb0, 0x54, 0xbb, 0x16};

const unsigned char kInvSbox[] = {
    0x52, 0x09, 0x6a, 0xd5, 0x30, 0x36, 0xa5, 0x38, 0xbf, 0x40, 0xa3, 0x9e, 0x81, 0xf3, 0xd7, 0xfb,
    0x7c, 0xe3, 0x39, 0x82, 0x9b, 0x2f, 0xff, 0x87, 0x34, 0x8e, 0x43, 0x44, 0xc4, 0xde, 0xe9, 0xcb,
    0x54, 0x7b, 0x94, 0x32, 0xa6, 0xc2, 0x23, 0x3d, 0xee, 0x4c, 0x95, 0x0b, 0x42, 0xfa, 0xc3, 0x4e,
    0x08, 0x2e, 0xa1, 0x66, 0x28, 0xd9, 0x24, 0xb2, 0x76, 0x5b, 0xa2, 0x49, 0x6d, 0x8b, 0xd1, 0x25,
    0x72, 0xf8, 0xf6, 0x64, 0x86, 0x68, 0x98, 0x16, 0xd4, 0xa4, 0x5c, 0xcc, 0x5d, 0x65, 0xb6, 0x92,
    0x6c, 0x70, 0x48, 0x50, 0xfd, 0xed, 0xb9, 0xda, 0x5e, 0x15, 0x46, 0x57, 0xa7, 0x8d, 0x9d, 0x84,
    0x90, 0xd8, 0xab, 0x00, 0x8c, 0xbc, 0xd3, 0x0a, 0xf7, 0xe4, 0x58, 0x05, 0xb8, 0xb3, 0x45, 0x06,
    0xd0, 0x2c, 0x1e, 0x8f, 0xca, 0x3f, 0x0f, 0x02, 0xc1, 0xaf, 0xbd, 0x03, 0x01, 0x13, 0x8a, 0x6b,
    0x3a, 0x91, 0x11, 0x41, 0x4f, 0x67, 0xdc, 0xea, 0x97, 0xf2, 0xcf, 0xce, 0xf0, 0xb4, 0xe6, 0x73,
    0x96, 0xac, 0x74, 0x22, 0xe7, 0xad, 0x35, 0x85, 0xe2, 0xf9, 0x37, 0xe8, 0x1c, 0x75, 0xdf, 0x6e,
    0x47, 0xf1, 0x1a, 0x71, 0x1d, 0x29, 0xc5, 0x89, 0x6f, 0xb7, 0x62, 0x0e, 0xaa, 0x18, 0xbe, 0x1b,
    0xfc, 0x56, 0x3e, 0x4b, 0xc6, 0xd2, 0x79, 0x20, 0x9a, 0xdb, 0xc0, 0xfe, 0x78, 0xcd, 0x5a, 0xf4,
    0x1f, 0xdd, 0xa8, 0x33, 0x88, 0x07, 0xc7, 0x31, 0xb1, 0x12, 0x10, 0x59, 0x27, 0x80, 0xec, 0x5f,
    0x60, 0x51, 0x7f, 0xa9, 0x19, 0xb5, 0x4a, 0x0d, 0x2d, 0xe5, 0x7a, 0x9f, 0x93, 0xc9, 0x9c, 0xef,
    0xa0, 0xe0, 0x3b, 0x4d, 0xae, 0x2a, 0xf5, 0xb0, 0xc8, 0xeb, 0xbb, 0x3c, 0x83, 0x53, 0x99, 0x61,
    0x17, 0x2b, 0x04, 0x7e, 0xba, 0x77, 0xd6, 0x26, 0xe1, 0x69, 0x14, 0x63, 0x55, 0x21, 0x0c, 0x7d};

inline Word sub_word(const Word w) {
    return kSbox[(w >> 0) & 0xFF] << 0 | kSbox[(w >> 8) & 0xFF] << 8 |
           kSbox[(w >> 16) & 0xFF] << 16 | kSbox[(w >> 24) & 0xFF] << 24;
}

inline Word inv_sub_word(const Word w) {
    return kInvSbox[(w >> 0) & 0xFF] << 0 | kInvSbox[(w >> 8) & 0xFF] << 8 |
           kInvSbox[(w >> 16) & 0xFF] << 16 | kInvSbox[(w >> 24) & 0xFF] << 24;
}

inline void sub_bytes(State& state) {
    for (int i = 0; i < kBlockSize; ++i) {
        state[i] = sub_word(state[i]);
    }
}

inline void inv_sub_bytes(State& state) {
    for (int i = 0; i < kBlockSize; ++i) {
        state[i] = inv_sub_word(state[i]);
    }
}

inline void shift_rows(State& state) {
    const State ori = {state[0], state[1], state[2], state[3]};
    for (int r = 1; r < kWordSize; ++r) {
        const Word m2 = 0xFF << (r * 8);
        const Word m1 = ~m2;
        for (int c = 0; c < kBlockSize; ++c) {
            state[c] = (state[c] & m1) | (ori[(c + r) % kBlockSize] & m2);
        }
    }
}

inline void inv_shift_rows(State& state) {
    const State ori = {state[0], state[1], state[2], state[3]};
    for (int r = 1; r < kWordSize; ++r) {
        const Word m2 = 0xFF << (r * 8);
        const Word m1 = ~m2;
        for (int c = 0; c < kBlockSize; ++c) {
            state[c] = (state[c] & m1) | (ori[(c + kBlockSize - r) % kWordSize] & m2);
        }
    }
}

inline unsigned char mul2(const unsigned char b) {
    unsigned char m2 = b << 1;
    if (b & 0x80) {
        m2 ^= 0x011B;
    }

    return m2;
}

inline unsigned char mul(const unsigned char b, const unsigned char m) {
    unsigned char v = 0;
    unsigned char t = b;
    for (int i = 0; i < 8; ++i) { // 8-bits
        if ((m >> i) & 0x01) {
            v ^= t;
        }

        t = mul2(t);
    }

    return v;
}

inline void mix_columns(State& state) {
    for (int i = 0; i < kBlockSize; ++i) {
        const unsigned char v0_1 = (state[i] >> 0) & 0xFF;
        const unsigned char v1_1 = (state[i] >> 8) & 0xFF;
        const unsigned char v2_1 = (state[i] >> 16) & 0xFF;
        const unsigned char v3_1 = (state[i] >> 24) & 0xFF;

        const unsigned char v0_2 = mul2(v0_1);
        const unsigned char v1_2 = mul2(v1_1);
        const unsigned char v2_2 = mul2(v2_1);
        const unsigned char v3_2 = mul2(v3_1);

        const unsigned char v0_3 = v0_2 ^ v0_1;
        const unsigned char v1_3 = v1_2 ^ v1_1;
        const unsigned char v2_3 = v2_2 ^ v2_1;
        const unsigned char v3_3 = v3_2 ^ v3_1;

        state[i] = (v0_2 ^ v1_3 ^ v2_1 ^ v3_1) << 0 | (v0_1 ^ v1_2 ^ v2_3 ^ v3_1) << 8 |
                   (v0_1 ^ v1_1 ^ v2_2 ^ v3_3) << 16 | (v0_3 ^ v1_1 ^ v2_1 ^ v3_2) << 24;
    }
}

inline void inv_mix_columns(State& state) {
    for (int i = 0; i < kBlockSize; ++i) {
        const unsigned char v0 = (state[i] >> 0) & 0xFF;
        const unsigned char v1 = (state[i] >> 8) & 0xFF;
        const unsigned char v2 = (state[i] >> 16) & 0xFF;
        const unsigned char v3 = (state[i] >> 24) & 0xFF;

        state[i] = (mul(v0, 0x0E) ^ mul(v1, 0x0B) ^ mul(v2, 0x0D) ^ mul(v3, 0x09)) << 0 |
                   (mul(v0, 0x09) ^ mul(v1, 0x0E) ^ mul(v2, 0x0B) ^ mul(v3, 0x0D)) << 8 |
                   (mul(v0, 0x0D) ^ mul(v1, 0x09) ^ mul(v2, 0x0E) ^ mul(v3, 0x0B)) << 16 |
                   (mul(v0, 0x0B) ^ mul(v1, 0x0D) ^ mul(v2, 0x09) ^ mul(v3, 0x0E)) << 24;
    }
}

inline Word rot_word(const Word v) {
    return ((v >> 8) & 0x00FFFFFF) | ((v & 0xFF) << 24);
}

/**
 * @private
 * @throws std::invalid_argument
 */
inline unsigned int get_round_count(const int key_size) {
    switch (key_size) {
    case 16:
        return 10;
    case 24:
        return 12;
    case 32:
        return 14;
    default:
        throw std::invalid_argument("Invalid key size");
    }
}

/**
 * @private
 * @throws std::invalid_argument
 */
inline RoundKeys expand_key(const unsigned char* key, const int key_size) {
    if (key_size != 16 && key_size != 24 && key_size != 32) {
        throw std::invalid_argument("Invalid key size");
    }

    const Word rcon[] = {0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80, 0x1b, 0x36};

    const int nb = kBlockSize;
    const int nk = key_size / nb;
    const int nr = get_round_count(key_size);

    std::vector<Word> w(nb * (nr + 1));
    for (int i = 0; i < nk; ++i) {
        memcpy(&w[i], key + (i * kWordSize), kWordSize);
    }

    for (int i = nk; i < nb * (nr + 1); ++i) {
        Word t = w[i - 1];
        if (i % nk == 0) {
            t = sub_word(rot_word(t)) ^ rcon[i / nk];
        } else if (nk > 6 && i % nk == 4) {
            t = sub_word(t);
        }

        w[i] = t ^ w[i - nk];
    }

    RoundKeys keys(nr + 1);
    memcpy(&keys[0], &w[0], w.size() * kWordSize);

    return keys;
}

inline void copy_bytes_to_state(const unsigned char data[16], State& state) {
    memcpy(&state[0], data + 0, kWordSize);
    memcpy(&state[1], data + 4, kWordSize);
    memcpy(&state[2], data + 8, kWordSize);
    memcpy(&state[3], data + 12, kWordSize);
}

inline void copy_state_to_bytes(const State& state, unsigned char buf[16]) {
    memcpy(buf + 0, &state[0], kWordSize);
    memcpy(buf + 4, &state[1], kWordSize);
    memcpy(buf + 8, &state[2], kWordSize);
    memcpy(buf + 12, &state[3], kWordSize);
}

inline void xor_data(unsigned char data[kStateSize], const unsigned char v[kStateSize]) {
    for (int i = 0; i < kStateSize; ++i) {
        data[i] ^= v[i];
    }
}

/** increment counter (128-bit int) by 1 */
inline void incr_counter(unsigned char counter[kStateSize]) {
    unsigned n = kStateSize, c = 1;
    do {
        --n;
        c += counter[n];
        counter[n] = c;
        c >>= 8;
    } while (n);
}

inline void encrypt_state(const RoundKeys& rkeys, const unsigned char data[16],
                          unsigned char encrypted[16]) {
    State s;
    copy_bytes_to_state(data, s);

    add_round_key(rkeys[0], s);

    for (unsigned int i = 1; i < rkeys.size() - 1; ++i) {
        sub_bytes(s);
        shift_rows(s);
        mix_columns(s);
        add_round_key(rkeys[i], s);
    }

    sub_bytes(s);
    shift_rows(s);
    add_round_key(rkeys.back(), s);

    copy_state_to_bytes(s, encrypted);
}

inline void decrypt_state(const RoundKeys& rkeys, const unsigned char data[16],
                          unsigned char decrypted[16]) {
    State s;
    copy_bytes_to_state(data, s);

    add_round_key(rkeys.back(), s);
    inv_shift_rows(s);
    inv_sub_bytes(s);

    for (std::size_t i = rkeys.size() - 2; i > 0; --i) {
        add_round_key(rkeys[i], s);
        inv_mix_columns(s);
        inv_shift_rows(s);
        inv_sub_bytes(s);
    }

    add_round_key(rkeys[0], s);

    copy_state_to_bytes(s, decrypted);
}

template <int KeyLen>
std::vector<unsigned char> key_from_string(const char (*key_str)[KeyLen]) {
    std::vector<unsigned char> key(KeyLen - 1);
    memcpy(&key[0], *key_str, KeyLen - 1);
    return key;
}

inline bool is_valid_key_size(const std::size_t key_size) {
    if (key_size != 16 && key_size != 24 && key_size != 32) {
        return false;
    } else {
        return true;
    }
}

namespace gcm {

const int kBlockBitSize = 128;
const int kBlockByteSize = kBlockBitSize / 8;

/**
 * @private
 * GCM operation unit as bit.
 * This library handles 128 bit little endian bit array.
 * e.g. 0^127 || 1 == "000...0001" (bit string) == 1
 */
typedef std::bitset<kBlockBitSize> bitset128;

/**
 * @private
 * GCM operation unit.
 * Little endian byte array
 *
 * If bitset128 is 1: 0^127 || 1 == "000...0001" (bit string) == 1
 * byte array is 0x00, 0x00, 0x00 ... 0x01 (low -> high).
 * Byte array is NOT 0x01, 0x00 ... 0x00.
 *
 * This library handles GCM bit string in two ways.
 * One is an array of bitset, which is a little endian 128-bit array's array.
 *
 * <- first byte
 * bitset128 || bitset128 || bitset128...
 *
 * The other one is a byte array.
 * <- first byte
 * byte || byte || byte...
 */
class Block {
public:
    Block() {
        init_v(0, 0);
    }

    Block(const unsigned char* bytes, const unsigned long bytes_size) {
        init_v(bytes, bytes_size);
    }

    Block(const std::vector<unsigned char>& bytes) {
        init_v(&bytes[0], bytes.size());
    }

    Block(const std::bitset<128>& bits); // implementation below

    inline unsigned char* data() {
        return v_;
    }

    inline const unsigned char* data() const {
        return v_;
    }

    inline std::bitset<128> to_bits() const {
        std::bitset<128> bits;
        for (int i = 0; i < 16; ++i) {
            bits <<= 8;
            bits |= v_[i];
        }

        return bits;
    }

    inline Block operator^(const Block& b) const {
        Block r;
        for (int i = 0; i < 16; ++i) {
            r.data()[i] = data()[i] ^ b.data()[i];
        }
        return r;
    }

private:
    unsigned char v_[16];

    inline void init_v(const unsigned char* bytes, const std::size_t bytes_size) {
        memset(v_, 0, sizeof(v_));

        const std::size_t cs = (std::min)(bytes_size, static_cast<std::size_t>(16));
        for (std::size_t i = 0; i < cs; ++i) {
            v_[i] = bytes[i];
        }
    }
};

// Workaround for clang optimization in 32-bit build via Visual Studio producing incorrect results
// (https://github.com/kkAyataka/plusaes/issues/43)
#if defined(__clang__) && defined(_WIN32) && !defined(_WIN64)
#pragma optimize("", off)
#endif
inline Block::Block(const std::bitset<128>& bits) {
    init_v(0, 0);
    const std::bitset<128> mask(0xFF); // 1 byte mask
    for (std::size_t i = 0; i < 16; ++i) {
        v_[15 - i] = static_cast<unsigned char>(((bits >> (i * 8)) & mask).to_ulong());
    }
}
#if defined(__clang__) && defined(_WIN32) && !defined(_WIN64)
#pragma optimize("", on)
#endif

template <typename T>
unsigned long ceil(const T v) {
    return static_cast<unsigned long>(std::ceil(v) + 0.5);
}

template <std::size_t N1, std::size_t N2>
std::bitset<N1 + N2> operator||(const std::bitset<N1>& v1, const std::bitset<N2>& v2) {
    std::bitset<N1 + N2> ret(v1.to_string() + v2.to_string());
    return ret;
}

template <std::size_t S, std::size_t N>
std::bitset<S> lsb(const std::bitset<N>& X) {
    std::bitset<S> r;
    for (std::size_t i = 0; i < S; ++i) {
        r[i] = X[i];
    }
    return r;
}

template <std::size_t S, std::size_t N>
std::bitset<S> msb(const std::bitset<N>& X) {
    std::bitset<S> r;
    for (std::size_t i = 0; i < S; ++i) {
        r[S - 1 - i] = X[X.size() - 1 - i];
    }
    return r;
}

template <std::size_t N>
std::bitset<N> inc32(const std::bitset<N> X) {
    const std::size_t S = 32;

    const auto a = msb<N - S>(X);
    const std::bitset<S> b(
        (lsb<S>(X).to_ulong() + 1)); // % (2^32);
                                     // lsb<32> is low 32-bit value
                                     // Spec.'s "mod 2^S" is not necessary when S is 32 (inc32).
                                     // ...and 2^32 is over 32-bit integer.

    return a || b;
}

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wself-assign"
#endif // __clang__

/** Algorithm 1 @private */
inline Block mul_blocks(const Block X, const Block Y) {
    const bitset128 R = (std::bitset<8>("11100001") || std::bitset<120>());

    bitset128 X_bits = X.to_bits();
    bitset128 Z;
    bitset128 V = Y.to_bits();
    for (int i = 127; i >= 0; --i) {
        // Z
        if (X_bits[i] == false) {
            Z = Z;
        } else {
            Z = Z ^ V;
        }

        // V
        if (V[0] == false) {
            V = V >> 1;
        } else {
            V = (V >> 1) ^ R;
        }
    }

    return Z;
}

#ifdef __clang__
#pragma clang diagnostic pop
#endif // __clang__

/** Algorithm 2 @private */
inline Block ghash(const Block& H, const std::vector<unsigned char>& X) {
    const std::size_t m = X.size() / kBlockByteSize;
    Block Ym;
    for (std::size_t i = 0; i < m; ++i) {
        const Block Xi(&X[i * kBlockByteSize], kBlockByteSize);
        Ym = mul_blocks((Ym ^ Xi), H);
    }

    return Ym;
}

template <std::size_t N>
std::bitset<N> make_bitset(const unsigned char* bytes, const std::size_t bytes_size) {
    std::bitset<N> bits;
    for (auto i = 0u; i < bytes_size; ++i) {
        bits <<= 8;
        bits |= bytes[i];
    }
    return bits;
}

/** Algorithm 3 @private */
inline std::vector<unsigned char> gctr(const detail::RoundKeys& rkeys, const Block& ICB,
                                       const unsigned char* X, const std::size_t X_size) {
    if (!X || X_size == 0) {
        return std::vector<unsigned char>();
    } else {
        const unsigned long n = ceil(X_size * 8.0 / kBlockBitSize);
        std::vector<unsigned char> Y(X_size);

        Block CB;
        for (std::size_t i = 0; i < n; ++i) {
            // CB
            if (i == 0) { // first
                CB = ICB;
            } else {
                CB = inc32(CB.to_bits());
            }

            // CIPH
            Block eCB;
            encrypt_state(rkeys, CB.data(), eCB.data());

            // Y
            int op_size = 0;
            if (i < n - 1) {
                op_size = kBlockByteSize;
            } else { // last
                op_size = (X_size % kBlockByteSize) ? (X_size % kBlockByteSize) : kBlockByteSize;
            }
            const Block Yi = Block(X + i * kBlockBitSize / 8, op_size) ^ eCB;
            memcpy(&Y[i * kBlockByteSize], Yi.data(), op_size);
        }

        return Y;
    }
}

inline void push_back(std::vector<unsigned char>& bytes, const unsigned char* data,
                      const std::size_t data_size) {
    bytes.insert(bytes.end(), data, data + data_size);
}

inline void push_back(std::vector<unsigned char>& bytes, const std::bitset<64>& bits) {
    const std::bitset<64> mask(0xFF); // 1 byte mask
    for (std::size_t i = 0; i < 8; ++i) {
        bytes.push_back(static_cast<unsigned char>(((bits >> ((7 - i) * 8)) & mask).to_ulong()));
    }
}

inline void push_back_zero_bits(std::vector<unsigned char>& bytes,
                                const std::size_t zero_bits_size) {
    const std::vector<unsigned char> zero_bytes(zero_bits_size / 8);
    bytes.insert(bytes.end(), zero_bytes.begin(), zero_bytes.end());
}

inline Block calc_H(const RoundKeys& rkeys) {
    std::vector<unsigned char> H_raw(gcm::kBlockByteSize);
    encrypt_state(rkeys, &H_raw[0], &H_raw[0]);
    return gcm::Block(H_raw);
}

inline Block calc_J0(const Block& H, const unsigned char* iv, const std::size_t iv_size) {
    if (iv_size == 12) {
        const std::bitset<96> iv_bits = gcm::make_bitset<96>(iv, iv_size);
        return iv_bits || std::bitset<31>() || std::bitset<1>(1);
    } else {
        const auto len_iv = iv_size * 8;
        const auto s = 128 * gcm::ceil(len_iv / 128.0) - len_iv;
        std::vector<unsigned char> ghash_in;
        gcm::push_back(ghash_in, iv, iv_size);
        gcm::push_back_zero_bits(ghash_in, s + 64);
        gcm::push_back(ghash_in, std::bitset<64>(len_iv));

        return gcm::ghash(H, ghash_in);
    }
}

inline void calc_gcm_tag(const unsigned char* data, const std::size_t data_size,
                         const unsigned char* aadata, const std::size_t aadata_size,
                         const unsigned char* key, const std::size_t key_size,
                         const unsigned char* iv, const std::size_t iv_size, unsigned char* tag,
                         const std::size_t tag_size) {
    const detail::RoundKeys rkeys = detail::expand_key(key, static_cast<int>(key_size));
    const gcm::Block H = gcm::calc_H(rkeys);
    const gcm::Block J0 = gcm::calc_J0(H, iv, iv_size);

    const auto lenC = data_size * 8;
    const auto lenA = aadata_size * 8;
    const std::size_t u = 128 * gcm::ceil(lenC / 128.0) - lenC;
    const std::size_t v = 128 * gcm::ceil(lenA / 128.0) - lenA;

    std::vector<unsigned char> ghash_in;
    ghash_in.reserve((aadata_size + v / 8) + (data_size + u / 8) + 8 + 8);
    gcm::push_back(ghash_in, aadata, aadata_size);
    gcm::push_back_zero_bits(ghash_in, v);
    gcm::push_back(ghash_in, data, data_size);
    gcm::push_back_zero_bits(ghash_in, u);
    gcm::push_back(ghash_in, std::bitset<64>(lenA));
    gcm::push_back(ghash_in, std::bitset<64>(lenC));
    const gcm::Block S = gcm::ghash(H, ghash_in);
    const std::vector<unsigned char> T = gcm::gctr(rkeys, J0, S.data(), gcm::kBlockByteSize);

    // return
    memcpy(tag, &T[0], (std::min)(tag_size, static_cast<std::size_t>(16)));
}

/** Algorithm 4 and 5 @private */
inline void crypt_gcm(const unsigned char* data, const std::size_t data_size,
                      const unsigned char* key, const std::size_t key_size, const unsigned char* iv,
                      const std::size_t iv_size, unsigned char* crypted) {
    const detail::RoundKeys rkeys = detail::expand_key(key, static_cast<int>(key_size));
    const gcm::Block H = gcm::calc_H(rkeys);
    const gcm::Block J0 = gcm::calc_J0(H, iv, iv_size);

    const std::vector<unsigned char> C =
        gcm::gctr(rkeys, gcm::inc32(J0.to_bits()), data, data_size);

    if (crypted) {
        memcpy(crypted, &C[0], data_size);
    }
}

} // namespace gcm

} // namespace detail

/** @defgroup Base Base
 * Base definitions and convenient functions
 * @{ */

/** Create 128-bit key from string. */
inline std::vector<unsigned char> key_from_string(const char (*key_str)[17]) {
    return detail::key_from_string<17>(key_str);
}

/** Create 192-bit key from string. */
inline std::vector<unsigned char> key_from_string(const char (*key_str)[25]) {
    return detail::key_from_string<25>(key_str);
}

/** Create 256-bit key from string. */
inline std::vector<unsigned char> key_from_string(const char (*key_str)[33]) {
    return detail::key_from_string<33>(key_str);
}

/** Calculates encrypted data size when padding is enabled. */
inline unsigned long get_padded_encrypted_size(const unsigned long data_size) {
    return data_size + detail::kStateSize - (data_size % detail::kStateSize);
}

/** Error code */
typedef enum {
    kErrorOk = 0,
    kErrorInvalidDataSize = 1,
    kErrorInvalidKeySize,
    kErrorInvalidBufferSize,
    kErrorInvalidKey,
    kErrorDeprecated, // kErrorInvalidNonceSize
    kErrorInvalidIvSize,
    kErrorInvalidTagSize,
    kErrorInvalidTag
} Error;

/** @} */

namespace detail {

inline Error check_encrypt_cond(const unsigned long data_size, const unsigned long key_size,
                                const unsigned long encrypted_size, const bool pads) {
    // check data size
    if (!pads && (data_size % kStateSize != 0)) {
        return kErrorInvalidDataSize;
    }

    // check key size
    if (!detail::is_valid_key_size(key_size)) {
        return kErrorInvalidKeySize;
    }

    // check encrypted buffer size
    if (pads) {
        const unsigned long required_size = get_padded_encrypted_size(data_size);
        if (encrypted_size < required_size) {
            return kErrorInvalidBufferSize;
        }
    } else {
        if (encrypted_size < data_size) {
            return kErrorInvalidBufferSize;
        }
    }
    return kErrorOk;
}

inline Error check_decrypt_cond(const unsigned long data_size, const unsigned long key_size,
                                const unsigned long decrypted_size,
                                const unsigned long* padded_size) {
    // check data size
    if (data_size % 16 != 0) {
        return kErrorInvalidDataSize;
    }

    // check key size
    if (!detail::is_valid_key_size(key_size)) {
        return kErrorInvalidKeySize;
    }

    // check decrypted buffer size
    if (!padded_size) {
        if (decrypted_size < data_size) {
            return kErrorInvalidBufferSize;
        }
    } else {
        if (decrypted_size < (data_size - kStateSize)) {
            return kErrorInvalidBufferSize;
        }
    }

    return kErrorOk;
}

inline bool check_padding(const unsigned long padding, const unsigned char data[kStateSize]) {
    if (padding > kStateSize) {
        return false;
    }

    for (unsigned long i = 0; i < padding; ++i) {
        if (data[kStateSize - 1 - i] != padding) {
            return false;
        }
    }

    return true;
}

inline Error check_gcm_cond(const std::size_t key_size, const std::size_t iv_size,
                            const std::size_t tag_size) {
    // check key size
    if (!detail::is_valid_key_size(key_size)) {
        return kErrorInvalidKeySize;
    }

    if (iv_size < 1) {
        return kErrorInvalidIvSize;
    }

    // check tag size
    if ((tag_size < 12 || 16 < tag_size) && (tag_size != 8) && (tag_size != 4)) {
        return kErrorInvalidTagSize;
    }

    return kErrorOk;
}

} // namespace detail

/** @defgroup ECB ECB
 * ECB mode functions
 * @{ */

/**
 * Encrypts data with ECB mode.
 * @param [in]  data Data.
 * @param [in]  data_size Data size.
 *  If the pads is false, data size must be multiple of 16.
 * @param [in]  key key bytes. The key length must be 16 (128-bit), 24 (192-bit) or 32 (256-bit).
 * @param [in]  key_size key size.
 * @param [out] encrypted Encrypted data buffer.
 * @param [in]  encrypted_size Encrypted data buffer size.
 * @param [in]  pads If this value is true, encrypted data is padded by PKCS.
 *  Encrypted data size must be multiple of 16.
 *  If the pads is true, encrypted data is padded with PKCS.
 *  So the data is multiple of 16, encrypted data size needs additonal 16 bytes.
 * @since 1.0.0
 */
inline Error encrypt_ecb(const unsigned char* data, const unsigned long data_size,
                         const unsigned char* key, const unsigned long key_size,
                         unsigned char* encrypted, const unsigned long encrypted_size,
                         const bool pads) {
    const Error e = detail::check_encrypt_cond(data_size, key_size, encrypted_size, pads);
    if (e != kErrorOk) {
        return e;
    }

    const detail::RoundKeys rkeys = detail::expand_key(key, static_cast<int>(key_size));

    const unsigned long bc = data_size / detail::kStateSize;
    for (unsigned long i = 0; i < bc; ++i) {
        detail::encrypt_state(rkeys, data + (i * detail::kStateSize),
                              encrypted + (i * detail::kStateSize));
    }

    if (pads) {
        const int rem = data_size % detail::kStateSize;
        const char pad_v = detail::kStateSize - rem;

        std::vector<unsigned char> ib(detail::kStateSize, pad_v), ob(detail::kStateSize);
        memcpy(&ib[0], data + data_size - rem, rem);

        detail::encrypt_state(rkeys, &ib[0], &ob[0]);
        memcpy(encrypted + (data_size - rem), &ob[0], detail::kStateSize);
    }

    return kErrorOk;
}

/**
 * Decrypts data with ECB mode.
 * @param [in]  data Data bytes.
 * @param [in]  data_size Data size.
 * @param [in]  key Key bytes.
 * @param [in]  key_size Key size.
 * @param [out] decrypted Decrypted data buffer.
 * @param [in]  decrypted_size Decrypted data buffer size.
 * @param [out] padded_size If this value is NULL, this function does not remove padding.
 *  If this value is not NULL, this function removes padding by PKCS
 *  and returns padded size using padded_size.
 * @since 1.0.0
 */
inline Error decrypt_ecb(const unsigned char* data, const unsigned long data_size,
                         const unsigned char* key, const unsigned long key_size,
                         unsigned char* decrypted, const unsigned long decrypted_size,
                         unsigned long* padded_size) {
    const Error e = detail::check_decrypt_cond(data_size, key_size, decrypted_size, padded_size);
    if (e != kErrorOk) {
        return e;
    }

    const detail::RoundKeys rkeys = detail::expand_key(key, static_cast<int>(key_size));

    const unsigned long bc = data_size / detail::kStateSize - 1;
    for (unsigned long i = 0; i < bc; ++i) {
        detail::decrypt_state(rkeys, data + (i * detail::kStateSize),
                              decrypted + (i * detail::kStateSize));
    }

    unsigned char last[detail::kStateSize] = {};
    detail::decrypt_state(rkeys, data + (bc * detail::kStateSize), last);

    if (padded_size) {
        *padded_size = last[detail::kStateSize - 1];
        const unsigned long cs = detail::kStateSize - *padded_size;

        if (!detail::check_padding(*padded_size, last)) {
            return kErrorInvalidKey;
        } else if (decrypted_size >= (bc * detail::kStateSize) + cs) {
            memcpy(decrypted + (bc * detail::kStateSize), last, cs);
        } else {
            return kErrorInvalidBufferSize;
        }
    } else {
        memcpy(decrypted + (bc * detail::kStateSize), last, sizeof(last));
    }

    return kErrorOk;
}

/** @} */

/** @defgroup CBC CBC
 * CBC mode functions
 * @{ */

/**
 * Encrypt data with CBC mode.
 * @param [in]  data Data.
 * @param [in]  data_size Data size.
 *  If the pads is false, data size must be multiple of 16.
 * @param [in]  key key bytes. The key length must be 16 (128-bit), 24 (192-bit) or 32 (256-bit).
 * @param [in]  key_size key size.
 * @param [in]  iv Initialize vector.
 * @param [out] encrypted Encrypted data buffer.
 * @param [in]  encrypted_size Encrypted data buffer size.
 * @param [in]  pads If this value is true, encrypted data is padded by PKCS.
 *  Encrypted data size must be multiple of 16.
 *  If the pads is true, encrypted data is padded with PKCS.
 *  So the data is multiple of 16, encrypted data size needs additonal 16 bytes.
 * @since 1.0.0
 */
inline Error encrypt_cbc(const unsigned char* data, const unsigned long data_size,
                         const unsigned char* key, const unsigned long key_size,
                         const unsigned char iv[16], unsigned char* encrypted,
                         const unsigned long encrypted_size, const bool pads) {
    const Error e = detail::check_encrypt_cond(data_size, key_size, encrypted_size, pads);
    if (e != kErrorOk) {
        return e;
    }

    const detail::RoundKeys rkeys = detail::expand_key(key, static_cast<int>(key_size));

    unsigned char s[detail::kStateSize] = {}; // encrypting data

    // calculate padding value
    const bool ge16 = (data_size >= detail::kStateSize);
    const int rem = data_size % detail::kStateSize;
    const unsigned char pad_v = detail::kStateSize - rem;

    // encrypt 1st state
    if (ge16) {
        memcpy(s, data, detail::kStateSize);
    } else {
        memset(s, pad_v, detail::kStateSize);
        memcpy(s, data, data_size);
    }
    if (iv) {
        detail::xor_data(s, iv);
    }
    detail::encrypt_state(rkeys, s, encrypted);

    // encrypt mid
    const unsigned long bc = data_size / detail::kStateSize;
    for (unsigned long i = 1; i < bc; ++i) {
        const long offset = i * detail::kStateSize;
        memcpy(s, data + offset, detail::kStateSize);
        detail::xor_data(s, encrypted + offset - detail::kStateSize);

        detail::encrypt_state(rkeys, s, encrypted + offset);
    }

    // enctypt last
    if (pads && ge16) {
        std::vector<unsigned char> ib(detail::kStateSize, pad_v), ob(detail::kStateSize);
        memcpy(&ib[0], data + data_size - rem, rem);

        detail::xor_data(&ib[0], encrypted + (bc - 1) * detail::kStateSize);

        detail::encrypt_state(rkeys, &ib[0], &ob[0]);
        memcpy(encrypted + (data_size - rem), &ob[0], detail::kStateSize);
    }

    return kErrorOk;
}

/**
 * Decrypt data with CBC mode.
 * @param [in]  data Data bytes.
 * @param [in]  data_size Data size.
 * @param [in]  key Key bytes.
 * @param [in]  key_size Key size.
 * @param [in]  iv Initialize vector.
 * @param [out] decrypted Decrypted data buffer.
 * @param [in]  decrypted_size Decrypted data buffer size.
 * @param [out] padded_size If this value is NULL, this function does not remove padding.
 *  If this value is not NULL, this function removes padding by PKCS
 *  and returns padded size using padded_size.
 * @since 1.0.0
 */
inline Error decrypt_cbc(const unsigned char* data, const unsigned long data_size,
                         const unsigned char* key, const unsigned long key_size,
                         const unsigned char iv[16], unsigned char* decrypted,
                         const unsigned long decrypted_size, unsigned long* padded_size) {
    const Error e = detail::check_decrypt_cond(data_size, key_size, decrypted_size, padded_size);
    if (e != kErrorOk) {
        return e;
    }

    const detail::RoundKeys rkeys = detail::expand_key(key, static_cast<int>(key_size));

    // decrypt 1st state
    detail::decrypt_state(rkeys, data, decrypted);
    if (iv) {
        detail::xor_data(decrypted, iv);
    }

    // decrypt mid
    const unsigned long bc = data_size / detail::kStateSize - 1;
    for (unsigned long i = 1; i < bc; ++i) {
        const long offset = i * detail::kStateSize;
        detail::decrypt_state(rkeys, data + offset, decrypted + offset);
        detail::xor_data(decrypted + offset, data + offset - detail::kStateSize);
    }

    // decrypt last
    unsigned char last[detail::kStateSize] = {};
    if (data_size > detail::kStateSize) {
        detail::decrypt_state(rkeys, data + (bc * detail::kStateSize), last);
        detail::xor_data(last, data + (bc * detail::kStateSize - detail::kStateSize));
    } else {
        memcpy(last, decrypted, data_size);
        memset(decrypted, 0, decrypted_size);
    }

    if (padded_size) {
        *padded_size = last[detail::kStateSize - 1];
        const unsigned long cs = detail::kStateSize - *padded_size;

        if (!detail::check_padding(*padded_size, last)) {
            return kErrorInvalidKey;
        } else if (decrypted_size >= (bc * detail::kStateSize) + cs) {
            memcpy(decrypted + (bc * detail::kStateSize), last, cs);
        } else {
            return kErrorInvalidBufferSize;
        }
    } else {
        memcpy(decrypted + (bc * detail::kStateSize), last, sizeof(last));
    }

    return kErrorOk;
}

/** @} */

/** @defgroup GCM GCM
 * GCM mode functions
 * @{ */

/**
 * Encrypts data with GCM mode and gets an authentication tag.
 *
 * You can specify iv size and tag size.
 * But usually you should use the other overloaded function whose iv and tag size is fixed.
 *
 * @returns kErrorOk
 * @returns kErrorInvalidKeySize
 * @returns kErrorInvalidIvSize
 * @returns kErrorInvalidTagSize
 */
inline Error encrypt_gcm(unsigned char* data, const std::size_t data_size,
                         const unsigned char* aadata, const std::size_t aadata_size,
                         const unsigned char* key, const std::size_t key_size,
                         const unsigned char* iv, const std::size_t iv_size, unsigned char* tag,
                         const std::size_t tag_size) {
    const Error err = detail::check_gcm_cond(key_size, iv_size, tag_size);
    if (err != kErrorOk) {
        return err;
    }

    detail::gcm::crypt_gcm(data, data_size, key, key_size, iv, iv_size, data);
    detail::gcm::calc_gcm_tag(data, data_size, aadata, aadata_size, key, key_size, iv, iv_size, tag,
                              tag_size);

    return kErrorOk;
}

/**
 * Encrypts data with GCM mode and gets an authentication tag.
 *
 * @param [in,out] data Input data and output buffer.
 *      This buffer is replaced with encrypted data.
 * @param [in] data_size data size
 * @param [in] aadata Additional Authenticated data
 * @param [in] aadata_size aadata size
 * @param [in] key Cipher key
 * @param [in] key_size Cipher key size. This value must be 16 (128-bit), 24 (192-bit), or 32
 * (256-bit).
 * @param [in] iv Initialization vector
 * @param [out] tag Calculated authentication tag data
 *
 * @returns kErrorOk
 * @returns kErrorInvalidKeySize
 */
inline Error encrypt_gcm(unsigned char* data, const std::size_t data_size,
                         const unsigned char* aadata, const std::size_t aadata_size,
                         const unsigned char* key, const std::size_t key_size,
                         const unsigned char (*iv)[12], unsigned char (*tag)[16]) {
    return encrypt_gcm(data, data_size, aadata, aadata_size, key, key_size, *iv, 12, *tag, 16);
}

/**
 * Decrypts data with GCM mode and checks an authentication tag.
 *
 * You can specify iv size and tag size.
 * But usually you should use the other overloaded function whose iv and tag size is fixed.
 *
 * @returns kErrorOk
 * @returns kErrorInvalidKeySize
 * @returns kErrorInvalidIvSize
 * @returns kErrorInvalidTagSize
 * @returns kErrorInvalidTag
 */
inline Error decrypt_gcm(unsigned char* data, const std::size_t data_size,
                         const unsigned char* aadata, const std::size_t aadata_size,
                         const unsigned char* key, const std::size_t key_size,
                         const unsigned char* iv, const std::size_t iv_size,
                         const unsigned char* tag, const std::size_t tag_size) {
    const Error err = detail::check_gcm_cond(key_size, iv_size, tag_size);
    if (err != kErrorOk) {
        return err;
    }

    unsigned char* C = data;
    const auto C_size = data_size;
    unsigned char tagd[16] = {};
    detail::gcm::calc_gcm_tag(C, C_size, aadata, aadata_size, key, key_size, iv, iv_size, tagd, 16);

    if (memcmp(tag, tagd, tag_size) != 0) {
        return kErrorInvalidTag;
    } else {
        detail::gcm::crypt_gcm(C, C_size, key, key_size, iv, iv_size, C);

        return kErrorOk;
    }
}

/**
 * Decrypts data with GCM mode and checks an authentication tag.
 *
 * @param [in,out] data Input data and output buffer.
 *      This buffer is replaced with decrypted data.
 * @param [in] data_size data size
 * @param [in] aadata Additional Authenticated data
 * @param [in] aadata_size aadata size
 * @param [in] key Cipher key
 * @param [in] key_size Cipher key size. This value must be 16 (128-bit), 24 (192-bit), or 32
 * (256-bit).
 * @param [in] iv Initialization vector
 * @param [in] tag Authentication tag data
 *
 * @returns kErrorOk
 * @returns kErrorInvalidKeySize
 * @returns kErrorInvalidTag
 */
inline Error decrypt_gcm(unsigned char* data, const std::size_t data_size,
                         const unsigned char* aadata, const std::size_t aadata_size,
                         const unsigned char* key, const std::size_t key_size,
                         const unsigned char (*iv)[12], const unsigned char (*tag)[16]) {
    return decrypt_gcm(data, data_size, aadata, aadata_size, key, key_size, *iv, 12, *tag, 16);
}

/** @} */

/** @defgroup CTR CTR
 * CTR mode function
 * @{ */

/**
 * Encrypts or decrypt data in-place with CTR mode.
 *
 * @param [in,out] data Input data and output buffer.
 *      This buffer is replaced with encrypted / decrypted data.
 * @param [in] data_size Data size.
 * @param [in] key Cipher key
 * @param [in] key_size Cipher key size. This value must be 16 (128-bit), 24 (192-bit), or 32
 * (256-bit).
 * @param [in] nonce Nonce of the counter initialization.
 *
 * @returns kErrorOk
 * @returns kErrorInvalidKeySize
 * @since 1.0.0
 */
inline Error crypt_ctr(unsigned char* data, const std::size_t data_size, const unsigned char* key,
                       const std::size_t key_size, const unsigned char (*nonce)[16]) {
    if (!detail::is_valid_key_size(key_size))
        return kErrorInvalidKeySize;
    const detail::RoundKeys rkeys = detail::expand_key(key, static_cast<int>(key_size));

    unsigned long pos = 0;
    unsigned long blkpos = detail::kStateSize;
    unsigned char blk[detail::kStateSize] = {};
    unsigned char counter[detail::kStateSize] = {};
    memcpy(counter, nonce, 16);

    while (pos < data_size) {
        if (blkpos == detail::kStateSize) {
            detail::encrypt_state(rkeys, counter, blk);
            detail::incr_counter(counter);
            blkpos = 0;
        }
        data[pos++] ^= blk[blkpos++];
    }

    return kErrorOk;
}

/** @} */

} // namespace aes
