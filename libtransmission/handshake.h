// This file Copyright © 2007-2023 Mnemosyne LLC.
// It may be used under GPLv2 (SPDX: GPL-2.0-only), GPLv3 (SPDX: GPL-3.0-only),
// or any future license endorsed by Mnemosyne LLC.
// License text can be found in the licenses/ folder.

#pragma once

#ifndef __TRANSMISSION__
#error only libtransmission should #include this header.
#endif

#include <algorithm> // for std::copy()
#include <array>
#include <chrono>
#include <cstdint> // for uintX_t
#include <cstddef> // for std::byte, size_t
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string_view>
#include <utility>

#include "libtransmission/transmission.h"

#include "libtransmission/net.h"
#include "libtransmission/peer-mse.h" // tr_message_stream_encryption::DH
#include "libtransmission/peer-io.h"
#include "libtransmission/timer.h"
#include "libtransmission/tr-macros.h" // tr_sha1_digest_t, tr_peer_id_t

struct tr_error;

// short-term class which manages the handshake phase of a tr_peerIo
class tr_handshake
{
public:
    using DH = tr_message_stream_encryption::DH;

    struct Result
    {
        std::shared_ptr<tr_peerIo> io;
        std::optional<tr_peer_id_t> peer_id;
        bool read_anything_from_peer = false;
        bool is_connected = false;
    };

    using DoneFunc = std::function<bool(Result const&)>;

    class Mediator
    {
    public:
        struct TorrentInfo
        {
            tr_sha1_digest_t info_hash;
            tr_peer_id_t client_peer_id;
            tr_torrent_id_t id;
            bool is_done;
        };

        virtual ~Mediator() = default;

        [[nodiscard]] virtual std::optional<TorrentInfo> torrent(tr_sha1_digest_t const& info_hash) const = 0;
        [[nodiscard]] virtual std::optional<TorrentInfo> torrent_from_obfuscated(tr_sha1_digest_t const& info_hash) const = 0;
        [[nodiscard]] virtual libtransmission::TimerMaker& timer_maker() = 0;
        [[nodiscard]] virtual bool allows_dht() const = 0;
        [[nodiscard]] virtual bool allows_tcp() const = 0;
        [[nodiscard]] virtual size_t pad(void* setme, size_t max_bytes) const = 0;
        [[nodiscard]] virtual DH::private_key_bigend_t private_key() const
        {
            return DH::randomPrivateKey();
        }

        virtual void set_utp_failed(tr_sha1_digest_t const& info_hash, tr_socket_address const& socket_address) = 0;
    };

    tr_handshake(Mediator* mediator, std::shared_ptr<tr_peerIo> peer_io, tr_encryption_mode mode_in, DoneFunc on_done);

private:
    enum class ParseResult
    {
        Ok,
        EncryptionWrong,
        BadTorrent,
        PeerIsSelf,
    };

    enum class State
    {
        // incoming
        AwaitingHandshake,
        AwaitingPeerId,
        AwaitingYa,
        AwaitingPadA,
        AwaitingCryptoProvide,
        AwaitingPadC,
        AwaitingIa,
        AwaitingPayloadStream,

        // outgoing
        AwaitingYb,
        AwaitingVc,
        AwaitingCryptoSelect,
        AwaitingPadD
    };

    ///

    [[nodiscard]] static std::string_view state_string(State state) noexcept;

    [[nodiscard]] static uint32_t get_crypto_select(tr_encryption_mode encryption_mode, uint32_t crypto_provide) noexcept;

    static ReadState can_read(tr_peerIo* peer_io, void* vhandshake, size_t* piece);

    static void on_error(tr_peerIo* io, tr_error const&, void* vhandshake);

    bool build_handshake_message(tr_peerIo* io, uint8_t* buf) const;

