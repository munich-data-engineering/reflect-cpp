#ifndef RFL_SIZE_HPP_
#define RFL_SIZE_HPP_

#include <map>
#include <utility>

#include "Ref.hpp"
#include "Result.hpp"
#include "parsing/schema/ValidationType.hpp"

namespace rfl {

/// Validates the size of a container or string using a validator V.
/// The validator V is applied to the size() method of the container.
template <class V>
struct Size {
  /// Validates that the size of the container satisfies the validator V.
  /// @tparam T The type of the container to validate
  /// @param _t The container whose size will be validated
  /// @return The container if validation passes, otherwise an error
  template <class T>
  static rfl::Result<T> validate(T _t) {
    auto res = V::validate(_t.size());
    if (!res) {
      return error("Size validation failed: " + res.error().what());
    }
    return std::move(_t);
  }

  /// Converts the size validator to a JSON schema type.
  /// @tparam T The type of the container being validated
  /// @return A ValidationType representing size constraints
  template <class T>
  static parsing::schema::ValidationType to_schema() {
    using ValidationType = parsing::schema::ValidationType;
    return ValidationType{ValidationType::Size{
        .size_limit_ =
            rfl::Ref<ValidationType>::make(V::template to_schema<size_t>())}};
  }
};

}  // namespace rfl

#endif
