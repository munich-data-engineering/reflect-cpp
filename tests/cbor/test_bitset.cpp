#include <jsoncons_ext/cbor/cbor.hpp>
#include <rfl/cbor.hpp>

#include "../check_bitset.hpp"

namespace test_bitset_cbor {

const auto write = [](const auto& value) { return rfl::cbor::write(value); };
const auto read = []<class T>(const auto& value) {
  return rfl::cbor::read<T>(value);
};
const auto native_write = [](const auto& value) {
  std::vector<char> bytes;
  jsoncons::cbor::encode_cbor(value, bytes);
  return bytes;
};
const auto native_read = []<class T>(const auto& bytes) {
  return jsoncons::cbor::decode_cbor<T>(bytes);
};

TEST(cbor, bitset_native_interop) {
  test_bitset_formats::check_native_sizes(write, read, native_write,
                                          native_read);
}

TEST(cbor, bitset_nested) { test_bitset_formats::check_nested(write, read); }

TEST(cbor, bitset_native_encoding_and_validation) {
  // Tag 23 (expected base16 conversion), two bytes, bits 1 and 8 set.
  const std::vector<char> expected{char(0xd7), char(0x42), char(0x40),
                                   char(0x80)};
  EXPECT_EQ(write(std::bitset<9>{258}), expected);
  EXPECT_EQ(write(std::bitset<256>{}).size(), 35u);

  for (const auto& value : {jsoncons::json(false), jsoncons::json("not hex"),
                            jsoncons::json(jsoncons::byte_string_arg,
                                           std::vector<uint8_t>{0x80})}) {
    EXPECT_FALSE(rfl::cbor::read<std::bitset<9>>(native_write(value)));
  }
  // Keep native acceptance of integers, hexadecimal strings, and byte strings,
  // including ignored trailing bytes/padding; do not impose a different codec.
  for (const auto& value :
       {jsoncons::json(258), jsoncons::json("4080"),
        jsoncons::json(jsoncons::byte_string_arg,
                       std::vector<uint8_t>{0x40, 0xff, 0xaa})}) {
    const auto bytes = native_write(value);
    const auto decoded = rfl::cbor::read<std::bitset<9>>(bytes);
    ASSERT_TRUE(decoded) << decoded.error().what();
    EXPECT_EQ(*decoded, native_read.template operator()<std::bitset<9>>(bytes));
  }
}

}  // namespace test_bitset_cbor