    ReadState read_crypto_provide(tr_peerIo*);
    ReadState read_crypto_select(tr_peerIo*);
    ReadState read_handshake(tr_peerIo*);
    ReadState read_ia(tr_peerIo*);
    ReadState read_pad_a(tr_peerIo*);
    ReadState read_pad_c(tr_peerIo*);
    ReadState read_pad_d(tr_peerIo*);
    ReadState read_payload_stream(tr_peerIo*);
    ReadState read_peer_id(tr_peerIo*);
    ReadState read_vc(tr_peerIo*);
    ReadState read_ya(tr_peerIo*);
    ReadState read_yb(tr_peerIo*);

    void send_ya(tr_peerIo*);

    ParseResult parse_handshake(tr_peerIo* peer_io);

    void set_peer_id(tr_peer_id_t const& id) noexcept
    {
        peer_id_ = id;
    }

    constexpr void set_have_read_anything_from_peer(bool val) noexcept
    {
        have_read_anything_from_peer_ = val;
    }

    ReadState done(bool is_connected)
    {
        peer_io_->clear_callbacks();
        return fire_done(is_connected) ? READ_LATER : READ_ERR;
    }

    [[nodiscard]] auto is_incoming() const noexcept
    {
        return peer_io_->is_incoming();
    }

    [[nodiscard]] constexpr auto state() const noexcept
    {
        return state_;
    }

    [[nodiscard]] constexpr auto is_state(State state) const noexcept
    {
        return state_ == state;
    }

    constexpr void set_state(State state) noexcept
    {
        state_ = state;
    }

    [[nodiscard]] std::string_view state_string() const noexcept
    {
        return state_string(state_);
    }

    [[nodiscard]] uint32_t crypto_provide() const noexcept;

    template<size_t PadMax>
    void send_public_key_and_pad(tr_peerIo* io)
    {
        auto const public_key = dh_.publicKey();
        auto outbuf = std::array<std::byte, std::size(public_key) + PadMax>{};
        auto const data = std::data(outbuf);
        auto walk = data;
        walk = std::copy(std::begin(public_key), std::end(public_key), walk);
        walk += mediator_->pad(walk, PadMax);
        io->write_bytes(data, walk - data, false);
    }

    bool fire_done(bool is_connected);

    ///

    static auto constexpr HandshakeTimeoutSec = std::chrono::seconds{ 30 };

    // bittorrent handshake constants
    // https://www.bittorrent.org/beps/bep_0003.html#peer-protocol
    // https://wiki.theory.org/BitTorrentSpecification#Handshake

    // > The handshake starts with character ninteen (decimal) followed by the string
    // > 'BitTorrent protocol'. The leading character is a length prefix.
    static auto constexpr HandshakeName = std::array<std::byte, 20>{
        std::byte{ 19 },  std::byte{ 'B' }, std::byte{ 'i' }, std::byte{ 't' }, std::byte{ 'T' },
        std::byte{ 'o' }, std::byte{ 'r' }, std::byte{ 'r' }, std::byte{ 'e' }, std::byte{ 'n' },
        std::byte{ 't' }, std::byte{ ' ' }, std::byte{ 'p' }, std::byte{ 'r' }, std::byte{ 'o' },
        std::byte{ 't' }, std::byte{ 'o' }, std::byte{ 'c' }, std::byte{ 'o' }, std::byte{ 'l' }
    };

    // [Next comes] eight reserved bytes [used for enabling ltep, dht, fext]
    static auto constexpr HandshakeFlagsBytes = size_t{ 8 };
    static auto constexpr HandshakeFlagsBits = size_t{ 64 };
    // https://www.bittorrent.org/beps/bep_0004.html
    // https://wiki.theory.org/BitTorrentSpecification#Reserved_Bytes
    static auto constexpr LtepFlag = size_t{ 43U };
    static auto constexpr FextFlag = size_t{ 61U };
    static auto constexpr DhtFlag = size_t{ 63U };

    // Next comes the 20 byte sha1 info_hash and the 20-byte peer_id
    static auto constexpr HandshakeSize = sizeof(HandshakeName) + HandshakeFlagsBytes + sizeof(tr_sha1_digest_t) +
        sizeof(tr_peer_id_t);
    static_assert(HandshakeSize == 68);

