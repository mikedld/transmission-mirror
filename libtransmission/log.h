// This file Copyright © 2010-2022 Mnemosyne LLC.
// It may be used under GPLv2 (SPDX: GPL-2.0), GPLv3 (SPDX: GPL-3.0),
// or any future license endorsed by Mnemosyne LLC.
// License text can be found in the licenses/ folder.

#pragma once

#include <ctime>
#include <string_view>

enum tr_log_level
{
    // No logging at all
    TR_LOG_OFF,

    // Errors that prevent Transmission from running
    TR_LOG_CRITICAL,

    // Errors that could prevent a single torrent from running, e.g. missing
    // files or a private torrent's tracker responding "unregistered torrent"
    TR_LOG_ERROR,

    // Smaller errors that don't stop the overall system,
    // e.g. unable to preallocate a file, or unable to connect to a tracker
    // when other trackers are available
    TR_LOG_WARN,

    // User-visible info, e.g. "torrent completed" or "running script"
    TR_LOG_INFO,

    // Debug messages
    TR_LOG_DEBUG,

    // High-volume debug messages, e.g. tracing peer protocol messages
    TR_LOG_TRACE
};

struct tr_log_message
{
    tr_log_level level;

    // location in the source code
    char const* file;
    int line;

    // when the message was generated
    time_t when;

    // torrent name or code module name associated with the message
    char* name;

    // the message
    char* message;

    // linked list of messages
    struct tr_log_message* next;
};

////

#define TR_LOG_MAX_QUEUE_LENGTH 10000

[[nodiscard]] bool tr_logGetQueueEnabled();

void tr_logSetQueueEnabled(bool isEnabled);

[[nodiscard]] tr_log_message* tr_logGetQueue();

void tr_logFreeQueue(tr_log_message* freeme);

////

void tr_logSetLevel(tr_log_level);

[[nodiscard]] tr_log_level tr_logGetLevel();

[[nodiscard]] bool tr_logLevelIsActive(tr_log_level level);

////

void tr_logAddMessage(
    char const* source_file,
    int source_line,
    tr_log_level level,
    std::string_view message,
    std::string_view module_name = "");

#define tr_logAddLevel(level, ...) \
    do \
    { \
        if (tr_logGetLevel() >= level) \
        { \
            tr_logAddMessage(__FILE__, __LINE__, level, __VA_ARGS__); \
        } \
    } while (0)

#define tr_logAddCritical(...) tr_logAddLevel(TR_LOG_CRITICAL, __VA_ARGS__)
#define tr_logAddError(...) tr_logAddLevel(TR_LOG_ERROR, __VA_ARGS__)
#define tr_logAddWarn(...) tr_logAddLevel(TR_LOG_WARN, __VA_ARGS__)
#define tr_logAddInfo(...) tr_logAddLevel(TR_LOG_INFO, __VA_ARGS__)
#define tr_logAddDebug(...) tr_logAddLevel(TR_LOG_DEBUG, __VA_ARGS__)
#define tr_logAddTrace(...) tr_logAddLevel(TR_LOG_TRACE, __VA_ARGS__)

////

char* tr_logGetTimeStr(char* buf, size_t buflen);
