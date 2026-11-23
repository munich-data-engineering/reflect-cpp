#ifndef RFL_INTERNAL_GETMETAFIELDS_HPP_
#define RFL_INTERNAL_GETMETAFIELDS_HPP_

#include <array>
#include <type_traits>
#include <utility>

#include "../MetaField.hpp"
#include "../Tuple.hpp"
#include "../type_name_t.hpp"

namespace rfl::internal {

template <class NamedTupleType, class... AlreadyExtracted>
auto get_meta_fields() {
  const auto get_one = [&]<size_t _i>(std::integral_constant<size_t, _i>) {
    using FieldType = tuple_element_t<_i, typename NamedTupleType::Fields>;
    auto name = typename FieldType::Name().str();
    auto type = type_name_t<typename FieldType::Type>().str();
    return MetaField(std::move(name), std::move(type));
  };

  constexpr size_t size = NamedTupleType::size();

  return [&]<size_t... _i>(std::index_sequence<_i...>) {
    return std::array<MetaField, size>{
        get_one(std::integral_constant<size_t, _i>{})...};
  }(std::make_index_sequence<size>{});
}

}  // namespace rfl::internal

#endif
