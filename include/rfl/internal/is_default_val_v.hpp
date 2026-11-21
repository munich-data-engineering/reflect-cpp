#ifndef RFL_INTERNAL_ISDEFAULTVAL_HPP_
#define RFL_INTERNAL_ISDEFAULTVAL_HPP_

#include <type_traits>

#include "../DefaultVal.hpp"
#include "../Rename.hpp"
#include "StringLiteral.hpp"

namespace rfl::internal {

template <class T>
class is_default_val;

template <class T>
class is_default_val : public std::false_type {};

template <class T>
class is_default_val<DefaultVal<T>> : public std::true_type {};

template <class T, internal::StringLiteral _new_name>
class is_default_val<Rename<_new_name, DefaultVal<T>>> : public std::true_type {
};

template <class T>
constexpr bool is_default_val_v =
    is_default_val<std::remove_cvref_t<std::remove_pointer_t<T>>>::value;

}  // namespace rfl::internal

#endif
