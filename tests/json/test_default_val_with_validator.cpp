#include <gtest/gtest.h>

#include <array>
#include <exception>
#include <optional>
#include <rfl.hpp>
#include <rfl/json.hpp>
#include <string>
#include <variant>
#include <vector>

namespace test_default_val_with_validator {

using Path = std::vector<rfl::ErrorDetail::PathSegment>;

using Foods =
    rfl::Validator<std::vector<std::string>, rfl::Size<rfl::Minimum<1>>>;
using Positive = rfl::Validator<int, rfl::Minimum<1>>;

const std::string foods_error =
    "Size validation failed: Value expected to be greater than or equal to 1, "
    "but got 0.";
const std::string positive_error =
    "Value expected to be greater than or equal to 1, but got 0.";

struct Person {
  rfl::DefaultVal<std::string> name = "Homer Simpson";
  Foods favorite_foods;
};

TEST(json, test_default_val_with_validator) {
  const auto homer =
      rfl::json::read<Person>(R"({"favorite_foods": ["donuts", "pizza"]})");
  ASSERT_TRUE(homer) << homer.error().what();
  EXPECT_EQ(homer->name.value(), "Homer Simpson");
  EXPECT_EQ(homer->favorite_foods.value(),
            std::vector<std::string>({"donuts", "pizza"}));

  const auto empty = rfl::json::read<Person>(R"({})");
  ASSERT_FALSE(empty);
  EXPECT_EQ(empty.error().what(), "Field named 'favorite_foods' not found.");

  const auto invalid = rfl::json::read<Person>(R"({"favorite_foods": []})");
  ASSERT_FALSE(invalid);
  EXPECT_EQ(invalid.error().what(),
            "Failed to parse field 'favorite_foods': " + foods_error);

  // Reading does not make the default value valid elsewhere.
  EXPECT_THROW((void)Person{}, std::exception);
  EXPECT_THROW((void)Foods{}, std::exception);
}

struct Reversed {
  Foods favorite_foods;
  rfl::DefaultVal<std::string> name = "Homer Simpson";
};

TEST(json, test_default_val_with_validator_reversed) {
  const auto res = rfl::json::read<Reversed>(
      R"({"name": "Bart", "favorite_foods": ["pizza"]})");
  ASSERT_TRUE(res) << res.error().what();
  EXPECT_EQ(rfl::json::write(*res),
            R"({"favorite_foods":["pizza"],"name":"Bart"})");
}

// A missing field keeps its default value, so with DefaultIfMissing or
// DefaultVal that value must be valid.
struct WithDefaultVal {
  rfl::DefaultVal<int> id;
  rfl::DefaultVal<Positive> retries;
};

TEST(json, test_default_val_with_validator_missing) {
  const auto with_dim =
      rfl::json::read<Person, rfl::DefaultIfMissing>(R"({})");
  ASSERT_FALSE(with_dim);
  EXPECT_EQ(with_dim.error().what(),
            "Field named 'favorite_foods' not found, and its default value is "
            "invalid: " +
                foods_error);
  ASSERT_EQ(with_dim.error().details().size(), 1);
  EXPECT_EQ(with_dim.error().details()[0].path, Path({"favorite_foods"}));

  const auto present = rfl::json::read<Person, rfl::DefaultIfMissing>(
      R"({"favorite_foods": ["donuts"]})");
  ASSERT_TRUE(present) << present.error().what();
  EXPECT_EQ(present->name.value(), "Homer Simpson");

  const auto missing = rfl::json::read<WithDefaultVal>(R"({"id": 1})");
  ASSERT_FALSE(missing);
  EXPECT_EQ(missing.error().what(),
            "Field named 'retries' not found, and its default value is "
            "invalid: " +
                positive_error);

  const auto res = rfl::json::read<WithDefaultVal>(R"({"retries": 3})");
  ASSERT_TRUE(res) << res.error().what();
  EXPECT_EQ(res->retries.value().value(), 3);
}

struct Family {
  rfl::DefaultVal<std::string> town = "Springfield";
  Person head;
  std::vector<Person> children;
};

TEST(json, test_default_val_with_validator_nested) {
  const auto res = rfl::json::read<Family>(
      R"({"head": {"favorite_foods": ["donuts"]},
          "children": [{"name": "Bart", "favorite_foods": ["pizza"]}]})");
  ASSERT_TRUE(res) << res.error().what();
  EXPECT_EQ(res->head.name.value(), "Homer Simpson");
  EXPECT_EQ(res->children.at(0).favorite_foods->at(0), "pizza");

