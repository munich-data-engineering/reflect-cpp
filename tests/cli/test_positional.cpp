#include <gtest/gtest.h>
#include <rfl.hpp>
#include <rfl/cli.hpp>
#include <cmath>
#include <map>
#include <optional>
#include <string>

namespace test_positional {

struct Config {
  rfl::Positional<std::string> input_file;
  rfl::Positional<std::string> output_file;
  bool verbose;
};

TEST(cli, test_positional_basic) {
  const char* args[] = {
      "program",
      "in.txt",
      "out.txt",
      "--verbose"
  };
  const auto result = rfl::cli::read<Config>(4, const_cast<char**>(args));
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().input_file(), "in.txt");
  EXPECT_EQ(result.value().output_file(), "out.txt");
  EXPECT_TRUE(result.value().verbose);
}

TEST(cli, test_positional_as_named) {
  const char* args[] = {
      "program",
      "--input-file=in.txt",
      "--output-file=out.txt",
      "--verbose"
  };
  const auto result = rfl::cli::read<Config>(4, const_cast<char**>(args));
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().input_file(), "in.txt");
  EXPECT_EQ(result.value().output_file(), "out.txt");
}

TEST(cli, test_positional_too_many) {
  const char* args[] = {
      "program",
      "in.txt",
      "out.txt",
      "extra.txt"
  };
  const auto result = rfl::cli::read<Config>(4, const_cast<char**>(args));
  ASSERT_FALSE(result);
}

struct SimpleConfig {
  rfl::Positional<std::string> input_file;
  rfl::Positional<std::string> output_file;
};

TEST(cli, test_positional_double_dash_separator) {
  const char* args[] = {
      "program",
      "--",
      "--not-a-flag",
      "out.txt"
  };
  const auto result =
      rfl::cli::read<SimpleConfig>(4, const_cast<char**>(args));
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().input_file(), "--not-a-flag");
  EXPECT_EQ(result.value().output_file(), "out.txt");
}

TEST(cli, test_positional_conflict_with_named) {
  const char* args[] = {
      "program",
      "in.txt",
      "--input-file=other.txt"
  };
  const auto result = rfl::cli::read<Config>(3, const_cast<char**>(args));
  ASSERT_FALSE(result);
}

struct OptionalPositional {
  rfl::Positional<std::string> required_file;
  rfl::Positional<std::optional<std::string>> optional_file;
};

TEST(cli, test_positional_optional_missing) {
  const char* args[] = {
      "program",
      "in.txt"
  };
  const auto result =
      rfl::cli::read<OptionalPositional>(2, const_cast<char**>(args));
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().required_file(), "in.txt");
  EXPECT_FALSE(result.value().optional_file().has_value());
}

struct NumberConfig {
  rfl::Positional<int> count;
  rfl::Positional<double> offset;
};

TEST(cli, test_positional_negative_numbers) {
  const char* args[] = {"program", "-5", "-.5"};
  const auto result = rfl::cli::read<NumberConfig>(3, const_cast<char**>(args));
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().count(), -5);
  EXPECT_DOUBLE_EQ(result.value().offset(), -0.5);
}

TEST(cli, test_positional_negative_number_forms) {
  const auto read = [](const char* _count, const char* _offset) {
    const char* args[] = {"program", _count, _offset};
    return rfl::cli::read<NumberConfig>(3, const_cast<char**>(args));
  };

  const auto a = read("-10", "-1.5");
  ASSERT_TRUE(a) << a.error().what();
  EXPECT_EQ(a.value().count(), -10);
  EXPECT_DOUBLE_EQ(a.value().offset(), -1.5);

  const auto b = read("-0", "-1e3");
  ASSERT_TRUE(b) << b.error().what();
  EXPECT_EQ(b.value().count(), 0);
  EXPECT_DOUBLE_EQ(b.value().offset(), -1000.0);
}

// -inf and -nan start with a letter, so they are short options. They need
// "--" (or --offset=-inf).
TEST(cli, test_positional_negative_infinity_needs_separator) {
  const char* args[] = {"program", "-5", "-inf"};
  const auto result = rfl::cli::read<NumberConfig>(3, const_cast<char**>(args));
  ASSERT_FALSE(result);
  EXPECT_TRUE(std::string(result.error().what())
                  .starts_with("Unknown short argument: -i"))
      << result.error().what();

  const char* separated[] = {"program", "--", "-5", "-inf"};
  const auto r =
      rfl::cli::read<NumberConfig>(4, const_cast<char**>(separated));
  ASSERT_TRUE(r) << r.error().what();
  EXPECT_EQ(r.value().count(), -5);
  EXPECT_TRUE(std::isinf(r.value().offset()));
  EXPECT_LT(r.value().offset(), 0.0);
}

struct NegativeOptions {
  int x;
  rfl::Short<"n", int> n;
};

TEST(cli, test_negative_option_values) {
  const char* args[] = {"program", "--x=-5", "-n", "-7"};
  const auto result =
      rfl::cli::read<NegativeOptions>(4, const_cast<char**>(args));
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().x, -5);
  EXPECT_EQ(result.value().n(), -7);

  const char* with_eq[] = {"program", "--x=-5", "-n=-7"};
  const auto r = rfl::cli::read<NegativeOptions>(3, const_cast<char**>(with_eq));
  ASSERT_TRUE(r) << r.error().what();
  EXPECT_EQ(r.value().n(), -7);
}

// As in argparse: if a short option's name looks like a number, tokens that
// look like negative numbers are short options.
struct Ls {
  rfl::Short<"1", std::optional<bool>> one_per_line;
  rfl::Positional<std::optional<int>> count;
};

TEST(cli, test_digit_short_option) {
  const char* args[] = {"ls", "-1"};
  const auto result = rfl::cli::read<Ls>(2, const_cast<char**>(args));
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().one_per_line(), std::optional<bool>(true));
  EXPECT_FALSE(result.value().count().has_value());
}

TEST(cli, test_digit_short_option_makes_negative_numbers_options) {
  const char* args[] = {"ls", "-5"};
  const auto result = rfl::cli::read<Ls>(2, const_cast<char**>(args));
  ASSERT_FALSE(result);
  EXPECT_EQ(std::string(result.error().what()), "Unknown short argument: -5");

  const char* separated[] = {"ls", "-1", "--", "-5"};
  const auto r = rfl::cli::read<Ls>(4, const_cast<char**>(separated));
  ASSERT_TRUE(r) << r.error().what();
  EXPECT_EQ(r.value().one_per_line(), std::optional<bool>(true));
  EXPECT_EQ(r.value().count(), std::optional<int>(-5));
}

// parse_argv does not know the struct, so it reads such tokens as short
// options. Only rfl::cli::read applies the rule.
TEST(cli, test_parse_argv_reads_negative_numbers_as_options) {
  const char* args[] = {"program", "-1", "x", "-5"};
  const auto parsed = rfl::cli::parse_argv(4, const_cast<char**>(args));
  ASSERT_TRUE(parsed) << parsed.error().what();
  EXPECT_EQ(parsed.value().short_args,
            (std::map<std::string, std::string>{{"1", "x"}, {"5", ""}}));
  EXPECT_TRUE(parsed.value().positional.empty());
}

}  // namespace test_positional
