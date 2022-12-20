// This file Copyright © 2007-2022 Mnemosyne LLC.
// It may be used under GPLv2 (SPDX: GPL-2.0-only), GPLv3 (SPDX: GPL-3.0-only),
// or any future license endorsed by Mnemosyne LLC.
// License text can be found in the licenses/ folder.

#include <algorithm>
#include <array>
#include <cctype>
#include <functional>
#include <iterator>
#include <random>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

extern "C"
{
#include <b64/cdecode.h>
#include <b64/cencode.h>
}

#include <fmt/format.h>

#include "transmission.h"
#include "crypto-utils.h"
#include "tr-assert.h"
#include "utils.h"

using namespace std::literals;

///

template<class T>
[[nodiscard]] T tr_rand_int(T upper_bound)
{
    static_assert(!std::is_signed<T>());
    TR_ASSERT(upper_bound > std::numeric_limits<T>::min());

    using dist_type = std::uniform_int_distribution<T>;
    thread_local auto rng = tr_urbg<T>{};
    thread_local auto dist = dist_type{};
    return dist(rng, typename dist_type::param_type(0, upper_bound - 1));
}

template size_t tr_rand_int(size_t upper_bound);
template unsigned char tr_rand_int(unsigned char upper_bound);
template unsigned int tr_rand_int(unsigned int upper_bound);

///

namespace
{
namespace ssha1_impl
{

auto constexpr DigestStringSize = TR_SHA1_DIGEST_STRLEN;
auto constexpr SaltedPrefix = "{"sv;

std::string tr_salt(std::string_view plaintext, std::string_view salt)
{
    static_assert(DigestStringSize == 40);

    // build a sha1 digest of the original content and the salt
    auto const digest = tr_sha1::digest(plaintext, salt);

    // convert it to a string. string holds three parts:
    // DigestPrefix, stringified digest of plaintext + salt, and the salt.
    return fmt::format(FMT_STRING("{:s}{:s}{:s}"), SaltedPrefix, tr_sha1_to_string(digest), salt);
}

} // namespace ssha1_impl
} // namespace

std::string tr_ssha1(std::string_view plaintext)
{
    using namespace ssha1_impl;

    // build an array of random Salter chars
    auto constexpr Salter = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ./"sv;
    static_assert(std::size(Salter) == 64);
    auto constexpr SaltSize = size_t{ 8 };
    auto salt = tr_rand_obj<std::array<char, SaltSize>>();
    std::transform(
        std::begin(salt),
        std::end(salt),
        std::begin(salt),
        [&Salter](auto ch) { return Salter[ch % std::size(Salter)]; });

    return tr_salt(plaintext, std::string_view{ std::data(salt), std::size(salt) });
}

bool tr_ssha1_test(std::string_view text)
{
    using namespace ssha1_impl;

    return tr_strvStartsWith(text, SaltedPrefix) && std::size(text) >= std::size(SaltedPrefix) + DigestStringSize;
}

bool tr_ssha1_matches(std::string_view ssha1, std::string_view plaintext)
{
    using namespace ssha1_impl;

    if (!tr_ssha1_test(ssha1))
    {
        return false;
    }

    auto const salt = ssha1.substr(std::size(SaltedPrefix) + DigestStringSize);
    return tr_salt(plaintext, salt) == ssha1;
}

/***
****
***/

namespace
{
namespace base64_impl
{

constexpr size_t base64AllocSize(std::string_view input)
{
    size_t ret_length = 4 * ((std::size(input) + 2) / 3); // NOLINT misc-const-correctness
#ifdef USE_SYSTEM_B64
    // Additional space is needed for newlines if we're using unpatched libb64
    ret_length += ret_length / 72 + 1;
#endif
    return ret_length * 8;
}

} // namespace base64_impl
} // namespace

std::string tr_base64_encode(std::string_view input)
{
    using namespace base64_impl;

    auto buf = std::vector<char>(base64AllocSize(input));
    auto state = base64_encodestate{};
    base64_init_encodestate(&state);
    size_t len = base64_encode_block(std::data(input), std::size(input), std::data(buf), &state);
    len += base64_encode_blockend(std::data(buf) + len, &state);
    auto str = std::string{};
    std::copy_if(
        std::data(buf),
        std::data(buf) + len,
        std::back_inserter(str),
        [](auto ch) { return !tr_strvContains("\r\n"sv, ch); });
    return str;
}

