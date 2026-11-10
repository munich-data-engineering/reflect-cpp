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

struct ConfigWithDefaults {
  rfl::Positional<rfl::DefaultVal<std::string>> input = std::string("in.txt");
  rfl::Short<"o", rfl::DefaultVal<std::string>> output = std::string("out.csv");
  rfl::Short<"j", rfl::Description<"Worker threads", rfl::DefaultVal<int>>>
      jobs = 4;
};

TEST(cli, test_short_default_val_given) {
  const char* args[] = {
      "program",
      "data.csv",
      "-o", "x.csv",
      "-j", "8"
  };
  const auto result =
      rfl::cli::read<ConfigWithDefaults>(6, const_cast<char**>(args));
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().input().value(), "data.csv");
  EXPECT_EQ(result.value().output().value(), "x.csv");
  EXPECT_EQ(result.value().jobs().value().value(), 8);
}

TEST(cli, test_short_default_val_given_by_long_name) {
  const char* args[] = {
      "program",
      "--output=x.csv",
      "--jobs=8"
  };
  const auto result =
      rfl::cli::read<ConfigWithDefaults>(3, const_cast<char**>(args));
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().input().value(), "in.txt");
  EXPECT_EQ(result.value().output().value(), "x.csv");
  EXPECT_EQ(result.value().jobs().value().value(), 8);
}

TEST(cli, test_short_and_positional_default_val_omitted) {
  const char* args[] = {"program"};
  const auto result =
      rfl::cli::read<ConfigWithDefaults>(1, const_cast<char**>(args));
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().input().value(), "in.txt");
  EXPECT_EQ(result.value().output().value(), "out.csv");
  EXPECT_EQ(result.value().jobs().value().value(), 4);
}

}  // namespace test_optional_short_and_positional
