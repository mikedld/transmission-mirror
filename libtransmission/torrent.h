// This file Copyright © 2009-2022 Mnemosyne LLC.
// It may be used under GPLv2 (SPDX: GPL-2.0), GPLv3 (SPDX: GPL-3.0),
// or any future license endorsed by Mnemosyne LLC.
// License text can be found in the licenses/ folder.

#pragma once

#ifndef __TRANSMISSION__
#error only libtransmission should #include this header.
#endif

#include <array>
#include <cstddef> // size_t
#include <ctime>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

#include "transmission.h"

#include "announce-list.h"
#include "bandwidth.h"
#include "bitfield.h"
#include "block-info.h"
#include "completion.h"
#include "file.h"
#include "file-piece-map.h"
#include "interned-string.h"
#include "log.h"
#include "session.h"
#include "torrent-metainfo.h"
#include "tr-macros.h"
#include "tr-strbuf.h"

class tr_swarm;
struct tr_error;
struct tr_magnet_info;
struct tr_metainfo_parsed;
struct tr_session;
struct tr_torrent;
struct tr_torrent_announcer;

using tr_labels_t = std::unordered_set<std::string>;

/**
***  Package-visible ctor API
**/

void tr_torrentFree(tr_torrent* tor);

void tr_ctorInitTorrentPriorities(tr_ctor const* ctor, tr_torrent* tor);

void tr_ctorInitTorrentWanted(tr_ctor const* ctor, tr_torrent* tor);

bool tr_ctorSaveContents(tr_ctor const* ctor, std::string_view filename, tr_error** error);

tr_session* tr_ctorGetSession(tr_ctor const* ctor);

bool tr_ctorGetIncompleteDir(tr_ctor const* ctor, char const** setmeIncompleteDir);

tr_labels_t tr_ctorGetLabels(tr_ctor const* ctor);

/**
***
**/

void tr_torrentSetLabels(tr_torrent* tor, tr_labels_t&& labels);

void tr_torrentChangeMyPort(tr_torrent* session);

tr_torrent* tr_torrentFindFromObfuscatedHash(tr_session* session, tr_sha1_digest_t const& hash);

bool tr_torrentReqIsValid(tr_torrent const* tor, tr_piece_index_t index, uint32_t offset, uint32_t length);

tr_block_span_t tr_torGetFileBlockSpan(tr_torrent const* tor, tr_file_index_t file);

void tr_torrentCheckSeedLimit(tr_torrent* tor);

/** save a torrent's .resume file if it's changed since the last time it was saved */
void tr_torrentSave(tr_torrent* tor);

enum tr_verify_state
{
    TR_VERIFY_NONE,
    TR_VERIFY_WAIT,
    TR_VERIFY_NOW
};

tr_torrent_activity tr_torrentGetActivity(tr_torrent const* tor);

struct tr_incomplete_metadata;

/** @brief Torrent object */
struct tr_torrent : public tr_completion::torrent_view
{
public:
    explicit tr_torrent(tr_torrent_metainfo&& tm)
        : metainfo_{ std::move(tm) }
        , completion{ this, &this->metainfo_.blockInfo() }
    {
    }

    ~tr_torrent() override = default;

    void setLocation(
        std::string_view location,
        bool move_from_current_location,
        double volatile* setme_progress,
        int volatile* setme_state);

    void renamePath(
        std::string_view oldpath,
        std::string_view newname,
        tr_torrent_rename_done_func callback,
        void* callback_user_data);

    tr_sha1_digest_t pieceHash(tr_piece_index_t i) const
    {
        return metainfo_.pieceHash(i);
    }

    // these functions should become private when possible,
    // but more refactoring is needed before that can happen
    // because much of tr_torrent's impl is in the non-member C bindings

    void setMetainfo(tr_torrent_metainfo const& tm);

    [[nodiscard]] auto unique_lock() const
    {
        return session->unique_lock();
    }

    /// SPEED LIMIT

    void setSpeedLimitBps(tr_direction, unsigned int Bps);

    [[nodiscard]] unsigned int speedLimitBps(tr_direction) const;

    /// BLOCK INFO

    [[nodiscard]] constexpr auto const& blockInfo() const
    {
        return metainfo_.blockInfo();
    }

