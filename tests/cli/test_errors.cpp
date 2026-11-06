#include <gtest/gtest.h>
#include <rfl.hpp>
#include <rfl/cli.hpp>
#include <string>
#include <vector>

namespace test_errors {

struct Config {
  std::string name;
  int port;
};

TEST(cli, test_error_invalid_arg_format) {
  const char* args[] = {"program", "not-a-flag"};
  const auto result = rfl::cli::read<Config>(2, const_cast<char**>(args));
  EXPECT_FALSE(result);
}

TEST(cli, test_error_invalid_int) {
  const char* args[] = {"program", "--name=test", "--port=abc"};
  const auto result = rfl::cli::read<Config>(3, const_cast<char**>(args));
  EXPECT_FALSE(result);
}

TEST(cli, test_error_duplicate_key) {
  const char* args[] = {"program", "--name=a", "--name=b", "--port=1"};
  const auto result = rfl::cli::read<Config>(4, const_cast<char**>(args));
  EXPECT_FALSE(result);
}

struct Files {
  rfl::Validator<std::string, rfl::Size<rfl::Minimum<5>>> input_file;
  std::string output_file;
};

struct Job {
  Files files;
  std::vector<int> retries;
};

TEST(cli, test_error_details) {
  const char* args[] = {"program", "--files.input-file=abcd",
                        "--retries=1,two"};
  const auto result = rfl::cli::read<Job>(3, const_cast<char**>(args));
  ASSERT_FALSE(result);

  using Path = std::vector<rfl::ErrorDetail::PathSegment>;
  const auto details = result.error().details();
  ASSERT_EQ(details.size(), 3u) << result.error().what();
  EXPECT_EQ(details[0].path, (Path{"files", "input-file"}));
  EXPECT_EQ(details[0].message,
            "Size validation failed: Value expected to be greater than or "
            "equal to 5, but got 4.");
  EXPECT_EQ(details[1].path, (Path{"files", "output-file"}));
  EXPECT_EQ(details[1].message, "Field named 'output-file' not found.");
  EXPECT_EQ(details[2].path, (Path{"retries", size_t{1}}));
}

}  // namespace test_errors
