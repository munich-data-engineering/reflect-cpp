#include <optional>
#include <rfl.hpp>
#include <string>

#include "write_and_read.hpp"

namespace test_wrapped_optionals_and_defaults {

struct Optionals {
  rfl::Short<"f", std::optional<std::string>> format;
  rfl::Positional<std::optional<std::string>> input;
  rfl::Description<"Log file", rfl::Short<"l", std::optional<std::string>>> log;
};

TEST(generic, test_wrapped_optionals) {
  write_and_read(Optionals{});
  write_and_read(Optionals{.format = std::string("csv"),
                           .input = std::string("in.txt"),
                           .log = std::string("log.txt")});

  EXPECT_EQ(rfl::to_generic(Optionals{}).to_object().value().size(), 0u);
}

}  // namespace test_wrapped_optionals_and_defaults
