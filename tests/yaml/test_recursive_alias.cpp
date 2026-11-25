#include <gtest/gtest.h>

#include <optional>
#include <rfl.hpp>
#include <rfl/yaml.hpp>
#include <string>
#include <vector>

namespace test_yaml_recursive_alias {

using Path = std::vector<rfl::ErrorDetail::PathSegment>;

struct Endpoint {
  std::string host;
  int port;
};

struct Cluster {
  Endpoint defaults;
  Endpoint primary;
  Endpoint secondary;
};

struct TreeNode {
  std::string name;
  std::optional<rfl::Box<TreeNode>> child;
};

TEST(yaml, test_non_recursive_alias_allowed) {
  const std::string yaml_str =
      "defaults: &def\n"
      "  host: localhost\n"
      "  port: 8080\n"
      "primary: *def\n"
      "secondary: *def\n";

  const auto result = rfl::yaml::read<Cluster>(yaml_str);
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result->primary.host, "localhost");
  EXPECT_EQ(result->primary.port, 8080);
  EXPECT_EQ(result->secondary.host, "localhost");
  EXPECT_EQ(result->secondary.port, 8080);
}

TEST(yaml, test_recursive_mapping_alias_rejected) {
  const std::string yaml_str =
      "&self\n"
      "name: loop\n"
      "child: *self\n";

  const auto result = rfl::yaml::read<TreeNode>(yaml_str);
  ASSERT_FALSE(result);
  const auto details = result.error().details();
  ASSERT_EQ(details.size(), 1) << result.error().what();
  EXPECT_EQ(details[0].path, (Path{"child"}));
  EXPECT_EQ(details[0].message, "Recursive YAML alias detected.");
}

TEST(yaml, test_recursive_sequence_alias_rejected) {
  const std::string yaml_str =
      "&seq\n"
      "- *seq\n";

  const auto result = rfl::yaml::read<rfl::Generic>(yaml_str);
  ASSERT_FALSE(result);
}

TEST(yaml, test_mutual_recursive_alias_rejected) {
  const std::string yaml_str =
      "&a\n"
      "name: a\n"
      "child: &b\n"
      "  name: b\n"
      "  child: *a\n";

  const auto result = rfl::yaml::read<TreeNode>(yaml_str);
  ASSERT_FALSE(result);
  const auto details = result.error().details();
  ASSERT_EQ(details.size(), 1) << result.error().what();
  EXPECT_EQ(details[0].path, (Path{"child", "child"}));
  EXPECT_EQ(details[0].message, "Recursive YAML alias detected.");
}

}  // namespace test_yaml_recursive_alias
