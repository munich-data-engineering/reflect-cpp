#include <gtest/gtest.h>

#include <array>
#include <map>
#include <rfl.hpp>
#include <rfl/json.hpp>
#include <set>
#include <string>
#include <tuple>
#include <variant>
#include <vector>

namespace test_error_details {

using Path = std::vector<rfl::ErrorDetail::PathSegment>;

struct FileSpec {
  rfl::Validator<std::string, rfl::Size<rfl::Minimum<5>>> input_file;
  std::string output_file;
};

struct Request {
  std::string name;
  std::vector<FileSpec> files;
};

TEST(json, test_error_details_of_top_level_fields) {
  const auto result =
      rfl::json::read<FileSpec>(R"({"input_file":"abcd","unknown":1})");
  ASSERT_FALSE(result);

  EXPECT_EQ(result.error().what(), R"(Found 2 errors:
1) Failed to parse field 'input_file': Size validation failed: Value expected to be greater than or equal to 5, but got 4.
2) Field named 'output_file' not found.)");

  const auto expected = std::vector<rfl::ErrorDetail>(
      {{.path = {"input_file"},
        .message = "Size validation failed: Value expected to be greater "
                   "than or equal to 5, but got 4."},
       {.path = {"output_file"},
        .message = "Field named 'output_file' not found."}});
  EXPECT_EQ(result.error().details(), expected);
}

TEST(json, test_error_details_of_nested_fields) {
  const auto result = rfl::json::read<Request, rfl::SnakeCaseToCamelCase>(
      R"({"name":"job","files":[
        {"inputFile":"first.txt","outputFile":"first.out"},
        {"inputFile":"second.txt","outputFile":"second.out"},
        {"inputFile":"abc","outputFile":1}]})");
  ASSERT_FALSE(result);

  const auto details = result.error().details();
  ASSERT_EQ(details.size(), 2);
  EXPECT_EQ(details[0].path, (Path{"files", size_t{2}, "inputFile"}));
  EXPECT_EQ(details[0].message,
            "Size validation failed: Value expected to be greater than or "
            "equal to 5, but got 3.");
  EXPECT_EQ(details[1].path, (Path{"files", size_t{2}, "outputFile"}));
  EXPECT_EQ(details[1].message, "Could not cast to string.");
}

TEST(json, test_error_details_of_root_error) {
  const auto result = rfl::json::read<Request>(R"({"name":"job";})");
  ASSERT_FALSE(result);

  const auto details = result.error().details();
  ASSERT_EQ(details.size(), 1);
  EXPECT_TRUE(details[0].path.empty());
  EXPECT_EQ(details[0].message, result.error().what());
}

TEST(json, test_error_details_of_containers) {
  struct Containers {
    std::map<std::string, int> map;
    std::array<int, 2> array;
    std::tuple<int, std::string> tuple;
    std::set<int> set;
  };

  const auto result = rfl::json::read<Containers>(
      R"({"map":{"a":1,"b":"two"},"array":[1,"two"],"tuple":[1,2],
          "set":[1,1,"three"]})");
  ASSERT_FALSE(result);

  const auto details = result.error().details();
  ASSERT_EQ(details.size(), 4);
  EXPECT_EQ(details[0].path, (Path{"map", "b"}));
  EXPECT_EQ(details[0].message, "Could not cast to int.");
  EXPECT_EQ(details[1].path, (Path{"array", size_t{1}}));
  EXPECT_EQ(details[1].message, "Could not cast to int.");
  EXPECT_EQ(details[2].path, (Path{"tuple", size_t{1}}));
  EXPECT_EQ(details[2].message, "Could not cast to string.");
  // The duplicate does not end up in the set, but it still counts as an
  // element of the input.
  EXPECT_EQ(details[3].path, (Path{"set", size_t{2}}));
}

TEST(json, test_error_details_of_extra_fields) {
  const auto result = rfl::json::read<FileSpec, rfl::NoExtraFields>(
      R"({"input_file":"input.txt","output_file":"output.txt","extra":1})");
  ASSERT_FALSE(result);

  const auto details = result.error().details();
  ASSERT_EQ(details.size(), 1);
  EXPECT_EQ(details[0].path, (Path{"extra"}));
  EXPECT_EQ(details[0].message, "Value named 'extra' not used.");
}

TEST(json, test_error_details_with_fail_fast) {
  const auto result = rfl::json::read<Request, rfl::FailFast>(
      R"({"name":"job","files":[{"input_file":"abc"}]})");
  ASSERT_FALSE(result);

  const auto details = result.error().details();
  ASSERT_EQ(details.size(), 1);
  EXPECT_EQ(details[0].path, (Path{"files", size_t{0}, "input_file"}));
}

TEST(json, test_error_details_with_default_if_missing) {
  struct WithDefaults {
    int count = 0;
    std::string label;
  };

  const auto result = rfl::json::read<WithDefaults, rfl::DefaultIfMissing>(
      R"({"count":"many"})");
  ASSERT_FALSE(result);

  const auto details = result.error().details();
  ASSERT_EQ(details.size(), 1);
  EXPECT_EQ(details[0].path, (Path{"count"}));
  EXPECT_EQ(details[0].message, "Could not cast to int.");
}

struct Circle {
  rfl::Validator<int, rfl::Minimum<0>> radius;
};

struct Square {
  double width;
};

struct Drawing {
  rfl::TaggedUnion<"shape", Circle, Square> frame;
  std::vector<rfl::TaggedUnion<"shape", Circle, Square>> shapes;
  std::variant<int, std::string> id;
};

TEST(json, test_error_details_of_tagged_unions_and_variants) {
  const auto result = rfl::json::read<Drawing>(
      R"({"frame":{"shape":"Square"},
          "shapes":[{"shape":"Square","width":1.0},
                    {"shape":"Circle","radius":-1}],
          "id":1.5})");
  ASSERT_FALSE(result);

  const auto details = result.error().details();
  ASSERT_EQ(details.size(), 3) << result.error().what();

  // The tag selects the alternative, so failures inside it keep their
  // location.
  EXPECT_EQ(details[0].path, (Path{"frame", "width"}));
  EXPECT_EQ(details[0].message, "Field named 'width' not found.");
  EXPECT_EQ(details[1].path, (Path{"shapes", size_t{1}, "radius"}));
  EXPECT_EQ(details[1].message,
            "Value expected to be greater than or equal to 0, but got -1.");

  // For variants without a tag, the failures of the individual alternatives
  // are reported as a single failure of the variant.
  EXPECT_EQ(details[2].path, (Path{"id"}));
  EXPECT_TRUE(details[2].message.starts_with("Could not parse the variant."))
      << details[2].message;
}

}  // namespace test_error_details
