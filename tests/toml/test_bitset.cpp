#include <rfl/toml.hpp>

#include "../check_bitset.hpp"

namespace test_bitset_toml {

const auto write = [](const auto& value) { return rfl::toml::write(value); };
const auto read = []<class T>(const auto& value) {
  return rfl::toml::read<T>(value);
};

TEST(toml, bitset_sizes_and_patterns) {
  test_bitset_formats::check_sizes(write, read);
}

TEST(toml, bitset_nested) { test_bitset_formats::check_nested(write, read); }

TEST(toml, bitset_representation_and_validation) {
  test_bitset_formats::check_text(write, read);
}

}  // namespace test_bitset_toml
