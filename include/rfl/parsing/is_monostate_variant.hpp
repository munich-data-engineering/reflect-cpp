#ifndef RFL_PARSING_IS_MONOSTATE_VARIANT_HPP_
#define RFL_PARSING_IS_MONOSTATE_VARIANT_HPP_

#include <type_traits>
#include <variant>

#include "../Variant.hpp"
#include "is_monostate.hpp"

namespace rfl::parsing {

template <class T>
struct is_monostate_variant : std::false_type {};

template <class T, class... Ts>
struct is_monostate_variant<std::variant<T, Ts...>>
    : std::bool_constant<is_monostate_v<T>> {};

template <class T, class... Ts>
struct is_monostate_variant<rfl::Variant<T, Ts...>>
    : std::bool_constant<is_monostate_v<T>> {};

template <class T>
constexpr bool is_monostate_variant_v =
    is_monostate_variant<std::remove_cvref_t<T>>::value;

}  // namespace rfl::parsing

#endif
