#include <gtest/gtest.h>

#include <array>
#include <list>
#include <ranges>
#include <rfl.hpp>
#include <rfl/cli.hpp>
#include <span>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace test_const_argv_and_ranges {

struct Config {
  std::string host;
  int port;
};

struct PositionalConfig {
  rfl::Positional<std::string> input_file;
  bool verbose;
};

struct OptionalConfig {
  std::optional<std::string> host;
};

struct CopyConfig {
  rfl::Positional<std::string> from;
  rfl::Positional<std::string> to;
};

struct NumberConfig {
  rfl::Positional<int> count;
};

template <class R>
concept Readable = requires(R&& _r) {
  rfl::cli::read<Config>(std::forward<R>(_r));
  rfl::cli::parse_argv(std::forward<R>(_r));
};

struct CopyString {
  std::string operator()(const std::string& _s) const { return _s; }
};

struct ViewString {
  std::string_view operator()(const std::string& _s) const { return _s; }
};

using Strings = std::vector<std::string>;

// The parser keeps a std::string_view to every narrow element, so a
// single-pass range (the next read overwrites the element) is rejected...
using IstreamView =
    std::ranges::basic_istream_view<std::string, char, std::char_traits<char>>;
static_assert(!Readable<IstreamView>);

// ...and so is a range of temporary std::strings (the view would dangle).
using TemporaryStrings =
    decltype(std::declval<Strings&>() | std::views::transform(CopyString()));
static_assert(!Readable<TemporaryStrings>);

// Multi-pass ranges of lvalue strings, pointers or string_views are fine.
static_assert(Readable<Strings&>);
static_assert(Readable<Strings>);
static_assert(Readable<std::list<std::string>&>);
static_assert(Readable<std::initializer_list<std::string_view>>);
static_assert(Readable<decltype(std::declval<Strings&>() |
                                std::views::transform(ViewString()))>);

// Wide elements are converted to UTF-8 one at a time, so a single-pass range
// of wide strings is fine.
static_assert(Readable<std::ranges::basic_istream_view<
                  std::wstring, wchar_t, std::char_traits<wchar_t>>>);

// There are (int, char**) overloads, so `&rfl::cli::read<T>` converts to a
// function pointer taking (int, char**).
using ReadFn = rfl::Result<Config> (*)(int, char**);
using ReadWideFn = rfl::Result<Config> (*)(int, wchar_t**);
using ParseFn = rfl::Result<rfl::cli::ParsedArgs> (*)(int, char**);
using ParseWideFn = rfl::Result<rfl::cli::ParsedArgs> (*)(int, wchar_t**);

template <class R>
void expect_config(R&& _args) {
  const auto result = rfl::cli::read<Config>(std::forward<R>(_args));
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().host, "localhost");
  EXPECT_EQ(result.value().port, 8080);
}

TEST(cli, test_address_of_mutable_argv_overloads) {
  const ReadFn read = &rfl::cli::read<Config>;
  const ReadWideFn read_wide = &rfl::cli::read<Config>;
  const ParseFn parse = &rfl::cli::parse_argv;
  const ParseWideFn parse_wide = &rfl::cli::parse_argv;

  char program[] = "program";
  char host[] = "--host=localhost";
  char port[] = "--port=8080";
  char* argv[] = {program, host, port};
  const auto result = read(3, argv);
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().port, 8080);
  ASSERT_TRUE(parse(3, argv));
  EXPECT_NE(read_wide, nullptr);
  EXPECT_NE(parse_wide, nullptr);
}

TEST(cli, test_const_argv) {
  const auto read = [](auto _argv) {
    const auto result = rfl::cli::read<Config>(3, _argv);
    ASSERT_TRUE(result) << result.error().what();
    EXPECT_EQ(result.value().host, "localhost");
    EXPECT_EQ(result.value().port, 8080);
    ASSERT_TRUE(rfl::cli::parse_argv(3, _argv));
  };

  const char* argv[] = {"program", "--host=localhost", "--port=8080"};
  read(argv);
  const char** pointer = argv;
  read(pointer);
  const char* const const_argv[] = {"program", "--host=localhost",
                                    "--port=8080"};
  read(const_argv);
  const wchar_t* wide[] = {L"program", L"--host=localhost", L"--port=8080"};
  read(wide);
}

TEST(cli, test_const_argv_still_skips_program_name) {
  // argv[0] is the program name, even if it looks like a positional.
  const char* argv[] = {"in.txt", "out.txt", "--verbose"};
  const auto result = rfl::cli::read<PositionalConfig>(3, argv);
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().input_file(), "out.txt");
}

TEST(cli, test_const_argv_invalid_argc) {
  const char* argv[] = {"program"};
  const auto result = rfl::cli::read<OptionalConfig>(-1, argv);
  ASSERT_FALSE(result);
  EXPECT_EQ(result.error().what(), "Invalid argc/argv.");
}

TEST(cli, test_null_element) {
  const char* argv[] = {"program", "--host=localhost", nullptr};
  const auto a = rfl::cli::read<Config>(3, argv);
  ASSERT_FALSE(a);
  EXPECT_EQ(a.error().what(), "Argument 2 is a null pointer.");

  const std::vector<const char*> args = {"--host=localhost", nullptr};
  const auto b = rfl::cli::read<Config>(args);
  ASSERT_FALSE(b);
  EXPECT_EQ(b.error().what(), "Argument 1 is a null pointer.");

  const wchar_t* wide[] = {L"--host=localhost", nullptr};
  const auto c = rfl::cli::read<Config>(wide);
  ASSERT_FALSE(c);
  EXPECT_EQ(c.error().what(), "Argument 1 is a null pointer.");
}

