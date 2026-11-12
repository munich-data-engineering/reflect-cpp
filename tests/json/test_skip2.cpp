#include <rfl.hpp>
#include <rfl/json.hpp>
#include <string>

#include "write_and_read.hpp"

namespace test_skip2 {

using Age = rfl::Validator<unsigned int, rfl::Minimum<0>, rfl::Maximum<130>>;

struct Person {
  rfl::Rename<"firstName", std::string> first_name;
  rfl::Rename<"lastName", std::string> last_name;
  rfl::Skip<rfl::Bytestring> town;
  Age age;
};

TEST(json, test_skip2) {
  const auto homer =
      Person{.first_name = "Homer", .last_name = "Simpson", .age = 45};

  write_and_read(homer,
                 R"({"firstName":"Homer","lastName":"Simpson","age":45})");
}
}  // namespace test_skip2
