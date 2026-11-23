#ifndef RFL_INTERNAL_TUP_TO_PTR_TUPLE_HPP_
#define RFL_INTERNAL_TUP_TO_PTR_TUPLE_HPP_

#include "../Field.hpp"
#include "../Tuple.hpp"
#include "../apply.hpp"
#include "../make_named_tuple.hpp"

namespace rfl::internal {

/// Generates a named tuple that contains pointers to the original values in
/// the struct from a tuple.
template <class TupleType>
auto tup_to_ptr_tuple(TupleType& _t) {
  return rfl::apply(
      [](auto&... _fields) { return rfl::make_tuple(&_fields...); }, _t);
}

}  // namespace rfl::internal

#endif