std::string tr_base64_decode(std::string_view input)
{
    auto buf = std::vector<char>(std::size(input) + 8);
    auto state = base64_decodestate{};
    base64_init_decodestate(&state);
    size_t const len = base64_decode_block(std::data(input), std::size(input), std::data(buf), &state);
    return std::string{ std::data(buf), len };
}

/***
****
***/

namespace
{
namespace hex_impl
{

constexpr void tr_binary_to_hex(void const* vinput, void* voutput, size_t byte_length)
{
    auto constexpr Hex = "0123456789abcdef"sv;

    auto const* input = static_cast<uint8_t const*>(vinput);
    auto* output = static_cast<char*>(voutput);

    /* go from back to front to allow for in-place conversion */
    input += byte_length;
    output += byte_length * 2;

    *output = '\0';

    while (byte_length-- > 0)
    {
        unsigned int const val = *(--input);
        *(--output) = Hex[val & 0xf];
        *(--output) = Hex[val >> 4];
    }
}

constexpr void tr_hex_to_binary(char const* input, void* voutput, size_t byte_length)
{
    auto constexpr Hex = "0123456789abcdef"sv;

    auto* output = static_cast<uint8_t*>(voutput);

    for (size_t i = 0; i < byte_length; ++i)
    {
        auto const upper_nibble = Hex.find(std::tolower(*input++));
        auto const lower_nibble = Hex.find(std::tolower(*input++));
        *output++ = (uint8_t)((upper_nibble << 4) | lower_nibble);
    }
}

} // namespace hex_impl
} // namespace

std::string tr_sha1_to_string(tr_sha1_digest_t const& digest)
{
    using namespace hex_impl;

    auto str = std::string(std::size(digest) * 2, '?');
    tr_binary_to_hex(digest.data(), str.data(), std::size(digest));
    return str;
}

std::string tr_sha256_to_string(tr_sha256_digest_t const& digest)
{
    using namespace hex_impl;

    auto str = std::string(std::size(digest) * 2, '?');
    tr_binary_to_hex(digest.data(), str.data(), std::size(digest));
    return str;
}

std::optional<tr_sha1_digest_t> tr_sha1_from_string(std::string_view hex)
{
    using namespace hex_impl;

    if (std::size(hex) != TR_SHA1_DIGEST_STRLEN)
    {
        return {};
    }

    if (!std::all_of(std::begin(hex), std::end(hex), [](unsigned char ch) { return isxdigit(ch); }))
    {
        return {};
    }

    auto digest = tr_sha1_digest_t{};
    tr_hex_to_binary(std::data(hex), std::data(digest), std::size(digest));
    return digest;
}

std::optional<tr_sha256_digest_t> tr_sha256_from_string(std::string_view hex)
{
    using namespace hex_impl;

    if (std::size(hex) != TR_SHA256_DIGEST_STRLEN)
    {
        return {};
    }

    if (!std::all_of(std::begin(hex), std::end(hex), [](unsigned char ch) { return isxdigit(ch); }))
    {
        return {};
    }

    auto digest = tr_sha256_digest_t{};
    tr_hex_to_binary(std::data(hex), std::data(digest), std::size(digest));
    return digest;
}

// fallback implementation in case the system crypto library's RNG fails
void tr_rand_buffer_fallback(void* buffer, size_t length)
{
    std::generate_n(
        static_cast<uint8_t*>(buffer),
        length,
        []()
        {
            thread_local auto engine = std::mt19937{ std::random_device{}() };
            thread_local auto dist = std::uniform_int_distribution<uint8_t>(uint8_t{});
            return dist(engine);
        });
}

void tr_rand_buffer(void* buffer, size_t length)
{
    extern bool tr_rand_buffer_impl(void* buffer, size_t length);

    if (!tr_rand_buffer_impl(buffer, length))
    {
        tr_rand_buffer_fallback(buffer, length);
    }
}
