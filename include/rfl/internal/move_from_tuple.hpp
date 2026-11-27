#ifndef RFL_MOVE_FROM_TUPLE_HPP_
#define RFL_MOVE_FROM_TUPLE_HPP_

#include <array>
#include <type_traits>

#include "../Tuple.hpp"
#include "../tuple_cat.hpp"
#include "Array.hpp"
#include "has_arrays.hpp"
#include "has_flatten_fields.hpp"
#include "is_flatten_field.hpp"
#include "ptr_tuple_t.hpp"
#include "tup_to_ptr_tuple.hpp"
#include "tuple_t.hpp"

namespace rfl::internal {

/**
 * Calculates the flattened size of a tuple.
 * @tparam T The type to calculate the size for.
 * @return The flattened size.
 */
template <class T>
consteval size_t calc_flattened_size() {
  if constexpr (is_flatten_field_v<T>) {
    using Tuple = ptr_tuple_t<typename T::Type>;
    return [&]<auto... _is>(std::integer_sequence<size_t, _is...>) {
      return (calc_flattened_size<tuple_element_t<_is, Tuple>>() + ...);
    }(std::make_integer_sequence<size_t, rfl::tuple_size_v<Tuple>>());
  } else {
    return 1;
  }
}

/**
 * Helper struct for calculating the indptr array.
 * @tparam _head The head of the indptr array.
 * @tparam _tail The tail of the indptr array.
 */
template <size_t _head, size_t... _tail>
struct IndptrHelper {
  using ArrayType = std::array<size_t, sizeof...(_tail) + 1>;
  static constexpr auto indptr_ = ArrayType{_head, _tail...};

  template <size_t... _is>
  consteval auto operator+(const IndptrHelper<_is...>&) const {
    constexpr auto last = std::get<sizeof...(_tail)>(indptr_);
    return IndptrHelper<_head, _tail..., (last + _is)...>{};
  }
};

/**
 * Calculates the indptr array for a tuple.
 * The indptr array is used to map the flattened tuple back to the original
 * tuple structure.
 * @tparam Tuple The tuple type.
 * @tparam _begin The starting index for the indptr.
 * @return The indptr array.
 */
template <class Tuple, size_t _begin>
consteval auto calc_indptr() {
  return [&]<auto... _is>(std::integer_sequence<size_t, _is...>) {
    return (IndptrHelper<_begin>{} + ... +
            IndptrHelper<calc_flattened_size<tuple_element_t<_is, Tuple>>()>{})
        .indptr_;
  }(std::make_integer_sequence<size_t, rfl::tuple_size_v<Tuple>>());
}

/**
 * Unflattens a tuple of pointers into a target tuple. T
 * This is meant for when the uses uses rfl::Flatten<...>.
 * @param _t Tuple of pointers to elements.
 * @return A tuple with unflattened elements.
 */
template <class TargetTupleType, class PtrTupleType, size_t _begin = 0>
auto unflatten_ptr_tuple(PtrTupleType& _t) {
  constexpr auto size = rfl::tuple_size_v<std::remove_cvref_t<TargetTupleType>>;

  constexpr auto indptr = calc_indptr<TargetTupleType, _begin>();

  const auto unflatten_one =
      [&]<size_t _i>(std::integral_constant<size_t, _i>) {
        using T = std::remove_cvref_t<
            std::remove_pointer_t<tuple_element_t<_i, TargetTupleType>>>;

        constexpr size_t j = std::get<_i>(indptr);

        if constexpr (is_flatten_field_v<T>) {
          using SubTargetTupleType =
              ptr_tuple_t<std::remove_pointer_t<typename T::Type>>;

          return unflatten_ptr_tuple<SubTargetTupleType, PtrTupleType, j>(_t);

        } else {
          return std::get<j>(_t);
        }
      };

  return [&]<size_t... _is>(std::integer_sequence<size_t, _is...>) {
    return rfl::make_tuple(
        unflatten_one(std::integral_constant<size_t, _is>{})...);
  }(std::make_integer_sequence<size_t, size>());
}

/**
 * Moves elements from a tuple of pointers to a target type (usually a struct).
 * @param _ptrs Tuple of pointers to elements.
 * @return A target struct with moved elements.
 */
template <class T, class Pointers>
auto move_from_pointers(Pointers&& _ptrs) {
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

/**
 * Flattens a single element into a tuple.
 * @param _v Source element.
 * @return A tuple containing the element.
 *
 */
template <class T>
auto flatten_array(T* _v) {
  return rfl::make_tuple(_v);
}

/**
 * Flattens a multi-dimensional array into a tuple.
 * @param _arr Source array.
 * @return A flattened tuple.
 */
template <class T, std::size_t _n>
auto flatten_array(std::array<T, _n>* _arr) {
  return std::apply(
      [](auto&... _v) { return rfl::tuple_cat(flatten_array(&_v)...); }, *_arr);
}

/**
 * Creates a tuple from a single element.
 * @param _v Source element.
 * @return A tuple containing the element.
 */
template <class T>
auto make_tuple_from_element(T _v) {
  return rfl::make_tuple(_v);
}

/**
 * Flattens an array into a tuple.
 * @param _arr Source array.
 * @return A flattened tuple.
 */
template <class T>
auto make_tuple_from_element(Array<T>* _arr) {
  return flatten_array(&(_arr->arr_));
}

/**
 * Flattens C-style arrays in a tuple.
 * @param _tup Source tuple.
 * @return A flattened tuple.
 */
auto flatten_c_arrays(auto&& _tup) {
  return rfl::apply(
      [](auto... _v) { return rfl::tuple_cat(make_tuple_from_element(_v)...); },
      _tup);
}

/**
 * Moves elements from a tuple to a target object.
 * @param _t Source tuple.
 * @return A target object with the elements moved from the tuple.
 */
template <class T, class TupleType>
auto move_from_tuple(TupleType&& _t) {
  using TargetTupleType = tuple_t<std::remove_cvref_t<T>>;
  auto ptr_tuple = tup_to_ptr_tuple(_t);
  return move_from_pointers<T>(
      flatten_c_arrays(unflatten_ptr_tuple<TargetTupleType>(ptr_tuple)));
}

}  // namespace rfl::internal

#endif