    [[nodiscard]] constexpr auto blockCount() const noexcept
    {
        return metainfo_.blockCount();
    }
    [[nodiscard]] auto byteLoc(uint64_t byte) const
    {
        return metainfo_.byteLoc(byte);
    }
    [[nodiscard]] auto blockLoc(tr_block_index_t block) const
    {
        return metainfo_.blockLoc(block);
    }
    [[nodiscard]] auto pieceLoc(tr_piece_index_t piece, uint32_t offset = 0, uint32_t length = 0) const
    {
        return metainfo_.pieceLoc(piece, offset, length);
    }
    [[nodiscard]] constexpr auto blockSize(tr_block_index_t block) const noexcept
    {
        return metainfo_.blockSize(block);
    }
    [[nodiscard]] constexpr auto blockSpanForPiece(tr_piece_index_t piece) const
    {
        return metainfo_.blockSpanForPiece(piece);
    }
    [[nodiscard]] constexpr auto pieceCount() const noexcept
    {
        return metainfo_.pieceCount();
    }
    [[nodiscard]] constexpr auto pieceSize() const noexcept
    {
        return metainfo_.pieceSize();
    }
    [[nodiscard]] constexpr auto pieceSize(tr_piece_index_t piece) const noexcept
    {
        return metainfo_.pieceSize(piece);
    }
    [[nodiscard]] constexpr auto totalSize() const noexcept
    {
        return metainfo_.totalSize();
    }

    /// COMPLETION

    [[nodiscard]] auto leftUntilDone() const
    {
        return completion.leftUntilDone();
    }

    [[nodiscard]] auto sizeWhenDone() const
    {
        return completion.sizeWhenDone();
    }

    [[nodiscard]] auto hasMetainfo() const noexcept
    {
        return completion.hasMetainfo();
    }

    [[nodiscard]] auto hasAll() const noexcept
    {
        return completion.hasAll();
    }

    [[nodiscard]] auto hasNone() const noexcept
    {
        return completion.hasNone();
    }

    [[nodiscard]] auto hasPiece(tr_piece_index_t piece) const
    {
        return completion.hasPiece(piece);
    }

    [[nodiscard]] auto hasBlock(tr_block_index_t block) const
    {
        return completion.hasBlock(block);
    }

    [[nodiscard]] auto countMissingBlocksInPiece(tr_piece_index_t piece) const
    {
        return completion.countMissingBlocksInPiece(piece);
    }

    [[nodiscard]] auto countMissingBytesInPiece(tr_piece_index_t piece) const
    {
        return completion.countMissingBytesInPiece(piece);
    }

    [[nodiscard]] auto hasTotal() const
    {
        return completion.hasTotal();
    }

    [[nodiscard]] auto createPieceBitfield() const
    {
        return completion.createPieceBitfield();
    }

    [[nodiscard]] constexpr bool isDone() const noexcept
    {
        return completeness != TR_LEECH;
    }

    [[nodiscard]] constexpr bool isSeed() const noexcept
    {
        return completeness == TR_SEED;
    }

    [[nodiscard]] constexpr bool isPartialSeed() const noexcept
    {
        return completeness == TR_PARTIAL_SEED;
    }

    [[nodiscard]] tr_bitfield const& blocks() const noexcept
    {
        return completion.blocks();
    }

    void amountDoneBins(float* tab, int n_tabs) const
    {
        return completion.amountDone(tab, n_tabs);
    }

    void setBlocks(tr_bitfield blocks);

    void setHasPiece(tr_piece_index_t piece, bool has)
    {
        completion.setHasPiece(piece, has);
    }

    /// FILE <-> PIECE

    [[nodiscard]] auto piecesInFile(tr_file_index_t file) const
    {
        return fpm_.pieceSpan(file);
    }

    [[nodiscard]] auto fileOffset(tr_block_info::Location loc) const
    {
        return fpm_.fileOffset(loc.byte);
    }

    /// WANTED

    [[nodiscard]] bool pieceIsWanted(tr_piece_index_t piece) const final
    {
        return files_wanted_.pieceWanted(piece);
    }

    [[nodiscard]] bool fileIsWanted(tr_file_index_t file) const
    {
        return files_wanted_.fileWanted(file);
    }

    void initFilesWanted(tr_file_index_t const* files, size_t n_files, bool wanted)
    {
        setFilesWanted(files, n_files, wanted, /*is_bootstrapping*/ true);
    }

    void setFilesWanted(tr_file_index_t const* files, size_t n_files, bool wanted)
    {
        setFilesWanted(files, n_files, wanted, /*is_bootstrapping*/ false);
    }

    void recheckCompleteness(); // TODO(ckerr): should be private

    /// PRIORITIES

    [[nodiscard]] tr_priority_t piecePriority(tr_piece_index_t piece) const
    {
        return file_priorities_.piecePriority(piece);
    }

