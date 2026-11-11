#include "../test_stringify_map_keys.hpp"

#include <rfl/generic/read.hpp>
#include <rfl/generic/write.hpp>

namespace test_stringify_map_keys {
TEST(generic, stringify_map_keys_roundtrip) {
  check(
      [](const auto& value) {
        return rfl::generic::write<rfl::StringifyMapKeys>(value);
      },
      []<class T>(const auto& value) {
        return rfl::generic::read<T, rfl::StringifyMapKeys>(value);
      });
}
}  // namespace test_stringify_map_keys
