#include <gtest/gtest.h>

#include <optional>
#include <rfl.hpp>
#include <rfl/cli.hpp>
#include <string>
#include <variant>
#include <vector>

// rfl::cli::read parses with the options of the struct: it knows while
// parsing which short options are bool flags and which take a value, so
// positional arguments keep their command-line order.
namespace test_option_table {

template <class T, class... Ps>
rfl::Result<T> read(std::vector<const char*> _args) {
  _args.insert(_args.begin(), "program");
  return rfl::cli::read<T, Ps...>(static_cast<int>(_args.size()),
                                  const_cast<char**>(_args.data()));
}

struct ShortFlags {
  rfl::Short<"q", rfl::DefaultVal<bool>> quiet = false;
  rfl::Short<"v", std::optional<bool>> verbose;
  rfl::Short<"n", bool> dry_run;
  rfl::Positional<std::optional<std::string>> input;
  rfl::Positional<std::optional<std::string>> output;
};

TEST(cli, test_short_flags_keep_argv_order) {
  const auto result = read<ShortFlags>({"in.rep", "-n", "out.csv", "-q"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().input().value(), "in.rep");
  EXPECT_EQ(result.value().output().value(), "out.csv");
  EXPECT_TRUE(result.value().dry_run());
  EXPECT_TRUE(result.value().quiet().value());

  // Wrapped bools are flags, too.
  const auto wrapped = read<ShortFlags>({"-v", "in.rep", "-q", "out.csv", "-n"});
  ASSERT_TRUE(wrapped) << wrapped.error().what();
  EXPECT_EQ(wrapped.value().verbose(), true);
  EXPECT_EQ(wrapped.value().input().value(), "in.rep");
  EXPECT_EQ(wrapped.value().output().value(), "out.csv");
}

TEST(cli, test_short_flags_explicit_value_is_not_reclaimed) {
  // A value after '=' belongs to the flag, also for a plain bool.
  const auto plain = read<ShortFlags>({"-n=in.rep"});
  ASSERT_FALSE(plain);
  EXPECT_NE(std::string(plain.error().what())
                .find("Could not cast 'in.rep' to boolean for key 'dry-run'."),
            std::string::npos)
      << plain.error().what();

  const auto wrapped = read<ShortFlags>({"-q=in.rep", "-n"});
  ASSERT_FALSE(wrapped);
  EXPECT_NE(std::string(wrapped.error().what())
                .find("Could not cast 'in.rep' to boolean for key 'quiet'."),
            std::string::npos)
      << wrapped.error().what();
}

TEST(cli, test_short_flags_without_options_table) {
  // parse_argv without the options lets -q take "in.rep". resolve_args<T>
  // gives it back to the positional arguments, as for Short<"n", bool>.
  const char* args[] = {"program", "-q", "in.rep", "-n"};
  const auto parsed = rfl::cli::parse_argv(4, const_cast<char**>(args));
  ASSERT_TRUE(parsed) << parsed.error().what();
  EXPECT_EQ(parsed.value().short_args.at("q"), "in.rep");
  const auto resolved = rfl::cli::resolve_args<
      ShortFlags, rfl::Processors<rfl::SnakeCaseToKebabCase>>(parsed.value());
  ASSERT_TRUE(resolved) << resolved.error().what();
  EXPECT_EQ(resolved.value().at("quiet"), "");
  EXPECT_EQ(resolved.value().at("input"), "in.rep");
}

struct ShortValues {
  rfl::Short<"o", std::string> output;
  rfl::Short<"p", rfl::DefaultVal<int>> port = 80;
};

TEST(cli, test_short_missing_value) {
  for (const auto& args : std::vector<std::vector<const char*>>{
           {"-o"}, {"-o", "-p", "1"}, {"-o", "--", "x"}}) {
    const auto result = read<ShortValues>(args);
    ASSERT_FALSE(result);
    EXPECT_EQ(std::string(result.error().what()),
              "Missing value for option '-o'.");
  }
  const auto empty = read<ShortValues>({"-o=", "-p", "-1"});
  ASSERT_TRUE(empty) << empty.error().what();
  EXPECT_EQ(empty.value().output(), "");
  EXPECT_EQ(empty.value().port().value(), -1);
}

struct ShortVariants {
  rfl::Short<"m", std::variant<int, std::string>> mode;
  rfl::Short<"g", std::optional<rfl::Generic>> extra;
  rfl::Positional<std::optional<std::string>> input_file;
};

TEST(cli, test_short_variant_and_generic_take_next_token) {
  // A short alias of a variant or rfl::Generic takes the next token unless
  // it looks like an option.
  const auto word = read<ShortVariants>({"-m", "fast", "in.txt"});
  ASSERT_TRUE(word) << word.error().what();
  EXPECT_EQ(word.value().mode(),
            (std::variant<int, std::string>(std::string("fast"))));
  EXPECT_EQ(word.value().input_file(), "in.txt");

  const auto number = read<ShortVariants>({"-m", "5", "-g", "x"});
  ASSERT_TRUE(number) << number.error().what();
  EXPECT_EQ(number.value().mode(), (std::variant<int, std::string>(5)));
  ASSERT_TRUE(number.value().extra().has_value());
  EXPECT_EQ(number.value().extra()->to_string().value(), "x");
  EXPECT_FALSE(number.value().input_file().has_value());
}

// Two fields with the same Short compile, but every read fails.
struct DuplicateShort {
  rfl::Short<"o", std::optional<std::string>> a;
  rfl::Short<"o", std::optional<std::string>> b;
};

TEST(cli, test_duplicate_short_name_fails_every_read) {
  for (const auto& args : std::vector<std::vector<const char*>>{
           {}, {"-o", "x"}, {"--a=x", "--b=y"}}) {
    const auto result = read<DuplicateShort>(args);
    ASSERT_FALSE(result);
    EXPECT_EQ(std::string(result.error().what()),
              "Duplicate short name '-o' in struct definition.");
  }

  const char* argv[] = {"program", "--a=x"};
  const auto resolved =
      rfl::cli::parse_argv(2, argv)
          .and_then(rfl::cli::resolve_args<
                    DuplicateShort, rfl::Processors<rfl::SnakeCaseToKebabCase>>);
  ASSERT_FALSE(resolved);
  EXPECT_EQ(std::string(resolved.error().what()),
            "Duplicate short name '-o' in struct definition.");
}

// The Short and Positional fields of a root rfl::NamedTuple, whose names the
// processors leave alone (output_dir is --output_dir, not --output-dir).
using RootTuple =
    rfl::NamedTuple<rfl::Field<"input_file", rfl::Positional<std::string>>,
                    rfl::Field<"output_dir", rfl::Short<"o", std::string>>>;

TEST(cli, test_root_named_tuple_short_and_positional) {
  const auto result = read<RootTuple>({"-o", "out", "in.txt"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().get<"input_file">().value(), "in.txt");
  EXPECT_EQ(result.value().get<"output_dir">().value(), "out");
}

}  // namespace test_option_table
