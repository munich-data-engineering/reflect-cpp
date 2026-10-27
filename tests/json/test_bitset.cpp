#include <rfl/json.hpp>

#include "../check_bitset.hpp"

namespace test_bitset_json {

const auto write = [](const auto& value) { return rfl::json::write(value); };
const auto read = []<class T>(const auto& value) {
  return rfl::json::read<T>(value);
};

TEST(json, bitset_sizes_and_patterns) {
  test_bitset_formats::check_sizes(write, read);
}

TEST(json, bitset_nested) { test_bitset_formats::check_nested(write, read); }

TEST(json, bitset_representation_and_validation) {
  test_bitset_formats::check_text(write, read);
}

TEST(json, bitset_schema) {
  const auto schema = rfl::json::to_schema<std::bitset<9>>();
  EXPECT_NE(schema.find(R"pattern("pattern":"^[01]{9}(?![\\s\\S])")pattern"),
            std::string::npos);
  EXPECT_EQ(rfl::json::write(std::bitset<9>{1}), R"("000000001")");
  EXPECT_FALSE(rfl::json::read<std::bitset<9>>("null"));
  EXPECT_FALSE(rfl::json::read<std::bitset<9>>("1"));
  EXPECT_FALSE(rfl::json::read<std::bitset<9>>(R"("00000000\u0000")"));
}

}  // namespace test_bitset_json
