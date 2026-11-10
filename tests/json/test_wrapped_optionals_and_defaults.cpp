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

struct Defaults {
  rfl::Short<"o", rfl::DefaultVal<std::string>> output = std::string("out.csv");
  rfl::Positional<rfl::DefaultVal<std::string>> input = std::string("in.txt");
  rfl::Description<"Worker threads", rfl::DefaultVal<int>> jobs = 4;
};

struct NestedDefaults {
  rfl::Description<"Log file", rfl::Short<"l", rfl::DefaultVal<std::string>>>
      log = std::string("log.txt");
  rfl::Short<"t", rfl::Description<"Title", rfl::DefaultVal<std::string>>>
      title = std::string("Report");
  rfl::Positional<rfl::Description<"Config file", rfl::DefaultVal<std::string>>>
      config = std::string("config.json");
  rfl::Deprecated<"Use jobs instead.", "Worker threads", rfl::DefaultVal<int>>
      threads = 4;
  rfl::Rename<"outDir", rfl::Short<"d", rfl::DefaultVal<std::string>>> out_dir =
      std::string("out");
};

TEST(json, test_wrapped_defaults_write_and_read) {
  write_and_read(Defaults{},
                 R"({"output":"out.csv","input":"in.txt","jobs":4})");
  write_and_read(Defaults{.output = std::string("x.csv"),
                          .input = std::string("x.txt"),
                          .jobs = 8},
                 R"({"output":"x.csv","input":"x.txt","jobs":8})");

  write_and_read(
      NestedDefaults{},
      R"({"log":"log.txt","title":"Report","config":"config.json","threads":4,"outDir":"out"})");
}

TEST(json, test_wrapped_defaults_read_missing_fields) {
  const auto defaults = rfl::json::read<Defaults>(R"({})");
  ASSERT_TRUE(defaults) << defaults.error().what();
  EXPECT_EQ(defaults.value().output().value(), "out.csv");
  EXPECT_EQ(defaults.value().input().value(), "in.txt");
  EXPECT_EQ(defaults.value().jobs().value(), 4);

  const auto partial = rfl::json::read<Defaults>(R"({"jobs":8})");
  ASSERT_TRUE(partial) << partial.error().what();
  EXPECT_EQ(partial.value().output().value(), "out.csv");
  EXPECT_EQ(partial.value().jobs().value(), 8);

  const auto nested = rfl::json::read<NestedDefaults>(R"({})");
  ASSERT_TRUE(nested) << nested.error().what();
  EXPECT_EQ(nested.value().log().value().value(), "log.txt");
  EXPECT_EQ(nested.value().title().value().value(), "Report");
  EXPECT_EQ(nested.value().config().value().value(), "config.json");
  EXPECT_EQ(nested.value().threads().value(), 4);
  EXPECT_EQ(nested.value().out_dir().value().value(), "out");
}

TEST(json, test_wrapped_defaults_schema) {
  EXPECT_EQ(
      rfl::json::to_schema<Defaults>(),
      R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$ref":"#/$defs/test_wrapped_optionals_and_defaults__Defaults","$defs":{"test_wrapped_optionals_and_defaults__Defaults":{"type":"object","properties":{"output":{"type":"string","default":"out.csv"},"input":{"type":"string","default":"in.txt"},"jobs":{"type":"integer","description":"Worker threads","default":4}},"required":[]}}})");

  EXPECT_EQ(
      rfl::json::to_schema<NestedDefaults>(),
      R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$ref":"#/$defs/test_wrapped_optionals_and_defaults__NestedDefaults","$defs":{"test_wrapped_optionals_and_defaults__NestedDefaults":{"type":"object","properties":{"log":{"type":"string","description":"Log file","default":"log.txt"},"title":{"type":"string","description":"Title","default":"Report"},"config":{"type":"string","description":"Config file","default":"config.json"},"threads":{"type":"integer","description":"Worker threads","deprecated":true,"deprecationMessage":"Use jobs instead.","default":4},"outDir":{"type":"string","default":"out"}},"required":[]}}})");
}

}  // namespace test_wrapped_optionals_and_defaults
