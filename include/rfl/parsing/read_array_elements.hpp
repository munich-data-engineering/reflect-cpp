#ifndef RFL_PARSING_READ_ARRAY_ELEMENTS_HPP_
#define RFL_PARSING_READ_ARRAY_ELEMENTS_HPP_

#include <cstddef>
#include <optional>

#include "../Result.hpp"

namespace rfl::parsing {

/// Reads elements from a sequential archive after its extent is known.
/// Fixed-size readers supply their compile-time extent; dynamic readers first
/// consume the length prefix. Each element advances the archive in _var.
template <class ArrayReader, class InputVarType>
std::optional<Error> read_array_elements(const ArrayReader& _array_reader,
                                         const InputVarType& _var,
                                         const std::size_t _size) {
  if constexpr (requires { _array_reader.reserve(_size); }) {
    if (const auto err = _array_reader.reserve(_size)) {
      return err;
    }
  }
  for (std::size_t i = 0; i < _size; ++i) {
    if (const auto err = _array_reader.read(_var)) {
      return err;
    }
  }
  return std::nullopt;
}

}  // namespace rfl::parsing

#endif
