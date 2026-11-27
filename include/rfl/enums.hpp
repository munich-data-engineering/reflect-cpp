#ifndef RFL_ENUMS_HPP_
#define RFL_ENUMS_HPP_

#include <charconv>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include "Result.hpp"
#include "internal/enums/from_string.hpp"
#include "internal/enums/get_enum_names.hpp"
#include "internal/enums/get_enum_range.hpp"
#include "internal/enums/is_flag_enum.hpp"
#include "internal/enums/to_string.hpp"
#include "internal/strings/strings.hpp"

namespace rfl {

// Returns a named tuple mapping names of enumerators of the given enum type to
// their values.
template <class EnumType>
auto get_enumerators() {
  return internal::enums::names_to_enumerator_named_tuple(
      internal::enums::get_enum_names<EnumType>());
}

// Returns a named tuple mapping names of enumerators of the given enum type to
// their underlying values.
template <class EnumType>
auto get_underlying_enumerators() {
  return internal::enums::names_to_underlying_enumerator_named_tuple(
      internal::enums::get_enum_names<EnumType>());
}

// Returns an std::array containing pairs of enumerator names (as
// std::string_view) and values.
template <class EnumType>
constexpr auto get_enumerator_array() {
  return internal::enums::names_to_enumerator_array(
      internal::enums::get_enum_names<EnumType>());
}

// Returns an std::array containing pairs of enumerator names (as
// std::string_view) and underlying values.
template <class EnumType>
constexpr auto get_underlying_enumerator_array() {
  return internal::enums::names_to_underlying_enumerator_array(
      internal::enums::get_enum_names<EnumType>());
}

// Returns the range of the given enum type as a pair of the minimum and maximum
template <class EnumType>
constexpr auto get_enum_range() {
  return internal::enums::get_enum_range<EnumType>();
}

// Converts an enum value to tis string representation.
template <class EnumType>
std::string enum_to_string(const EnumType _enum) {
  if constexpr (internal::enums::is_flag_enum<EnumType>) {
    // Iterates through the enum bit by bit and matches it against the flags.
    using T = std::underlying_type_t<EnumType>;
    auto val = static_cast<T>(_enum);
    int i = 0;
    std::vector<std::string> flags;
    while (val != 0) {
      const auto bit = val & static_cast<T>(1);
      if (bit == 1) {
        auto str = internal::enums::to_string(
            static_cast<EnumType>(static_cast<T>(1) << i));
        flags.emplace_back(std::move(str));
      }
      ++i;
      val >>= 1;
    }
    if (flags.empty()) {
      return internal::enums::to_string(static_cast<EnumType>(0));
    }
    return internal::strings::join("|", flags);
  } else {
    return internal::enums::to_string(_enum);
  }
}

// Converts a string to a value of the given enum type.
//
// By default, numeric values are accepted in addition to the declared
// enumerator names, even when they do not correspond to a declared
// enumerator (for instance, reading "4" into an enum with values 1, 2 and 3
// will produce the cast value 4). Pass enum_names_only = true (or use the
// EnumNamesOnly processor with a parser) to accept only the declared
// enumerator names instead.
template <class EnumType, bool enum_names_only = false>
Result<EnumType> string_to_enum(const std::string_view _str) {
  const auto make_error_msg = [](const std::string_view name) {
    std::string msg = "Invalid enum value: '";
    msg += name;
    msg += "'. Must be one of [";
    const char* sep = "";
    for (const auto& p : get_enumerator_array<EnumType>()) {
      msg += sep;
      msg += p.first;
      sep = ", ";
    }
    msg += "].";
    return error(msg);
  };

  const auto cast_numbers_or_names =
      [&](const std::string_view name) -> Result<EnumType> {
    const auto r = internal::enums::from_string<EnumType>(name);
    if (r) {
      return *r;
    }
    if constexpr (enum_names_only) {
      return make_error_msg(name);
    } else {
      if (name.empty()) {
        return make_error_msg(name);
      }
      using Underlying = std::underlying_type_t<EnumType>;
      using ParseType = std::conditional_t<
          (sizeof(Underlying) < sizeof(int)), int,
          std::conditional_t<
              std::is_signed_v<Underlying>,
              std::conditional_t<
                  (sizeof(Underlying) <= sizeof(int)), int,
                  std::conditional_t<(sizeof(Underlying) <= sizeof(long)), long,
                                     long long>>,
              std::conditional_t<
                  (sizeof(Underlying) <= sizeof(unsigned int)), unsigned int,
                  std::conditional_t<
                      (sizeof(Underlying) <= sizeof(unsigned long)),
                      unsigned long, unsigned long long>>>>;
      ParseType val{};
      const auto* begin = name.data();
      const auto* end = begin + name.size();
      const auto [ptr, ec] = std::from_chars(begin, end, val);
      if (ec == std::errc{} && ptr == end) {
        return static_cast<EnumType>(val);
      }
      return make_error_msg(name);
    }
  };

  if constexpr (internal::enums::is_flag_enum<EnumType>) {
    using T = std::underlying_type_t<EnumType>;
    auto res = static_cast<T>(0);
    std::string_view remaining = _str;
    while (true) {
      const auto pos = remaining.find('|');
      const auto token = remaining.substr(0, pos);
      const auto r = cast_numbers_or_names(token);
      if (!r) {
        return r;
      }
      res |= static_cast<T>(*r);
      if (pos == std::string_view::npos) {
        break;
      }
      remaining.remove_prefix(pos + 1);
    }
    return static_cast<EnumType>(res);
  } else {
    return cast_numbers_or_names(_str);
  }
}

}  // namespace rfl

#endif  // RFL_ENUMS_HPP_
