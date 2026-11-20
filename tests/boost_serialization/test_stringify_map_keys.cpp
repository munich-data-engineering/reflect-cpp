#include "../test_stringify_map_keys.hpp"

#include <rfl/boost_serialization.hpp>

namespace test_stringify_map_keys {
TEST(boost_serialization, stringify_map_keys_roundtrip) {
  check(
      [](const auto& value) {
        return rfl::boost_serialization::write<rfl::StringifyMapKeys>(value);
      },
      []<class T>(const auto& value) {
        return rfl::boost_serialization::read<T, rfl::StringifyMapKeys>(value);
      });
}
}  // namespace test_stringify_map_keys
