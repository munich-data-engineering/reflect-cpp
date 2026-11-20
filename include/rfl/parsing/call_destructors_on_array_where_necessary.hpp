
#ifndef RFL_PARSING_CALL_DESTRUCTORS_ON_ARRAY_WHERE_NECESSARY_HPP_
#define RFL_PARSING_CALL_DESTRUCTORS_ON_ARRAY_WHERE_NECESSARY_HPP_

#include <array>
#include <type_traits>

#include "call_destructors_where_necessary.hpp"

namespace rfl::parsing {

/**
 * @brief Calls the destructors on the elements of an array, if necessary.
 *
 * @tparam T The type of the elements in the array.
 * @tparam _size The size of the array.
 * @param _num_set The number of elements that have been set.
 * @param _array The array to call the destructors on.
 */
template <class T, size_t _size>
void call_destructors_on_array_where_necessary(const size_t _num_set,
                                               std::array<T, _size>* _array) {
  for (size_t i = 0; i < std::min(_num_set, _size); ++i) {
    if constexpr (!std::is_array_v<T> &&
                  std::is_destructible_v<std::remove_cv_t<T>>) {
      (*_array)[i].~T();
    } else if constexpr (std::is_array_v<T>) {
      auto ptr = (*_array)[i];
      call_destructor_on_array(sizeof(*ptr) / sizeof(**ptr), *ptr);
    }
  }
}

}  // namespace rfl::parsing

#endif
