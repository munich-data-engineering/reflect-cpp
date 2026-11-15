#include <string>

#include "write_and_read.hpp"

namespace test_c_array_const_with_default {

// The elements of a const c-array field are re-created in place, like any
// other const field of a struct that is default-constructed first.
struct Names {
  const std::string names[2][2] = {{"a", "b"}, {"c", "d"}};
  rfl::DefaultVal<int> n = 1;
};

TEST(json, test_c_array_const_with_default) {
  const auto test =
      rfl::json::read<Names>(R"({"names":[["e","f"],["g","h"]]})").value();
  EXPECT_EQ(test.names[0][0], "e");
  EXPECT_EQ(test.names[0][1], "f");
  EXPECT_EQ(test.names[1][0], "g");
  EXPECT_EQ(test.names[1][1], "h");
  EXPECT_EQ(test.n.value(), 1);

  const auto defaults =
      rfl::json::read<Names, rfl::DefaultIfMissing>(R"({"n":2})").value();
  EXPECT_EQ(defaults.names[1][1], "d");
  EXPECT_EQ(defaults.n.value(), 2);
}

}  // namespace test_c_array_const_with_default
