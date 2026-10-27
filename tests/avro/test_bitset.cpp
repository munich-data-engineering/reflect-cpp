#include <rfl/avro.hpp>

#include "../check_bitset.hpp"

namespace test_bitset_avro {

const auto write = [](const auto& value) { return rfl::avro::write(value); };
const auto read = []<class T>(const auto& value) {
  return rfl::avro::read<T>(value);
};

TEST(avro, bitset_sizes_and_patterns) {
  test_bitset_formats::check_sizes(write, read);
}

TEST(avro, bitset_nested) { test_bitset_formats::check_nested(write, read); }

TEST(avro, bitset_representation_and_validation) {
  test_bitset_formats::check_packed<rfl::Bytestring>(write, read);
}

}  // namespace test_bitset_avro