    void setFilePriorities(tr_file_index_t const* files, tr_file_index_t fileCount, tr_priority_t priority)
    {
        file_priorities_.set(files, fileCount, priority);
        setDirty();
    }

    void setFilePriority(tr_file_index_t file, tr_priority_t priority)
    {
        file_priorities_.set(file, priority);
        setDirty();
    }

    /// LOCATION

    [[nodiscard]] constexpr tr_interned_string currentDir() const noexcept
    {
        return this->current_dir;
    }

    [[nodiscard]] constexpr tr_interned_string downloadDir() const noexcept
    {
        return this->download_dir;
    }

    [[nodiscard]] constexpr tr_interned_string incompleteDir() const noexcept
    {
        return this->incomplete_dir;
    }

    /// METAINFO - FILES

    [[nodiscard]] tr_file_index_t fileCount() const noexcept
    {
        return metainfo_.fileCount();
    }

    [[nodiscard]] std::string const& fileSubpath(tr_file_index_t i) const
    {
        return metainfo_.fileSubpath(i);
    }

    [[nodiscard]] auto fileSize(tr_file_index_t i) const
    {
        return metainfo_.fileSize(i);
    }

    void setFileSubpath(tr_file_index_t i, std::string_view subpath)
    {
        metainfo_.setFileSubpath(i, subpath);
    }

    [[nodiscard]] auto findFile(tr_file_index_t file_index) const
    {
        auto n_paths = size_t{ 0U };
        auto paths = std::array<std::string_view, 2>{};

        if (auto const path = downloadDir(); !std::empty(path))
        {
            paths[n_paths++] = path.sv();
        }

        if (auto const path = incompleteDir(); !std::empty(path))
        {
            paths[n_paths++] = path.sv();
        }

        return metainfo_.files().find(std::data(paths), n_paths, file_index);
    }

    /// METAINFO - TRACKERS

    [[nodiscard]] auto const& announceList() const noexcept
    {
        return metainfo_.announceList();
    }

    [[nodiscard]] auto& announceList() noexcept
    {
        return metainfo_.announceList();
    }

    [[nodiscard]] auto trackerCount() const noexcept
    {
        return std::size(this->announceList());
    }

    [[nodiscard]] auto const& tracker(size_t i) const
    {
        return this->announceList().at(i);
    }

    [[nodiscard]] auto tiers() const
    {
        return this->announceList().tiers();
    }

    [[nodiscard]] auto trackerList() const
    {
        return this->announceList().toString();
    }

    bool setTrackerList(std::string_view text);

    /// METAINFO - WEBSEEDS

    [[nodiscard]] auto webseedCount() const noexcept
    {
        return metainfo_.webseedCount();
    }

    [[nodiscard]] auto const& webseed(size_t i) const
    {
        return metainfo_.webseed(i);
    }

    /// METAINFO - OTHER

    void setName(std::string_view name)
    {
        metainfo_.setName(name);
    }

    [[nodiscard]] auto const& name() const noexcept
    {
        return metainfo_.name();
    }

    [[nodiscard]] auto const& infoHash() const noexcept
    {
        return metainfo_.infoHash();
    }

    [[nodiscard]] auto isPrivate() const noexcept
    {
        return metainfo_.isPrivate();
    }

    [[nodiscard]] auto isPublic() const noexcept
    {
        return !this->isPrivate();
    }

    [[nodiscard]] auto const& infoHashString() const noexcept
    {
        return metainfo_.infoHashString();
    }

    [[nodiscard]] auto dateCreated() const noexcept
    {
        return metainfo_.dateCreated();
    }

    [[nodiscard]] auto torrentFile() const
    {
        return metainfo_.torrentFile(this->session->torrent_dir);
    }

    [[nodiscard]] auto magnetFile() const
    {
        return metainfo_.magnetFile(this->session->torrent_dir);
    }

    [[nodiscard]] auto resumeFile() const
    {
        return metainfo_.resumeFile(this->session->resume_dir);
    }

    [[nodiscard]] auto magnet() const
    {
        return metainfo_.magnet();
    }

    [[nodiscard]] auto const& comment() const noexcept
    {
        return metainfo_.comment();
    }

    [[nodiscard]] auto const& creator() const noexcept
    {
        return metainfo_.creator();
    }

    [[nodiscard]] auto const& source() const noexcept
    {
        return metainfo_.source();
    }

    [[nodiscard]] auto infoDictSize() const noexcept
    {
        return metainfo_.infoDictSize();
    }

    [[nodiscard]] auto infoDictOffset() const noexcept
    {
        return metainfo_.infoDictOffset();
    }

