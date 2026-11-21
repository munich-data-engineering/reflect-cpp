#ifndef RFL_INTERNAL_PTR_FIELD_TUPLE_T_HPP_
#define RFL_INTERNAL_PTR_FIELD_TUPLE_T_HPP_

#include <type_traits>
#include <utility>

#include "to_ptr_named_tuple.hpp"

namespace rfl::internal {

template <class T>
using ptr_named_tuple_t =
    decltype(to_ptr_named_tuple<T>(std::declval<T>()));

}  // namespace rfl::internal

#endif
