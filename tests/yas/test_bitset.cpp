#include <rfl/NoFieldNames.hpp>
#include <rfl/yas.hpp>
#include <unordered_map>

#include "../check_bitset.hpp"

namespace test_bitset_yas {

const auto write = [](const auto& value) { return rfl::yas::write(value); };
const auto read = []<class T>(const auto& value) {
  return rfl::yas::read<T>(value);
};
const auto native_write = [](const auto& value) {
  yas::mem_ostream stream;
  yas::binary_oarchive<yas::mem_ostream> archive(stream);
  archive & value;
  const auto bytes = stream.get_intrusive_buffer();
  return std::vector<char>(bytes.data, bytes.data + bytes.size);
};
const auto native_read = []<class T>(const auto& bytes) {
  yas::mem_istream stream(bytes.data(), bytes.size());
  yas::binary_iarchive<yas::mem_istream> archive(stream);
  T value;
  archive & value;
  return value;
};

TEST(yas, bitset_native_interop) {
  test_bitset_formats::check_native_sizes(write, read, native_write,
                                          native_read);
}

TEST(yas, bitset_nested) {
  test_bitset_formats::check_nested(write, read);
  test_bitset_formats::check_nested(
      [](const auto& value) {
        return rfl::yas::write<rfl::NoFieldNames>(value);
      },
      []<class T>(const auto& bytes) {
        return rfl::yas::read<T, rfl::NoFieldNames>(bytes);
      });
  test_bitset_formats::roundtrip(std::map<int, std::bitset<9>>{{7, {258}}},
                                 write, read);
  test_bitset_formats::roundtrip(
      std::unordered_map<std::bitset<9>, int>{{{258}, 7}}, write, read);
}

TEST(yas, bitset_invalid_size) {
  EXPECT_FALSE(rfl::yas::read<std::bitset<9>>(native_write(std::bitset<8>{})));
  EXPECT_FALSE(rfl::yas::read<std::bitset<0>>(native_write(std::bitset<1>{})));
}

}  // namespace test_bitset_yas
