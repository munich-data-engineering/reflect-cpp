#ifndef RFL_TEST_TYPED_MAPS_HPP_
#define RFL_TEST_TYPED_MAPS_HPP_

#include <gtest/gtest.h>

#include <cmath>
#include <cstdint>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <optional>
#include <rfl.hpp>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace test_typed_maps {

template <class T>
void roundtrip(const auto& write, const auto& read, const T& value) {
  const auto bytes = write(value);
  const auto result = read.template operator()<T>(bytes);
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(*result, value);
}

struct Nested {
  std::map<int, std::map<double, std::vector<int>>> values;
  std::map<int, int> empty;
  int after;
};

inline void check(const auto& write, const auto& read) {
  roundtrip(write, read, std::map<int, int>{});
  roundtrip(write, read,
            std::map<std::int64_t, int>{
                {std::numeric_limits<std::int64_t>::min(), 1},
                {-1, 2},
                {0, 3},
                {std::numeric_limits<std::int64_t>::max(), 4}});
  roundtrip(write, read,
            std::map<std::uint64_t, int>{
                {0, 1}, {std::numeric_limits<std::uint64_t>::max(), 2}});
  roundtrip(write, read,
            std::unordered_map<int, std::string>{{-7, "a"}, {42, "b"}});
  roundtrip(write, read, std::map<bool, int>{{false, 1}, {true, 2}});
  roundtrip(write, read, std::map<std::string, int>{{"", 1}, {"42", 2}});

  roundtrip(write, read, std::map<int, int, std::greater<int>>{{1, 2}, {3, 4}});
  roundtrip(write, read,
            std::vector<std::map<int, int>>{{{1, 2}}, {}, {{3, 4}}});
  roundtrip(write, read,
            std::variant<int, std::map<int, int>>(std::map<int, int>{{3, 4}}));

  roundtrip(
      write, read,
      std::map<int, std::variant<int, std::string>>{{1, 42}, {2, "value"}});
  roundtrip(write, read,
            std::map<int, std::optional<int>>{{1, std::nullopt}, {2, 42}});

  // These distinct keys collapse to the same string with std::to_string.
  const double adjacent = std::nextafter(1.0, 2.0);
  roundtrip(write, read, std::map<double, int>{{1.0, 1}, {adjacent, 2}});
  roundtrip(write, read,
            std::map<float, int>{{1.0f, 1}, {std::nextafter(1.0f, 2.0f), 2}});

  const Nested nested{{{7, {{adjacent, {1, 2, 3}}}}}, {}, 12345};
  const auto result = read.template operator()<Nested>(write(nested));
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result->values, nested.values);
  EXPECT_TRUE(result->empty.empty());
  EXPECT_EQ(result->after, nested.after);

  using Key = rfl::Validator<int, rfl::Minimum<0>>;
  using ValidatedMap = std::map<Key, int>;
  const auto valid = read.template operator()<ValidatedMap>(
      write(std::map<int, int>{{7, 12}}));
  ASSERT_TRUE(valid) << valid.error().what();
  EXPECT_EQ(valid->begin()->first.reflection(), 7);
  EXPECT_EQ(write(*valid), write(std::map<int, int>{{7, 12}}));
  const auto invalid = read.template operator()<ValidatedMap>(
      write(std::map<int, int>{{-1, 12}}));
  EXPECT_FALSE(invalid);

  // Writing must not require copying mapped values into an intermediate vector.
  std::map<int, std::unique_ptr<int>> owning;
  owning.emplace(7, std::make_unique<int>(42));
  const auto owned = read.template operator()<decltype(owning)>(write(owning));
  ASSERT_TRUE(owned) << owned.error().what();
  ASSERT_EQ(owned->size(), 1);
  ASSERT_TRUE(owned->at(7));
  EXPECT_EQ(*owned->at(7), 42);

  auto truncated = write(std::map<int, int>{{7, 42}});
  truncated.pop_back();
  EXPECT_FALSE((read.template operator()<std::map<int, int>>(truncated)));
}

}  // namespace test_typed_maps

#endif
