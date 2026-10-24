#include "../test_typed_maps.hpp"

#include <cereal/types/map.hpp>
#include <rfl/cereal.hpp>
#include <sstream>

TEST(cereal, typed_map_keys) {
  test_typed_maps::check(
      [](const auto& value) { return rfl::cereal::write(value); },
      []<class T>(const auto& bytes) { return rfl::cereal::read<T>(bytes); });
}

TEST(cereal, typed_map_wire_format) {
  const std::map<double, int> values{{1.0, 3}, {std::nextafter(1.0, 2.0), 4}};
  std::ostringstream stream;
  {
    cereal::PortableBinaryOutputArchive archive(stream);
    archive(values);
  }
  const auto native = stream.str();
  const std::vector<char> expected(native.begin(), native.end());
  EXPECT_EQ(rfl::cereal::write(values), expected);
  const auto decoded =
      rfl::cereal::read<std::remove_cvref_t<decltype(values)>>(expected);
  ASSERT_TRUE(decoded) << decoded.error().what();
  EXPECT_EQ(*decoded, values);
}

TEST(cereal, typed_map_bytestring_values) {
  test_typed_maps::roundtrip(
      [](const auto& value) { return rfl::cereal::write(value); },
      []<class T>(const auto& bytes) { return rfl::cereal::read<T>(bytes); },
      std::map<int, rfl::Bytestring>{{1, {std::byte{0}, std::byte{255}}}});
}
