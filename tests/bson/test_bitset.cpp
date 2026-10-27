#include <rfl/bson.hpp>

#include "../check_bitset.hpp"

namespace test_bitset_bson {

const auto write = [](const auto& value) { return rfl::bson::write(value); };
const auto read = []<class T>(const auto& value) {
  return rfl::bson::read<T>(value);
};

TEST(bson, bitset_sizes_and_patterns) {
  test_bitset_formats::check_sizes(write, read);
}

TEST(bson, bitset_nested) { test_bitset_formats::check_nested(write, read); }

TEST(bson, bitset_representation_and_validation) {
  test_bitset_formats::check_packed<rfl::Bytestring>(write, read);
}

}  // namespace test_bitset_bson
