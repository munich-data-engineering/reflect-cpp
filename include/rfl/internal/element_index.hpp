#ifndef RFL_INTERNAL_ELEMENT_INDEX_HPP_
#define RFL_INTERNAL_ELEMENT_INDEX_HPP_

#include <type_traits>
#include <utility>

namespace rfl::internal {

template <class T1, class T2, int _i>
struct ElementIndexWrapper {
  static constexpr int index = _i;

  consteval auto operator||(auto other) const {
    if constexpr (std::is_same_v<T1, T2>) {
      return *this;
    } else {
      return other;
    }
  }
};

/**
 * Finds the index of the first type T in the list of AlternativeTypes.
 * If T is not found, returns -1.
 */
template <class T, class... AlternativeTypes>
consteval int element_index() {
  return []<auto... _is>(std::integer_sequence<int, _is...>) {
    return ((ElementIndexWrapper<std::remove_cvref_t<T>,
                                 std::remove_cvref_t<AlternativeTypes>,
                                 _is>{} ||
             ...) ||
            ElementIndexWrapper<T, T, -1>{})
        .index;
  }(std::make_integer_sequence<int, sizeof...(AlternativeTypes)>());
}

}  // namespace rfl::internal

#endif
