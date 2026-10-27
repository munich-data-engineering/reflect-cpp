#ifndef RFL_INTERNAL_IS_BITSET_HPP_
#define RFL_INTERNAL_IS_BITSET_HPP_

#include <bitset>
#include <cstddef>
#include <type_traits>

namespace rfl::internal {

template <class T>
struct is_bitset : std::false_type {};

template <std::size_t N>
struct is_bitset<std::bitset<N>> : std::true_type {};

template <class T>
inline constexpr bool is_bitset_v = is_bitset<std::remove_cvref_t<T>>::value;

}  // namespace rfl::internal

#endif
