#include <rfl/ubjson.hpp>

#include "../check_bitset.hpp"

namespace test_bitset_ubjson {

const auto write = [](const auto& value) { return rfl::ubjson::write(value); };
const auto read = []<class T>(const auto& value) {
  return rfl::ubjson::read<T>(value);
};

TEST(ubjson, bitset_sizes_and_patterns) {
  test_bitset_formats::check_sizes(write, read);
}

TEST(ubjson, bitset_nested) { test_bitset_formats::check_nested(write, read); }

TEST(ubjson, bitset_representation_and_validation) {
  test_bitset_formats::check_packed<rfl::Bytestring>(write, read);
}

}  // namespace test_bitset_ubjson
