#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <rfl/NoFieldNames.hpp>
#include <rfl/boost_serialization.hpp>
#include <sstream>
#include <unordered_map>

#include "../check_bitset.hpp"

namespace test_bitset_boost {

constexpr auto flags = boost::archive::no_header | boost::archive::no_tracking;
const auto write = [](const auto& value) {
  return rfl::boost_serialization::write(value);
};
const auto read = []<class T>(const auto& bytes) {
  return rfl::boost_serialization::read<T>(bytes);
};
const auto native_write = [](const auto& value) {
  std::ostringstream stream;
  {
    boost::archive::binary_oarchive archive(stream, flags);
    archive << value;
  }
  const auto bytes = stream.str();
  return std::vector<char>(bytes.begin(), bytes.end());
};
const auto native_read = []<class T>(const auto& bytes) {
  std::istringstream stream(std::string(bytes.begin(), bytes.end()));
  boost::archive::binary_iarchive archive(stream, flags);
  T value;
  archive >> value;
  return value;
};

TEST(boost_serialization, bitset_native_interop) {
  test_bitset_formats::check_native_sizes(write, read, native_write,
                                          native_read);
}

TEST(boost_serialization, bitset_nested) {
  test_bitset_formats::check_nested(write, read);
  test_bitset_formats::check_nested(
      [](const auto& value) {
        return rfl::boost_serialization::write<rfl::NoFieldNames>(value);
      },
      []<class T>(const auto& bytes) {
        return rfl::boost_serialization::read<T, rfl::NoFieldNames>(bytes);
      });
  test_bitset_formats::roundtrip(std::map<int, std::bitset<9>>{{7, {258}}},
                                 write, read);
  test_bitset_formats::roundtrip(
      std::unordered_map<std::bitset<9>, int>{{{258}, 7}}, write, read);
}

TEST(boost_serialization, bitset_invalid_character) {
  auto bytes = write(std::bitset<9>{258});
  bytes.back() = 'x';
  EXPECT_FALSE(rfl::boost_serialization::read<std::bitset<9>>(bytes));
}

TEST(boost_serialization, bitset_text_archive) {
  const std::bitset<9> bits{258};
  std::ostringstream reflected;
  {
    boost::archive::text_oarchive archive(reflected);
    rfl::boost_serialization::write<boost::archive::text_iarchive>(archive,
                                                                   bits);
  }
  std::ostringstream native;
  {
    boost::archive::text_oarchive archive(native);
    archive << bits;
  }
  EXPECT_EQ(reflected.str(), native.str());
  std::istringstream stream(native.str());
  boost::archive::text_iarchive archive(stream);
  const auto decoded = rfl::boost_serialization::read_from_archive<
      std::bitset<9>, boost::archive::text_iarchive,
      boost::archive::text_oarchive>(archive);
  ASSERT_TRUE(decoded) << decoded.error().what();
  EXPECT_EQ(*decoded, bits);
}

}  // namespace test_bitset_boost
