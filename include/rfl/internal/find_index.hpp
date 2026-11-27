#ifndef RFL_FIND_INDEX_HPP_
#define RFL_FIND_INDEX_HPP_

#include "../Tuple.hpp"
#include "StringLiteral.hpp"

namespace rfl {
namespace internal {

template <class FieldType, StringLiteral _field_name, int _i>
struct FieldWrapper {
  constexpr static int i_ = _i;
};

template <StringLiteral _field_name, class F1, int _i1, class F2, int _i2>
constexpr auto operator|(const FieldWrapper<F1, _field_name, _i1>& _f1,
                         const FieldWrapper<F2, _field_name, _i2>& _f2) {
  if constexpr (F1::name_ == _field_name) {
    return _f1;
  } else {
    return _f2;
  }
}

template <class Head, class... Tail>
constexpr auto find_matching_field(const Head& _head, const Tail&... _tail) {
  return (_head | ... | _tail);
}

template <StringLiteral _field_name, class Fields, int... _is>
constexpr auto wrap_fields(std::integer_sequence<int, _is...>) {
  return find_matching_field(FieldWrapper<rfl::tuple_element_t<_is, Fields>,
                                          _field_name, _is>{}...)
      .i_;
}

/// Finds the index of the field signified by _field_name
template <StringLiteral _field_name, class Fields>
constexpr int find_index() {
  constexpr int ix = wrap_fields<_field_name, Fields>(
      std::make_integer_sequence<int, rfl::tuple_size_v<Fields>>());
  static_assert(rfl::tuple_element_t<ix, Fields>::name_ == _field_name,
                "No matching field found.");
  return ix;
}

/// Finds the index of the field signified by _field_name or -1.
template <StringLiteral _field_name, class Fields>
constexpr int find_index_or_minus_one() {
  if constexpr (rfl::tuple_size_v<Fields> == 0) {
    return -1;
  } else {
    constexpr int ix = wrap_fields<_field_name, Fields>(
        std::make_integer_sequence<int, rfl::tuple_size_v<Fields>>());
    if constexpr (rfl::tuple_element_t<ix, Fields>::name_ == _field_name) {
      return ix;
    } else {
      return -1;
    }
  }
}

}  // namespace internal
}  // namespace rfl

#endif
