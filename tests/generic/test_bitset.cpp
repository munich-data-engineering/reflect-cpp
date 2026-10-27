#include <rfl/generic/read.hpp>
#include <rfl/generic/write.hpp>

#include "../check_bitset.hpp"

namespace test_bitset_generic {

const auto write = [](const auto& value) { return rfl::generic::write(value); };
const auto read = []<class T>(const auto& value) {
  return rfl::generic::read<T>(value);
};

TEST(generic, bitset_sizes_and_patterns) {
  test_bitset_formats::check_sizes(write, read);
}

TEST(generic, bitset_nested) { test_bitset_formats::check_nested(write, read); }

TEST(generic, bitset_representation_and_validation) {
  test_bitset_formats::check_text(write, read);
}

}  // namespace test_bitset_generic
