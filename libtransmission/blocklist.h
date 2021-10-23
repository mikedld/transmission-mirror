// This file Copyright © 2008-2022 Mnemosyne LLC.
// It may be used under GPLv2 (SPDX: GPL-2.0), GPLv3 (SPDX: GPL-3.0),
// or any future license endorsed by Mnemosyne LLC.
// License text can be found in the licenses/ folder.

#pragma once

#ifndef __TRANSMISSION__
#error only libtransmission should #include this header.
#endif

#include <string>
#include <vector>

#include "file.h" // for tr_sys_file_t
#include "tr-assert.h"
#include "tr-macros.h"

struct tr_address;

struct BlocklistFile
{
public:
    BlocklistFile(char const* filename, bool isEnabled)
        : is_enabled_(isEnabled)
        , fd_{ TR_BAD_SYS_FILE }
        , filename_(filename)
    {
    }

    ~BlocklistFile();

    void close();

    [[nodiscard]] bool exists() const;

    [[nodiscard]] char const* getFilename() const;

    // TODO: This function should be const, but cannot be const due to it calling ensureLoaded()
    size_t getRuleCount();

    [[nodiscard]] constexpr bool isEnabled() const
    {
        return is_enabled_;
    }

    void setEnabled(bool isEnabled);

    bool hasAddress(tr_address const& addr);

    /// @brief Read the file of ranges, sort and merge, write to our own file, and reload from it
    size_t setContent(char const* filename);

private:
    struct IPv4Range
    {
        uint32_t begin_;
        uint32_t end_;

        /// @brief Used for std::bsearch of an IPv4 address
        static int compareAddressToRange(void const* va, void const* vb)
        {
            auto const* a = reinterpret_cast<uint32_t const*>(va);
            auto const* b = reinterpret_cast<IPv4Range const*>(vb);

            if (*a < b->begin_)
            {
                return -1;
            }

            if (*a > b->end_)
            {
                return 1;
            }

            return 0;
        }
    };

    void ensureLoaded();
    void load();
    static bool parseLine(char const* line, IPv4Range* range);
    static bool compareAddressRangesByFirstAddress(IPv4Range const& a, IPv4Range const& b);

    static bool parseLine1(std::string_view line, struct IPv4Range* range);
    static bool parseLine2(std::string_view line, struct IPv4Range* range);
    static bool parseLine3(char const* line, IPv4Range* range);

#ifdef TR_ENABLE_ASSERTS
    /// @brief Sanity checks: make sure the rules are sorted in ascending order and don't overlap
    void assertValidRules(std::vector<IPv4Range>& ranges);
#endif

    bool is_enabled_;
    tr_sys_file_t fd_;
    size_t rule_count_ = 0;
    uint64_t byte_count_ = 0;
    std::string const filename_;

    /// @brief Not a container, memory mapped file
    IPv4Range* rules_ = nullptr;
};
