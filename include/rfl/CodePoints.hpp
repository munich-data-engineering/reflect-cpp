#ifndef RFL_CODEPOINTS_HPP_
#define RFL_CODEPOINTS_HPP_

#include <cstddef>
#include <string_view>
#include <utility>

#include "Result.hpp"
#include "Size.hpp"
#include "parsing/schema/ValidationType.hpp"

namespace rfl {

/// Validates the length of a UTF-8 encoded string, measured in Unicode code
/// points, using a validator V.
///
/// Unlike rfl::Size, which applies V to the number of bytes returned by
/// size(), this counts code points. This matches the semantics of the
/// minLength/maxLength keywords in JSON Schema, which is what the generated
/// schema emits.
///
/// Note that this counts code points, not grapheme clusters (user-perceived
/// characters), so e.g. an emoji with a skin-tone modifier counts as two.
/// It also does not check that the input is well-formed UTF-8: it simply
/// counts every byte that is not a UTF-8 continuation byte (10xxxxxx).
template <class V>
struct CodePoints {
  /// Validates that the number of code points in the string satisfies the
  /// validator V.
  /// @tparam T The type of the string to validate
  /// @param _t The string whose code point count will be validated
  /// @return The string if validation passes, otherwise an error
  template <class T>
  static rfl::Result<T> validate(T _t) {
    const auto view = std::string_view(_t);
    std::size_t n = 0;
    for (const char c : view) {
      if ((static_cast<unsigned char>(c) & 0xC0) != 0x80) {
        ++n;
      }
    }
    auto res = V::validate(n);
    if (!res) {
      return error("Code point count validation failed: " +
                   res.error().what());
    }
    return std::move(_t);
  }

  /// Converts the code point validator to a JSON schema type. The output is
  /// identical to that of rfl::Size, i.e. minLength/maxLength for strings.
  /// @tparam T The type of the string being validated
  /// @return A ValidationType representing length constraints
  template <class T>
  static parsing::schema::ValidationType to_schema() {
    return Size<V>::template to_schema<T>();
  }
};

}  // namespace rfl

#endif
