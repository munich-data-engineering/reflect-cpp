#ifndef RFL_PARSING_IS_EMPTY_HPP_
#define RFL_PARSING_IS_EMPTY_HPP_

#include <type_traits>

#include "../internal/has_reflection_method_v.hpp"
#include "../internal/has_reflection_type_v.hpp"
#include "../internal/is_rename.hpp"
#include "../internal/is_skip.hpp"
#include "is_map_like.hpp"
#include "is_monostate_variant.hpp"
#include "is_vector_like.hpp"

namespace rfl {
namespace parsing {

/**
 * @brief Checks if a variable is empty.
 *
 * @tparam T The type of the variable.
 * @param _var The variable to check.
 * @return True if the variable is empty, false otherwise.
 */
template <class T>
static bool is_empty(const T& _var) {
  using Type = std::remove_cvref_t<T>;
  if constexpr (std::is_pointer_v<Type>) {
    return !_var || is_empty(*_var);
  } else if constexpr (internal::is_skip_v<Type>) {
    return Type::skip_serialization_;
  } else if constexpr (internal::has_reflection_type_v<Type>) {
    if constexpr (internal::has_reflection_method_v<Type>) {
      return is_empty(_var.reflection());
    } else {
      const auto& [r] = _var;
      return is_empty(r);
    }
  } else if constexpr (internal::is_rename_v<Type>) {
    return is_empty(_var.value());
  } else if constexpr (is_monostate_variant_v<Type>) {
    return _var.index() == 0;
  } else if constexpr (is_map_like_v<Type> || is_vector_like_v<Type>) {
    return _var.begin() == _var.end();
  } else {
    return !_var;
  }
}

}  // namespace parsing
}  // namespace rfl

#endif
