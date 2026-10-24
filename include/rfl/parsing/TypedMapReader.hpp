#ifndef RFL_PARSING_TYPEDMAPREADER_HPP_
#define RFL_PARSING_TYPEDMAPREADER_HPP_

#include <exception>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>

#include "../Result.hpp"
#include "Parser_base.hpp"

namespace rfl::parsing {

/// Parses map keys as values, preserving their serialized types.
template <class R, class W, class MapType, class ProcessorsType>
struct TypedMapReader {
  using InputVarType = typename R::InputVarType;
  using Key = std::remove_cvref_t<typename MapType::value_type::first_type>;
  using Value = std::remove_cvref_t<typename MapType::value_type::second_type>;

  const R* reader;
  MapType* map;

  std::optional<Error> read(const InputVarType& _key,
                            const InputVarType& _value) const noexcept {
    // Archive readers can pass the same stream for both arguments. Always
    // finish reading the key before starting the value, and stop on failure.
    auto key = Parser<R, W, Key, ProcessorsType>::read(*reader, _key);
    if (!key) {
      return Error("Failed to parse map key: " + key.error().what());
    }
    auto value = Parser<R, W, Value, ProcessorsType>::read(*reader, _value);
    if (!value) {
      return Error("Failed to parse map value: " + value.error().what());
    }
    try {
      map->emplace(std::move(*key), std::move(*value));
      return std::nullopt;
    } catch (const std::exception& e) {
      return Error(e.what());
    }
  }
};

}  // namespace rfl::parsing

#endif
