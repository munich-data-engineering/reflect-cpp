#ifndef RFL_PARSING_IS_MONOSTATE_HPP_
#define RFL_PARSING_IS_MONOSTATE_HPP_

#include <type_traits>
#include <variant>

namespace rfl::parsing {

template <class T>
struct is_monostate : std::false_type {};

template <>
struct is_monostate<std::monostate> : std::true_type {};

template <class T>
constexpr bool is_monostate_v = is_monostate<std::remove_cvref_t<T>>::value;

}  // namespace rfl::parsing

#endif
