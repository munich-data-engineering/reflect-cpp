#include <gtest/gtest.h>

#include <optional>
#include <rfl.hpp>
#include <rfl/json.hpp>
#include <string>

// rfl::DefaultVal in places where the struct around it is not
// default-constructed before it is read: rfl::NamedTuple and the schema of an
// rfl::TaggedUnion alternative.
namespace test_default_val_without_default_construction {

struct NoDefaultConstructor {
  using ReflectionType = int;
  NoDefaultConstructor(int _v) : v_(_v) {}
  int reflection() const { return v_; }
  int v_;
};

// A NamedTuple has no in-class initializers, so a missing rfl::DefaultVal
// field is value-initialized. It is constructed before it is used.
TEST(json, test_default_val_named_tuple_missing_field) {
  using WithString =
      rfl::NamedTuple<rfl::Field<"n", int>,
                      rfl::Field<"s", rfl::DefaultVal<std::string>>,
                      rfl::Field<"opt", std::optional<int>>>;
  const auto str = rfl::json::read<WithString>(R"({"n":3})");
  ASSERT_TRUE(str) << str.error().what();
  EXPECT_EQ(rfl::get<"s">(str.value()).value(), "");
  EXPECT_FALSE(rfl::get<"opt">(str.value()));
}

// If the value cannot be value-initialized, or its constructor throws, the
// missing field is an error.
TEST(json, test_default_val_named_tuple_missing_field_without_zero_value) {
  using NoZero =
      rfl::NamedTuple<rfl::Field<"o", rfl::DefaultVal<NoDefaultConstructor>>,
                      rfl::Field<"z", int>>;
  const auto no_zero = rfl::json::read<NoZero>(R"({"z":1})");
  ASSERT_FALSE(no_zero);
  EXPECT_EQ(no_zero.error().what(), std::string("Field named 'o' not found."));

  using Invalid = rfl::NamedTuple<
      rfl::Field<"v", rfl::DefaultVal<rfl::Validator<int, rfl::Minimum<1>>>>,
      rfl::Field<"z", int>>;
  const auto invalid = rfl::json::read<Invalid>(R"({"z":1})");
  ASSERT_FALSE(invalid);
  EXPECT_EQ(invalid.error().what(),
            std::string("Field named 'v' not found, and its default value "
                        "could not be constructed: Value expected to be "
                        "greater than or equal to 1, but got 0."));

  const auto present = rfl::json::read<Invalid>(R"({"v":3,"z":1})");
  ASSERT_TRUE(present) << present.error().what();
  EXPECT_EQ(rfl::get<"v">(present.value()).value().value(), 3);
}

// The schema reads the defaults of an rfl::TaggedUnion alternative from a
// default-constructed instance of that alternative.
struct DefaultAlternative {
  rfl::DefaultVal<std::string> output = std::string("out.csv");
  int k = 0;
};

struct OtherAlternative {
  std::string q;
};

TEST(json, test_default_val_tagged_union_schema) {
  EXPECT_EQ(
      (rfl::json::to_schema<
          rfl::TaggedUnion<"type", DefaultAlternative, OtherAlternative>>()),
      R"({"$schema":"https://json-schema.org/draft/2020-12/schema","anyOf":[{"$ref":"#/$defs/test_default_val_without_default_construction__DefaultAlternative__tagged"},{"$ref":"#/$defs/test_default_val_without_default_construction__OtherAlternative__tagged"}],"$defs":{"test_default_val_without_default_construction__DefaultAlternative__tagged":{"type":"object","properties":{"type":{"type":"string","enum":["DefaultAlternative"]},"output":{"type":"string","default":"out.csv"},"k":{"type":"integer"}},"required":["type","k"]},"test_default_val_without_default_construction__OtherAlternative__tagged":{"type":"object","properties":{"type":{"type":"string","enum":["OtherAlternative"]},"q":{"type":"string"}},"required":["type","q"]}}})");
}

}  // namespace test_default_val_without_default_construction
