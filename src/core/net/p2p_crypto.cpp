// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#include <algorithm>

#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <openssl/rand.h>

#include "core/net/p2p_crypto.h"

namespace Core::Net::P2P {

constexpr size_t IvSeedSize = 4;
constexpr size_t SignatureSize = 4;

bool Crypt(std::vector<u8>& payload, const P2PKey& key, const std::array<u8, 4>& iv_seed,
           bool encrypt) {
    std::array<u8, 16> iv;
    for (size_t i = 0; i < iv.size(); ++i) {
        iv[i] = iv_seed[i & 3];
    }
    EVP_CIPHER_CTX* context = EVP_CIPHER_CTX_new();
    if (!context) {
        return false;
    }
    int output_size = 0;
    int final_size = 0;
    const bool ok = EVP_CipherInit_ex(context, EVP_aes_128_cfb128(), nullptr, key.value.data(),
                                      iv.data(), encrypt ? 1 : 0) == 1 &&
                    EVP_CipherUpdate(context, payload.data(), &output_size, payload.data(),
                                     static_cast<int>(payload.size())) == 1 &&
                    EVP_CipherFinal_ex(context, payload.data() + output_size, &final_size) == 1;
    EVP_CIPHER_CTX_free(context);
    return ok && static_cast<size_t>(output_size + final_size) == payload.size();
}

std::array<u8, SignatureSize> Sign(std::span<const u8> body, const P2PKey& key) {
    std::array<u8, EVP_MAX_MD_SIZE> digest{};
    unsigned int digest_size = 0;
    HMAC(EVP_sha1(), key.value.data(), static_cast<int>(key.value.size()), body.data(), body.size(),
         digest.data(), &digest_size);
    std::array<u8, SignatureSize> signature{};
    std::copy_n(digest.begin(), signature.size(), signature.begin());
    return signature;
}

std::array<u8, 4> DeriveCommunicationId(std::span<const u8, 16> value) {
    std::array<u8, EVP_MAX_MD_SIZE> digest{};
    unsigned int digest_size = 0;
    const u8 empty = 0;
    HMAC(EVP_sha1(), value.data(), static_cast<int>(value.size()), &empty, 0, digest.data(),
         &digest_size);
    std::array<u8, 4> id{};
    std::copy_n(digest.begin(), id.size(), id.begin());
    return id;
}

bool ProtectPayload(std::span<const u8> data, bool encrypt, bool sign, const P2PKey& key,
                    std::vector<u8>& output) {
    std::vector<u8> body(data.begin(), data.end());
    std::array<u8, IvSeedSize> iv{};
    if (encrypt &&
        (RAND_bytes(iv.data(), static_cast<int>(iv.size())) != 1 || !Crypt(body, key, iv, true))) {
        return false;
    }
    output.clear();
    output.reserve(body.size() + (encrypt ? IvSeedSize : 0) + (sign ? SignatureSize : 0));
    if (encrypt) {
        output.insert(output.end(), iv.begin(), iv.end());
    }
    if (sign) {
        const auto signature = Sign(body, key); // signs ciphertext
        output.insert(output.end(), signature.begin(), signature.end());
    }
    output.insert(output.end(), body.begin(), body.end());
    return true;
}

bool UnprotectPayload(std::span<const u8> data, bool encrypt, bool sign, const P2PKey& key,
                      std::vector<u8>& output) {
    const size_t prefix = (encrypt ? IvSeedSize : 0) + (sign ? SignatureSize : 0);
    if (data.size() < prefix) {
        return false;
    }
    std::array<u8, IvSeedSize> iv{};
    std::array<u8, SignatureSize> signature{};
    size_t offset = 0;
    if (encrypt) {
        std::copy_n(data.begin(), iv.size(), iv.begin());
        offset += iv.size();
    }
    if (sign) {
        std::copy_n(data.begin() + offset, signature.size(), signature.begin());
        offset += signature.size();
    }
    output.assign(data.begin() + offset, data.end());
    if (sign) {
        const auto expected = Sign(output, key);
        if (CRYPTO_memcmp(signature.data(), expected.data(), signature.size()) != 0) {
            return false;
        }
    }
    return !encrypt || Crypt(output, key, iv, false);
}

} // namespace Core::Net::P2P
