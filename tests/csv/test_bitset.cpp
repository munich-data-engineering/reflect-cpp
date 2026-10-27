#include <rfl/csv.hpp>

#include "../check_bitset.hpp"

namespace test_bitset_csv {

const auto write = [](const auto& value) {
  return rfl::csv::write(std::vector{value});
};
const auto read = []<class T>(const auto& bytes) -> rfl::Result<T> {
  return rfl::csv::read<std::vector<T>>(bytes).and_then(
      [](const auto& rows) -> rfl::Result<T> {
        if (rows.size() != 1) return rfl::error("Expected one row.");
        return rows.front();
      });
};

TEST(csv, bitset_sizes_and_patterns) {
  test_bitset_formats::check_sizes(write, read);
}

TEST(csv, bitset_representation_and_validation) {
  test_bitset_formats::check_text(write, read);
}

TEST(csv, bitset_optional_and_renamed_columns) {
  struct Row {
    rfl::Rename<"descriptor", std::bitset<9>> bits;
    std::optional<std::bitset<9>> optional_bits;
    std::string label;
  };
  const std::vector<Row> input{{std::bitset<9>{2}, std::bitset<9>{1}, "000123"},
                               {std::bitset<9>{256}, std::nullopt, "000456"}};
  const auto decoded = rfl::csv::read<std::vector<Row>>(rfl::csv::write(input));
  ASSERT_TRUE(decoded) << decoded.error().what();
  ASSERT_EQ(decoded->size(), input.size());
  for (std::size_t i = 0; i < input.size(); ++i) {
    EXPECT_EQ((*decoded)[i].bits.value(), input[i].bits.value());
    EXPECT_EQ((*decoded)[i].optional_bits, input[i].optional_bits);
    EXPECT_EQ((*decoded)[i].label, input[i].label);
  }
}

}  // namespace test_bitset_csv
