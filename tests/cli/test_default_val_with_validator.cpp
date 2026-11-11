#include <gtest/gtest.h>

#include <rfl.hpp>
#include <rfl/cli.hpp>
#include <string>
#include <vector>

namespace test_default_val_with_validator {

using Path = std::vector<rfl::ErrorDetail::PathSegment>;

std::vector<Path> paths(const rfl::Error& _err) {
  std::vector<Path> result;
  for (const auto& detail : _err.details()) {
    result.push_back(detail.path);
  }
  return result;
}

using Positive = rfl::Validator<int, rfl::Minimum<1>>;

struct Config {
  rfl::DefaultVal<int> port = 8080;
  rfl::Validator<std::string, rfl::Size<rfl::Minimum<1>>> host_name;
  rfl::DefaultVal<Positive> max_retries;
};

TEST(cli, test_default_val_with_validator) {
  const char* args[] = {"program", "--host-name=srv", "--max-retries=3"};
  const auto res = rfl::cli::read<Config>(3, const_cast<char**>(args));
  ASSERT_TRUE(res) << res.error().what();
  EXPECT_EQ(res->port.value(), 8080);
  EXPECT_EQ(res->host_name.value(), "srv");
  EXPECT_EQ(res->max_retries.value().value(), 3);
}

TEST(cli, test_default_val_with_validator_invalid_default) {
  const char* args[] = {"program", "--host-name=srv"};
  const auto res = rfl::cli::read<Config>(2, const_cast<char**>(args));
  ASSERT_FALSE(res);
  EXPECT_EQ(res.error().what(),
            "Field named 'max-retries' not found, and its default value is "
            "invalid: Value expected to be greater than or equal to 1, but "
            "got 0.");
  EXPECT_EQ(paths(res.error()), std::vector<Path>({{"max-retries"}}));
}

TEST(cli, test_default_val_with_validator_required) {
  const char* args[] = {"program", "--max-retries=3"};
  const auto res = rfl::cli::read<Config>(2, const_cast<char**>(args));
  ASSERT_FALSE(res);
  EXPECT_EQ(paths(res.error()), std::vector<Path>({{"host-name"}}));
}

}  // namespace test_default_val_with_validator
