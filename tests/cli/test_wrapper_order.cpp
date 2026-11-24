#include <gtest/gtest.h>

#include <optional>
#include <rfl.hpp>
#include <rfl/cli.hpp>
#include <string>
#include <vector>

// rfl::Short and rfl::Positional work through rfl::DefaultVal, std::optional,
// rfl::Description, rfl::Rename and rfl::Deprecated, in either order.
namespace test_wrapper_order {

template <class T, class... Ps>
rfl::Result<T> read(std::vector<const char*> _args) {
  _args.insert(_args.begin(), "program");
  return rfl::cli::read<T, Ps...>(static_cast<int>(_args.size()),
                                  const_cast<char**>(_args.data()));
}

struct Shorts {
  rfl::DefaultVal<rfl::Short<"o", std::string>> output = std::string("o");
  rfl::Short<"O", std::optional<std::string>> other_output;
  std::optional<rfl::Short<"f", std::string>> format;
  rfl::Short<"F", std::optional<std::string>> other_format;
  rfl::Description<"Log file", rfl::Short<"l", std::optional<std::string>>>
      log;
  rfl::Short<"L", rfl::Description<"Level", std::optional<int>>> level;
  rfl::Rename<"outDir", rfl::Short<"d", std::optional<std::string>>> out_dir;
  rfl::Deprecated<"Use -j.", "Threads", rfl::Short<"t", std::optional<int>>>
      threads;
  rfl::Short<"j", rfl::Deprecated<"", "Jobs", std::optional<int>>> jobs;
  rfl::Description<"Title", std::optional<rfl::Short<"T", std::string>>>
      title;
};

TEST(cli, test_short_through_wrappers_given) {
  const auto result = read<Shorts, rfl::NoExtraFields>(
      {"-o", "a", "-O", "b", "-f", "c", "-F", "d", "-l", "e", "-L", "4", "-d",
       "f", "-t", "5", "-j", "6", "-T", "g"});
  ASSERT_TRUE(result) << result.error().what();
  const auto& s = result.value();
  EXPECT_EQ(s.output().value(), "a");
  EXPECT_EQ(s.other_output().value(), "b");
  EXPECT_EQ(s.format.value().value(), "c");
  EXPECT_EQ(s.other_format().value(), "d");
  EXPECT_EQ(s.log().value().value(), "e");
  EXPECT_EQ(s.level().value().value(), 4);
  EXPECT_EQ(s.out_dir().value().value(), "f");
  EXPECT_EQ(s.threads().value().value(), 5);
  EXPECT_EQ(s.jobs().value().value(), 6);
  EXPECT_EQ(s.title().value().value(), "g");
}

TEST(cli, test_short_through_wrappers_absent) {
  const auto result = read<Shorts>({});
  ASSERT_TRUE(result) << result.error().what();
  const auto& s = result.value();
  EXPECT_EQ(s.output().value(), "o");
  EXPECT_FALSE(s.other_output().has_value());
  EXPECT_FALSE(s.format.has_value());
  EXPECT_FALSE(s.other_format().has_value());
  EXPECT_FALSE(s.log().value().has_value());
  EXPECT_FALSE(s.level().value().has_value());
  EXPECT_FALSE(s.out_dir().value().has_value());
  EXPECT_FALSE(s.threads().value().has_value());
  EXPECT_FALSE(s.jobs().value().has_value());
  EXPECT_FALSE(s.title().has_value());
}

TEST(cli, test_short_through_wrappers_conflict_with_long) {
  const auto result = read<Shorts>({"-d", "a", "--outDir=b"});
  EXPECT_FALSE(result);
}

// Positional fields take bare arguments in declaration order, wherever
// rfl::Positional sits among their wrappers.
struct Positionals {
  rfl::DefaultVal<rfl::Positional<std::string>> first = std::string("1");
  rfl::Positional<std::optional<std::string>> second;
  rfl::Description<"Third", rfl::Positional<std::optional<std::string>>> third;
  rfl::Positional<rfl::Description<"Fourth", std::optional<std::string>>>
      fourth;
  std::optional<rfl::Positional<std::string>> fifth;
  rfl::Rename<"sixth", rfl::Positional<std::optional<std::string>>> sixth_;
  rfl::Deprecated<"", "Seventh", rfl::Positional<std::optional<std::string>>>
      seventh;
  std::optional<bool> verbose;
};

TEST(cli, test_positional_through_wrappers_given) {
  const auto result = read<Positionals>(
      {"a", "b", "c", "--verbose", "d", "e", "f", "g"});
  ASSERT_TRUE(result) << result.error().what();
  const auto& p = result.value();
  EXPECT_EQ(p.first().value(), "a");
  EXPECT_EQ(p.second().value(), "b");
  EXPECT_EQ(p.third().value().value(), "c");
  EXPECT_EQ(p.fourth().value().value(), "d");
  EXPECT_EQ(p.fifth.value().value(), "e");
  EXPECT_EQ(p.sixth_().value().value(), "f");
  EXPECT_EQ(p.seventh().value().value(), "g");
  EXPECT_EQ(p.verbose, true);
}

TEST(cli, test_positional_through_wrappers_absent) {
  const auto result = read<Positionals>({"a"});
  ASSERT_TRUE(result) << result.error().what();
  const auto& p = result.value();
  EXPECT_EQ(p.first().value(), "a");
  EXPECT_FALSE(p.second().has_value());
  EXPECT_FALSE(p.third().value().has_value());
  EXPECT_FALSE(p.fifth.has_value());
  EXPECT_FALSE(p.seventh().value().has_value());

  const auto none = read<Positionals>({});
  ASSERT_TRUE(none) << none.error().what();
  EXPECT_EQ(none.value().first().value(), "1");
  EXPECT_FALSE(none.value().second().has_value());
}

TEST(cli, test_positional_through_wrappers_by_name) {
  const auto result = read<Positionals>({"--sixth=f", "a"});
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().first().value(), "a");
  EXPECT_EQ(result.value().sixth_().value().value(), "f");
}

