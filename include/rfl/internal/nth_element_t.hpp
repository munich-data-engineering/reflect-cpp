#ifndef RFL_INTERNAL_NTH_ELEMENT_T_HPP_
#define RFL_INTERNAL_NTH_ELEMENT_T_HPP_

#include <utility>

#include "nth_element.hpp"

namespace rfl::internal {

#if defined(__clang__)

template <int N, class... Ts>
using nth_element_t = __type_pack_element<N, Ts...>;

#else

template <class T>
struct TypeWrapper {
  using Type = T;
};

template <int N, class... Ts>
using nth_element_t =
    decltype(nth_element<N, TypeWrapper<Ts>...>(
                 std::declval<TypeWrapper<Ts>>()...))::Type;

#endif

}  // namespace rfl::internal

#endif
