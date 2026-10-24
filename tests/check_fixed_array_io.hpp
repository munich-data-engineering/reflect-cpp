#ifndef CHECK_FIXED_ARRAY_IO_HPP_
#define CHECK_FIXED_ARRAY_IO_HPP_

#include <gtest/gtest.h>

#include <array>
#include <cstdint>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

namespace test_fixed_array_io {

template <class T, class Write, class Read>
void check_roundtrip(const T& input, Write write, Read read) {
  const auto decoded = read.template operator()<T>(write(input));
  ASSERT_TRUE(decoded) << decoded.error().what();
  EXPECT_EQ(*decoded, input);
}

template <class Write, class Read>
void check_sizes(Write write, Read read, const size_t prefix_size) {
  using Array = std::array<std::int32_t, 3>;
  const Array input{12, -34, 56};
  const auto bytes = write(input);
  const auto header_size = write(std::int32_t{0}).size() - sizeof(std::int32_t);
  EXPECT_EQ(bytes.size(), header_size + 3 * sizeof(std::int32_t));
  EXPECT_EQ(write(std::vector<std::int32_t>(input.begin(), input.end())).size(),
            bytes.size() + prefix_size);
  check_roundtrip(input, write, read);

  const std::array<std::int32_t, 0> empty{};
  EXPECT_EQ(write(empty).size(), header_size);
  check_roundtrip(empty, write, read);
  check_roundtrip(std::array<bool, 3>{true, false, true}, write, read);
  check_roundtrip(std::array<std::string, 2>{"hello", "world"}, write, read);

  const std::array<std::array<std::int32_t, 2>, 2> nested{{{1, 2}, {3, 4}}};
  EXPECT_EQ(write(nested).size(), header_size + 4 * sizeof(std::int32_t));
  check_roundtrip(nested, write, read);
  check_roundtrip(std::array<std::array<int, 0>, 3>{}, write, read);
  check_roundtrip(std::array<std::vector<int>, 2>{{{1, 2}, {}}}, write, read);

  auto truncated = bytes;
  truncated.pop_back();
  EXPECT_FALSE(read.template operator()<Array>(truncated));
  auto strings = write(std::array<std::string, 2>{"first", "second"});
  strings.pop_back();
  const auto invalid =
      read.template operator()<std::array<std::string, 2>>(strings);
  ASSERT_FALSE(invalid);
  EXPECT_NE(invalid.error().what().find("element 1"), std::string::npos);
}

struct Nested {
  std::array<int, 2> values;
  std::array<int, 0> empty;
  std::vector<std::array<int, 2>> rows;
  std::map<std::string, std::array<int, 2>> lookup;
  std::optional<std::array<int, 2>> optional;
  std::variant<int, std::array<int, 2>> choice;
  int tail;

  bool operator==(const Nested&) const = default;
};

template <class Write, class Read>
void check_parents(Write write, Read read) {
  const Nested input{{1, 2},
                     {},
                     {{3, 4}, {5, 6}},
                     {{"key", {7, 8}}},
                     std::array<int, 2>{9, 10},
                     std::array<int, 2>{11, 12},
                     42};
  check_roundtrip(input, write, read);
  check_roundtrip(std::variant<int, std::array<int, 0>>{std::array<int, 0>{}},
                  write, read);
  check_roundtrip(
      std::map<std::string, std::array<int, 0>>{{"a", {}}, {"b", {}}}, write,
      read);

  // C arrays share the std::array reader, so their writer must agree with it.
  const int c_arrays[2][2]{{1, 2}, {3, 4}};
  using CArray = int[2][2];
  const auto decoded = read.template operator()<CArray>(write(c_arrays));
  ASSERT_TRUE(decoded) << decoded.error().what();
  EXPECT_EQ(decoded->arr_[0][0], 1);
  EXPECT_EQ(decoded->arr_[0][1], 2);
  EXPECT_EQ(decoded->arr_[1][0], 3);
  EXPECT_EQ(decoded->arr_[1][1], 4);

  using MoveOnly = std::array<std::unique_ptr<int>, 2>;
  const MoveOnly pointers{std::make_unique<int>(13), std::make_unique<int>(14)};
  const auto moved = read.template operator()<MoveOnly>(write(pointers));
  ASSERT_TRUE(moved) << moved.error().what();
  ASSERT_TRUE((*moved)[0]);
  ASSERT_TRUE((*moved)[1]);
  EXPECT_EQ(*(*moved)[0], 13);
  EXPECT_EQ(*(*moved)[1], 14);
}

}  // namespace test_fixed_array_io

#endif
