// This file is part of PadOS.
//
// Copyright (C) 2018-2021 Kurt Skauen <http://kavionic.com/>
//
// PadOS is free software : you can redistribute it and / or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// PadOS is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with PadOS. If not, see <http://www.gnu.org/licenses/>.
///////////////////////////////////////////////////////////////////////////////
// Created: 28.03.2018 20:51:21

#pragma once

#include <string>
#include <vector>
#include <stdarg.h>

#include <System/Types.h>
#include <System/TimeValue.h>

#ifdef PADOS_OPT_USE_FMT_FORMATTING

#include <fmt/format.h>

#define PFORMATTER_NAMESPACE namespace fmt

template<typename... ARGS>
using PFormatString = fmt::format_string<ARGS...>;

#else // PADOS_OPT_USE_FMT_FORMATTING

#include <format>

#define PFORMATTER_NAMESPACE namespace std

template<typename... ARGS>
using PFormatString = std::format_string<ARGS...>;

#endif // PADOS_OPT_USE_FMT_FORMATTING

enum class PUnitSystem
{
    Auto,
    SI,
    IEC
};

class PString : public std::string
{
public:
    template<typename... ARGS>
    PString(ARGS&&... args) : std::string(args...) {}
    PString(const wchar16_t* utf16, size_t length = npos);

    PString& assign_utf16(const wchar16_t* source, size_t length = npos);

    size_t copy_utf16(wchar16_t* dst, size_t length, size_t pos = 0) const;

    PString& append(uint32_t unicode);

    int compare_nocase(const std::string& rhs) const;
    int compare_nocase(const char* rhs) const;

    bool starts_with(const char* token, size_t length = INVALID_INDEX) const;
    bool starts_with_nocase(const char* token, size_t length = INVALID_INDEX) const;

    bool ends_with(const char* token, size_t length = INVALID_INDEX) const;
    bool ends_with_nocase(const char* token, size_t length = INVALID_INDEX) const;

    bool containes(const char* token, size_t length = INVALID_INDEX) const;
    bool containes_nocase(const char* token, size_t length = INVALID_INDEX) const;

    bool        is_dot() const                           { return size() == 1 && at(0) == '.'; }
    static bool is_dot(const char* str)                  { return str[0] == '.' && str[1] == '\0'; }
    static bool is_dot(const char* str, size_t length)   { return length == 1 && str[0] == '.'; }

    bool        is_dot_dot() const                            { return size() == 2 && at(0) == '.' && at(1) == '.'; }
    static bool is_dot_dot(const char* str)                   { return str[0] == '.' && str[1] == '.' && str[3] == '\0'; }
    static bool is_dot_dot(const char* str, size_t length)    { return length == 2 && str[0] == '.' && str[1] == '.'; }

    bool        is_dot_or_dot_dot() const                           { return is_dot() || is_dot_dot(); }
    static bool is_dot_or_dot_dot(const char* str)                  { return is_dot(str) || is_dot_dot(str); }
    static bool is_dot_or_dot_dot(const char* str, size_t length)   { return is_dot(str, length) || is_dot_dot(str, length); }

    size_t  count_chars() const;
    PString& strip();
    PString& lstrip();
    PString& rstrip();
    PString& lower();
    PString& upper();

    template<typename ...ARGS>
    static PString format_string(PFormatString<ARGS...>&& fmt, ARGS&&... args)
    {
#ifdef PADOS_OPT_USE_FMT_FORMATTING
        return fmt::format(std::forward<PFormatString<ARGS...>>(fmt), std::forward<ARGS>(args)...);
#else // PADOS_OPT_USE_FMT_FORMATTING
        return std::format(std::forward<PFormatString<ARGS...>>(fmt), std::forward<ARGS>(args)...);
#endif // PADOS_OPT_USE_FMT_FORMATTING
    }

    template<typename ...ARGS>
    static PString vformat_string(std::string_view fmt, ARGS&&... args)
    {
#ifdef PADOS_OPT_USE_FMT_FORMATTING
        return fmt::vformat(fmt, fmt::make_format_args(args...));
#else // PADOS_OPT_USE_FMT_FORMATTING
        return std::vformat(fmt, std::make_format_args(args...));
#endif // PADOS_OPT_USE_FMT_FORMATTING
    }

    template<typename ...ARGS>
    PString& format(PFormatString<ARGS...>&& fmt, ARGS&&... args)
    {
        *this = format_string(std::forward<PFormatString<ARGS...>>(fmt), std::forward<ARGS>(args)...);
        return *this;
    }

    template<typename ...ARGS>
    PString& vformat(std::string_view fmt, ARGS&&... args)
    {
        *this = vformat_string(fmt, std::forward<ARGS>(args)...);
        return *this;
    }

    static PString format_byte_size(int64_t size, int numDecimalsOrDigits, PUnitSystem unitSystem = PUnitSystem::IEC);
    static PString format_time_period(const TimeValNanos& timeVal, bool includeUnits, size_t maxCharacters = 0);
    static PString format_file_permissions(mode_t mode);

    static std::optional<int64_t> parse_byte_size(const PString& text, PUnitSystem unitSystem);

    // FNV-1a 32bit hashing algorithm.
    inline static constexpr uint32_t hash_string_literal(char const* s, size_t count)
    {
        return hash_string_literal_recurse(s, 2166136261u, count);
    }
    inline static constexpr uint32_t hash_string_literal_nocase(char const* s, size_t count)
    {
        return hash_string_literal_nocase_recurse(s, 2166136261u, count);
    }


    inline static constexpr uint32_t hash_string_literal(char const* s)
    {
        return hash_string_literal_recurse(s, 2166136261u);
    }
    inline static constexpr uint32_t hash_string_literal_nocase(char const* s)
    {
        return hash_string_literal_nocase_recurse(s, 2166136261u);
    }

    static PString zero;
private:
    inline static constexpr uint32_t hash_string_literal_recurse(char const* s, uint32_t hash, size_t count)
    {
        return count ? hash_string_literal_recurse(s + 1, (hash ^ *s) * 16777619u, count - 1) : hash;
    }
    inline static constexpr uint32_t hash_string_literal_nocase_recurse(char const* s, uint32_t hash, size_t count)
    {
        return count ? hash_string_literal_nocase_recurse(s + 1, (hash ^ tolower(*s)) * 16777619u, count - 1) : hash;
    }
    inline static constexpr uint32_t hash_string_literal_recurse(char const* s, uint32_t hash)
    {
        return s[0] ? hash_string_literal_recurse(s + 1, (hash ^ *s) * 16777619u) : hash;
    }
    inline static constexpr uint32_t hash_string_literal_nocase_recurse(char const* s, uint32_t hash)
    {
        return s[0] ? hash_string_literal_nocase_recurse(s + 1, (hash ^ tolower(*s)) * 16777619u) : hash;
    }

    static std::optional<int> prefix_to_exp(char prefixUpper);
    static std::optional<int> parse_suffix(const PString& suffixIn, PUnitSystem& outUnitSystem);
    static size_t find_suffix_start(const PString& text);

};

PFORMATTER_NAMESPACE
{
template<>
struct formatter<PString> : formatter<::std::basic_string<PString::value_type, PString::traits_type, PString::allocator_type>, PString::value_type> {};
}
