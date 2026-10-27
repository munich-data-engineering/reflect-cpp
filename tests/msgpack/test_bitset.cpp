#include <rfl/msgpack.hpp>

#include "../check_bitset.hpp"

namespace test_bitset_msgpack {

const auto write = [](const auto& value) { return rfl::msgpack::write(value); };
const auto read = []<class T>(const auto& value) {
  return rfl::msgpack::read<T>(value);
};

TEST(msgpack, bitset_sizes_and_patterns) {
  test_bitset_formats::check_sizes(write, read);
}

TEST(msgpack, bitset_nested) { test_bitset_formats::check_nested(write, read); }

TEST(msgpack, bitset_representation_and_validation) {
  test_bitset_formats::check_packed<rfl::Bytestring>(write, read);
}

}  // namespace test_bitset_msgpack
