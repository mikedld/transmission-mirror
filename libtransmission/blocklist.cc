// This file Copyright © 2008-2022 Mnemosyne LLC.
// It may be used under GPLv2 (SPDX: GPL-2.0-only), GPLv3 (SPDX: GPL-3.0-only),
// or any future license endorsed by Mnemosyne LLC.
// License text can be found in the licenses/ folder.

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib> // bsearch()
#include <fstream>
#include <string_view>
#include <vector>

#include <fmt/core.h>

#include "transmission.h"

#include "blocklist.h"
#include "error.h"
#include "file.h"
#include "log.h"
#include "net.h"
#include "utils.h"

/***
****  PRIVATE
***/

void BlocklistFile::close()
{
    if (rules_ != nullptr)
    {
        tr_sys_file_unmap(rules_, byte_count_);
        tr_sys_file_close(fd_);
        rules_ = nullptr;
        rule_count_ = 0;
        byte_count_ = 0;
        fd_ = TR_BAD_SYS_FILE;
    }
}

void BlocklistFile::load()
{
    close();

    auto const info = tr_sys_path_get_info(getFilename());
    if (!info)
    {
        return;
    }

    auto const byte_count = info->size;
    if (byte_count == 0)
    {
        return;
    }

    tr_error* error = nullptr;
    auto const fd = tr_sys_file_open(getFilename(), TR_SYS_FILE_READ, 0, &error);
    if (fd == TR_BAD_SYS_FILE)
    {
        tr_logAddWarn(fmt::format(
            _("Couldn't read '{path}': {error} ({error_code})"),
            fmt::arg("path", getFilename()),
            fmt::arg("error", error->message),
            fmt::arg("error_code", error->code)));
        tr_error_free(error);
        return;
    }

    rules_ = static_cast<struct IPv4Range*>(tr_sys_file_map_for_reading(fd, 0, byte_count, &error));
    if (rules_ == nullptr)
    {
        tr_logAddWarn(fmt::format(
            _("Couldn't read '{path}': {error} ({error_code})"),
            fmt::arg("path", getFilename()),
            fmt::arg("error", error->message),
            fmt::arg("error_code", error->code)));
        tr_sys_file_close(fd);
        tr_error_free(error);
        return;
    }

    fd_ = fd;
    byte_count_ = byte_count;
    rule_count_ = byte_count / sizeof(IPv4Range);

    tr_logAddInfo(fmt::format(
        ngettext("Blocklist '{path}' has {count} entry", "Blocklist '{path}' has {count} entries", rule_count_),
        fmt::arg("path", tr_sys_path_basename(getFilename())),
        fmt::arg("count", rule_count_)));
}

void BlocklistFile::ensureLoaded()
{
    if (rules_ == nullptr)
    {
        load();
    }
}

// TODO: unused
//static void blocklistDelete(tr_blocklistFile* b)
//{
//    blocklistClose(b);
//    tr_sys_path_remove(b->filename, nullptr);
//}

/***
****  PACKAGE-VISIBLE
***/

bool BlocklistFile::hasAddress(tr_address const& addr)
{
    TR_ASSERT(tr_address_is_valid(&addr));

    if (!is_enabled_ || !addr.isIPv4())
    {
        return false;
    }

    ensureLoaded();

    if (rules_ == nullptr || rule_count_ == 0)
    {
        return false;
    }

    auto const needle = ntohl(addr.addr.addr4.s_addr);

    // std::binary_search works differently and requires a less-than comparison
    // and two arguments of the same type. std::bsearch is the right choice.
    auto const* range = static_cast<IPv4Range const*>(
        std::bsearch(&needle, rules_, rule_count_, sizeof(IPv4Range), IPv4Range::compareAddressToRange));

    return range != nullptr;
}

/*
 * P2P plaintext format: "comment:x.x.x.x-y.y.y.y"
 * http://wiki.phoenixlabs.org/wiki/P2P_Format
 * https://en.wikipedia.org/wiki/PeerGuardian#P2P_plaintext_format
 */
bool BlocklistFile::parseLine1(std::string_view line, struct IPv4Range* range)
{
    // remove leading "comment:"
    auto pos = line.find(':');
    if (pos == std::string_view::npos)
    {
        return false;
    }
    line = line.substr(pos + 1);

    // parse the leading 'x.x.x.x'
    pos = line.find('-');
    if (pos == std::string_view::npos)
    {
        return false;
    }
    if (auto const addr = tr_address::fromString(line.substr(0, pos)); addr)
    {
        range->begin_ = ntohl(addr->addr.addr4.s_addr);
    }
    else
    {
        return false;
    }
    line = line.substr(pos + 1);

    // parse the trailing 'y.y.y.y'
    if (auto const addr = tr_address::fromString(line); addr)
    {
        range->end_ = ntohl(addr->addr.addr4.s_addr);
    }
    else
    {
        return false;
    }

    return true;
}

/*
 * DAT / eMule format: "000.000.000.000 - 000.255.255.255 , 000 , invalid ip"a
 * https://sourceforge.net/p/peerguardian/wiki/dev-blocklist-format-dat/
 */
