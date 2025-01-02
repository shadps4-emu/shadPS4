// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <span>
#include <cryptopp/aes.h>
#include <cryptopp/filters.h>
#include <cryptopp/modes.h>
#include <cryptopp/oaep.h>
#include <cryptopp/osrng.h>
#include <cryptopp/rsa.h>
#include <cryptopp/sha.h>

#include "common/types.h"
#include "keys.h"

class Crypto {
public:
    CryptoPP::RSA::PrivateKey key_pkg_derived_key3_keyset_init();
    CryptoPP::RSA::PrivateKey FakeKeyset_keyset_init();
    CryptoPP::RSA::PrivateKey DebugRifKeyset_init();

    void RSA2048Decrypt(std::span<CryptoPP::byte, 32> dk3,
                        std::span<const CryptoPP::byte, 256> ciphertext,
                        bool is_dk3); // RSAES_PKCS1v15_
    void ivKeyHASH256(std::span<const CryptoPP::byte, 64> cipher_input,
                      std::span<CryptoPP::byte, 32> ivkey_result);
    void aesCbcCfb128Decrypt(std::span<const CryptoPP::byte, 32> ivkey,
                             std::span<const CryptoPP::byte, 256> ciphertext,
                             std::span<CryptoPP::byte, 256> decrypted);
    void aesCbcCfb128DecryptEntry(std::span<const CryptoPP::byte, 32> ivkey,
                                  std::span<CryptoPP::byte> ciphertext,
                                  std::span<CryptoPP::byte> decrypted);
    void decryptEFSM(std::span<CryptoPP::byte, 16> trophyKey,
                     std::span<CryptoPP::byte, 16> NPcommID, std::span<CryptoPP::byte, 16> efsmIv,
                     std::span<CryptoPP::byte> ciphertext, std::span<CryptoPP::byte> decrypted);
    void PfsGenCryptoKey(std::span<const CryptoPP::byte, 32> ekpfs,
                         std::span<const CryptoPP::byte, 16> seed,
                         std::span<CryptoPP::byte, 16> dataKey,
                         std::span<CryptoPP::byte, 16> tweakKey);
    void decryptPFS(std::span<const CryptoPP::byte, 16> dataKey,
                    std::span<const CryptoPP::byte, 16> tweakKey, std::span<const u8> src_image,
                    std::span<CryptoPP::byte> dst_image, u64 sector);

    void xtsXorBlock(CryptoPP::byte* x, const CryptoPP::byte* a, const CryptoPP::byte* b) {
        for (int i = 0; i < 16; i++) {
            x[i] = a[i] ^ b[i];
        }
    }

    void xtsMult(std::span<CryptoPP::byte, 16> encryptedTweak) {
        int feedback = 0;
        for (int k = 0; k < encryptedTweak.size(); k++) {
            const auto tmp = (encryptedTweak[k] >> 7) & 1;
            encryptedTweak[k] = ((encryptedTweak[k] << 1) + feedback) & 0xFF;
            feedback = tmp;
        }
        if (feedback != 0) {
            encryptedTweak[0] ^= 0x87;
        }
    }
};
