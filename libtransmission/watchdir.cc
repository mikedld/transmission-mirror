// This file Copyright © 2015-2022 Mnemosyne LLC.
// It may be used under GPLv2 (SPDX: GPL-2.0-only), GPLv3 (SPDX: GPL-3.0-only),
// or any future license endorsed by Mnemosyne LLC.
// License text can be found in the licenses/ folder.

#define LIBTRANSMISSION_WATCHDIR_MODULE

#include <set>

#include "transmission.h"

#include "error-types.h"
#include "error.h"
#include "file.h"
#include "log.h"
#include "tr-strbuf.h"
#include "tr-assert.h"
#include "utils.h" // for _()
#include "watchdir-base.h"

using namespace std::literals;

namespace
{

[[nodiscard]] constexpr std::string_view actionToString(tr_watchdir::Action action)
{
    switch (action)
    {
    case tr_watchdir::Action::Retry:
        return "retry";

    case tr_watchdir::Action::Done:
        return "done";
    }

    TR_ASSERT(false);
    return "???";
}

[[nodiscard]] bool isRegularFile(std::string_view dir, std::string_view name)
{
    auto const path = tr_pathbuf{ dir, '/', name };

    tr_error* error = nullptr;
    auto const info = tr_sys_path_get_info(path, 0, &error);
    if (error != nullptr)
    {
        if (!TR_ERROR_IS_ENOENT(error->code))
        {
            tr_logAddWarn(fmt::format(
                _("Skipping '{path}': {error} ({error_code})"),
                fmt::arg("path", path),
                fmt::arg("error", error->message),
                fmt::arg("error_code", error->code)));
        }

        tr_error_free(error);
    }

    return info && info->isFile();
}

} // namespace

std::chrono::milliseconds tr_watchdir::generic_rescan_interval_ = tr_watchdir::DefaultGenericRescanInterval;

void tr_watchdir_base::processFile(std::string_view basename)
{
    if (!isRegularFile(dirname_, basename) || handled_.count(basename) != 0)
    {
        return;
    }

    auto const action = callback_(dirname_, basename);
    tr_logAddDebug(fmt::format("Callback decided to {:s} file '{:s}'", actionToString(action), basename));
    if (action == Action::Retry)
    {
        auto const [iter, added] = pending_.try_emplace(std::string{ basename }, Pending{});

        auto& info = iter->second;
        ++info.strikes;
        info.last_kick_at = std::chrono::steady_clock::now();

        if (info.strikes < retry_limit_)
        {
            setNextKickTime(info);
            restartTimerIfPending();
        }
        else
        {
            tr_logAddWarn(fmt::format(_("Couldn't add torrent file '{path}'"), fmt::arg("path", basename)));
            pending_.erase(iter);
        }
    }
    else if (action == Action::Done)
    {
        handled_.insert(std::string{ basename });
    }
}

void tr_watchdir_base::scan()
{
    auto new_dir_entries = std::set<std::string>{};

    tr_error* error = nullptr;
    auto const dir = tr_sys_dir_open(dirname_.c_str(), &error);
    if (dir == TR_BAD_SYS_DIR)
    {
        tr_logAddWarn(fmt::format(
            _("Couldn't read '{path}': {error} ({error_code})"),
            fmt::arg("path", dirname()),
            fmt::arg("error", error->message),
            fmt::arg("error_code", error->code)));
        tr_error_free(error);
        return;
    }

    for (;;)
    {
        char const* const name = tr_sys_dir_read_name(dir, &error);
        if (name == nullptr)
        {
            break;
        }

        if ("."sv == name || ".."sv == name)
        {
            continue;
        }

        processFile(name);
    }

    if (error != nullptr)
    {
        tr_logAddWarn(fmt::format(
            _("Couldn't read '{path}': {error} ({error_code})"),
            fmt::arg("path", dirname()),
            fmt::arg("error", error->message),
            fmt::arg("error_code", error->code)));
        tr_error_free(error);
    }

    tr_sys_dir_close(dir);
}