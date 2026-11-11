#ifndef RFL_PARSING_STRINGMAPKEY_HPP_
#define RFL_PARSING_STRINGMAPKEY_HPP_

#include <exception>
#include <string>
#include <type_traits>

#include "../Result.hpp"
#include "../always_false.hpp"
#include "../enums.hpp"
#include "../internal/enum_names_only_v.hpp"
#include "../internal/has_reflector.hpp"
#include "../internal/stringify_map_keys_v.hpp"
#include "../internal/underlying_enums_v.hpp"

namespace rfl::parsing {

/// Converts the additional key types enabled by StringifyMapKeys. Keep the
/// reflector ahead of enum conversion, just as the ordinary value parser does.
template <class Key, class ProcessorsType>
struct StringMapKey {
  static constexpr bool enabled = [] {
    if constexpr (!internal::stringify_map_keys_v<ProcessorsType>) {
      return false;
    } else if constexpr (internal::has_read_reflector<Key> ||
                         internal::has_write_reflector<Key>) {
      return std::is_same_v<typename Reflector<Key>::ReflType, std::string>;
    } else {
      return std::is_enum_v<Key> &&
             !internal::underlying_enums_v<ProcessorsType>;
    }
  }();

  static Result<Key> read(const std::string& _name) noexcept {
    if constexpr (internal::has_read_reflector<Key>) {
      try {
        return Reflector<Key>::to(_name);
      } catch (const std::exception& e) {
        return error(e.what());
      }
    } else if constexpr (std::is_enum_v<Key>) {
      return rfl::string_to_enum<Key,
                                 internal::enum_names_only_v<ProcessorsType>>(
          _name);
    } else {
      static_assert(
          always_false_v<Key>,
          "Reading string-reflected map keys requires Reflector::to.");
    }
  }

  static std::string write(const Key& _key) {
    if constexpr (internal::has_write_reflector<Key>) {
      return Reflector<Key>::from(_key);
    } else if constexpr (std::is_enum_v<Key>) {
      return rfl::enum_to_string(_key);
    } else {
      static_assert(
          always_false_v<Key>,
          "Writing string-reflected map keys requires Reflector::from.");
    }
  }
};

}  // namespace rfl::parsing

#endif
