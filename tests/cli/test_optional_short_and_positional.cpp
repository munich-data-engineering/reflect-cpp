#include <gtest/gtest.h>
#include <optional>
#include <rfl.hpp>
#include <rfl/cli.hpp>
#include <rfl/json.hpp>
#include <string>

namespace test_optional_short_and_positional {

struct Config {
  rfl::Positional<std::optional<std::string>> input;
  rfl::Short<"f", std::optional<std::string>> format;
  bool verbose = false;
};

TEST(cli, test_optional_positional_and_short_given) {
  const char* args[] = {
      "program",
      "data.csv",
      "-f", "json",
      "--verbose"
  };
  const auto result = rfl::cli::read<Config>(5, const_cast<char**>(args));
  ASSERT_TRUE(result) << result.error().what();
  ASSERT_TRUE(result.value().input().has_value());
  EXPECT_EQ(*result.value().input(), "data.csv");
  ASSERT_TRUE(result.value().format().has_value());
  EXPECT_EQ(*result.value().format(), "json");
  EXPECT_EQ(rfl::json::write(result.value()),
            R"({"input":"data.csv","format":"json","verbose":true})");
}

TEST(cli, test_optional_positional_and_short_omitted) {
  const char* args[] = {
      "program",
      "--verbose"
  };
  const auto result = rfl::cli::read<Config>(2, const_cast<char**>(args));
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_FALSE(result.value().input().has_value());
  EXPECT_FALSE(result.value().format().has_value());
  EXPECT_EQ(rfl::json::write(result.value()), R"({"verbose":true})");
}

}  // namespace test_optional_short_and_positional
