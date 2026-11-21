#ifndef RFL_INTERNAL_PROCESSED_T_HPP_
#define RFL_INTERNAL_PROCESSED_T_HPP_

#include <type_traits>
#include <utility>

#include "../Processors.hpp"
#include "ptr_named_tuple_t.hpp"
#include "remove_ptrs_nt.hpp"

namespace rfl::internal {

template <class StructType, class ProcessorsType>
struct Processed;

template <class StructType, class... Ps>
struct Processed<StructType, Processors<Ps...>> {
  using PtrNamedTupleType = ptr_named_tuple_t<StructType>;
  using type = typename remove_ptrs_nt<
      decltype(Processors<Ps...>::template process<StructType, PtrNamedTupleType>(
          std::declval<PtrNamedTupleType>()))>::NamedTupleType;
};

template <class StructType, class ProcessorsType>
using processed_t = typename Processed<StructType, ProcessorsType>::type;

}  // namespace rfl::internal

#endif
