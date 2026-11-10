#include <gtest/gtest.h>

#include <optional>
#include <rfl.hpp>
#include <rfl/json.hpp>
#include <string>

#include "write_and_read.hpp"

// rfl::Short, rfl::Positional and rfl::Description are transparent for JSON,
// so std::optional and rfl::DefaultVal must behave the same inside them as
// they do on their own: omitted when empty, filled in when missing, and not
// required in the schema.
namespace test_wrapped_optionals_and_defaults {

struct Optionals {
  rfl::Short<"f", std::optional<std::string>> format;
  rfl::Positional<std::optional<std::string>> input;
};

struct NestedOptionals {
  rfl::Description<"Log file", rfl::Short<"l", std::optional<std::string>>> log;
  rfl::Short<"t", rfl::Description<"Title", std::optional<std::string>>> title;
  rfl::Positional<rfl::Description<"Config file", std::optional<std::string>>>
      config;
  rfl::Rename<"outDir", rfl::Positional<std::optional<std::string>>> out_dir;
};

TEST(json, test_wrapped_optionals_write_and_read) {
  write_and_read(Optionals{}, R"({})");
  write_and_read(
      Optionals{.format = std::string("csv"), .input = std::string("in.txt")},
      R"({"format":"csv","input":"in.txt"})");

  write_and_read(NestedOptionals{}, R"({})");
  write_and_read(
      NestedOptionals{.log = std::string("log.txt"),
                      .title = std::string("Report"),
                      .config = std::string("config.json"),
                      .out_dir = std::string("out")},
      R"({"log":"log.txt","title":"Report","config":"config.json","outDir":"out"})");
}

TEST(json, test_wrapped_optionals_schema) {
  EXPECT_EQ(
      rfl::json::to_schema<Optionals>(),
      R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$ref":"#/$defs/test_wrapped_optionals_and_defaults__Optionals","$defs":{"test_wrapped_optionals_and_defaults__Optionals":{"type":"object","properties":{"format":{"anyOf":[{"type":"string"},{"type":"null"}]},"input":{"anyOf":[{"type":"string"},{"type":"null"}]}},"required":[]}}})");

  EXPECT_EQ(
      rfl::json::to_schema<NestedOptionals>(),
      R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$ref":"#/$defs/test_wrapped_optionals_and_defaults__NestedOptionals","$defs":{"test_wrapped_optionals_and_defaults__NestedOptionals":{"type":"object","properties":{"log":{"description":"Log file","anyOf":[{"type":"string"},{"type":"null"}]},"title":{"description":"Title","anyOf":[{"type":"string"},{"type":"null"}]},"config":{"description":"Config file","anyOf":[{"type":"string"},{"type":"null"}]},"outDir":{"anyOf":[{"type":"string"},{"type":"null"}]}},"required":[]}}})");
}

}  // namespace test_wrapped_optionals_and_defaults
