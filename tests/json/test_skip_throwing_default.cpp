#include <gtest/gtest.h>

#include <optional>
#include <rfl.hpp>
#include <rfl/json.hpp>
#include <stdexcept>
#include <tuple>
#include <vector>

namespace test_skip_throwing_default {

using Positive = rfl::Validator<int, rfl::Minimum<1>>;

using Path = std::vector<rfl::ErrorDetail::PathSegment>;

struct WithSkip {
  rfl::Skip<Positive> s;
  int y;
};

struct SkipLast {
  int y;
  rfl::Skip<Positive> s;
};

/// Not required, because its ReflectionType is a std::optional, and counts
/// the instances that are alive.
struct ThrowingDefault {
  using ReflectionType = std::optional<int>;

  static inline int alive = 0;

  ThrowingDefault() { throw std::runtime_error("no default"); }
  ThrowingDefault(const ReflectionType&) { ++alive; }
  ThrowingDefault(const ThrowingDefault&) { ++alive; }
  ~ThrowingDefault() { --alive; }

  ReflectionType reflection() const { return std::nullopt; }
};

struct WithThrowingDefault {
  int y;
  ThrowingDefault t;
};

TEST(json, test_skip_throwing_default) {
  const auto res = rfl::json::read<WithSkip>(R"({"y": 1, "s": 5})");
  ASSERT_FALSE(res);
  EXPECT_EQ(res.error().what(),
            "Failed to parse field 's': Value expected to be greater than or "
            "equal to 1, but got 0.");
}

TEST(json, test_skip_throwing_default_field_absent) {
  const auto res = rfl::json::read<WithSkip>(R"({"y": 1})");
  ASSERT_FALSE(res);
  EXPECT_EQ(res.error().what(),
            "Field named 's' not found, and its default value could not be "
            "constructed: Value expected to be greater than or equal to 1, "
            "but got 0.");
  const auto details = res.error().details();
  ASSERT_EQ(details.size(), 1);
  EXPECT_EQ(details[0].path, (Path{"s"}));
  EXPECT_EQ(details[0].message, res.error().what());
}

TEST(json, test_skip_throwing_default_field_absent_no_field_names) {
  const auto res = rfl::json::read<SkipLast, rfl::NoFieldNames>(R"([1])");
  ASSERT_FALSE(res);
  EXPECT_EQ(res.error().what(),
            "Field named 's' not found, and its default value could not be "
            "constructed: Value expected to be greater than or equal to 1, "
            "but got 0.");
}

TEST(json, test_skip_throwing_default_tuple_element_absent) {
  const auto res =
      rfl::json::read<std::tuple<int, rfl::Skip<Positive>>>(R"([1])");
  ASSERT_FALSE(res);
  EXPECT_EQ(res.error().what(),
            "Field 1 was missing, and its default value could not be "
            "constructed: Value expected to be greater than or equal to 1, "
            "but got 0.");
  const auto details = res.error().details();
  ASSERT_EQ(details.size(), 1);
  EXPECT_EQ(details[0].path, (Path{size_t{1}}));
}

// No destructor may run for the field that could not be constructed.
TEST(json, test_skip_throwing_default_does_not_destroy_the_field) {
  const auto res = rfl::json::read<WithThrowingDefault>(R"({"y": 1})");
  ASSERT_FALSE(res);
  EXPECT_EQ(res.error().what(),
            "Field named 't' not found, and its default value could not be "
            "constructed: no default");
  EXPECT_EQ(ThrowingDefault::alive, 0);

  const auto tup =
      rfl::json::read<std::tuple<int, ThrowingDefault>>(R"([1])");
  ASSERT_FALSE(tup);
  EXPECT_EQ(ThrowingDefault::alive, 0);

  ASSERT_TRUE(rfl::json::read<WithThrowingDefault>(R"({"y": 1, "t": 2})"));
  EXPECT_EQ(ThrowingDefault::alive, 0);
}

}  // namespace test_skip_throwing_default
