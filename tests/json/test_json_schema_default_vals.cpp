#include <optional>
#include <rfl.hpp>
#include <rfl/json.hpp>
#include <string>
#include <vector>

namespace test_schema_default {

struct Config {
  int port = 80;
  bool autostart = true;
};

struct DefaultValField {
  rfl::DefaultVal<int> with_default = 10;
};

struct DefaultWithConfig {
  rfl::DefaultVal<Config> with_default = Config{443, true};
};

/// Mixes `DefaultVal` and a plain field.
struct MixedDefaults {
  std::string name;
  rfl::DefaultVal<int> with_default = 10;
};

/// Has `ReflectionType` (and a converting constructor from it), but no
/// `reflection()` method or single `ReflectionType` member, so no default.
struct NonWritable {
  int copied;
  int missing;
  struct Inner {
    int value;
  };
  using ReflectionType = Inner;
  NonWritable() : copied(0), missing(0) {}
  explicit NonWritable(ReflectionType&& _r)
      : copied(_r.value), missing(0) {}
};

struct WithNonWritableDefault {
  rfl::DefaultVal<NonWritable> inner;
};

/// Has snake case fields for testing with `SnakeCaseToCamelCase`.
struct SnakeFields {
  int some_value = 7;
  bool other_flag = true;
};

struct WithProcessedDefault {
  rfl::DefaultVal<SnakeFields> nested = SnakeFields{};
};

TEST(json, test_with_default) {
  auto json_schema = rfl::json::to_schema<DefaultValField>();

  std::string expected =
      R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$ref":"#/$defs/test_schema_default__DefaultValField","$defs":{"test_schema_default__DefaultValField":{"type":"object","properties":{"with_default":{"type":"integer","default":10}},"required":[]}}})";

  EXPECT_EQ(json_schema, expected) << json_schema;

  json_schema = rfl::json::to_schema<DefaultWithConfig>();
  expected =
      R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$ref":"#/$defs/test_schema_default__DefaultWithConfig","$defs":{"test_schema_default__Config":{"type":"object","properties":{"port":{"type":"integer"},"autostart":{"type":"boolean"}},"required":["port","autostart"]},"test_schema_default__DefaultWithConfig":{"type":"object","properties":{"with_default":{"$ref":"#/$defs/test_schema_default__Config","default":{"port":443,"autostart":true}}},"required":[]}}})";

  EXPECT_EQ(json_schema, expected) << "is " << json_schema;
}

TEST(json, test_no_default_when_value_cannot_be_written) {
  const auto json_schema = rfl::json::to_schema<WithNonWritableDefault>();

  EXPECT_EQ(json_schema.find("\"default\""), std::string::npos)
      << "expected no default: " << json_schema;
  EXPECT_NE(json_schema.find("\"required\":[]"), std::string::npos)
      << "expected the field to remain optional: " << json_schema;
}

TEST(json, test_default_with_processors) {
  const auto json_schema =
      rfl::json::to_schema<WithProcessedDefault, rfl::SnakeCaseToCamelCase>();

  EXPECT_NE(json_schema.find(R"("default":{"someValue":7,"otherFlag":true})"),
            std::string::npos)
      << "default uses renamed properties: " << json_schema;
  EXPECT_EQ(json_schema.find("some_value"), std::string::npos)
      << "default was written without the processors: " << json_schema;

  // Same with a nested processor pack.
  const auto flat =
      rfl::json::to_schema<WithProcessedDefault, rfl::SnakeCaseToCamelCase>();
  const auto nested =
      rfl::json::to_schema<WithProcessedDefault,
                           rfl::Processors<rfl::SnakeCaseToCamelCase>>();

  EXPECT_EQ(flat, nested);
}

TEST(json, test_with_default_mixed_with_plain_fields) {
  const auto json_schema = rfl::json::to_schema<MixedDefaults>();

  const std::string expected =
      R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$ref":"#/$defs/test_schema_default__MixedDefaults","$defs":{"test_schema_default__MixedDefaults":{"type":"object","properties":{"name":{"type":"string"},"with_default":{"type":"integer","default":10}},"required":["name"]}}})";

  EXPECT_EQ(json_schema, expected) << json_schema;
}

/// The default-constructed `port` does not satisfy its validator, so the
/// struct cannot be default-constructed to read the defaults from.
struct DefaultWithInvalidValidator {
  rfl::Validator<int, rfl::Minimum<1>> port;
  rfl::DefaultVal<int> with_default = 10;
};

TEST(json, test_no_default_when_struct_cannot_be_default_constructed) {
  const auto json_schema = rfl::json::to_schema<DefaultWithInvalidValidator>();

  EXPECT_EQ(json_schema.find("\"default\""), std::string::npos)
      << "expected no default: " << json_schema;
  EXPECT_NE(json_schema.find("\"required\":[\"port\"]"), std::string::npos)
      << "expected with_default to remain optional: " << json_schema;
}

}  // namespace test_schema_default
