#include "../test_typed_maps.hpp"

#include <rfl/msgpack.hpp>

TEST(msgpack, typed_map_keys) {
  test_typed_maps::check(
      [](const auto& value) { return rfl::msgpack::write(value); },
      []<class T>(const auto& bytes) { return rfl::msgpack::read<T>(bytes); });
}

TEST(msgpack, typed_map_wire_format) {
  const std::map<int, int> values{{-1, 3}, {1, 2}};
  const std::vector<char> expected{char(0x82), char(0xff), 3, 1, 2};
  EXPECT_EQ(rfl::msgpack::write(values), expected);
  const auto decoded =
      rfl::msgpack::read<std::remove_cvref_t<decltype(values)>>(expected);
  ASSERT_TRUE(decoded) << decoded.error().what();
  EXPECT_EQ(*decoded, values);

  // Numeric map keys must be numbers on the wire, not numeric strings.
  const auto strings =
      rfl::msgpack::write(std::map<std::string, int>{{"1", 2}});
  EXPECT_FALSE((rfl::msgpack::read<std::map<int, int>>(strings)));
  EXPECT_FALSE((rfl::msgpack::read<std::map<std::string, int>>(expected)));
}
