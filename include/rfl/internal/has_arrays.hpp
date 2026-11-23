#ifndef RFL_INTERNAL_HASARRAYS_HPP_
#define RFL_INTERNAL_HASARRAYS_HPP_

#include <type_traits>
#include <utility>

#include "../Tuple.hpp"
#include "is_array.hpp"

namespace rfl::internal {

template <class TupleType>
constexpr bool has_arrays() {
  const auto is_true_for_one =
      []<int _i>(std::integral_constant<int, _i>) -> bool {
    using T = std::remove_cvref_t<tuple_element_t<_i, TupleType>>;
    return is_array_v<T>;
  };

  return [&]<int... _is>(std::integer_sequence<int, _is...>) {
    return (false || ... ||
            is_true_for_one(std::integral_constant<int, _is>{}));
  }(std::make_integer_sequence<int, rfl::tuple_size_v<TupleType>>());
}

}  // namespace rfl::internal

#endif
