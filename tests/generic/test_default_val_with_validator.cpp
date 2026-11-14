#include <gtest/gtest.h>

#include <rfl.hpp>
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

using Foods =
    rfl::Validator<std::vector<std::string>, rfl::Size<rfl::Minimum<1>>>;
using Positive = rfl::Validator<int, rfl::Minimum<1>>;

struct Person {
  rfl::DefaultVal<std::string> name = "Homer Simpson";
  Foods favorite_foods;
  rfl::DefaultVal<Positive> age;
};

rfl::Generic foods() {
  return rfl::Generic(
      rfl::Generic::Array({rfl::Generic(std::string("donuts"))}));
}

TEST(generic, test_default_val_with_validator) {
  auto obj = rfl::Generic::Object();
  obj["favorite_foods"] = foods();
  obj["age"] = rfl::Generic(int64_t{39});
  const auto res = rfl::from_generic<Person>(rfl::Generic(std::move(obj)));
  ASSERT_TRUE(res) << res.error().what();
  EXPECT_EQ(res->name.value(), "Homer Simpson");
  EXPECT_EQ(res->favorite_foods->at(0), "donuts");
  EXPECT_EQ(res->age.value().value(), 39);
}

TEST(generic, test_default_val_with_validator_invalid_default) {
  auto obj = rfl::Generic::Object();
  obj["favorite_foods"] = foods();
  const auto res = rfl::from_generic<Person>(rfl::Generic(std::move(obj)));
  ASSERT_FALSE(res);
  EXPECT_EQ(res.error().what(),
            "Field named 'age' not found, and its default value is invalid: "
            "Value expected to be greater than or equal to 1, but got 0.");
  EXPECT_EQ(paths(res.error()), std::vector<Path>({{"age"}}));
}

TEST(generic, test_default_val_with_validator_default_if_missing) {
  auto obj = rfl::Generic::Object();
  obj["age"] = rfl::Generic(int64_t{3});
  const auto res = rfl::from_generic<Person, rfl::DefaultIfMissing>(
      rfl::Generic(std::move(obj)));
  ASSERT_FALSE(res);
  EXPECT_EQ(paths(res.error()), std::vector<Path>({{"favorite_foods"}}));
}

}  // namespace test_default_val_with_validator
