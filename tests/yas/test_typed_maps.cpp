#include "../test_typed_maps.hpp"

#include <rfl/yas.hpp>

TEST(yas, typed_map_keys) {
  test_typed_maps::check(
      [](const auto& value) { return rfl::yas::write(value); },
      []<class T>(const auto& bytes) { return rfl::yas::read<T>(bytes); });
}

TEST(yas, typed_map_wire_format) {
  const std::map<int, int> values{{-1, 3}, {42, 4}};
  yas::mem_ostream stream;
  yas::binary_oarchive<yas::mem_ostream> archive(stream);
  const std::size_t size = values.size();
  archive & size;
  for (const auto& [key, value] : values) {
    archive & key & value;
  }
  const auto buffer = stream.get_intrusive_buffer();
  const std::vector<char> expected(buffer.data, buffer.data + buffer.size);
  EXPECT_EQ(rfl::yas::write(values), expected);
  const auto decoded = rfl::yas::read<std::map<int, int>>(expected);
  ASSERT_TRUE(decoded) << decoded.error().what();
  EXPECT_EQ(*decoded, values);
}
