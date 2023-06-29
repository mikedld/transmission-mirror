// This file Copyright (C) 2013-2022 Mnemosyne LLC.
// It may be used under GPLv2 (SPDX: GPL-2.0-only), GPLv3 (SPDX: GPL-3.0-only),
// or any future license endorsed by Mnemosyne LLC.
// License text can be found in the licenses/ folder.

#include <algorithm>
#include <array>
#include <cstdlib> // mktemp()
#include <numeric>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <fmt/core.h>

#include <libtransmission/transmission.h>

#include <libtransmission/crypto-utils.h>
#include <libtransmission/file.h>
#include <libtransmission/makemeta.h>
#include <libtransmission/session.h> // TR_NAME
#include <libtransmission/torrent-metainfo.h>

#include "test-fixtures.h"

using namespace std::literals;

namespace libtransmission::test
{

class MakemetaTest : public SandboxedTest
{
protected:
    static auto constexpr DefaultMaxFileCount = size_t{ 16 };
    static auto constexpr DefaultMaxFileSize = size_t{ 1024 };

    auto makeRandomFiles(
        std::string_view top,
        size_t n_files = std::max(size_t{ 1U }, static_cast<size_t>(tr_rand_int(DefaultMaxFileCount))),
        size_t max_size = DefaultMaxFileSize)
    {
        auto files = std::vector<std::pair<std::string, std::vector<std::byte>>>{};

        for (size_t i = 0; i < n_files; ++i)
        {
            auto payload = std::vector<std::byte>{};
            // TODO(5.0.0): zero-sized files are disabled in these test
            // because tr_torrent_metainfo discards them, throwing off the
            // builder-to-metainfo comparisons here. tr_torrent_metainfo
            // will behave when BEP52 support is added in Transmission 5.
            static auto constexpr MinFileSize = size_t{ 1U };
            payload.resize(std::max(MinFileSize, static_cast<size_t>(tr_rand_int(max_size))));
            tr_rand_buffer(std::data(payload), std::size(payload));

            auto filename = tr_pathbuf{ top, '/', "test.XXXXXX" };
            createTmpfileWithContents(std::data(filename), std::data(payload), std::size(payload));
            tr_sys_path_native_separators(std::data(filename));

            files.emplace_back(std::string{ filename.sv() }, payload);
        }

        return files;
    }

