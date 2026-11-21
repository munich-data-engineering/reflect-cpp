#ifndef RFL_VIEW_T_HPP_
#define RFL_VIEW_T_HPP_

#include <type_traits>
#include <utility>

#include "Processors.hpp"
#include "internal/ptr_named_tuple_t.hpp"

namespace rfl {

/// Generates the named tuple that would be the result of to_view
template <class T, class... Ps>
using view_t = decltype(Processors<Ps...>::template process<
                          T, internal::ptr_named_tuple_t<T>>(
    std::declval<internal::ptr_named_tuple_t<T>>()));

}  // namespace rfl

#endif
