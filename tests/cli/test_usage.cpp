#include <gtest/gtest.h>

#include <map>
#include <optional>
#include <rfl.hpp>
#include <rfl/cli.hpp>
#include <string>
#include <vector>

namespace test_usage {

enum class LogLevel { debug, info, warn };

struct Database {
  std::string host;
  int port;
};

struct Config {
  rfl::Positional<std::string> input_file;
  std::string host_name;
  bool verbose;
  std::vector<std::string> tags;
  rfl::Short<"p", int> port;
  Database database;
  std::optional<LogLevel> log_level;
};

TEST(cli, test_usage_synopsis) {
  // The exact string proposed in #692.
  EXPECT_EQ(rfl::cli::usage<Config>("my_binary"),
            "Usage: my_binary <input-file> --host-name=<value> "
            "--verbose[=<bool>] --tags=<v1,v2,...> -p|--port=<value> "
            "--database.host=<value> --database.port=<value> "
            "[--log-level=<debug|info|warn>]\n");
}

struct Described {
  rfl::Positional<rfl::Description<"Path to the input file", std::string>>
      input_file;
  rfl::Description<"Host to connect to", std::string> host_name;
  bool verbose;
  rfl::Short<"p", rfl::Description<"Port to connect on", int>> port;
  rfl::Description<"Minimum level to log", std::optional<LogLevel>> log_level;
};

TEST(cli, test_usage_detailed) {
  EXPECT_EQ(
      rfl::cli::usage<Described>("my_binary", true),
      "Usage: my_binary <input-file> --host-name=<value> --verbose[=<bool>] "
      "-p|--port=<value> [--log-level=<debug|info|warn>]\n"
      "\n"
      "Arguments:\n"
      "  <input-file>                   Path to the input file. Required.\n"
      "  --host-name=<value>            Host to connect to. Required.\n"
      "  --verbose[=<bool>]             Required.\n"
      "  -p, --port=<value>             Port to connect on. Required.\n"
      "  --log-level=<debug|info|warn>  Minimum level to log\n");

  // What the help advertises is what the parser accepts.
  const char* args[] = {"my_binary",   "in.txt", "--host-name=localhost",
                        "--verbose",   "-p",     "8080",
                        "--log-level=warn"};
  const auto result =
      rfl::cli::read<Described>(7, const_cast<char**>(args));
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().input_file().get(), "in.txt");
  EXPECT_EQ(result.value().port().get(), 8080);
  EXPECT_EQ(result.value().log_level.get(), LogLevel::warn);

  // A plain bool is required, as the help says.
  const char* no_verbose[] = {"my_binary", "in.txt", "--host-name=localhost",
                              "-p", "8080"};
  EXPECT_FALSE(rfl::cli::read<Described>(5, const_cast<char**>(no_verbose)));
}

struct DescriptionOutside {
  rfl::Description<"Path to the input file", rfl::Positional<std::string>>
      input_file;
};

TEST(cli, test_usage_description_outside_positional) {
  // The parser only treats a field as positional if rfl::Positional is the
  // outermost wrapper, so the help shows a named argument.
  EXPECT_EQ(rfl::cli::usage<DescriptionOutside>("app", true),
            "Usage: app --input-file=<value>\n"
            "\n"
            "Arguments:\n"
            "  --input-file=<value>  Path to the input file. Required.\n");

  const char* positional[] = {"app", "in.txt"};
  EXPECT_FALSE(
      rfl::cli::read<DescriptionOutside>(2, const_cast<char**>(positional)));
  const char* named[] = {"app", "--input-file=in.txt"};
  EXPECT_TRUE(rfl::cli::read<DescriptionOutside>(2, const_cast<char**>(named)));
}

struct Validated {
  rfl::Description<"Port to listen on",
                   rfl::Validator<int, rfl::Minimum<1>, rfl::Maximum<65535>>>
      port;
  rfl::Validator<double, rfl::ExclusiveMinimum<0>, rfl::Maximum<1>> ratio;
  rfl::Validator<int, rfl::ExclusiveMaximum<100>> percent;
  rfl::Validator<int, rfl::AnyOf<rfl::Maximum<0>, rfl::Minimum<10>>> outside;
  rfl::Validator<int, rfl::OneOf<rfl::EqualTo<1>, rfl::EqualTo<2>>> one_or_two;
  rfl::Validator<int, rfl::NotEqualTo<0>> nonzero;
  rfl::Validator<std::vector<std::string>, rfl::Size<rfl::Minimum<1>>> tags;
  rfl::Validator<std::vector<int>,
                 rfl::Size<rfl::AllOf<rfl::Minimum<1>, rfl::Maximum<3>>>>
      ids;
  rfl::Validator<std::string, rfl::Size<rfl::Minimum<1>>,
                 rfl::PatternValidator<"^[a-z]+$", "Lowercase">>
      user;
  rfl::Literal<"fast", "slow"> mode;
};

