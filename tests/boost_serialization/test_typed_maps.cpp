#include "../test_typed_maps.hpp"

#include <rfl/boost_serialization.hpp>
#include <sstream>

TEST(boost_serialization, typed_map_keys) {
  test_typed_maps::check(
      [](const auto& value) { return rfl::boost_serialization::write(value); },
      []<class T>(const auto& bytes) {
        return rfl::boost_serialization::read<T>(bytes);
      });
}

TEST(boost_serialization, typed_map_wire_format) {
  const std::map<int, int> values{{-1, 3}, {42, 4}};
  std::ostringstream stream;
  {
    boost::archive::binary_oarchive archive(
        stream, boost::archive::no_header | boost::archive::no_tracking);
    const std::uint64_t size = values.size();
    archive << size;
    for (const auto& [key, value] : values) {
      archive << key << value;
    }
  }
  const auto native = stream.str();
  const std::vector<char> expected(native.begin(), native.end());
  EXPECT_EQ(rfl::boost_serialization::write(values), expected);
  const auto decoded =
      rfl::boost_serialization::read<std::map<int, int>>(expected);
  ASSERT_TRUE(decoded) << decoded.error().what();
  EXPECT_EQ(*decoded, values);
}
