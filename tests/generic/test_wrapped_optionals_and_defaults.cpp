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

struct Defaults {
  rfl::Short<"o", rfl::DefaultVal<std::string>> output = std::string("out.csv");
  rfl::Positional<rfl::DefaultVal<std::string>> input = std::string("in.txt");
  rfl::Description<"Worker threads", rfl::DefaultVal<int>> jobs = 4;
};

TEST(generic, test_wrapped_defaults) {
  write_and_read(Defaults{});

  const auto res = rfl::from_generic<Defaults>(rfl::Generic::Object());
  ASSERT_TRUE(res) << res.error().what();
  EXPECT_EQ(res.value().output().value(), "out.csv");
  EXPECT_EQ(res.value().input().value(), "in.txt");
  EXPECT_EQ(res.value().jobs().value(), 4);
}

}  // namespace test_wrapped_optionals_and_defaults