TEST(cli, test_usage_validators) {
  EXPECT_EQ(rfl::cli::usage<Validated>("app", true),
            "Usage: app --port=<value> --ratio=<value> --percent=<value> "
            "--outside=<value> --one-or-two=<value> --nonzero=<value> "
            "--tags=<v1,v2,...> --ids=<v1,v2,...> --user=<value> "
            "--mode=<fast|slow>\n"
            "\n"
            "Arguments:\n"
            "  --port=<value>        Port to listen on. Range: 1..65535. "
            "Required.\n"
            "  --ratio=<value>       Range: > 0 and <= 1. Required.\n"
            "  --percent=<value>     Range: < 100. Required.\n"
            "  --outside=<value>     Range: <= 0 or >= 10. Required.\n"
            "  --one-or-two=<value>  Range: 1 or 2. Required.\n"
            "  --nonzero=<value>     Range: != 0. Required.\n"
            "  --tags=<v1,v2,...>    Size: >= 1. Required.\n"
            "  --ids=<v1,v2,...>     Size: 1..3. Required.\n"
            "  --user=<value>        Size: >= 1. Pattern: ^[a-z]+$. "
            "Required.\n"
            "  --mode=<fast|slow>    Required.\n");
}

struct WithDefaultVal {
  rfl::DefaultVal<int> port = 8080;
  rfl::DefaultVal<std::string> host = "localhost";
  rfl::DefaultVal<std::vector<std::string>> tags =
      std::vector<std::string>{"a", "b"};
  rfl::DefaultVal<LogLevel> level = LogLevel::info;
  rfl::DefaultVal<double> rate = 0.25;
  rfl::DefaultVal<bool> dry_run = false;
  int count = 3;
  rfl::Short<"r", rfl::DefaultVal<int>> retries = 5;
};

TEST(cli, test_usage_default_val) {
  // Without rfl::DefaultIfMissing, `count` is required, so its member
  // initializer is never used and not shown. The parser only treats a field
  // as optional if rfl::DefaultVal is its outermost wrapper, so `retries` is
  // required, too.
  EXPECT_EQ(rfl::cli::usage<WithDefaultVal>("app", true),
            "Usage: app [--port=<value>] [--host=<value>] "
            "[--tags=<v1,v2,...>] [--level=<debug|info|warn>] "
            "[--rate=<value>] [--dry-run[=<bool>]] --count=<value> "
            "-r|--retries=<value>\n"
            "\n"
            "Arguments:\n"
            "  --port=<value>             Default: 8080.\n"
            "  --host=<value>             Default: localhost.\n"
            "  --tags=<v1,v2,...>         Default: a,b.\n"
            "  --level=<debug|info|warn>  Default: info.\n"
            "  --rate=<value>             Default: 0.25.\n"
            "  --dry-run[=<bool>]         Default: false.\n"
            "  --count=<value>            Required.\n"
            "  -r, --retries=<value>      Required.\n");

  const char* args[] = {"app", "--count=1", "-r", "2"};
  const auto result =
      rfl::cli::read<WithDefaultVal>(4, const_cast<char**>(args));
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().port.get(), 8080);
  EXPECT_EQ(result.value().host.get(), "localhost");

  const char* no_retries[] = {"app", "--count=1"};
  EXPECT_FALSE(
      rfl::cli::read<WithDefaultVal>(2, const_cast<char**>(no_retries)));
}

/// The default-constructed `port` does not satisfy its validator.
struct WithInvalidDefault {
  rfl::Validator<int, rfl::Minimum<1>> port;
  rfl::DefaultVal<int> threads = 4;
};

TEST(cli, test_usage_default_val_struct_not_default_constructible) {
  // The defaults are read from a default-constructed struct, which throws
  // here. The help leaves them out instead of aborting.
  EXPECT_EQ(rfl::cli::usage<WithInvalidDefault>("app", true),
            "Usage: app --port=<value> [--threads=<value>]\n"
            "\n"
            "Arguments:\n"
            "  --port=<value>     Range: >= 1. Required.\n"
            "  --threads=<value>\n");
}

struct Inner {
  std::string host = "db";
  int port = 5432;
};

struct WithInitializers {
  int count = 3;
  std::string name;
  bool verbose = false;
  std::optional<int> limit;
  Inner database;
};