struct ShortFlags {
  rfl::Short<"q", rfl::DefaultVal<bool>> quiet = false;
  rfl::Short<"v", std::optional<bool>> verbose;
  rfl::Description<"Dry run", rfl::Short<"n", bool>> dry_run;
  rfl::DefaultVal<rfl::Short<"f", bool>> force = false;
  rfl::Positional<std::optional<std::string>> input;
  rfl::Positional<std::optional<std::string>> output;
};

TEST(cli, test_short_flags_through_wrappers_do_not_consume_token) {
  for (const char* flag : {"-q", "-v", "-n", "-f"}) {
    std::vector<const char*> args = {flag, "in.rep"};
    if (std::string(flag) != "-n") {
      args.push_back("--dry-run=false");
    }
    if (std::string(flag) != "-q") {
      args.push_back("--quiet=false");
    }
    const auto result = read<ShortFlags>(args);
    ASSERT_TRUE(result) << flag << ": " << result.error().what();
    EXPECT_EQ(result.value().input().value(), "in.rep") << flag;
  }

  const auto result =
      read<ShortFlags>({"-q", "-v", "in.rep", "-n", "out.csv", "-f"});
  ASSERT_TRUE(result) << result.error().what();
  const auto& f = result.value();
  EXPECT_TRUE(f.quiet().value());
  EXPECT_EQ(f.verbose(), true);
  EXPECT_TRUE(f.dry_run().value());
  EXPECT_TRUE(f.force().value());
  EXPECT_EQ(f.input().value(), "in.rep");
  EXPECT_EQ(f.output().value(), "out.csv");
}

TEST(cli, test_short_flags_through_wrappers_take_bool_literal) {
  const auto result =
      read<ShortFlags>({"-q", "true", "-v", "0", "-n", "false", "-f=1", "x"});
  ASSERT_TRUE(result) << result.error().what();
  const auto& f = result.value();
  EXPECT_TRUE(f.quiet().value());
  EXPECT_EQ(f.verbose(), false);
  EXPECT_FALSE(f.dry_run().value());
  EXPECT_TRUE(f.force().value());
  EXPECT_EQ(f.input().value(), "x");
}

// Two fields with the same short name fail every read, wherever rfl::Short
// sits among their wrappers. A field with two rfl::Short or rfl::Positional
// wrappers, such as rfl::Short<"o", rfl::Description<"...",
// rfl::Positional<T>>>, does not compile.
struct WrappedAndDirectShort {
  rfl::Description<"Name", rfl::Short<"o", std::string>> name;
  rfl::Short<"o", int> count;
};

TEST(cli, test_wrapped_duplicate_short_name) {
  const auto result = read<WrappedAndDirectShort>({"--count=5", "--name=x"});
  ASSERT_FALSE(result);
  EXPECT_EQ(std::string(result.error().what()),
            "Duplicate short name '-o' in struct definition.");
}

}  // namespace test_wrapper_order
