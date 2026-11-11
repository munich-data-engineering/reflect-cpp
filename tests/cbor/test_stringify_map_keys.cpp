#include "../test_stringify_map_keys.hpp"

#include <rfl/cbor.hpp>

namespace test_stringify_map_keys {
TEST(cbor, stringify_map_keys_roundtrip) {
  check(
      [](const auto& value) {
        return rfl::cbor::write<rfl::StringifyMapKeys>(value);
      },
      []<class T>(const auto& value) {
        return rfl::cbor::read<T, rfl::StringifyMapKeys>(value);
      });
}
}  // namespace test_stringify_map_keys