TEST(cli, test_wide_argv_program_name_is_not_converted) {
  // An unpaired surrogate is not valid UTF-16 or UTF-32.
  const wchar_t bad[] = {static_cast<wchar_t>(0xD800), L'\0'};
  const wchar_t* argv[] = {bad, L"--host=localhost", L"--port=8080"};
  const auto result = rfl::cli::read<Config>(3, argv);
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().port, 8080);

  const wchar_t* argv2[] = {L"program", L"--host=localhost", bad};
  const auto result2 = rfl::cli::read<Config>(3, argv2);
  ASSERT_FALSE(result2);
  EXPECT_EQ(result2.error().what(),
            "Could not convert argument 2 from wide to UTF-8.");
}

TEST(cli, test_sequence_containers) {
  expect_config(std::vector<const char*>{"--host=localhost", "--port=8080"});
  expect_config(std::vector<std::string>{"--host=localhost", "--port=8080"});
  expect_config(
      std::vector<std::string_view>{"--host=localhost", "--port=8080"});
  expect_config(std::list<std::string>{"--host=localhost", "--port=8080"});
  expect_config(
      std::array<const char*, 2>{"--host=localhost", "--port=8080"});
  const char* const array[] = {"--host=localhost", "--port=8080"};
  expect_config(array);
  const std::vector<std::string> storage = {"--host=localhost",
                                            "--port=8080"};
  expect_config(storage | std::views::transform(ViewString()));

  const auto braced =
      rfl::cli::read<Config>({"--host=localhost", "--port=8080"});
  ASSERT_TRUE(braced) << braced.error().what();
  EXPECT_EQ(braced.value().port, 8080);

  const auto empty = rfl::cli::read<OptionalConfig>({});
  ASSERT_TRUE(empty) << empty.error().what();
  EXPECT_FALSE(empty.value().host);
}

TEST(cli, test_sequence_does_not_skip_first_element) {
  const char* args[] = {"in.txt", "--verbose"};
  const auto result = rfl::cli::read<PositionalConfig>(args);
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().input_file(), "in.txt");
  EXPECT_TRUE(result.value().verbose);
}

TEST(cli, test_sequence_first_element_is_an_option) {
  // With argc/argv, "--host=localhost" would be dropped as the program name.
  const char* args[] = {"--host=localhost", "--port=8080"};
  const auto result = rfl::cli::read<Config>(std::span(args));
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().host, "localhost");
}

TEST(cli, test_sequence_over_whole_argv_parses_program_name) {
  // The pitfall the docs warn about: std::span(argv, argc) keeps argv[0].
  const char* argv[] = {"program", "--verbose"};
  const auto result = rfl::cli::read<PositionalConfig>(std::span(argv, 2));
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().input_file(), "program");
}

TEST(cli, test_sequence_over_rest_of_mutable_argv) {
  // The program handles "program serve" itself and passes on the rest.
  char program[] = "program";
  char subcommand[] = "serve";
  char host[] = "--host=localhost";
  char port[] = "--port=8080";
  char* argv[] = {program, subcommand, host, port};
  const int argc = 4;
  const auto rest = std::span(argv, argc).subspan(2);
  const auto result = rfl::cli::read<Config>(rest);
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().host, "localhost");
  EXPECT_EQ(result.value().port, 8080);
}

TEST(cli, test_sequence_empty) {
  const auto result =
      rfl::cli::read<OptionalConfig>(std::span<const char* const>());
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_FALSE(result.value().host);
}

TEST(cli, test_sequence_missing_field) {
  const std::vector<const char*> args = {"--host=localhost"};
  EXPECT_FALSE(rfl::cli::read<Config>(args));
}

TEST(cli, test_sequence_negative_positional) {
  const std::vector<std::string> args = {"-5"};
  const auto result = rfl::cli::read<NumberConfig>(args);
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().count(), -5);
}

TEST(cli, test_wide_sequence_from_istream) {
  std::wistringstream in(L"from.txt to__.txt");
  const auto result = rfl::cli::read<CopyConfig>(
      std::ranges::basic_istream_view<std::wstring, wchar_t,
                                      std::char_traits<wchar_t>>(in));
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().from(), "from.txt");
  EXPECT_EQ(result.value().to(), "to__.txt");
}

TEST(cli, test_wide_sequence) {
  expect_config(std::vector<std::wstring>{L"--host=localhost", L"--port=8080"});

  // The first element is parsed, as with narrow strings.
  const wchar_t* args[] = {L"in.txt", L"--verbose"};
  const auto result = rfl::cli::read<PositionalConfig>(args);
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().input_file(), "in.txt");
}

TEST(cli, test_parse_argv_sequence) {
  const char* args[] = {"first", "--host=localhost", "-p", "8080"};
  const auto result = rfl::cli::parse_argv(args);
  ASSERT_TRUE(result) << result.error().what();
  ASSERT_EQ(result.value().positional.size(), 1u);
  EXPECT_EQ(result.value().positional[0], "first");
  EXPECT_EQ(result.value().named.at("host"), "localhost");
  EXPECT_EQ(result.value().short_args.at("p"), "8080");

  const auto braced = rfl::cli::parse_argv({"first", "--host=localhost"});
  ASSERT_TRUE(braced) << braced.error().what();
  ASSERT_EQ(braced.value().positional.size(), 1u);
  EXPECT_EQ(braced.value().positional[0], "first");
}

TEST(cli, test_parse_argv_const_argv_skips_program_name) {
  const char* argv[] = {"first", "second"};
  const auto result = rfl::cli::parse_argv(2, argv);
  ASSERT_TRUE(result) << result.error().what();
  ASSERT_EQ(result.value().positional.size(), 1u);
  EXPECT_EQ(result.value().positional[0], "second");
}

}  // namespace test_const_argv_and_ranges
