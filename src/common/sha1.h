// SPDX-FileCopyrightText: 2012 SAURAV MOHAPATRA <mohaps@gmail.com>
// SPDX-License-Identifier: MIT

#pragma once

#include <cstdint>
#include <cstdlib>
#include <cstring>

namespace sha1 {
class SHA1 {
public:
    typedef uint32_t digest32_t[5];
    typedef uint8_t digest8_t[20];
    inline static uint32_t LeftRotate(uint32_t value, size_t count) {
        return (value << count) ^ (value >> (32 - count));
    }
    SHA1() {
        reset();
    }
    virtual ~SHA1() {}
    SHA1(const SHA1& s) {
        *this = s;
    }
    const SHA1& operator=(const SHA1& s) {
        memcpy(m_digest, s.m_digest, 5 * sizeof(uint32_t));
        memcpy(m_block, s.m_block, 64);
        m_blockByteIndex = s.m_blockByteIndex;
        m_byteCount = s.m_byteCount;
        return *this;
    }
    SHA1& reset() {
        m_digest[0] = 0x67452301;
        m_digest[1] = 0xEFCDAB89;
        m_digest[2] = 0x98BADCFE;
        m_digest[3] = 0x10325476;
        m_digest[4] = 0xC3D2E1F0;
        m_blockByteIndex = 0;
        m_byteCount = 0;
        return *this;
    }
    SHA1& processByte(uint8_t octet) {
        this->m_block[this->m_blockByteIndex++] = octet;
        ++this->m_byteCount;
        if (m_blockByteIndex == 64) {
            this->m_blockByteIndex = 0;
            processBlock();
        }
        return *this;
    }
    SHA1& processBlock(const void* const start, const void* const end) {
        const uint8_t* begin = static_cast<const uint8_t*>(start);
        const uint8_t* finish = static_cast<const uint8_t*>(end);
        while (begin != finish) {
            processByte(*begin);
            begin++;
        }
        return *this;
    }
    SHA1& processBytes(const void* const data, size_t len) {
        const uint8_t* block = static_cast<const uint8_t*>(data);
        processBlock(block, block + len);
        return *this;
    }
    const uint32_t* getDigest(digest32_t digest) {
        size_t bitCount = this->m_byteCount * 8;
        processByte(0x80);
        if (this->m_blockByteIndex > 56) {
            while (m_blockByteIndex != 0) {
                processByte(0);
            }
            while (m_blockByteIndex < 56) {
                processByte(0);
            }
        } else {
            while (m_blockByteIndex < 56) {
                processByte(0);
            }
        }
        processByte(0);
        processByte(0);
        processByte(0);
        processByte(0);
        processByte(static_cast<unsigned char>((bitCount >> 24) & 0xFF));
        processByte(static_cast<unsigned char>((bitCount >> 16) & 0xFF));
        processByte(static_cast<unsigned char>((bitCount >> 8) & 0xFF));
        processByte(static_cast<unsigned char>((bitCount) & 0xFF));

        memcpy(digest, m_digest, 5 * sizeof(uint32_t));
        return digest;
    }
    const uint8_t* getDigestBytes(digest8_t digest) {
        digest32_t d32;
        getDigest(d32);
        size_t di = 0;
        digest[di++] = ((d32[0] >> 24) & 0xFF);
        digest[di++] = ((d32[0] >> 16) & 0xFF);
        digest[di++] = ((d32[0] >> 8) & 0xFF);
        digest[di++] = ((d32[0]) & 0xFF);

        digest[di++] = ((d32[1] >> 24) & 0xFF);
        digest[di++] = ((d32[1] >> 16) & 0xFF);
        digest[di++] = ((d32[1] >> 8) & 0xFF);
        digest[di++] = ((d32[1]) & 0xFF);

        digest[di++] = ((d32[2] >> 24) & 0xFF);
        digest[di++] = ((d32[2] >> 16) & 0xFF);
        digest[di++] = ((d32[2] >> 8) & 0xFF);
        digest[di++] = ((d32[2]) & 0xFF);

        digest[di++] = ((d32[3] >> 24) & 0xFF);
        digest[di++] = ((d32[3] >> 16) & 0xFF);
        digest[di++] = ((d32[3] >> 8) & 0xFF);
        digest[di++] = ((d32[3]) & 0xFF);

        digest[di++] = ((d32[4] >> 24) & 0xFF);
        digest[di++] = ((d32[4] >> 16) & 0xFF);
        digest[di++] = ((d32[4] >> 8) & 0xFF);
        digest[di++] = ((d32[4]) & 0xFF);
        return digest;
    }

protected:
    void processBlock() {
        uint32_t w[80];
        for (size_t i = 0; i < 16; i++) {
            w[i] = (m_block[i * 4 + 0] << 24);
            w[i] |= (m_block[i * 4 + 1] << 16);
            w[i] |= (m_block[i * 4 + 2] << 8);
            w[i] |= (m_block[i * 4 + 3]);
        }
        for (size_t i = 16; i < 80; i++) {
            w[i] = LeftRotate((w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16]), 1);
        }

        uint32_t a = m_digest[0];
        uint32_t b = m_digest[1];
        uint32_t c = m_digest[2];
        uint32_t d = m_digest[3];
        uint32_t e = m_digest[4];

        for (std::size_t i = 0; i < 80; ++i) {
            uint32_t f = 0;
            uint32_t k = 0;

            if (i < 20) {
                f = (b & c) | (~b & d);
                k = 0x5A827999;
            } else if (i < 40) {
                f = b ^ c ^ d;
                k = 0x6ED9EBA1;
            } else if (i < 60) {
                f = (b & c) | (b & d) | (c & d);
                k = 0x8F1BBCDC;
            } else {
                f = b ^ c ^ d;
                k = 0xCA62C1D6;
            }
            uint32_t temp = LeftRotate(a, 5) + f + e + k + w[i];
            e = d;
            d = c;
            c = LeftRotate(b, 30);
            b = a;
            a = temp;
        }

        m_digest[0] += a;
        m_digest[1] += b;
        m_digest[2] += c;
        m_digest[3] += d;
        m_digest[4] += e;
    }

private:
    digest32_t m_digest;
    uint8_t m_block[64];
    size_t m_blockByteIndex;
    size_t m_byteCount;
};
} // namespace sha1
