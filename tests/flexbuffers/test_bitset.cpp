#include <rfl/flexbuf.hpp>

#include "../check_bitset.hpp"

namespace test_bitset_flexbuffers {

const auto write = [](const auto& value) { return rfl::flexbuf::write(value); };
const auto read = []<class T>(const auto& value) {
  return rfl::flexbuf::read<T>(value);
};

TEST(flexbuffers, bitset_sizes_and_patterns) {
  test_bitset_formats::check_sizes(write, read);
}

TEST(flexbuffers, bitset_nested) {
  test_bitset_formats::check_nested(write, read);
}

TEST(flexbuffers, bitset_representation_and_validation) {
  test_bitset_formats::check_packed<rfl::Bytestring>(write, read);
}

}  // namespace test_bitset_flexbuffers
