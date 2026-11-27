#include <gtest/gtest.h>

#include <optional>
#include <rfl.hpp>
#include <rfl/cli.hpp>
#include <string>
#include <vector>

namespace test_short_clusters {

template <class T, class... Ps>
rfl::Result<T> read(std::vector<const char*> _args) {
  _args.insert(_args.begin(), "program");
  return rfl::cli::read<T, Ps...>(static_cast<int>(_args.size()),
                                  const_cast<char**>(_args.data()));
}

template <class T>
std::string error_of(const rfl::Result<T>& _result) {
  return _result ? std::string("(no error)")
                 : std::string(_result.error().what());
}

struct Opts {
  rfl::Short<"a", bool> all;
  rfl::DefaultVal<rfl::Short<"b", bool>> brief = false;
  rfl::Short<"c", std::optional<bool>> color;
  rfl::DefaultVal<rfl::Short<"q", bool>> quiet = false;
  rfl::Short<"f", std::optional<std::string>> format;
  rfl::Short<"p", std::optional<int>> port;
  rfl::Positional<std::optional<std::string>> input;
  rfl::Positional<std::optional<std::string>> output;
};

TEST(cli, test_short_cluster_flags) {
  const auto result = read<Opts>({"-abc"});
  ASSERT_TRUE(result) << result.error().what();
  const auto& o = result.value();
  EXPECT_TRUE(o.all());
  EXPECT_TRUE(o.brief().value());
  EXPECT_EQ(o.color(), true);
  EXPECT_FALSE(o.quiet().value());
  EXPECT_FALSE(o.format());
}

TEST(cli, test_short_cluster_flags_any_order) {
  const auto result = read<Opts>({"-cba", "in.txt"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_TRUE(result.value().all());
  EXPECT_TRUE(result.value().brief().value());
  EXPECT_EQ(result.value().color(), true);
  EXPECT_EQ(result.value().input(), "in.txt");
}

TEST(cli, test_short_attached_value) {
  const auto result = read<Opts>({"-a", "-fcsv"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().format(), "csv");
}

TEST(cli, test_short_cluster_then_attached_value) {
  const auto result = read<Opts>({"-a", "-qfcsv", "in.txt"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_TRUE(result.value().quiet().value());
  EXPECT_EQ(result.value().format(), "csv");
  EXPECT_EQ(result.value().input(), "in.txt");
}

TEST(cli, test_short_cluster_value_takes_next_token) {
  const auto result = read<Opts>({"-a", "-qf", "csv", "in.txt"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_TRUE(result.value().quiet().value());
  EXPECT_EQ(result.value().format(), "csv");
  EXPECT_EQ(result.value().input(), "in.txt");
}

TEST(cli, test_short_cluster_value_takes_rest_verbatim) {
  // Everything after the first option that takes a value is its value, even
  // letters that are short options themselves.
  const auto result = read<Opts>({"-afabc"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_TRUE(result.value().all());
  EXPECT_FALSE(result.value().brief().value());
  EXPECT_EQ(result.value().format(), "abc");
}

TEST(cli, test_short_cluster_value_missing) {
  for (const auto& args : std::vector<std::vector<const char*>>{
           {"-a", "-qf"}, {"-aqf", "-b"}, {"-aqf", "--format=x"}}) {
    const auto result = read<Opts>(args);
    EXPECT_EQ(error_of(result), "Missing value for option '-f'.");
  }
}

TEST(cli, test_short_equals_value) {
  // rfl keeps `-f=csv` meaning "csv", also in a group. getopt would give
  // "=csv".
  for (const auto& args : std::vector<std::vector<const char*>>{
           {"-a", "-f=csv"}, {"-aqf=csv"}}) {
    const auto result = read<Opts>(args);
    ASSERT_TRUE(result) << result.error().what();
    EXPECT_EQ(result.value().format(), "csv");
  }
  const auto empty = read<Opts>({"-af="});
  ASSERT_TRUE(empty) << empty.error().what();
  EXPECT_EQ(empty.value().format(), "");
  const auto twice = read<Opts>({"-af==x"});
  ASSERT_TRUE(twice) << twice.error().what();
  EXPECT_EQ(twice.value().format(), "=x");
}

TEST(cli, test_short_flag_equals_value_alone) {
  const auto result = read<Opts>({"-a=false", "-q=true"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_FALSE(result.value().all());
  EXPECT_TRUE(result.value().quiet().value());
}

TEST(cli, test_short_cluster_flag_takes_no_attached_value) {
  const auto result = read<Opts>({"-aq=false"});
  EXPECT_EQ(error_of(result),
            "Option '-q' does not take a value in '-aq=false'. Write "
            "'-q=false' on its own.");
  const auto second = read<Opts>({"-a", "-bq=false"});
  EXPECT_EQ(error_of(second),
            "Option '-q' does not take a value in '-bq=false'. Write "
            "'-q=false' on its own.");
}

TEST(cli, test_short_cluster_last_flag_takes_bool_literal) {
  // The last option of a group treats the next token as it would alone.
  const auto result = read<Opts>({"-qa", "false", "in.txt"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_TRUE(result.value().quiet().value());
  EXPECT_FALSE(result.value().all());
  EXPECT_EQ(result.value().input(), "in.txt");
  EXPECT_FALSE(result.value().output());
}

TEST(cli, test_short_cluster_flag_does_not_take_other_tokens) {
  const auto result = read<Opts>({"-ab", "in.txt", "out.txt"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_TRUE(result.value().all());
  EXPECT_TRUE(result.value().brief().value());
  EXPECT_EQ(result.value().input(), "in.txt");
  EXPECT_EQ(result.value().output(), "out.txt");
}

TEST(cli, test_short_cluster_unknown_letter) {
  const auto middle = read<Opts>({"-axf", "csv"});
  EXPECT_EQ(error_of(middle), "Unknown short argument: -x (in '-axf').");
  const auto first = read<Opts>({"-xa"});
  EXPECT_EQ(error_of(first), "Unknown short argument: -x (in '-xa').");
  const auto last = read<Opts>({"-abz"});
  EXPECT_EQ(error_of(last), "Unknown short argument: -z (in '-abz').");
  // A character that takes several bytes in UTF-8 is named whole.
  const auto utf8 = read<Opts>({"-a\xC3\xA9", "-\xE2\x82\xAC"});
  EXPECT_EQ(error_of(utf8),
            "Unknown short argument: -\xC3\xA9 (in '-a\xC3\xA9').");
  const auto utf8_alone = read<Opts>({"-a", "-\xE2\x82\xAC"});
  EXPECT_EQ(error_of(utf8_alone), "Unknown short argument: -\xE2\x82\xAC");
}

TEST(cli, test_short_unknown_alone) {
  EXPECT_FALSE(read<Opts>({"-a", "-x"}));
  EXPECT_FALSE(read<Opts>({"-a", "-x=1"}));
}

TEST(cli, test_short_cluster_duplicate) {
  EXPECT_FALSE(read<Opts>({"-aa"}));
  EXPECT_FALSE(read<Opts>({"-ab", "-ba"}));
}

TEST(cli, test_short_cluster_conflicts_with_long) {
  EXPECT_FALSE(read<Opts>({"-afcsv", "--format=json"}));
}

TEST(cli, test_short_negative_number_value) {
  for (const auto& args : std::vector<std::vector<const char*>>{
           {"-a", "-p", "-42"}, {"-a", "-p-42"}, {"-ap", "-42"},
           {"-ap-42"}, {"-a", "-p=-42"}}) {
    const auto result = read<Opts>(args);
    ASSERT_TRUE(result) << result.error().what();
    EXPECT_EQ(result.value().port(), -42);
  }
}

struct Numbers {
  rfl::Short<"v", bool> verbose;
  rfl::Short<"n", std::optional<double>> number;
  rfl::Positional<double> x;
  rfl::Positional<std::optional<double>> y;
};

TEST(cli, test_short_negative_number_positional) {
  const auto result = read<Numbers>({"-v", "-42", "-.5"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_TRUE(result.value().verbose());
  EXPECT_DOUBLE_EQ(result.value().x(), -42.0);
  EXPECT_DOUBLE_EQ(result.value().y().value(), -0.5);

  const auto value = read<Numbers>({"-vn", "-1.5", "-3"});
  ASSERT_TRUE(value) << value.error().what();
  EXPECT_DOUBLE_EQ(value.value().number().value(), -1.5);
  EXPECT_DOUBLE_EQ(value.value().x(), -3.0);
}

struct DigitShort {
  rfl::Short<"1", bool> one_per_line;
  rfl::Short<"n", std::optional<int>> count;
  rfl::Positional<std::optional<int>> x;
};

struct DigitShortNumbers {
  rfl::Short<"1", bool> one_per_line;
  rfl::Positional<std::optional<double>> x;
  rfl::Positional<std::optional<double>> y;
  rfl::Positional<std::optional<double>> z;
};

TEST(cli, test_short_digit_option_negative_numbers_are_groups) {
  // Once a short option's name is a digit, a token that looks like a
  // negative number is a group of short options.
  const auto grouped = read<DigitShort>({"-1n2"});
  ASSERT_TRUE(grouped) << grouped.error().what();
  EXPECT_TRUE(grouped.value().one_per_line());
  EXPECT_EQ(grouped.value().count(), 2);

  EXPECT_EQ(error_of(read<DigitShort>({"-1", "-5"})),
            "Unknown short argument: -5");
  EXPECT_EQ(error_of(read<DigitShort>({"-2"})), "Unknown short argument: -2");
  EXPECT_EQ(error_of(read<DigitShort>({"-.5"})),
            "Unknown short argument: -. (in '-.5').");
  EXPECT_EQ(error_of(read<DigitShortNumbers>({"-1.5"})),
            "Unknown short argument: -. (in '-1.5').");
  EXPECT_EQ(error_of(read<DigitShortNumbers>({"-10"})),
            "Unknown short argument: -0 (in '-10').");
  EXPECT_EQ(error_of(read<DigitShortNumbers>({"-15"})),
            "Unknown short argument: -5 (in '-15').");

  const auto separated =
      read<DigitShortNumbers>({"-1", "--", "-1.5", "-10", "-15"});
  ASSERT_TRUE(separated) << separated.error().what();
  EXPECT_TRUE(separated.value().one_per_line());
  EXPECT_DOUBLE_EQ(separated.value().x().value(), -1.5);
  EXPECT_DOUBLE_EQ(separated.value().y().value(), -10.0);
  EXPECT_DOUBLE_EQ(separated.value().z().value(), -15.0);
}

TEST(cli, test_short_cluster_after_separator) {
  const auto result = read<Opts>({"-ab", "--", "-qfcsv", "-x"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_TRUE(result.value().all());
  EXPECT_FALSE(result.value().quiet().value());
  EXPECT_FALSE(result.value().format());
  EXPECT_EQ(result.value().input(), "-qfcsv");
  EXPECT_EQ(result.value().output(), "-x");
}

TEST(cli, test_short_cluster_value_after_separator_token) {
  // `--` after an option that needs a value is not its value.
  const auto result = read<Opts>({"-af", "--", "x"});
  EXPECT_EQ(error_of(result), "Missing value for option '-f'.");
}

struct Nested {
  rfl::Description<"Be quiet", rfl::Short<"q", bool>> quiet;
  rfl::DefaultVal<rfl::Short<"o", std::string>> output;
  std::optional<rfl::Short<"v", bool>> verbose;
};

TEST(cli, test_short_cluster_nested_wrappers) {
  const auto result = read<Nested>({"-vqoout.csv"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_TRUE(result.value().quiet().value());
  EXPECT_EQ(result.value().output().value(), "out.csv");
  ASSERT_TRUE(result.value().verbose.has_value());
  EXPECT_TRUE(result.value().verbose.value().value());
}

TEST(cli, test_short_cluster_invalid_utf8_lead_byte) {
  // Invalid/truncated UTF-8 lead byte must not swallow a following '=' or ASCII
  // character.
  const auto res = read<Opts>({"-\xC3=1"});
  ASSERT_FALSE(res);
  EXPECT_EQ(std::string(res.error().what()), "Unknown short argument: -\xC3");

  const auto res2 = read<Opts>({"-q\xE9v"});
  ASSERT_FALSE(res2);
  EXPECT_EQ(std::string(res2.error().what()),
            "Unknown short argument: -\xE9 (in '-q\xE9v').");
}

TEST(cli, test_short_leading_equals_rejected) {
  // `-=` and `-=foo` must be rejected cleanly (never produce an empty short key
  // or consume the following token).
  EXPECT_FALSE(read<Opts>({"-="}));
  EXPECT_FALSE(read<Opts>({"-=foo"}));
  EXPECT_FALSE(read<Opts>({"-=", "value"}));

  const rfl::cli::internal::Options no_group{};
  const auto raw = rfl::cli::internal::parse_string_views(
      std::vector<std::string_view>{"-="}, no_group);
  EXPECT_FALSE(raw);
  const auto raw_val = rfl::cli::internal::parse_string_views(
      std::vector<std::string_view>{"-=foo"}, no_group);
  EXPECT_FALSE(raw_val);
}

}  // namespace test_short_clusters