    static auto testBuilder(tr_metainfo_builder& builder)
    {
        tr_error* error = builder.make_checksums().get();
        EXPECT_EQ(error, nullptr) << *error;

        auto metainfo = tr_torrent_metainfo{};
        EXPECT_TRUE(metainfo.parse_benc(builder.benc()));
        EXPECT_EQ(builder.file_count(), metainfo.file_count());
        EXPECT_EQ(builder.piece_size(), metainfo.piece_size());
        EXPECT_EQ(builder.total_size(), metainfo.total_size());
        EXPECT_EQ(builder.total_size(), metainfo.total_size());
        for (size_t i = 0, n = std::min(builder.file_count(), metainfo.file_count()); i < n; ++i)
        {
            EXPECT_EQ(builder.file_size(i), metainfo.files().fileSize(i));
            EXPECT_EQ(builder.path(i), metainfo.files().path(i));
        }
        EXPECT_EQ(builder.name(), metainfo.name());
        EXPECT_EQ(builder.comment(), metainfo.comment());
        EXPECT_EQ(builder.is_private(), metainfo.is_private());
        EXPECT_EQ(builder.announce_list().to_string(), metainfo.announce_list().to_string());
        return metainfo;
    }
};

TEST_F(MakemetaTest, comment)
{
    auto const files = makeRandomFiles(sandboxDir(), 1);
    auto const [filename, payload] = files.front();
    auto builder = tr_metainfo_builder{ filename };

    static auto constexpr Comment = "This is the comment"sv;
    builder.set_comment(Comment);

    EXPECT_EQ(Comment, testBuilder(builder).comment());
}

TEST_F(MakemetaTest, source)
{
    auto const files = makeRandomFiles(sandboxDir(), 1);
    auto const [filename, payload] = files.front();
    auto builder = tr_metainfo_builder{ filename };

    static auto constexpr Source = "This is the source"sv;
    builder.set_source(Source);

    EXPECT_EQ(Source, testBuilder(builder).source());
}

TEST_F(MakemetaTest, isPrivate)
{
    auto const files = makeRandomFiles(sandboxDir(), 1);
    auto const [filename, payload] = files.front();

    for (bool const is_private : { true, false })
    {
        auto builder = tr_metainfo_builder{ filename };
        builder.set_private(is_private);
        EXPECT_EQ(is_private, testBuilder(builder).is_private());
    }
}

TEST_F(MakemetaTest, pieceSize)
{
    auto const files = makeRandomFiles(sandboxDir(), 1);
    auto const [filename, payload] = files.front();

    for (uint32_t const piece_size : { 16384, 32768 })
    {
        auto builder = tr_metainfo_builder{ filename };
        builder.set_piece_size(piece_size);
        EXPECT_EQ(piece_size, testBuilder(builder).piece_size());
    }
}

TEST_F(MakemetaTest, webseeds)
{
    auto const files = makeRandomFiles(sandboxDir(), 1);
    auto const [filename, payload] = files.front();
    auto builder = tr_metainfo_builder{ filename };

    static auto constexpr Webseed = "https://www.example.com/linux.iso"sv;
    builder.set_webseeds(std::vector<std::string>{ std::string{ Webseed } });

    auto const metainfo = testBuilder(builder);
    EXPECT_EQ(1U, metainfo.webseed_count());
    EXPECT_EQ(Webseed, metainfo.webseed(0));
}

TEST_F(MakemetaTest, nameIsRootSingleFile)
{
    auto const files = makeRandomFiles(sandboxDir(), 1);
    auto const [filename, payload] = files.front();
    auto builder = tr_metainfo_builder{ filename };
    EXPECT_EQ(tr_sys_path_basename(filename), testBuilder(builder).name());
}

TEST_F(MakemetaTest, anonymizeTrue)
{
    auto const files = makeRandomFiles(sandboxDir(), 1);
    auto const [filename, payload] = files.front();

    auto builder = tr_metainfo_builder{ filename };
    builder.set_anonymize(true);
    auto const metainfo = testBuilder(builder);
    EXPECT_EQ(""sv, metainfo.creator());
    EXPECT_EQ(time_t{}, metainfo.date_created());
}

TEST_F(MakemetaTest, anonymizeFalse)
{
    auto const files = makeRandomFiles(sandboxDir(), 1);
    auto const [filename, payload] = files.front();

    auto builder = tr_metainfo_builder{ filename };
    builder.set_anonymize(false);
    auto const metainfo = testBuilder(builder);
    EXPECT_TRUE(tr_strv_contains(metainfo.creator(), TR_NAME)) << metainfo.creator();
    auto const now = time(nullptr);
    EXPECT_LE(metainfo.date_created(), now);
    EXPECT_LE(now - 60, metainfo.date_created());
}

TEST_F(MakemetaTest, nameIsRootMultifile)
{
    auto const files = makeRandomFiles(sandboxDir(), 10);
    auto const [filename, payload] = files.front();
    auto builder = tr_metainfo_builder{ filename };
    EXPECT_EQ(tr_sys_path_basename(filename), testBuilder(builder).name());
}

TEST_F(MakemetaTest, singleFile)
{
    auto const files = makeRandomFiles(sandboxDir(), 1);
    auto const [filename, payload] = files.front();
    auto builder = tr_metainfo_builder{ filename };

    auto trackers = tr_announce_list{};
    trackers.add("udp://tracker.openbittorrent.com:80"sv, trackers.nextTier());
    trackers.add("udp://tracker.publicbt.com:80"sv, trackers.nextTier());
    builder.set_announce_list(std::move(trackers));

    static auto constexpr Comment = "This is the comment"sv;
    builder.set_comment(Comment);

    static auto constexpr IsPrivate = false;
    builder.set_private(IsPrivate);

    static auto constexpr Anonymize = false;
    builder.set_anonymize(Anonymize);

    testBuilder(builder);
}

TEST_F(MakemetaTest, announceSingleTracker)
{
    auto const files = makeRandomFiles(sandboxDir(), 1);
    auto const [filename, payload] = files.front();
    auto builder = tr_metainfo_builder{ filename };

    // add a tracker
    static auto constexpr SingleAnnounce = "udp://tracker.openbittorrent.com:80"sv;
    auto trackers = tr_announce_list{};
    trackers.add(SingleAnnounce, trackers.nextTier());
    builder.set_announce_list(std::move(trackers));

    // generate the torrent and parse it as a variant
    EXPECT_EQ(nullptr, builder.make_checksums().get());
    auto top = tr_variant{};
    auto const benc = builder.benc();
    EXPECT_TRUE(tr_variantFromBuf(&top, TR_VARIANT_PARSE_BENC | TR_VARIANT_PARSE_INPLACE, benc));

    // confirm there's an "announce" entry
    auto single_announce = std::string_view{};
    EXPECT_TRUE(tr_variantDictFindStrView(&top, TR_KEY_announce, &single_announce));
    EXPECT_EQ(SingleAnnounce, single_announce);

    // confirm there's not an "announce-list" entry
    EXPECT_EQ(nullptr, tr_variantDictFind(&top, TR_KEY_announce_list));

    tr_variantClear(&top);
}

TEST_F(MakemetaTest, announceMultiTracker)
{
    auto const files = makeRandomFiles(sandboxDir(), 1);
    auto const [filename, payload] = files.front();
    auto builder = tr_metainfo_builder{ filename };

    // add the trackers
    auto trackers = tr_announce_list{};
    for (auto const& url : { "udp://tracker.openbittorrent.com:80"sv, "udp://tracker.publicbt.com:80"sv })
    {
        trackers.add(url, trackers.nextTier());
    }
    builder.set_announce_list(std::move(trackers));

    // generate the torrent and parse it as a variant
    EXPECT_EQ(nullptr, builder.make_checksums().get());
    auto top = tr_variant{};
    auto const benc = builder.benc();
    EXPECT_TRUE(tr_variantFromBuf(&top, TR_VARIANT_PARSE_BENC | TR_VARIANT_PARSE_INPLACE, benc));

    // confirm there's an "announce" entry
    auto single_announce = std::string_view{};
    EXPECT_TRUE(tr_variantDictFindStrView(&top, TR_KEY_announce, &single_announce));
    EXPECT_EQ(builder.announce_list().at(0).announce.sv(), single_announce);

    // confirm there's an "announce-list" entry
    tr_variant* announce_list_variant = nullptr;
    EXPECT_TRUE(tr_variantDictFindList(&top, TR_KEY_announce_list, &announce_list_variant));
    EXPECT_NE(nullptr, announce_list_variant);
    EXPECT_EQ(std::size(builder.announce_list()), tr_variantListSize(announce_list_variant));

    tr_variantClear(&top);
}

TEST_F(MakemetaTest, privateAndSourceHasDifferentInfoHash)
{
    auto const files = makeRandomFiles(sandboxDir(), 1);
    auto const [filename, payload] = files.front();
    auto builder = tr_metainfo_builder{ filename };
    auto trackers = tr_announce_list{};
    trackers.add("udp://tracker.openbittorrent.com:80"sv, trackers.nextTier());
    builder.set_announce_list(std::move(trackers));
    auto base_metainfo = testBuilder(builder);

    builder.set_private(true);
    auto private_metainfo = testBuilder(builder);
    EXPECT_NE(base_metainfo.info_hash(), private_metainfo.info_hash());

    builder.set_source("FOO");
    auto private_source_metainfo = testBuilder(builder);
    EXPECT_NE(base_metainfo.info_hash(), private_source_metainfo.info_hash());
    EXPECT_NE(private_metainfo.info_hash(), private_source_metainfo.info_hash());
}

} // namespace libtransmission::test
