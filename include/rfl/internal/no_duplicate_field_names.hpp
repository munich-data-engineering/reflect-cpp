#ifndef RFL_INTERNAL_NO_DUPLICATE_FIELD_NAMES_HPP_
#define RFL_INTERNAL_NO_DUPLICATE_FIELD_NAMES_HPP_

#include <array>
#include <cstddef>
#include <string_view>
#include <type_traits>
#include <utility>

#include "../Tuple.hpp"

namespace rfl {
namespace internal {

template <class Fields>
constexpr inline bool no_duplicate_field_names() {
  constexpr std::size_t num_fields = rfl::tuple_size_v<Fields>;

  if constexpr (num_fields <= 1) {
    return true;
  } else {
    constexpr bool result =
        []<std::size_t... _is>(std::index_sequence<_is...>) {
          std::array<std::string_view, sizeof...(_is)> arr{
              std::remove_cvref_t<tuple_element_t<_is, Fields>>::name_
                  .string_view()...};
          for (std::size_t gap = arr.size() / 2; gap > 0; gap /= 2) {
            for (std::size_t i = gap; i < arr.size(); ++i) {
              const auto key = arr[i];
              std::size_t j = i;
              while (j >= gap && key < arr[j - gap]) {
                arr[j] = arr[j - gap];
                j -= gap;
              }
              arr[j] = key;
            }
          }
          for (std::size_t i = 1; i < arr.size(); ++i) {
            if (arr[i] == arr[i - 1]) {
              return false;
            }
          }
          return true;
        }(std::make_index_sequence<num_fields>());
    return result;
  }
}

}  // namespace internal
}  // namespace rfl

#endif
