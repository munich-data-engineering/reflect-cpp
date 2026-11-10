#include <gtest/gtest.h>

#include <rfl.hpp>
#include <rfl/json.hpp>

namespace test_skip_throwing_default {

using Positive = rfl::Validator<int, rfl::Minimum<1>>;

struct WithSkip {
  rfl::Skip<Positive> s;
  int y;
};

TEST(json, test_skip_throwing_default) {
  const auto res = rfl::json::read<WithSkip>(R"({"y": 1, "s": 5})");
  ASSERT_FALSE(res);
  EXPECT_EQ(res.error().what(),
            "Failed to parse field 's': Value expected to be greater than or "
            "equal to 1, but got 0.");
}

}  // namespace test_skip_throwing_default
