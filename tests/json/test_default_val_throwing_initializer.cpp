#include <gtest/gtest.h>

#include <exception>
#include <rfl.hpp>
#include <rfl/json.hpp>
#include <type_traits>

namespace test_default_val_throwing_initializer {

using Positive = rfl::Validator<int, rfl::Minimum<1>>;

static_assert(!std::is_nothrow_constructible_v<rfl::DefaultVal<Positive>, int>,
              "Converting to a Validator can throw.");
static_assert(std::is_nothrow_constructible_v<rfl::DefaultVal<int>, short>);

struct InvalidInitializer {
  rfl::DefaultVal<Positive> retries = 0;
};

struct ValidInitializer {
  rfl::DefaultVal<Positive> retries = 5;
};

TEST(json, test_default_val_throwing_initializer_throws) {
  EXPECT_THROW((void)InvalidInitializer{}, std::exception);
}

TEST(json, test_default_val_throwing_initializer_is_an_error_when_reading) {
  const auto res = rfl::json::read<InvalidInitializer>(R"({"retries":3})");
  ASSERT_FALSE(res);
  EXPECT_EQ(res.error().what(),
            "Value expected to be greater than or equal to 1, but got 0.");
}

TEST(json, test_default_val_throwing_initializer_valid_default_is_used) {
  const auto res = rfl::json::read<ValidInitializer>(R"({})");
  ASSERT_TRUE(res) << res.error().what();
  EXPECT_EQ(res->retries.value().value(), 5);
}

}  // namespace test_default_val_throwing_initializer
