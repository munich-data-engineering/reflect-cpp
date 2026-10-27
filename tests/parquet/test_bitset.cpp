#include <rfl/parquet.hpp>

#include "../check_bitset.hpp"

namespace test_bitset_parquet {

const auto write = [](const auto& value) {
  return rfl::parquet::write(std::vector{value});
};
const auto read = []<class T>(const auto& bytes) -> rfl::Result<T> {
  return rfl::parquet::read<std::vector<T>>(bytes).and_then(
      [](const auto& rows) -> rfl::Result<T> {
        if (rows.size() != 1) return rfl::error("Expected one row.");
        return rows.front();
      });
};

TEST(parquet, bitset_sizes_and_patterns) {
  test_bitset_formats::check_sizes(write, read);
}

TEST(parquet, bitset_representation_and_validation) {
  test_bitset_formats::check_packed<rfl::Bytestring>(write, read);
}

}  // namespace test_bitset_parquet
