#ifndef RFL_MOVE_FROM_TUPLE_HPP_
#define RFL_MOVE_FROM_TUPLE_HPP_

#include <type_traits>

#include "../Tuple.hpp"
#include "../tuple_cat.hpp"
#include "Array.hpp"
#include "is_flatten_field.hpp"
#include "is_named_tuple.hpp"
#include "ptr_tuple_t.hpp"
#include "tup_to_ptr_tuple.hpp"
#include "tuple_t.hpp"

namespace rfl::internal {

template <class Tuple>
constexpr int calc_flattened_size() {
  constexpr auto calc_one =
      []<size_t _i>(std::integral_constant<size_t, _i>) -> int {
    using T = std::remove_pointer_t<tuple_element_t<_i, Tuple>>;
    if constexpr (is_flatten_field_v<T>) {
      return calc_flattened_size<ptr_tuple_t<typename T::Type>>();
    } else {
      return 1;
    }
  };
  return [&]<auto... _is>(std::integer_sequence<size_t, _is...>) {
    return (calc_one(std::integral_constant<size_t, _is>{}) + ...);
  }(std::make_integer_sequence<size_t, rfl::tuple_size_v<Tuple>>());
}

// TODO: Non-recursive implementation
template <class TargetTupleType, class PtrTupleType, int _j = 0, class... Args>
auto unflatten_ptr_tuple(PtrTupleType& _t, Args... _args) {
  constexpr auto i = sizeof...(Args);

  constexpr auto size = rfl::tuple_size_v<std::remove_cvref_t<TargetTupleType>>;

  if constexpr (i == size) {
    return rfl::make_tuple(_args...);
  } else {
    using T = std::remove_cvref_t<
        std::remove_pointer_t<tuple_element_t<i, TargetTupleType>>>;

    if constexpr (is_flatten_field_v<T>) {
      using SubTargetTupleType =
          ptr_tuple_t<std::remove_pointer_t<typename T::Type>>;

      constexpr int flattened_size = calc_flattened_size<SubTargetTupleType>();

      return unflatten_ptr_tuple<TargetTupleType, PtrTupleType,
                                 _j + flattened_size>(
          _t, _args...,
          unflatten_ptr_tuple<SubTargetTupleType, PtrTupleType, _j>(_t));

    } else {
      return unflatten_ptr_tuple<TargetTupleType, PtrTupleType, _j + 1>(
          _t, _args..., std::get<_j>(_t));
    }
  }
}

template <class T, class Pointers>
auto move_from_pointers(Pointers& _ptrs) {
  const auto move_one = [&]<size_t _i>(std::integral_constant<size_t, _i>) {
    using FieldType = tuple_element_t<_i, std::remove_cvref_t<Pointers>>;

    if constexpr (std::is_pointer_v<FieldType>) {
      return std::move(*std::get<_i>(_ptrs));

    } else {
      using PtrTupleType = ptr_tuple_t<std::remove_cvref_t<T>>;

      using U = std::remove_cvref_t<typename std::remove_pointer_t<
          tuple_element_t<_i, PtrTupleType>>::Type>;

      return move_from_pointers<U>(std::get<_i>(_ptrs));
    }
  };
  return [&]<auto... _is>(std::integer_sequence<size_t, _is...>) {
    return std::remove_cvref_t<T>{
        move_one(std::integral_constant<size_t, _is>{})...};
  }(std::make_integer_sequence<size_t, rfl::tuple_size_v<Pointers>>());
}

template <class T>
auto flatten_array(T* _v) {
  return rfl::make_tuple(_v);
}

template <class T, std::size_t _n>
auto flatten_array(std::array<T, _n>* _arr) {
  return std::apply(
      [](auto&... _v) { return rfl::tuple_cat(flatten_array(&_v)...); }, *_arr);
}

template <class T>
auto make_tuple_from_element(T _v) {
  return rfl::make_tuple(_v);
}

template <class T>
auto make_tuple_from_element(Array<T>* _arr) {
  return flatten_array(&(_arr->arr_));
}

auto flatten_c_arrays(const auto& _tup) {
  return rfl::apply(
      [](auto... _v) { return rfl::tuple_cat(make_tuple_from_element(_v)...); },
      _tup);
}

/// Creates a struct of type T from a tuple by moving the underlying
/// fields.
template <class T, class TupleType>
auto move_from_tuple(TupleType&& _t) {
  auto ptr_tuple = tup_to_ptr_tuple(_t);

  using TargetTupleType = tuple_t<std::remove_cvref_t<T>>;

  auto pointers =
      flatten_c_arrays(unflatten_ptr_tuple<TargetTupleType>(ptr_tuple));

  return move_from_pointers<T>(pointers);
}

}  // namespace rfl::internal

#endif