bool BlocklistFile::parseLine2(std::string_view line, struct IPv4Range* range)
{
    static auto constexpr Delim1 = std::string_view{ " - " };
    static auto constexpr Delim2 = std::string_view{ " , " };

    auto pos = line.find(Delim1);
    if (pos == std::string_view::npos)
    {
        return false;
    }

    if (auto const addr = tr_address::fromString(line.substr(0, pos)); addr)
    {
        range->begin_ = ntohl(addr->addr.addr4.s_addr);
    }
    else
    {
        return false;
    }

    line = line.substr(pos + std::size(Delim1));
    pos = line.find(Delim2);
    if (pos == std::string_view::npos)
    {
        return false;
    }

    if (auto const addr = tr_address::fromString(line.substr(0, pos)); addr)
    {
        range->end_ = ntohl(addr->addr.addr4.s_addr);
    }
    else
    {
        return false;
    }

    return true;
}

/*
 * CIDR notation: "0.0.0.0/8", IPv4 only
 * https://en.wikipedia.org/wiki/Classless_Inter-Domain_Routing#CIDR_notation
 */
bool BlocklistFile::parseLine3(char const* line, IPv4Range* range)
{
    auto ip = std::array<unsigned int, 4>{};
    unsigned int pflen = 0;
    uint32_t ip_u = 0;
    uint32_t mask = 0xffffffff;

    // NOLINTNEXTLINE readability-container-data-pointer
    if (sscanf(line, "%u.%u.%u.%u/%u", TR_ARG_TUPLE(&ip[0], &ip[1], &ip[2], &ip[3]), &pflen) != 5)
    {
        return false;
    }

    if (pflen > 32 || ip[0] > 0xff || ip[1] > 0xff || ip[2] > 0xff || ip[3] > 0xff)
    {
        return false;
    }

    /* this is host order */
    mask <<= 32 - pflen;
    ip_u = ip[0] << 24 | ip[1] << 16 | ip[2] << 8 | ip[3];

    /* fill the non-prefix bits the way we need it */
    range->begin_ = ip_u & mask;
    range->end_ = ip_u | (~mask);

    return true;
}

bool BlocklistFile::parseLine(char const* line, IPv4Range* range)
{
    return parseLine1(line, range) || parseLine2(line, range) || parseLine3(line, range);
}

bool BlocklistFile::compareAddressRangesByFirstAddress(IPv4Range const& a, IPv4Range const& b)
{
    return a.begin_ < b.begin_;
}

size_t BlocklistFile::setContent(char const* filename)
{
    if (filename == nullptr)
    {
        return {};
    }

    auto in = std::ifstream{ filename };
    if (!in.is_open())
    {
        tr_logAddWarn(fmt::format(
            _("Couldn't read '{path}': {error} ({error_code})"),
            fmt::arg("path", filename),
            fmt::arg("error", tr_strerror(errno)),
            fmt::arg("error_code", errno)));
        return {};
    }

    auto line = std::string{};
    auto line_number = size_t{ 0U };
    auto ranges = std::vector<IPv4Range>{};
    while (std::getline(in, line))
    {
        ++line_number;
        auto range = IPv4Range{};
        if (!parseLine(std::data(line), &range))
        {
            /* don't try to display the actual lines - it causes issues */
            tr_logAddWarn(fmt::format(_("Couldn't parse line: '{line}'"), fmt::arg("line", line_number)));
            continue;
        }

        ranges.push_back(range);
    }
    in.close();

    if (std::empty(ranges))
    {
        return {};
    }

    close();

    size_t keep = 0; // index in ranges

    std::sort(std::begin(ranges), std::end(ranges), BlocklistFile::compareAddressRangesByFirstAddress);

    // merge
    for (auto const& r : ranges)
    {
        if (ranges[keep].end_ < r.begin_)
        {
            ranges[++keep] = r;
        }
        else if (ranges[keep].end_ < r.end_)
        {
            ranges[keep].end_ = r.end_;
        }
    }

    TR_ASSERT_MSG(keep + 1 <= std::size(ranges), "Can shrink `ranges` or leave intact, but not grow");
    ranges.resize(keep + 1);

#ifdef TR_ENABLE_ASSERTS
    assertValidRules(ranges);
#endif

    auto out = std::ofstream{ getFilename(), std::ios_base::out | std::ios_base::trunc };
    if (!out.is_open())
    {
        tr_logAddWarn(fmt::format(
            _("Couldn't read '{path}': {error} ({error_code})"),
            fmt::arg("path", getFilename()),
            fmt::arg("error", tr_strerror(errno)),
            fmt::arg("error_code", errno)));
        return {};
    }

    if (!out.write(reinterpret_cast<char const*>(ranges.data()), std::size(ranges) * sizeof(IPv4Range)))
    {
        tr_logAddWarn(fmt::format(
            _("Couldn't save '{path}': {error} ({error_code})"),
            fmt::arg("path", getFilename()),
            fmt::arg("error", tr_strerror(errno)),
            fmt::arg("error_code", errno)));
    }
    else
    {
        tr_logAddInfo(fmt::format(
            ngettext("Blocklist '{path}' has {count} entry", "Blocklist '{path}' has {count} entries", rule_count_),
            fmt::arg("path", tr_sys_path_basename(getFilename())),
            fmt::arg("count", rule_count_)));
    }

    out.close();

    load();

    return std::size(ranges);
}

#ifdef TR_ENABLE_ASSERTS
void BlocklistFile::assertValidRules(std::vector<IPv4Range> const& ranges)
{
    for (auto const& r : ranges)
    {
        TR_ASSERT(r.begin_ <= r.end_);
    }

    for (size_t i = 1; i < std::size(ranges); ++i)
    {
        TR_ASSERT(ranges[i - 1].end_ < ranges[i].begin_);
    }
}
#endif
