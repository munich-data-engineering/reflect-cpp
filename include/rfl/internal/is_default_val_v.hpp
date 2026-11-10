#ifndef RFL_INTERNAL_ISDEFAULTVAL_HPP_
#define RFL_INTERNAL_ISDEFAULTVAL_HPP_

#include <type_traits>
#include <utility>

#include "../DefaultVal.hpp"
#include "../Deprecated.hpp"
#include "../Description.hpp"
#include "../Positional.hpp"
#include "../Rename.hpp"
#include "../Short.hpp"
#include "StringLiteral.hpp"

namespace rfl::internal {

template <class T>
class is_default_val;

template <class T>
class is_default_val : public std::false_type {};

template <class T>
class is_default_val<DefaultVal<T>> : public std::true_type {};

// The wrappers below do not change how a field is read, so a DefaultVal
// inside them, in any order, still provides the field's default value.

template <internal::StringLiteral _new_name, class T>
class is_default_val<Rename<_new_name, T>> : public is_default_val<T> {};

template <internal::StringLiteral _short_name, class T>
class is_default_val<Short<_short_name, T>> : public is_default_val<T> {};

template <class T>
class is_default_val<Positional<T>> : public is_default_val<T> {};

template <internal::StringLiteral _description, class T>
class is_default_val<Description<_description, T>> : public is_default_val<T> {
};

template <internal::StringLiteral _deprecation_message,
          internal::StringLiteral _description, class T>
class is_default_val<Deprecated<_deprecation_message, _description, T>>
    : public is_default_val<T> {};

template <class T>
constexpr bool is_default_val_v =
    is_default_val<std::remove_cvref_t<std::remove_pointer_t<T>>>::value;

/// Returns the DefaultVal inside a field for which is_default_val_v is true.
template <class T>
const DefaultVal<T>& get_default_val(const DefaultVal<T>& _field) noexcept {
  return _field;
}

template <class T>
  requires is_default_val_v<T>
const auto& get_default_val(const T& _field) noexcept {
  return get_default_val(_field.value());
}

/// The DefaultVal type inside a field for which is_default_val_v is true.
template <class T>
using default_val_t = std::remove_cvref_t<decltype(get_default_val(
    std::declval<const std::remove_cvref_t<T>&>()))>;

}  // namespace rfl::internal

#endif
