// This file Copyright (C) 2013-2022 Mnemosyne LLC.
// It may be used under GPLv2 (SPDX: GPL-2.0), GPLv3 (SPDX: GPL-3.0),
// or any future license endorsed by Mnemosyne LLC.
// License text can be found in the licenses/ folder.

#include "transmission.h"

#include "crypto-utils.h"
#include "file.h"
#include "resume.h"
#include "torrent.h" // tr_isTorrent()
#include "tr-assert.h"
#include "tr-strbuf.h"
#include "utils.h"
#include "variant.h"

#include "test-fixtures.h"

#include <array>
#include <cerrno>
#include <cstdio> // fopen()
#include <cstring> // strcmp()
#include <string>
#include <string_view>

using namespace std::literals;

namespace libtransmission
{

namespace test
{

class RenameTest : public SessionTest
{
    static auto constexpr MaxWaitMsec = 3000;

protected:
    void torrentRemoveAndWait(tr_torrent* tor, int expected_torrent_count)
    {
        tr_torrentRemove(tor, false, nullptr);
        auto const test = [this, expected_torrent_count]()
        {
            return tr_sessionCountTorrents(session_) == expected_torrent_count;
        };
        EXPECT_TRUE(waitFor(test, MaxWaitMsec));
    }

    void createSingleFileTorrentContents(std::string_view top)
    {
        auto const path = tr_pathbuf{ top, "/hello-world.txt" };
        createFileWithContents(path, "hello, world!\n");
    }

    void createMultifileTorrentContents(std::string_view top)
    {
        auto path = tr_pathbuf{ top, "/Felidae/Felinae/Acinonyx/Cheetah/Chester"sv };
        createFileWithContents(path, "It ain't easy bein' cheesy.\n");

        path.assign(top, "/Felidae/Pantherinae/Panthera/Tiger/Tony"sv);
        createFileWithContents(path, "They’re Grrrrreat!\n");

        path.assign(top, "/Felidae/Felinae/Felis/catus/Kyphi"sv);
        createFileWithContents(path, "Inquisitive\n");

        path.assign(top, "/Felidae/Felinae/Felis/catus/Saffron"sv);
        createFileWithContents(path, "Tough\n");

        sync();
    }

    static tr_torrent* createTorrentFromBase64Metainfo(tr_ctor* ctor, char const* benc_base64)
    {
        // create the torrent ctor
        auto const benc = tr_base64_decode(benc_base64);
        EXPECT_LT(0U, std::size(benc));
        tr_error* error = nullptr;
        EXPECT_TRUE(tr_ctorSetMetainfo(ctor, std::data(benc), std::size(benc), &error));
        EXPECT_EQ(nullptr, error) << *error;
        tr_ctorSetPaused(ctor, TR_FORCE, true);

        // create the torrent
        auto* const tor = tr_torrentNew(ctor, nullptr);
        EXPECT_NE(nullptr, tor);

        // cleanup
        return tor;
    }

    static bool testFileExistsAndConsistsOfThisString(tr_torrent const* tor, tr_file_index_t file_index, std::string const& str)
    {
        auto const str_len = str.size();
        auto success = false;

        auto* path = tr_torrentFindFile(tor, file_index);
        if (path != nullptr)
        {
            EXPECT_TRUE(tr_sys_path_exists(path));

            size_t contents_len;
            uint8_t* contents = tr_loadFile(path, &contents_len, nullptr);

            success = contents != nullptr && str_len == contents_len && memcmp(contents, str.data(), contents_len) == 0;

            tr_free(contents);
            tr_free(path);
        }

        return success;
    }

    static void expectHaveNone(tr_torrent* tor, uint64_t total_size)
    {
        auto const* tst = tr_torrentStat(tor);
        EXPECT_EQ(TR_STATUS_STOPPED, tst->activity);
        EXPECT_EQ(TR_STAT_OK, tst->error);
        EXPECT_EQ(total_size, tst->sizeWhenDone);
        EXPECT_EQ(total_size, tst->leftUntilDone);
        EXPECT_EQ(total_size, tor->totalSize());
        EXPECT_EQ(0, tst->haveValid);
    }