    /// METAINFO - PIECE CHECKSUMS

    [[nodiscard]] bool isPieceChecked(tr_piece_index_t piece) const
    {
        return checked_pieces_.test(piece);
    }

    [[nodiscard]] bool checkPiece(tr_piece_index_t piece);

    [[nodiscard]] bool ensurePieceIsChecked(tr_piece_index_t piece);

    void initCheckedPieces(tr_bitfield const& checked, time_t const* mtimes /*fileCount()*/);

    ///

    [[nodiscard]] constexpr auto isQueued() const noexcept
    {
        return this->is_queued;
    }

    [[nodiscard]] constexpr auto queueDirection() const noexcept
    {
        return this->isDone() ? TR_UP : TR_DOWN;
    }

    [[nodiscard]] auto allowsPex() const noexcept
    {
        return this->isPublic() && this->session->isPexEnabled;
    }

    [[nodiscard]] auto allowsDht() const
    {
        return this->isPublic() && tr_sessionAllowsDHT(this->session);
    }

    [[nodiscard]] auto allowsLpd() const // local peer discovery
    {
        return this->isPublic() && tr_sessionAllowsLPD(this->session);
    }

    [[nodiscard]] bool isPieceTransferAllowed(tr_direction direction) const;

    [[nodiscard]] bool clientCanDownload() const
    {
        return this->isPieceTransferAllowed(TR_PEER_TO_CLIENT);
    }

    [[nodiscard]] bool clientCanUpload() const
    {
        return this->isPieceTransferAllowed(TR_CLIENT_TO_PEER);
    }

    void setLocalError(std::string_view errmsg)
    {
        this->error = TR_STAT_LOCAL_ERROR;
        this->error_announce_url = TR_KEY_NONE;
        this->error_string = errmsg;
    }

    void setVerifyState(tr_verify_state state);

    void setDateActive(time_t t);

    /** Return the mime-type (e.g. "audio/x-flac") that matches more of the
        torrent's content than any other mime-type. */
    [[nodiscard]] std::string_view primaryMimeType() const;

    static constexpr std::string_view PartialFileSuffix = std::string_view{ ".part" };

    tr_torrent_metainfo metainfo_;

    // TODO(ckerr): make private once some of torrent.cc's `tr_torrentFoo()` methods are member functions
    tr_completion completion;

    tr_session* session = nullptr;

    tr_torrent_announcer* torrent_announcer = nullptr;

    Bandwidth bandwidth_;

    tr_swarm* swarm = nullptr;

    // TODO: is this actually still needed?
    int const magicNumber = MagicNumber;

    std::optional<double> verify_progress;

    tr_stat_errtype error = TR_STAT_OK;
    tr_interned_string error_announce_url;
    std::string error_string;

    tr_sha1_digest_t obfuscated_hash = {};

    /* Used when the torrent has been created with a magnet link
     * and we're in the process of downloading the metainfo from
     * other peers */
    struct tr_incomplete_metadata* incompleteMetadata = nullptr;

    /* If the initiator of the connection receives a handshake in which the
     * peer_id does not match the expected peerid, then the initiator is
     * expected to drop the connection. Note that the initiator presumably
     * received the peer information from the tracker, which includes the
     * peer_id that was registered by the peer. The peer_id from the tracker
     * and in the handshake are expected to match.
     */
    std::optional<tr_peer_id_t> peer_id;

    time_t peer_id_creation_time = 0;

    // Where the files are when the torrent is complete.
    tr_interned_string download_dir;

    // Where the files are when the torrent is incomplete.
    // a value of TR_KEY_NONE indicates the 'incomplete_dir' feature is unused
    tr_interned_string incomplete_dir;

    // Where the files are now.
    // Will equal either download_dir or incomplete_dir
    tr_interned_string current_dir;

    tr_completeness completeness = TR_LEECH;

    time_t dhtAnnounceAt = 0;
    time_t dhtAnnounce6At = 0;
    bool dhtAnnounceInProgress = false;
    bool dhtAnnounce6InProgress = false;

    time_t lpdAnnounceAt = 0;

    uint64_t downloadedCur = 0;
    uint64_t downloadedPrev = 0;
    uint64_t uploadedCur = 0;
    uint64_t uploadedPrev = 0;
    uint64_t corruptCur = 0;
    uint64_t corruptPrev = 0;

    uint64_t etaDLSpeedCalculatedAt = 0;
    uint64_t etaULSpeedCalculatedAt = 0;
    unsigned int etaDLSpeed_Bps = 0;
    unsigned int etaULSpeed_Bps = 0;

