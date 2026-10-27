#include <rfl/xml.hpp>

#include "../check_bitset.hpp"

namespace test_bitset_xml {

const auto write = [](const auto& value) {
  return rfl::xml::write<"root">(value);
};
const auto read = []<class T>(const auto& value) {
  return rfl::xml::read<T>(value);
};

TEST(xml, bitset_sizes_and_patterns) {
  test_bitset_formats::check_sizes(write, read);
}

TEST(xml, bitset_nested) { test_bitset_formats::check_nested(write, read); }

TEST(xml, bitset_representation_and_validation) {
  test_bitset_formats::check_text(write, read);
}

}  // namespace test_bitset_xml