    static int torrentRenameAndWait(tr_torrent* tor, char const* oldpath, char const* newname)
    {
        auto const on_rename_done =
            [](tr_torrent* /*tor*/, char const* /*oldpath*/, char const* /*newname*/, int error, void* user_data) noexcept
        {
            *static_cast<int*>(user_data) = error;
        };

        int error = -1;
        tr_torrentRenamePath(tor, oldpath, newname, on_rename_done, &error);
        auto test = [&error]()
        {
            return error != -1;
        };
        EXPECT_TRUE(waitFor(test, MaxWaitMsec));
        return error;
    }
};

TEST_F(RenameTest, singleFilenameTorrent)
{
    static auto constexpr TotalSize = size_t{ 14 };

    // this is a single-file torrent whose file is hello-world.txt, holding the string "hello, world!"
    auto* ctor = tr_ctorNew(session_);
    auto* tor = createTorrentFromBase64Metainfo(
        ctor,
        "ZDEwOmNyZWF0ZWQgYnkyNTpUcmFuc21pc3Npb24vMi42MSAoMTM0MDcpMTM6Y3JlYXRpb24gZGF0"
        "ZWkxMzU4NTQ5MDk4ZTg6ZW5jb2Rpbmc1OlVURi04NDppbmZvZDY6bGVuZ3RoaTE0ZTQ6bmFtZTE1"
        "OmhlbGxvLXdvcmxkLnR4dDEyOnBpZWNlIGxlbmd0aGkzMjc2OGU2OnBpZWNlczIwOukboJcrkFUY"
        "f6LvqLXBVvSHqCk6Nzpwcml2YXRlaTBlZWU=");
    EXPECT_TRUE(tr_isTorrent(tor));

    // sanity check the info
    EXPECT_EQ(tr_file_index_t{ 1 }, tor->fileCount());
    EXPECT_STREQ("hello-world.txt", tr_torrentFile(tor, 0).name);

    // sanity check the (empty) stats
    blockingTorrentVerify(tor);
    expectHaveNone(tor, TotalSize);

    createSingleFileTorrentContents(tor->currentDir().sv());

    // sanity check the stats again, now that we've added the file
    blockingTorrentVerify(tor);
    auto const* st = tr_torrentStat(tor);
    EXPECT_EQ(TR_STATUS_STOPPED, st->activity);
    EXPECT_EQ(TR_STAT_OK, st->error);
    EXPECT_EQ(0, st->leftUntilDone);
    EXPECT_EQ(0, st->haveUnchecked);
    EXPECT_EQ(0, st->desiredAvailable);
    EXPECT_EQ(TotalSize, st->sizeWhenDone);
    EXPECT_EQ(TotalSize, st->haveValid);

    /**
    ***  okay! we've finally put together all the scaffolding to test
    ***  renaming a single-file torrent
    **/

    // confirm that bad inputs get caught

    EXPECT_EQ(EINVAL, torrentRenameAndWait(tor, "hello-world.txt", nullptr));
    EXPECT_EQ(EINVAL, torrentRenameAndWait(tor, "hello-world.txt", ""));
    EXPECT_EQ(EINVAL, torrentRenameAndWait(tor, "hello-world.txt", "."));
    EXPECT_EQ(EINVAL, torrentRenameAndWait(tor, "hello-world.txt", ".."));
    EXPECT_EQ(0, torrentRenameAndWait(tor, "hello-world.txt", "hello-world.txt"));
    EXPECT_EQ(EINVAL, torrentRenameAndWait(tor, "hello-world.txt", "hello/world.txt"));
    EXPECT_STREQ("hello-world.txt", tr_torrentFile(tor, 0).name);

    /***
    ****  Now try a rename that should succeed
    ***/

    auto tmpstr = tr_pathbuf{ tor->currentDir(), "/hello-world.txt" };
    EXPECT_TRUE(tr_sys_path_exists(tmpstr));
    EXPECT_STREQ("hello-world.txt", tr_torrentName(tor));
    EXPECT_EQ(0, torrentRenameAndWait(tor, tr_torrentName(tor), "foobar"));
    EXPECT_FALSE(tr_sys_path_exists(tmpstr)); // confirm the old filename can't be found
    EXPECT_STREQ("foobar", tr_torrentName(tor)); // confirm the torrent's name is now 'foobar'
    EXPECT_STREQ("foobar", tr_torrentFile(tor, 0).name); // confirm the file's name is now 'foobar'
    char* const torrent_filename = tr_torrentFilename(tor);
    EXPECT_STREQ(nullptr, strstr(torrent_filename, "foobar")); // confirm torrent file hasn't changed
    tr_free(torrent_filename);
    tmpstr.assign(tor->currentDir(), "/foobar");
    EXPECT_TRUE(tr_sys_path_exists(tmpstr)); // confirm the file's name is now 'foobar' on the disk
    EXPECT_TRUE(testFileExistsAndConsistsOfThisString(tor, 0, "hello, world!\n")); // confirm the contents are right

    // (while it's renamed: confirm that the .resume file remembers the changes)
    tr_resume::save(tor);
    sync();
    auto const loaded = tr_resume::load(tor, tr_resume::All, ctor, nullptr);
    EXPECT_STREQ("foobar", tr_torrentName(tor));
    EXPECT_NE(decltype(loaded){ 0 }, (loaded & tr_resume::Name));

    /***
    ****  ...and rename it back again
    ***/

    tmpstr.assign(tor->currentDir(), "/foobar");
    EXPECT_TRUE(tr_sys_path_exists(tmpstr));
    EXPECT_EQ(0, torrentRenameAndWait(tor, "foobar", "hello-world.txt"));
    EXPECT_FALSE(tr_sys_path_exists(tmpstr));
    EXPECT_STREQ("hello-world.txt", tr_torrentName(tor));
    EXPECT_STREQ("hello-world.txt", tr_torrentFile(tor, 0).name);
    EXPECT_TRUE(testFileExistsAndConsistsOfThisString(tor, 0, "hello, world!\n"));

    // cleanup
    tr_ctorFree(ctor);
    torrentRemoveAndWait(tor, 0);
}

/***
****
****
****
***/

TEST_F(RenameTest, multifileTorrent)
{
    char* str;
    auto constexpr TotalSize = size_t{ 67 };
    auto const expected_files = std::array<std::string, 4>{
        "Felidae/Felinae/Acinonyx/Cheetah/Chester",
        "Felidae/Felinae/Felis/catus/Kyphi",
        "Felidae/Felinae/Felis/catus/Saffron",
        "Felidae/Pantherinae/Panthera/Tiger/Tony",
    };
    auto const expected_contents = std::array<std::string, 4>{
        "It ain't easy bein' cheesy.\n",
        "Inquisitive\n",
        "Tough\n",
        "They’re Grrrrreat!\n",
    };

    auto* ctor = tr_ctorNew(session_);
    auto* tor = createTorrentFromBase64Metainfo(
        ctor,
        "ZDEwOmNyZWF0ZWQgYnkyNTpUcmFuc21pc3Npb24vMi42MSAoMTM0MDcpMTM6Y3JlYXRpb24gZGF0"
        "ZWkxMzU4NTU1NDIwZTg6ZW5jb2Rpbmc1OlVURi04NDppbmZvZDU6ZmlsZXNsZDY6bGVuZ3RoaTI4"
        "ZTQ6cGF0aGw3OkZlbGluYWU4OkFjaW5vbnl4NzpDaGVldGFoNzpDaGVzdGVyZWVkNjpsZW5ndGhp"
        "MTJlNDpwYXRobDc6RmVsaW5hZTU6RmVsaXM1OmNhdHVzNTpLeXBoaWVlZDY6bGVuZ3RoaTZlNDpw"
        "YXRobDc6RmVsaW5hZTU6RmVsaXM1OmNhdHVzNzpTYWZmcm9uZWVkNjpsZW5ndGhpMjFlNDpwYXRo"
        "bDExOlBhbnRoZXJpbmFlODpQYW50aGVyYTU6VGlnZXI0OlRvbnllZWU0Om5hbWU3OkZlbGlkYWUx"
        "MjpwaWVjZSBsZW5ndGhpMzI3NjhlNjpwaWVjZXMyMDp27buFkmy8ICfNX4nsJmt0Ckm2Ljc6cHJp"
        "dmF0ZWkwZWVl");
    EXPECT_TRUE(tr_isTorrent(tor));

    // sanity check the info
    EXPECT_STREQ("Felidae", tr_torrentName(tor));
    EXPECT_EQ(TotalSize, tor->totalSize());
    EXPECT_EQ(tr_file_index_t{ 4 }, tor->fileCount());

    for (tr_file_index_t i = 0; i < 4; ++i)
    {
        EXPECT_EQ(expected_files[i], tr_torrentFile(tor, i).name);
    }

    // sanity check the (empty) stats
    blockingTorrentVerify(tor);
    expectHaveNone(tor, TotalSize);

    // build the local data
    createMultifileTorrentContents(tor->currentDir().sv());

    // sanity check the (full) stats
    blockingTorrentVerify(tor);
    auto const* st = tr_torrentStat(tor);
    EXPECT_EQ(TR_STATUS_STOPPED, st->activity);
    EXPECT_EQ(TR_STAT_OK, st->error);
    EXPECT_EQ(0, st->leftUntilDone);
    EXPECT_EQ(0, st->haveUnchecked);
    EXPECT_EQ(0, st->desiredAvailable);
    EXPECT_EQ(TotalSize, st->sizeWhenDone);
    EXPECT_EQ(TotalSize, st->haveValid);

    /**
    ***  okay! let's test renaming.
    **/

    // rename a leaf...
    EXPECT_EQ(0, torrentRenameAndWait(tor, "Felidae/Felinae/Felis/catus/Kyphi", "placeholder"));
    EXPECT_STREQ("Felidae/Felinae/Felis/catus/placeholder", tr_torrentFile(tor, 1).name);
    EXPECT_TRUE(testFileExistsAndConsistsOfThisString(tor, 1, "Inquisitive\n"));

    // ...and back again
    EXPECT_EQ(0, torrentRenameAndWait(tor, "Felidae/Felinae/Felis/catus/placeholder", "Kyphi"));
    EXPECT_STREQ("Felidae/Felinae/Felis/catus/Kyphi", tr_torrentFile(tor, 1).name);
    testFileExistsAndConsistsOfThisString(tor, 1, "Inquisitive\n");

    // rename a branch...
    EXPECT_EQ(0, torrentRenameAndWait(tor, "Felidae/Felinae/Felis/catus", "placeholder"));
    EXPECT_EQ(expected_files[0], tr_torrentFile(tor, 0).name);
    EXPECT_STREQ("Felidae/Felinae/Felis/placeholder/Kyphi", tr_torrentFile(tor, 1).name);
    EXPECT_STREQ("Felidae/Felinae/Felis/placeholder/Saffron", tr_torrentFile(tor, 2).name);
    EXPECT_EQ(expected_files[3], tr_torrentFile(tor, 3).name);
    EXPECT_TRUE(testFileExistsAndConsistsOfThisString(tor, 1, expected_contents[1]));
    EXPECT_TRUE(testFileExistsAndConsistsOfThisString(tor, 2, expected_contents[2]));

    // (while the branch is renamed: confirm that the .resume file remembers the changes)
    tr_resume::save(tor);
    // this is a bit dodgy code-wise, but let's make sure the .resume file got the name
    tor->setFileSubpath(1, "gabba gabba hey"sv);
    auto const loaded = tr_resume::load(tor, tr_resume::All, ctor, nullptr);
    EXPECT_NE(decltype(loaded){ 0 }, (loaded & tr_resume::Filenames));
    EXPECT_EQ(expected_files[0], tr_torrentFile(tor, 0).name);
    EXPECT_STREQ("Felidae/Felinae/Felis/placeholder/Kyphi", tr_torrentFile(tor, 1).name);
    EXPECT_STREQ("Felidae/Felinae/Felis/placeholder/Saffron", tr_torrentFile(tor, 2).name);
    EXPECT_EQ(expected_files[3], tr_torrentFile(tor, 3).name);

    // ...and back again
    EXPECT_EQ(0, torrentRenameAndWait(tor, "Felidae/Felinae/Felis/placeholder", "catus"));

    for (tr_file_index_t i = 0; i < 4; ++i)
    {
        EXPECT_EQ(expected_files[i], tr_torrentFile(tor, i).name);
        EXPECT_TRUE(testFileExistsAndConsistsOfThisString(tor, i, expected_contents[i]));
    }

    /***
    ****  Test it an incomplete torrent...
    ***/

    // remove the directory Felidae/Felinae/Felis/catus
    str = tr_torrentFindFile(tor, 1);
    EXPECT_NE(nullptr, str);
    tr_sys_path_remove(str);
    tr_free(str);
    str = tr_torrentFindFile(tor, 2);
    EXPECT_NE(nullptr, str);
    tr_sys_path_remove(str);
    tr_sys_path_remove(std::string{ tr_sys_path_dirname(str) }.c_str());
    tr_free(str);
    sync();
    blockingTorrentVerify(tor);
    testFileExistsAndConsistsOfThisString(tor, 0, expected_contents[0]);

    for (tr_file_index_t i = 1; i <= 2; ++i)
    {
        str = tr_torrentFindFile(tor, i);
        EXPECT_STREQ(nullptr, str);
        tr_free(str);
    }

    testFileExistsAndConsistsOfThisString(tor, 3, expected_contents[3]);

    // rename a branch...
    EXPECT_EQ(0, torrentRenameAndWait(tor, "Felidae/Felinae/Felis/catus", "foo"));
    EXPECT_EQ(expected_files[0], tr_torrentFile(tor, 0).name);
    EXPECT_STREQ("Felidae/Felinae/Felis/foo/Kyphi", tr_torrentFile(tor, 1).name);
    EXPECT_STREQ("Felidae/Felinae/Felis/foo/Saffron", tr_torrentFile(tor, 2).name);
    EXPECT_EQ(expected_files[3], tr_torrentFile(tor, 3).name);

    // ...and back again
    EXPECT_EQ(0, torrentRenameAndWait(tor, "Felidae/Felinae/Felis/foo", "catus"));

    for (tr_file_index_t i = 0; i < 4; ++i)
    {
        EXPECT_EQ(expected_files[i], tr_torrentFile(tor, i).name);
    }

    EXPECT_EQ(0, torrentRenameAndWait(tor, "Felidae", "gabba"));
    auto strings = std::array<char const*, 4>{};
    strings[0] = "gabba/Felinae/Acinonyx/Cheetah/Chester";
    strings[1] = "gabba/Felinae/Felis/catus/Kyphi";
    strings[2] = "gabba/Felinae/Felis/catus/Saffron";
    strings[3] = "gabba/Pantherinae/Panthera/Tiger/Tony";

    for (tr_file_index_t i = 0; i < 4; ++i)
    {
        EXPECT_STREQ(strings[i], tr_torrentFile(tor, i).name);
        testFileExistsAndConsistsOfThisString(tor, i, expected_contents[i]);
    }

    // rename the root, then a branch, and then a leaf...
    EXPECT_EQ(0, torrentRenameAndWait(tor, "gabba", "Felidae"));
    EXPECT_EQ(0, torrentRenameAndWait(tor, "Felidae/Pantherinae/Panthera/Tiger", "Snow Leopard"));
    EXPECT_EQ(0, torrentRenameAndWait(tor, "Felidae/Pantherinae/Panthera/Snow Leopard/Tony", "10.6"));
    strings[0] = "Felidae/Felinae/Acinonyx/Cheetah/Chester";
    strings[1] = "Felidae/Felinae/Felis/catus/Kyphi";
    strings[2] = "Felidae/Felinae/Felis/catus/Saffron";
    strings[3] = "Felidae/Pantherinae/Panthera/Snow Leopard/10.6";

    for (tr_file_index_t i = 0; i < 4; ++i)
    {
        EXPECT_STREQ(strings[i], tr_torrentFile(tor, i).name);
        testFileExistsAndConsistsOfThisString(tor, i, expected_contents[i]);
    }

    tr_ctorFree(ctor);
    torrentRemoveAndWait(tor, 0);

    /**
    ***  Test renaming prefixes (shouldn't work)
    **/

    ctor = tr_ctorNew(session_);
    tor = createTorrentFromBase64Metainfo(
        ctor,
        "ZDEwOmNyZWF0ZWQgYnkyNTpUcmFuc21pc3Npb24vMi42MSAoMTM0MDcpMTM6Y3JlYXRpb24gZGF0"
        "ZWkxMzU4NTU1NDIwZTg6ZW5jb2Rpbmc1OlVURi04NDppbmZvZDU6ZmlsZXNsZDY6bGVuZ3RoaTI4"
        "ZTQ6cGF0aGw3OkZlbGluYWU4OkFjaW5vbnl4NzpDaGVldGFoNzpDaGVzdGVyZWVkNjpsZW5ndGhp"
        "MTJlNDpwYXRobDc6RmVsaW5hZTU6RmVsaXM1OmNhdHVzNTpLeXBoaWVlZDY6bGVuZ3RoaTZlNDpw"
        "YXRobDc6RmVsaW5hZTU6RmVsaXM1OmNhdHVzNzpTYWZmcm9uZWVkNjpsZW5ndGhpMjFlNDpwYXRo"
        "bDExOlBhbnRoZXJpbmFlODpQYW50aGVyYTU6VGlnZXI0OlRvbnllZWU0Om5hbWU3OkZlbGlkYWUx"
        "MjpwaWVjZSBsZW5ndGhpMzI3NjhlNjpwaWVjZXMyMDp27buFkmy8ICfNX4nsJmt0Ckm2Ljc6cHJp"
        "dmF0ZWkwZWVl");
    EXPECT_TRUE(tr_isTorrent(tor));

    // rename prefix of top
    EXPECT_EQ(EINVAL, torrentRenameAndWait(tor, "Feli", "FelidaeX"));
    EXPECT_STREQ("Felidae", tr_torrentName(tor));

    // rename false path
    EXPECT_EQ(EINVAL, torrentRenameAndWait(tor, "Felidae/FelinaeX", "Genus Felinae"));
    EXPECT_STREQ("Felidae", tr_torrentName(tor));

    /***
    ****
    ***/

    // cleanup
    tr_ctorFree(ctor);
    torrentRemoveAndWait(tor, 0);
}

/***
****
***/

TEST_F(RenameTest, partialFile)
{
    auto constexpr PieceCount = uint32_t{ 33 };
    auto constexpr PieceSize = uint32_t{ 32768 };
    auto constexpr Length = std::array<uint32_t, 3>{ 1048576, 4096, 512 };
    auto constexpr TotalSize = uint64_t(Length[0]) + Length[1] + Length[2];

    /***
    ****  create our test torrent with an incomplete .part file
    ***/

    auto* tor = zeroTorrentInit(ZeroTorrentState::Partial);
    EXPECT_EQ(TotalSize, tor->totalSize());
    EXPECT_EQ(PieceSize, tor->pieceSize());
    EXPECT_EQ(PieceCount, tor->pieceCount());
    EXPECT_EQ("files-filled-with-zeroes/1048576"sv, tor->fileSubpath(0));
    EXPECT_EQ("files-filled-with-zeroes/4096"sv, tor->fileSubpath(1));
    EXPECT_EQ("files-filled-with-zeroes/512"sv, tor->fileSubpath(2));
    EXPECT_NE(0, tr_torrentFile(tor, 0).have);
    EXPECT_EQ(Length[0], tr_torrentFile(tor, 0).have + PieceSize);
    EXPECT_EQ(Length[1], tr_torrentFile(tor, 1).have);
    EXPECT_EQ(Length[2], tr_torrentFile(tor, 2).have);
    auto const* st = tr_torrentStat(tor);
    EXPECT_EQ(TotalSize, st->sizeWhenDone);
    EXPECT_EQ(PieceSize, st->leftUntilDone);

    /***
    ****
    ***/

    EXPECT_EQ(0, torrentRenameAndWait(tor, "files-filled-with-zeroes", "foo"));
    EXPECT_EQ(0, torrentRenameAndWait(tor, "foo/1048576", "bar"));
    auto strings = std::array<std::string_view, 3>{};
    strings[0] = "foo/bar"sv;
    strings[1] = "foo/4096"sv;
    strings[2] = "foo/512"sv;

    for (tr_file_index_t i = 0; i < 3; ++i)
    {
        EXPECT_EQ(strings[i], tor->fileSubpath(i));
    }

    strings[0] = "foo/bar.part";

    for (tr_file_index_t i = 0; i < 3; ++i)
    {
        auto const expected = tr_pathbuf{ tor->currentDir(), '/', strings[i] };
        char* path = tr_torrentFindFile(tor, i);
        EXPECT_EQ(expected, path);
        tr_free(path);
    }

    torrentRemoveAndWait(tor, 0);
}

} // namespace test

} // namespace libtransmission
