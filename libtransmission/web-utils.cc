/*
 * This file Copyright (C) Mnemosyne LLC
 *
 * It may be used under the GNU GPL versions 2 or 3
 * or any future license endorsed by Mnemosyne LLC.
 *
 */

#include <algorithm>
#include <cstddef>
#include <optional>
#include <string_view>

#include <curl/curl.h>

#include <event2/buffer.h>

#include "transmission.h"

#include "net.h"
#include "web-utils.h"
#include "utils.h"

using namespace std::literals;

/***
****
***/

bool tr_addressIsIP(char const* str)
{
    tr_address tmp;
    return tr_address_from_string(&tmp, str);
}

char const* tr_webGetResponseStr(long code)
{
    switch (code)
    {
    case 0:
        return "No Response";

    case 101:
        return "Switching Protocols";

    case 200:
        return "OK";

    case 201:
        return "Created";

    case 202:
        return "Accepted";

    case 203:
        return "Non-Authoritative Information";

    case 204:
        return "No Content";

    case 205:
        return "Reset Content";

    case 206:
        return "Partial Content";

    case 300:
        return "Multiple Choices";

    case 301:
        return "Moved Permanently";

    case 302:
        return "Found";

    case 303:
        return "See Other";

    case 304:
        return "Not Modified";

    case 305:
        return "Use Proxy";

    case 306:
        return " (Unused)";

    case 307:
        return "Temporary Redirect";

    case 400:
        return "Bad Request";

    case 401:
        return "Unauthorized";

    case 402:
        return "Payment Required";

    case 403:
        return "Forbidden";

    case 404:
        return "Not Found";

    case 405:
        return "Method Not Allowed";

    case 406:
        return "Not Acceptable";

    case 407:
        return "Proxy Authentication Required";

    case 408:
        return "Request Timeout";

    case 409:
        return "Conflict";

    case 410:
        return "Gone";

    case 411:
        return "Length Required";

    case 412:
        return "Precondition Failed";

    case 413:
        return "Request Entity Too Large";

    case 414:
        return "Request-URI Too Long";

    case 415:
        return "Unsupported Media Type";

    case 416:
        return "Requested Range Not Satisfiable";

    case 417:
        return "Expectation Failed";

    case 421:
        return "Misdirected Request";

    case 500:
        return "Internal Server Error";

    case 501:
        return "Not Implemented";

    case 502:
        return "Bad Gateway";

    case 503:
        return "Service Unavailable";

    case 504:
        return "Gateway Timeout";

    case 505:
        return "HTTP Version Not Supported";

    default:
        return "Unknown Error";
    }
}

void tr_http_escape(struct evbuffer* out, std::string_view str, bool escape_reserved)
{
    auto constexpr ReservedChars = std::string_view{ "!*'();:@&=+$,/?%#[]" };
    auto constexpr UnescapedChars = std::string_view{ "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ-_.~" };

    for (auto& ch : str)
    {
        if ((UnescapedChars.find(ch) != std::string_view::npos) || (ReservedChars.find(ch) && !escape_reserved))
        {
            evbuffer_add_printf(out, "%c", ch);
        }
        else
        {
            evbuffer_add_printf(out, "%%%02X", (unsigned)(ch & 0xFF));
        }
    }
}

char* tr_http_unescape(char const* str, size_t len)
{
    char* tmp = curl_unescape(str, len);
    char* ret = tr_strdup(tmp);
    curl_free(tmp);
    return ret;
}

static bool is_rfc2396_alnum(uint8_t ch)
{
    return ('0' <= ch && ch <= '9') || ('A' <= ch && ch <= 'Z') || ('a' <= ch && ch <= 'z') || ch == '.' || ch == '-' ||
        ch == '_' || ch == '~';
}

void tr_http_escape_sha1(char* out, tr_sha1_digest_t const& digest)
{
    for (auto const b : digest)
    {
        if (is_rfc2396_alnum(uint8_t(b)))
        {
            *out++ = (char)b;
        }
        else
        {
            out += tr_snprintf(out, 4, "%%%02x", (unsigned int)b);
        }
    }

    *out = '\0';
}

void tr_http_escape_sha1(char* out, uint8_t const* sha1_digest)
{
    auto digest = tr_sha1_digest_t{};
    std::copy_n(reinterpret_cast<std::byte const*>(sha1_digest), std::size(digest), std::begin(digest));
    tr_http_escape_sha1(out, digest);
}

//// URLs

