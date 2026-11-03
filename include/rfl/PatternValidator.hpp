#ifndef RFL_PATTERNVALIDATOR_HPP_
#define RFL_PATTERNVALIDATOR_HPP_

#include <sstream>
#include <string>
#include <utility>

#if __has_include(<ctre.hpp>)
#include <ctre.hpp>
#else
#include "thirdparty/ctre.hpp"
#endif

#include "Literal.hpp"
#include "Result.hpp"
#include "internal/StringLiteral.hpp"
#include "parsing/schema/ValidationType.hpp"

namespace rfl {

/// Validator that checks if a string matches a compile-time regex pattern.
/// Uses the CTRE library for compile-time regular expression matching.
/// @tparam _regex The compile-time regular expression pattern
/// @tparam _name A human-readable name for the pattern (used in error messages)
template <internal::StringLiteral _regex, internal::StringLiteral _name>
struct PatternValidator {
  using Name = Literal<_name>;
  using Regex = Literal<_regex>;

  /// Validates that a string matches the regex pattern.
  /// @param _str The string to validate
  /// @return Result containing the string if it matches, or an error if it doesn't
  static Result<std::string> validate(std::string _str) noexcept {
    if (ctre::match<ctll::fixed_string<_regex.length>{
            ctll::construct_from_pointer, _regex.arr_.data()}>(_str)) {
      return std::move(_str);
    } else {
      std::stringstream stream;
      stream << "String '" << _str << "' did not match format '" << _name.str()
             << "': '" << _regex.str() << "'.";
      return error(stream.str());
    }
  }

  /// Converts the pattern validator to a JSON schema type.
  /// @tparam T The type being validated (unused, for template compatibility)
  /// @return A ValidationType representing the regex constraint
  template <class T>
  static parsing::schema::ValidationType to_schema() {
    using ValidationType = parsing::schema::ValidationType;
    return ValidationType{ValidationType::Regex{.pattern_ = Regex().str()}};
  }
};

}  // namespace rfl

#endif
