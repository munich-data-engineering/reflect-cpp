#ifndef RFL_INTERNAL_ELEMENT_INDEX_HPP_
#define RFL_INTERNAL_ELEMENT_INDEX_HPP_

#include <type_traits>

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

template <class T, class... AlternativeTypes>
consteval int element_index() {
  return []<auto... _is>(std::integer_sequence<size_t, _is...>) {
    return ((ElementIndexWrapper<std::remove_cvref_t<T>,
                                 std::remove_cvref_t<AlternativeTypes>,
                                 _is>{} ||
             ...) ||
            ElementIndexWrapper<T, T, -1>{})
        .index;
  }(std::make_integer_sequence<size_t, sizeof...(AlternativeTypes)>());
}

}  // namespace rfl::internal

#endif
