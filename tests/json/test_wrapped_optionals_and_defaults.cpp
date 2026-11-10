#include <gtest/gtest.h>

#include <optional>
#include <rfl.hpp>
#include <rfl/json.hpp>
#include <string>

#include "write_and_read.hpp"

// rfl::Short, rfl::Positional and rfl::Description are transparent for JSON,
// so a std::optional inside them is written as on its own: omitted when
// empty.
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

}  // namespace test_wrapped_optionals_and_defaults