    // Length of handhshake up through the info_hash. From theory.org:
    // > The recipient may wait for the initiator's handshake... however,
    // > the recipient must respond as soon as it sees the info_hash part
    // > of the handshake (the peer id will presumably be sent after the
    // > recipient sends its own handshake).
    static auto constexpr IncomingHandshakeLen = sizeof(HandshakeName) + HandshakeFlagsBytes + sizeof(tr_sha1_digest_t);
    static_assert(IncomingHandshakeLen == 48);

    // MSE constants.
    // http://wiki.vuze.com/w/Message_Stream_Encryption
    // > crypto_provide and crypto_select are a 32bit bitfields.
    // > As of now 0x01 means plaintext, 0x02 means RC4. (see Functions)
    // > The remaining bits are reserved for future use.
    static auto constexpr CryptoProvidePlaintext = size_t{ 0x01 };
    static auto constexpr CryptoProvideCrypto = size_t{ 0x02 };

    // MSE constants.
    // http://wiki.vuze.com/w/Message_Stream_Encryption
    // > PadA, PadB: Random data with a random length of 0 to 512 bytes each
    // > PadC, PadD: Arbitrary data with a length of 0 to 512 bytes
    static auto constexpr PadaMaxlen = int{ 512 };
    static auto constexpr PadbMaxlen = int{ 512 };
    static auto constexpr PadcMaxlen = int{ 512 };

    // "VC is a verification constant that is used to verify whether the
    // other side knows S and SKEY and thus defeats replay attacks of the
    // SKEY hash. As of this version VC is a String of 8 bytes set to 0x00."
    // https://wiki.vuze.com/w/Message_Stream_Encryption
    using vc_t = std::array<std::byte, 8>;
    static auto constexpr VC = vc_t{};

    // Used when resynchronizing in read_vc(). This value is cached to avoid
    // the cost of recomputing it. MSE spec: "Since the length of [PadB is]
    // unknown, A will be able to resynchronize on ENCRYPT(VC)".
    std::optional<vc_t> encrypted_vc_;

    ///

    static constexpr auto DhPoolMaxSize = size_t{ 32 };
    static inline auto dh_pool_size_ = size_t{};
    static inline auto dh_pool_ = std::array<tr_message_stream_encryption::DH, DhPoolMaxSize>{};
    static inline auto dh_pool_mutex_ = std::mutex{};

    [[nodiscard]] static DH get_dh(Mediator* mediator)
    {
        auto lock = std::unique_lock(dh_pool_mutex_);

        if (dh_pool_size_ > 0U)
        {
            auto dh = DH{};
            std::swap(dh, dh_pool_[dh_pool_size_ - 1U]);
            --dh_pool_size_;
            return dh;
        }

        return DH{ mediator->private_key() };
    }

    static void add_dh(DH&& dh)
    {
        auto lock = std::unique_lock(dh_pool_mutex_);

        if (dh_pool_size_ < std::size(dh_pool_))
        {
            dh_pool_[dh_pool_size_] = std::move(dh);
            ++dh_pool_size_;
        }
    }

    void maybe_recycle_dh()
    {
        // keys are expensive to make, so recycle iff the peer was unreachable

        if (have_read_anything_from_peer_)
        {
            return;
        }

        auto dh = DH{};
        std::swap(dh_, dh);
        add_dh(std::move(dh));
    }

    ///

    DH dh_ = {};

    DoneFunc on_done_;

    std::optional<tr_peer_id_t> peer_id_;

    std::shared_ptr<tr_peerIo> peer_io_;

    std::unique_ptr<libtransmission::Timer> timeout_timer_;

    Mediator* mediator_ = nullptr;

    State state_ = State::AwaitingHandshake;

    tr_encryption_mode encryption_mode_;

    uint32_t crypto_select_ = {};
    uint32_t crypto_provide_ = {};
    uint16_t pad_c_len_ = {};
    uint16_t pad_d_len_ = {};
    uint16_t ia_len_ = {};

    bool have_read_anything_from_peer_ = false;

    bool have_sent_bittorrent_handshake_ = false;
};
