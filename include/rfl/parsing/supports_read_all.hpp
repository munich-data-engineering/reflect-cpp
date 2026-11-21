#ifndef RFL_PARSING_SUPPORTS_READ_ALL_HPP_
#define RFL_PARSING_SUPPORTS_READ_ALL_HPP_

#include <optional>
#include <type_traits>

namespace rfl::parsing {

template <class T>
concept supports_read_all = requires(T t) {
  { T::size() } -> std::convertible_to<size_t>;
  { t.read_all() } -> std::convertible_to<std::optional<Error>>;
};

}  // namespace rfl::parsing

#endif
