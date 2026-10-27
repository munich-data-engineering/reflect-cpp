#include <rfl/yaml.hpp>

#include "../check_bitset.hpp"

namespace test_bitset_yaml {

const auto write = [](const auto& value) { return rfl::yaml::write(value); };
const auto read = []<class T>(const auto& value) {
  return rfl::yaml::read<T>(value);
};

TEST(yaml, bitset_sizes_and_patterns) {
  test_bitset_formats::check_sizes(write, read);
}

TEST(yaml, bitset_nested) { test_bitset_formats::check_nested(write, read); }

TEST(yaml, bitset_representation_and_validation) {
  test_bitset_formats::check_text(write, read);
}

}  // namespace test_bitset_yaml