TEST(cli, test_usage_default_if_missing) {
  EXPECT_EQ(
      (rfl::cli::usage<WithInitializers, rfl::DefaultIfMissing>("app", true)),
      "Usage: app [--count=<value>] [--name=<value>] [--verbose[=<bool>]] "
      "[--limit=<value>] [--database.host=<value>] [--database.port=<value>]\n"
      "\n"
      "Arguments:\n"
      "  --count=<value>          Default: 3.\n"
      "  --name=<value>           Default: \"\".\n"
      "  --verbose[=<bool>]       Default: false.\n"
      "  --limit=<value>\n"
      "  --database.host=<value>  Default: db.\n"
      "  --database.port=<value>  Default: 5432.\n");

  const char* args[] = {"app"};
  const auto result = rfl::cli::read<WithInitializers, rfl::DefaultIfMissing>(
      1, const_cast<char**>(args));
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().count, 3);
  EXPECT_EQ(result.value().database.port, 5432);
}

TEST(cli, test_usage_without_default_if_missing) {
  EXPECT_EQ(rfl::cli::usage<WithInitializers>("app"),
            "Usage: app --count=<value> --name=<value> --verbose[=<bool>] "
            "[--limit=<value>] --database.host=<value> "
            "--database.port=<value>\n");

  const char* args[] = {"app"};
  EXPECT_FALSE(rfl::cli::read<WithInitializers>(1, const_cast<char**>(args)));
}

/// Has no default constructor, so neither has a struct that contains it.
class Port {
 public:
  using ReflectionType = int;
  Port(int _value) : value_(_value) {}
  int reflection() const { return value_; }

 private:
  int value_;
};

struct WithPort {
  Port port;
  int retries = 3;
};

TEST(cli, test_usage_default_if_missing_not_default_constructible) {
  static_assert(!std::is_default_constructible_v<WithPort>);
  // Nothing is required, but the defaults cannot be known without T{}, so
  // usage() still compiles and leaves them out.
  EXPECT_EQ((rfl::cli::usage<WithPort, rfl::DefaultIfMissing>("app", true)),
            "Usage: app [--port=<value>] [--retries=<value>]\n"
            "\n"
            "Arguments:\n"
            "  --port=<value>\n"
            "  --retries=<value>\n");
}

struct Address {
  std::string city;
  int zip;
};

struct Names {
  rfl::Rename<"output_dir", std::string> output_dir;
  rfl::Flatten<Address> address;
  std::optional<Database> backup;
  std::map<std::string, std::string> labels;
};

TEST(cli, test_usage_names) {
  EXPECT_EQ(rfl::cli::usage<Names>("app", true),
            "Usage: app --output_dir=<value> --city=<value> --zip=<value> "
            "[--backup.host=<value>] [--backup.port=<value>] "
            "--labels.<key>=<value>\n"
            "\n"
            "Arguments:\n"
            "  --output_dir=<value>    Required.\n"
            "  --city=<value>          Required.\n"
            "  --zip=<value>           Required.\n"
            "  --backup.host=<value>\n"
            "  --backup.port=<value>\n"
            "  --labels.<key>=<value>  Required.\n");

  const char* args[] = {"app", "--output_dir=/tmp", "--city=Springfield",
                        "--zip=12345", "--labels.env=prod"};
  const auto result = rfl::cli::read<Names>(5, const_cast<char**>(args));
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result.value().labels.at("env"), "prod");
}

struct Optionals {
  rfl::Positional<std::optional<LogLevel>> level;
  rfl::Short<"v", std::optional<bool>> verbose;
  rfl::Deprecated<"Use --port instead", "Old port", std::optional<int>>
      old_port;
  rfl::Description<"First line\nSecond line", std::string> notes;
};

TEST(cli, test_usage_optionals) {
  EXPECT_EQ(rfl::cli::usage<Optionals>("app", true),
            "Usage: app [<level>] [-v|--verbose[=<bool>]] "
            "[--old-port=<value>] --notes=<value>\n"
            "\n"
            "Arguments:\n"
            "  <level>                 Choices: debug|info|warn.\n"
            "  -v, --verbose[=<bool>]\n"
            "  --old-port=<value>      Old port. Deprecated: Use --port "
            "instead.\n"
            "  --notes=<value>         First line\n"
            "                          Second line. Required.\n");
}

TEST(cli, test_usage_no_optionals) {
  // Processors are applied exactly as in read: with rfl::NoOptionals,
  // optional fields are required.
  EXPECT_EQ((rfl::cli::usage<Optionals, rfl::NoOptionals>("app")),
            "Usage: app <level> -v|--verbose[=<bool>] --old-port=<value> "
            "--notes=<value>\n");
}

struct Empty {};

TEST(cli, test_usage_empty) {
  EXPECT_EQ(rfl::cli::usage<Empty>("app"), "Usage: app\n");
  EXPECT_EQ(rfl::cli::usage<Empty>("app", true), "Usage: app\n");
}

}  // namespace test_usage