    time_t activityDate = 0;
    time_t addedDate = 0;
    time_t anyDate = 0;
    time_t doneDate = 0;
    time_t editDate = 0;
    time_t startDate = 0;

    int secondsDownloading = 0;
    int secondsSeeding = 0;

    int queuePosition = 0;

    tr_torrent_metadata_func metadata_func = nullptr;
    void* metadata_func_user_data = nullptr;

    tr_torrent_completeness_func completeness_func = nullptr;
    void* completeness_func_user_data = nullptr;

    tr_torrent_ratio_limit_hit_func ratio_limit_hit_func = nullptr;
    void* ratio_limit_hit_func_user_data = nullptr;

    tr_torrent_idle_limit_hit_func idle_limit_hit_func = nullptr;
    void* idle_limit_hit_func_user_data = nullptr;

    void* queue_started_user_data = nullptr;
    void (*queue_started_callback)(tr_torrent*, void* queue_started_user_data) = nullptr;

    bool isDeleting = false;
    bool isDirty = false;
    bool is_queued = false;
    bool isRunning = false;
    bool isStopping = false;
    bool startAfterVerify = false;

    bool prefetchMagnetMetadata = false;
    bool magnetVerify = false;

    void setDirty()
    {
        this->isDirty = true;
    }

    void markEdited();
    void markChanged();

    uint16_t maxConnectedPeers = TR_DEFAULT_PEER_LIMIT_TORRENT;

    tr_verify_state verifyState = TR_VERIFY_NONE;

    time_t lastStatTime = 0;
    tr_stat stats = {};

    int uniqueId = 0;

    float desiredRatio = 0.0F;
    tr_ratiolimit ratioLimitMode = TR_RATIOLIMIT_GLOBAL;

    uint16_t idleLimitMinutes = 0;
    tr_idlelimit idleLimitMode = TR_IDLELIMIT_GLOBAL;
    bool finishedSeedingByIdle = false;

    tr_labels_t labels;

    std::string group;
    /* Set the bandwidth group the torrent belongs to */
    void setGroup(std::string_view groupName);

    static auto constexpr MagicNumber = int{ 95549 };

    tr_file_piece_map fpm_ = tr_file_piece_map{ metainfo_ };
    tr_file_priorities file_priorities_{ &fpm_ };
    tr_files_wanted files_wanted_{ &fpm_ };

    // when Transmission thinks the torrent's files were last changed
    std::vector<time_t> file_mtimes_;

    // true iff the piece was verified more recently than any of the piece's
    // files' mtimes (file_mtimes_). If checked_pieces_.test(piece) is false,
    // it means that piece needs to be checked before its data is used.
    tr_bitfield checked_pieces_ = tr_bitfield{ 0 };

private:
    void setFilesWanted(tr_file_index_t const* files, size_t n_files, bool wanted, bool is_bootstrapping)
    {
        auto const lock = unique_lock();

        files_wanted_.set(files, n_files, wanted);
        completion.invalidateSizeWhenDone();

        if (!is_bootstrapping)
        {
            setDirty();
            recheckCompleteness();
        }
    }
};

/***
****
***/

constexpr bool tr_isTorrent(tr_torrent const* tor)
{
    return tor != nullptr && tor->magicNumber == tr_torrent::MagicNumber && tr_isSession(tor->session);
}

/**
 * Tell the tr_torrent that it's gotten a block
 */
void tr_torrentGotBlock(tr_torrent* tor, tr_block_index_t blockIndex);

tr_peer_id_t const& tr_torrentGetPeerId(tr_torrent* tor);

tr_torrent_metainfo tr_ctorStealMetainfo(tr_ctor* ctor);

bool tr_ctorSetMetainfoFromFile(tr_ctor* ctor, std::string const& filename, tr_error** error);
bool tr_ctorSetMetainfoFromMagnetLink(tr_ctor* ctor, std::string const& filename, tr_error** error);
void tr_ctorSetLabels(tr_ctor* ctor, tr_labels_t&& labels);

#define tr_logAddCriticalTor(tor, msg) tr_logAddCritical(msg, (tor)->name())
#define tr_logAddErrorTor(tor, msg) tr_logAddError(msg, (tor)->name())
#define tr_logAddWarnTor(tor, msg) tr_logAddWarn(msg, (tor)->name())
#define tr_logAddInfoTor(tor, msg) tr_logAddInfo(msg, (tor)->name())
#define tr_logAddDebugTor(tor, msg) tr_logAddDebug(msg, (tor)->name())
#define tr_logAddTraceTor(tor, msg) tr_logAddTrace(msg, (tor)->name())
