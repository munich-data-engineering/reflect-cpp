#ifndef RFL_INTERNAL_STRINGS_UTF8_CONVERSIONS_HPP_
#define RFL_INTERNAL_STRINGS_UTF8_CONVERSIONS_HPP_

#include <optional>
#include <string>

namespace rfl::internal::strings {

// Locale-independent conversion between UTF-8 (std::string) and std::wstring.
//
// JSON is defined to be UTF-8 (RFC 8259), so the std::string side is always
// UTF-8. wchar_t is treated as UTF-32 where it is >= 4 bytes (Linux, macOS) and
// as UTF-16 where it is 2 bytes (Windows). Unlike std::mbsrtowcs / std::wcsrtombs
// this does not depend on the process's C locale and cannot return an
// unchecked error sentinel.

namespace utf8_detail {

/// Decodes UTF-8 bytes into Unicode code points. Returns std::nullopt if the
/// input is not well-formed UTF-8 (truncated, overlong, bad continuation byte,
/// surrogate code point, or out of range).
inline std::optional<std::u32string> decode_utf8(const std::string& _str) {
  std::u32string out;
  std::size_t i = 0;
  const std::size_t n = _str.size();
  while (i < n) {
    const auto c = static_cast<unsigned char>(_str[i]);
    char32_t cp = 0;
    int extra = 0;
    if (c < 0x80) {
      cp = c;
      extra = 0;
    } else if ((c >> 5) == 0x6) {
      cp = c & 0x1F;
      extra = 1;
    } else if ((c >> 4) == 0xE) {
      cp = c & 0x0F;
      extra = 2;
    } else if ((c >> 3) == 0x1E) {
      cp = c & 0x07;
      extra = 3;
    } else {
      return std::nullopt;  // invalid leading byte
    }
    if (i + static_cast<std::size_t>(extra) >= n) {
      return std::nullopt;  // truncated multi-byte sequence
    }
    for (int k = 1; k <= extra; ++k) {
      const auto cc =
          static_cast<unsigned char>(_str[i + static_cast<std::size_t>(k)]);
      if ((cc >> 6) != 0x2) {
        return std::nullopt;  // bad continuation byte
      }
      cp = (cp << 6) | (cc & 0x3F);
    }
    static constexpr char32_t mins[] = {0, 0x80, 0x800, 0x10000};
    if (cp < mins[extra]) {
      return std::nullopt;  // overlong encoding
    }
    if (cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) {
      return std::nullopt;  // out of range or surrogate code point
    }
    out.push_back(cp);
    i += static_cast<std::size_t>(extra) + 1;
  }
  return out;
}

/// Encodes Unicode code points as UTF-8 bytes.
inline std::string encode_utf8(const std::u32string& _cps) {
  std::string out;
  for (const char32_t cp : _cps) {
    if (cp < 0x80) {
      out.push_back(static_cast<char>(cp));
    } else if (cp < 0x800) {
      out.push_back(static_cast<char>(0xC0 | (cp >> 6)));
      out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else if (cp < 0x10000) {
      out.push_back(static_cast<char>(0xE0 | (cp >> 12)));
      out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
      out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    } else {
      out.push_back(static_cast<char>(0xF0 | (cp >> 18)));
      out.push_back(static_cast<char>(0x80 | ((cp >> 12) & 0x3F)));
      out.push_back(static_cast<char>(0x80 | ((cp >> 6) & 0x3F)));
      out.push_back(static_cast<char>(0x80 | (cp & 0x3F)));
    }
  }
  return out;
}

/// Encodes Unicode code points as a std::wstring (UTF-32 or UTF-16 depending on
/// the width of wchar_t).
inline std::wstring codepoints_to_wide(const std::u32string& _cps) {
  std::wstring out;
  for (const char32_t cp : _cps) {
    if constexpr (sizeof(wchar_t) >= 4) {
      out.push_back(static_cast<wchar_t>(cp));
    } else {  // UTF-16
      if (cp < 0x10000) {
        out.push_back(static_cast<wchar_t>(cp));
      } else {
        const char32_t v = cp - 0x10000;
        out.push_back(static_cast<wchar_t>(0xD800 + (v >> 10)));
        out.push_back(static_cast<wchar_t>(0xDC00 + (v & 0x3FF)));
      }
    }
  }
  return out;
}

/// Decodes a std::wstring into Unicode code points. Returns std::nullopt if the
/// input contains unpaired surrogates or out-of-range code units.
inline std::optional<std::u32string> wide_to_codepoints(const std::wstring& _str) {
  std::u32string out;
  if constexpr (sizeof(wchar_t) >= 4) {
    for (const wchar_t w : _str) {
      const auto cp = static_cast<char32_t>(w);
      if (cp > 0x10FFFF || (cp >= 0xD800 && cp <= 0xDFFF)) {
        return std::nullopt;
      }
      out.push_back(cp);
    }
  } else {  // UTF-16
    for (std::size_t i = 0; i < _str.size(); ++i) {
      const auto u = static_cast<char32_t>(static_cast<char16_t>(_str[i]));
      if (u >= 0xD800 && u <= 0xDBFF) {  // high surrogate
        if (i + 1 >= _str.size()) {
          return std::nullopt;  // unpaired high surrogate
        }
        const auto lo = static_cast<char32_t>(static_cast<char16_t>(_str[i + 1]));
        if (lo < 0xDC00 || lo > 0xDFFF) {
          return std::nullopt;  // high surrogate not followed by low surrogate
        }
        out.push_back(0x10000 + ((u - 0xD800) << 10) + (lo - 0xDC00));
        ++i;
      } else if (u >= 0xDC00 && u <= 0xDFFF) {
        return std::nullopt;  // unpaired low surrogate
      } else {
        out.push_back(u);
      }
    }
  }
  return out;
}

}  // namespace utf8_detail

/// Converts a UTF-8-encoded std::string to a std::wstring. Returns std::nullopt
/// if the input is not well-formed UTF-8.
inline std::optional<std::wstring> utf8_to_wstring(const std::string& _str) {
  const auto cps = utf8_detail::decode_utf8(_str);
  if (!cps) {
    return std::nullopt;
  }
  return utf8_detail::codepoints_to_wide(*cps);
}

/// Converts a std::wstring to a UTF-8-encoded std::string. Returns std::nullopt
/// if the input contains unpaired surrogates or otherwise invalid code units.
inline std::optional<std::string> wstring_to_utf8(const std::wstring& _str) {
  const auto cps = utf8_detail::wide_to_codepoints(_str);
  if (!cps) {
    return std::nullopt;
  }
  return utf8_detail::encode_utf8(*cps);
}

}  // namespace rfl::internal::strings

#endif
