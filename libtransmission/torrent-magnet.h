// This file Copyright © Mnemosyne LLC.
// It may be used under GPLv2 (SPDX: GPL-2.0-only), GPLv3 (SPDX: GPL-3.0-only),
// or any future license endorsed by Mnemosyne LLC.
// License text can be found in the licenses/ folder.

#pragma once

#ifndef __TRANSMISSION__
#error only libtransmission should #include this header.
#endif

#include <cstddef> // size_t
#include <cstdint> // int64_t
#include <ctime> // time_t
#include <deque>
#include <limits>
#include <memory>
#include <optional>
#include <vector>

#include <small/vector.hpp>

struct tr_error;
struct tr_torrent;
struct tr_torrent_metainfo;

// defined by BEP #9
inline constexpr int MetadataPieceSize = 1024 * 16;

using tr_metadata_piece = small::max_size_vector<std::byte, MetadataPieceSize>;

class tr_incomplete_metadata
{
public:
    struct Mediator
    {
        virtual ~Mediator() = default;

        [[nodiscard]] virtual std::string log_name() const noexcept = 0;
    };

    tr_incomplete_metadata(std::unique_ptr<Mediator> mediator, int64_t size);

    [[nodiscard]] static constexpr auto is_valid_metadata_size(int64_t const size) noexcept
    {
        return size > 0 && size <= std::numeric_limits<int>::max();
    }

    [[nodiscard]] constexpr size_t get_piece_length(int const piece) const noexcept
    {
        return piece + 1 == piece_count_ ? // last piece
            std::size(metadata_) - (piece * MetadataPieceSize) :
            MetadataPieceSize;
    }

    bool set_metadata_piece(int piece, void const* data, size_t len);

    [[nodiscard]] std::optional<int> get_next_metadata_request(time_t now) noexcept;

    [[nodiscard]] double get_metadata_percent() const noexcept;

    [[nodiscard]] constexpr auto const& metadata() const noexcept
    {
        return metadata_;
    }

    [[nodiscard]] auto log_name() const noexcept
    {
        return mediator_->log_name();
    }

private:
    struct metadata_node
    {
        time_t requested_at = 0U;
        int piece = 0;
    };

    void create_all_needed(int n_pieces) noexcept;

    std::vector<char> metadata_;
    std::deque<metadata_node> pieces_needed_;
    int piece_count_ = 0;

    std::unique_ptr<Mediator> mediator_;
};
