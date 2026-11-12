#include <gtest/gtest.h>

#include <rfl.hpp>
#include <rfl/json.hpp>
#include <stdexcept>
#include <string>
#include <vector>

namespace test_default_construct_error {

using Foods =
    rfl::Validator<std::vector<std::string>, rfl::Size<rfl::Minimum<1>>>;

struct Person {
  rfl::DefaultVal<std::string> name = "Homer Simpson";
  Foods favorite_foods;
};

struct PersonWithValidDefault {
  rfl::DefaultVal<std::string> name = "Homer Simpson";
  Foods favorite_foods = std::vector<std::string>({"none"});
};

struct Pet {
  std::string name;
  Foods favorite_foods;
};

struct Family {
  std::string town;
  Person head;
};

inline int no_default_port() { throw std::runtime_error("No default port"); }

struct Server {
  rfl::DefaultVal<std::string> host = "localhost";
  int port = no_default_port();
};

bool contains(const std::string& _str, const std::string& _substr) {
  return _str.find(_substr) != std::string::npos;
}

const std::string validation_error =
    "Size validation failed: Value expected to be greater than or equal to 1, "
    "but got 0.";

TEST(json, test_default_construct_error_with_default_val) {
  const auto res =
      rfl::json::read<Person>(R"({"favorite_foods":["donuts","pizza"]})");

  ASSERT_FALSE(res && true);
  const auto what = res.error().what();
  EXPECT_TRUE(contains(what, "default-construct")) << what;
  EXPECT_TRUE(contains(what, "Person")) << what;
  EXPECT_TRUE(contains(what, "rfl::DefaultVal")) << what;
  EXPECT_TRUE(contains(what, "valid default value")) << what;
  EXPECT_TRUE(contains(what, validation_error)) << what;
}

TEST(json, test_default_construct_error_with_default_if_missing) {
  const auto json_string = R"({"name":"Bart","favorite_foods":["pizza"]})";

  EXPECT_TRUE(rfl::json::read<Pet>(json_string) && true);

  const auto res = rfl::json::read<Pet, rfl::DefaultIfMissing>(json_string);

  ASSERT_FALSE(res && true);
  const auto what = res.error().what();
  EXPECT_TRUE(contains(what, "default-construct")) << what;
  EXPECT_TRUE(contains(what, "Pet")) << what;
  EXPECT_TRUE(contains(what, "rfl::DefaultIfMissing")) << what;
  EXPECT_TRUE(contains(what, validation_error)) << what;
}

TEST(json, test_default_construct_error_in_nested_struct) {
  const auto res = rfl::json::read<Family>(
      R"({"town":"Springfield","head":{"favorite_foods":["donuts"]}})");

  ASSERT_FALSE(res && true);
  const auto what = res.error().what();
  EXPECT_TRUE(contains(what, "Failed to parse field 'head': ")) << what;

  const auto details = res.error().details();
  ASSERT_EQ(details.size(), 1);
  EXPECT_EQ(details[0].path,
            std::vector<rfl::ErrorDetail::PathSegment>({"head"}));
  EXPECT_TRUE(contains(details[0].message, "default-construct"));
  EXPECT_TRUE(contains(details[0].message, "Person"));
  EXPECT_TRUE(contains(details[0].message, validation_error));
}

TEST(json, test_default_construct_error_without_validator) {
  const auto res = rfl::json::read<Server>(R"({"port":8080})");

  ASSERT_FALSE(res && true);
  const auto what = res.error().what();
  EXPECT_TRUE(contains(what, "default-construct")) << what;
  EXPECT_TRUE(contains(what, "Server")) << what;
  EXPECT_TRUE(contains(what, "No default port")) << what;
}

TEST(json, test_default_construct_error_valid_default) {
  const auto homer = rfl::json::read<PersonWithValidDefault>(
                         R"({"favorite_foods":["donuts","pizza"]})")
                         .value();

  EXPECT_EQ(homer.name.value(), "Homer Simpson");
  EXPECT_EQ(homer.favorite_foods.value(),
            std::vector<std::string>({"donuts", "pizza"}));

  // The initializer does not make the field optional.
  const auto missing = rfl::json::read<PersonWithValidDefault>(R"({})");

  ASSERT_FALSE(missing && true);
  EXPECT_EQ(missing.error().what(), "Field named 'favorite_foods' not found.");

  // With rfl::DefaultIfMissing, the initializer is the value of a missing
  // field.
  const auto with_default =
      rfl::json::read<PersonWithValidDefault, rfl::DefaultIfMissing>(R"({})")
          .value();

  EXPECT_EQ(with_default.favorite_foods.value(),
            std::vector<std::string>({"none"}));
}

TEST(json, test_default_construct_error_not_used_for_later_failures) {
  const auto wrong_type =
      rfl::json::read<PersonWithValidDefault>(R"({"favorite_foods":"donuts"})");

  ASSERT_FALSE(wrong_type && true);
  EXPECT_TRUE(contains(wrong_type.error().what(),
                       "Failed to parse field 'favorite_foods': "))
      << wrong_type.error().what();
  EXPECT_FALSE(contains(wrong_type.error().what(), "default-construct"))
      << wrong_type.error().what();

  const auto invalid =
      rfl::json::read<PersonWithValidDefault>(R"({"favorite_foods":[]})");

  ASSERT_FALSE(invalid && true);
  EXPECT_EQ(invalid.error().what(),
            "Failed to parse field 'favorite_foods': " + validation_error);
}
}  // namespace test_default_construct_error
