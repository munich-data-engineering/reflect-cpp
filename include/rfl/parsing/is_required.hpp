#ifndef RFL_PARSING_IS_REQUIRED_HPP_
#define RFL_PARSING_IS_REQUIRED_HPP_

#include <memory>
#include <optional>
#include <type_traits>

#include "../Generic.hpp"
#include "../Positional.hpp"
#include "../Rename.hpp"
#include "../Short.hpp"
#include "../internal/StringLiteral.hpp"
#include "../internal/has_reflection_type_v.hpp"
#include "../internal/is_positional.hpp"
#include "../internal/is_rename.hpp"
#include "../internal/is_short.hpp"
#include "is_map_like.hpp"
#include "is_monostate_variant.hpp"
#include "is_vector_like.hpp"

namespace rfl {
namespace parsing {

/**
 * @brief Trait to check if a type is never required.
 *
 * @tparam T The type to check.
 */
template <class T>
class is_never_required;

template <class T>
class is_never_required : public std::false_type {};

template <class T>
class is_never_required<std::optional<T>> : public std::true_type {};

template <class T>
class is_never_required<std::shared_ptr<T>> : public std::true_type {};

template <class T>
class is_never_required<std::unique_ptr<T>> : public std::true_type {};

template <class T>
constexpr bool is_never_required_v = is_never_required<T>::value;

/**
 * @brief Checks if a field of type T is required.
 *
 * @tparam T The type of the field.
 * @tparam _ignore_empty_containers Whether to ignore empty containers.
 * @return True if the field is required, false otherwise.
 */
template <class T, bool _ignore_empty_containers>
consteval bool is_required() {
  using Type = std::remove_cvref_t<std::remove_pointer_t<T>>;
  if constexpr (internal::has_reflection_type_v<Type> &&
                !std::is_same_v<Type, rfl::Generic>) {
    return is_required<typename Type::ReflectionType,
                       _ignore_empty_containers>();
  } else if constexpr (internal::is_rename_v<Type>) {
    return is_required<typename Type::Type, _ignore_empty_containers>();
  } else if constexpr (internal::is_positional_v<Type>) {
    return is_required<typename Type::Type, _ignore_empty_containers>();
  } else if constexpr (internal::is_short_v<Type>) {
    return is_required<typename Type::Type, _ignore_empty_containers>();
  } else {
    return !(is_never_required_v<Type> || is_monostate_variant_v<Type> ||
             (_ignore_empty_containers &&
              (is_vector_like_v<Type> || is_map_like_v<Type>)));
  }
}

}  // namespace parsing
}  // namespace rfl

#endif
