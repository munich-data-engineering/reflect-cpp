#ifndef RFL_INTERNAL_STRINGIFYMAPKEYS_HPP_
#define RFL_INTERNAL_STRINGIFYMAPKEYS_HPP_

#include <type_traits>

#include "../Processors.hpp"
#include "../StringifyMapKeys.hpp"

namespace rfl::internal {

template <class T>
class stringify_map_keys;

template <class T>
class stringify_map_keys : public std::false_type {};

template <>
class stringify_map_keys<StringifyMapKeys> : public std::true_type {};

template <class Head, class... Tail>
struct stringify_map_keys<Processors<Head, Tail...>> {
  static constexpr bool value = (stringify_map_keys<Head>::value || ... ||
                                 stringify_map_keys<Tail>::value);
};

template <class T>
constexpr bool stringify_map_keys_v =
    stringify_map_keys<std::remove_cvref_t<std::remove_pointer_t<T>>>::value;

}  // namespace rfl::internal

#endif