namespace
{

int parsePort(std::string_view port)
{
    auto tmp = std::array<char, 16>{};

    if (std::size(port) >= std::size(tmp))
    {
        return -1;
    }

    std::copy(std::begin(port), std::end(port), std::begin(tmp));
    char* end = nullptr;
    long port_num = strtol(std::data(tmp), &end, 10);
    if (*end != '\0' || port_num <= 0 || port_num >= 65536)
    {
        port_num = -1;
    }

    return int(port_num);
}

constexpr std::string_view getPortForScheme(std::string_view scheme)
{
    auto constexpr KnownSchemes = std::array<std::pair<std::string_view, std::string_view>, 5>{ {
        { "ftp"sv, "21"sv },
        { "http"sv, "80"sv },
        { "https"sv, "443"sv },
        { "sftp"sv, "22"sv },
        { "udp"sv, "80"sv },
    } };

    for (auto const& [known_scheme, port] : KnownSchemes)
    {
        if (scheme == known_scheme)
        {
            return port;
        }
    }

    return "-1"sv;
}

bool urlCharsAreValid(std::string_view url)
{
    // rfc2396
    auto constexpr ValidChars = std::string_view{
        "abcdefghijklmnopqrstuvwxyz" // lowalpha
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ" // upalpha
        "0123456789" // digit
        "-_.!~*'()" // mark
        ";/?:@&=+$," // reserved
        "<>#%<\"" // delims
        "{}|\\^[]`" // unwise
    };

    return !std::empty(url) &&
        std::all_of(std::begin(url), std::end(url), [&ValidChars](auto ch) { return ValidChars.find(ch) != ValidChars.npos; });
}

bool tr_isValidTrackerScheme(std::string_view scheme)
{
    auto constexpr Schemes = std::array<std::string_view, 3>{ "http"sv, "https"sv, "udp"sv };
    return std::find(std::begin(Schemes), std::end(Schemes), scheme) != std::end(Schemes);
}

} // namespace

std::optional<tr_url_parsed_t> tr_urlParse(std::string_view url)
{
    url = tr_strvstrip(url);

    if (!urlCharsAreValid(url))
    {
        return {};
    }

    // scheme
    auto key = "://"sv;
    auto pos = url.find(key);
    if (pos == std::string_view::npos || pos == 0)
    {
        return {};
    }
    auto const scheme = url.substr(0, pos);
    url.remove_prefix(pos + std::size(key));

    // authority
    key = "/"sv;
    pos = url.find(key);
    if (pos == 0)
    {
        return {};
    }
    auto const authority = url.substr(0, pos);
    url.remove_prefix(std::size(authority));
    auto const path = std::empty(url) ? "/"sv : url;

    // host
    key = ":"sv;
    pos = authority.find(key);
    auto const host = pos == std::string_view::npos ? authority : authority.substr(0, pos);
    if (std::empty(host))
    {
        return {};
    }

    // port
    auto const portstr = pos == std::string_view::npos ? getPortForScheme(scheme) : authority.substr(pos + std::size(key));
    auto const port = parsePort(portstr);

    return tr_url_parsed_t{ scheme, host, path, portstr, port };
}

std::optional<tr_url_parsed_t> tr_urlParseTracker(std::string_view url)
{
    auto const parsed = tr_urlParse(url);
    return parsed && tr_isValidTrackerScheme(parsed->scheme) ? *parsed : std::optional<tr_url_parsed_t>{};
}

bool tr_urlIsValidTracker(std::string_view url)
{
    return !!tr_urlParseTracker(url);
}

bool tr_urlIsValid(std::string_view url)
{
    auto constexpr Schemes = std::array<std::string_view, 5>{ "http"sv, "https"sv, "ftp"sv, "sftp"sv, "udp"sv };
    auto const parsed = tr_urlParse(url);
    return parsed && std::find(std::begin(Schemes), std::end(Schemes), parsed->scheme) != std::end(Schemes);
}

std::optional<tr_url_query_walk_t> tr_urlNextQueryPair(std::string_view query_remain)
{
    auto in = query_remain;

    // get the next pair
    auto key = "&"sv;
    auto pos = in.find(key);
    auto const pair = in.substr(0, pos);
    in = pos == in.npos ? ""sv : in.substr(pos + std::size(key));
    if (std::empty(pair))
    {
        return {};
    }

    // split it into key and value
    key = "="sv;
    pos = pair.find(key);
    auto const key_sv = pair.substr(0, pos);
    auto const value_sv = pos == pair.npos ? ""sv : pair.substr(pos + std::size(key));

    return tr_url_query_walk_t{ key_sv, value_sv, in };
}