  const auto missing =
      rfl::json::read<Family, rfl::DefaultIfMissing>(R"({"children": []})");
  ASSERT_FALSE(missing);
  EXPECT_EQ(missing.error().what(),
            "Field named 'head' not found, and its default value is invalid: " +
                foods_error);

  // The nested struct is read as a whole, so its own missing field is
  // reported, with the full path.
  const auto inner_missing =
      rfl::json::read<Family, rfl::DefaultIfMissing>(R"({"head": {}})");
  ASSERT_FALSE(inner_missing);
  ASSERT_EQ(inner_missing.error().details().size(), 1);
  EXPECT_EQ(inner_missing.error().details()[0].path,
            Path({"head", "favorite_foods"}));
}

struct Flat {
  Positive f2;
};

struct Containers {
  rfl::DefaultVal<int> id;
  std::array<Positive, 2> array;
  std::optional<Positive> optional;
  std::variant<Positive, std::string> variant;
  rfl::TaggedUnion<"type", Person, Flat> tagged;
  rfl::Flatten<Flat> flat;
  rfl::Rename<"renamed", Positive> rename;
  rfl::ExtraFields<Positive> extra;
};

TEST(json, test_default_val_with_validator_containers) {
  const auto res = rfl::json::read<Containers>(
      R"({"array": [1, 2], "variant": 3,
          "tagged": {"type": "Person", "favorite_foods": ["donuts"]},
          "f2": 4, "renamed": 5, "other": 6})");
  ASSERT_TRUE(res) << res.error().what();
  EXPECT_EQ(rfl::json::write(*res),
            R"({"id":0,"array":[1,2],"variant":3,)"
            R"("tagged":{"type":"Person","name":"Homer Simpson",)"
            R"("favorite_foods":["donuts"]},"f2":4,"renamed":5,"other":6})");

  const auto missing = rfl::json::read<Containers, rfl::DefaultIfMissing>(
      R"({"array": [1, 2], "variant": 3, "f2": 4, "renamed": 5})");
  ASSERT_FALSE(missing);
  EXPECT_EQ(missing.error().what(),
            "Field named 'tagged' not found, and its default value is "
            "invalid: " +
                foods_error);

  const auto invalid_extra = rfl::json::read<Containers>(
      R"({"array": [1, 2], "variant": 3,
          "tagged": {"type": "Flat", "f2": 1},
          "f2": 4, "renamed": 5, "other": 0})");
  ASSERT_FALSE(invalid_extra);
  EXPECT_EQ(invalid_extra.error().what(),
            "Failed to parse field 'other': " + positive_error);
}

TEST(json, test_default_val_with_validator_no_field_names) {
  const auto res = rfl::json::read<Person, rfl::NoFieldNames>(
      R"(["Bart", ["pizza"]])");
  ASSERT_TRUE(res) << res.error().what();
  EXPECT_EQ(res->name.value(), "Bart");

  const auto missing =
      rfl::json::read<Person, rfl::NoFieldNames, rfl::DefaultIfMissing>(
          R"(["Bart"])");
  ASSERT_FALSE(missing);
  EXPECT_EQ(missing.error().what(),
            "Field named 'favorite_foods' not found, and its default value is "
            "invalid: " +
                foods_error);
}

struct Snake {
  rfl::DefaultVal<int> user_id;
  Positive retry_count;
};

TEST(json, test_default_val_with_validator_processors) {
  using Processors =
      rfl::Processors<rfl::SnakeCaseToCamelCase, rfl::AddStructName<"type">>;
  const auto res = rfl::json::read<Snake, Processors>(
      R"({"type": "Snake", "retryCount": 2})");
  ASSERT_TRUE(res) << res.error().what();
  EXPECT_EQ(res->retry_count.value(), 2);

  const auto missing =
      rfl::json::read<Snake, Processors, rfl::DefaultIfMissing>(
          R"({"type": "Snake"})");
  ASSERT_FALSE(missing);
  EXPECT_EQ(missing.error().details().at(0).path, Path({"retryCount"}));

  const auto fail_fast =
      rfl::json::read<Family, rfl::DefaultIfMissing, rfl::FailFast>(R"({})");
  ASSERT_FALSE(fail_fast);
  EXPECT_EQ(fail_fast.error().details().size(), 1);
}

}  // namespace test_default_val_with_validator
