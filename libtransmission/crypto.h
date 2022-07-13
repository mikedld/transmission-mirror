// This file Copyright © 2007-2022 Mnemosyne LLC.
// It may be used under GPLv2 (SPDX: GPL-2.0), GPLv3 (SPDX: GPL-3.0),
// or any future license endorsed by Mnemosyne LLC.
// License text can be found in the licenses/ folder.

// NB: crypto-test-ref.h needs this, so use it instead of #pragma once
#ifndef TR_ENCRYPTION_H
#define TR_ENCRYPTION_H

#ifndef __TRANSMISSION__
#error only libtransmission should #include this header.
#endif

#include <array>
#include <cstddef> // size_t
#include <cstdint> // uint8_t
#include <optional>
#include <string_view>
#include <vector>

#include <arc4.h>

#include "tr-macros.h" // tr_sha1_digest_t

enum
{
    KEY_LEN = 96
};

struct arc4_context;

/** @brief Holds state information for encrypted peer communications */
struct tr_crypto
{
    static auto constexpr PrivateKeySize = size_t{ 20 };
    using private_key_bigend_t = std::array<std::byte, PrivateKeySize>;

    static auto constexpr KeySize = size_t{ 96 };
    using key_bigend_t = std::array<std::byte, KeySize>;

    tr_crypto(tr_sha1_digest_t const* torrent_hash = nullptr, bool is_incoming = true);

    tr_crypto& operator=(tr_crypto const&) = delete;
    tr_crypto& operator=(tr_crypto&&) = delete;
    tr_crypto(tr_crypto const&) = delete;
    tr_crypto(tr_crypto&&) = delete;

    void setTorrentHash(tr_sha1_digest_t hash) noexcept
    {
        torrent_hash_ = hash;
    }

    [[nodiscard]] constexpr auto const& torrentHash() const noexcept
    {
        return torrent_hash_;
    }

    [[nodiscard]] constexpr auto myPublicKey()
    {
        ensureKeyExists();
        return wi_public_key_;
    }

    [[nodiscard]] auto publicKey()
    {
        ensureKeyExists();
        return wi_public_key_;
    }

    void setPeerPublicKey(key_bigend_t const& peer_public_key)
    {
        (void)computeSecret(std::data(peer_public_key), std::size(peer_public_key));
    }

    [[nodiscard]] bool computeSecret(void const* peer_public_key, size_t len);

    [[nodiscard]] constexpr auto secret() const noexcept
    {
        return secret_;
    }

    [[nodiscard]] constexpr auto privateKey() const noexcept
    {
        return wi_private_key_;
    }

    [[nodiscard]] std::optional<tr_sha1_digest_t> secretKeySha1(
        void const* prepend,
        size_t prepend_len,
        void const* append,
        size_t append_len) const;

    [[nodiscard]] constexpr auto isIncoming() const noexcept
    {
        return is_incoming_;
    }

    [[nodiscard]] virtual std::vector<uint8_t> pad(size_t maxlen) const;

    void decryptInit();
    void decrypt(size_t buflen, void const* buf_in, void* buf_out);
    void encryptInit();
    void encrypt(size_t buflen, void const* buf_in, void* buf_out);

    private_key_bigend_t wi_private_key_ = {};
    key_bigend_t wi_public_key_ = {};
    key_bigend_t secret_ = {};

private:
    void ensureKeyExists();

    std::optional<tr_sha1_digest_t> torrent_hash_;
    arc4_context dec_key_;
    arc4_context enc_key_;
    bool const is_incoming_;
};

#endif // TR_ENCRYPTION_H
