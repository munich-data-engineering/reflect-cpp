#include <rfl.hpp>
#include <rfl/json.hpp>
#include <string>

#include "write_and_read.hpp"

namespace test_const_field_with_default {

// Structs with rfl::DefaultVal fields, or read with rfl::DefaultIfMissing,
// are default-constructed first and then filled in. The const fields in them
// must be re-created in place rather than assigned to.

struct Person {
  const std::string first_name = "Homer";
  rfl::DefaultVal<std::string> last_name = "Simpson";
  const int age = 45;
};

TEST(json, test_const_field_with_default_val) {
  const auto bart = rfl::json::read<Person>(
                        R"({"first_name":"Bart","last_name":"S","age":10})")
                        .value();
  EXPECT_EQ(bart.first_name, "Bart");
  EXPECT_EQ(bart.last_name.value(), "S");
  EXPECT_EQ(bart.age, 10);
}

TEST(json, test_const_field_with_default_val_missing) {
  const auto homer =
      rfl::json::read<Person, rfl::DefaultIfMissing>(R"({"last_name":"S"})")
          .value();
  EXPECT_EQ(homer.first_name, "Homer");
  EXPECT_EQ(homer.last_name.value(), "S");
  EXPECT_EQ(homer.age, 45);
}

struct Town {
  const std::string name = "Springfield";
  const int population = 100;
  std::string state;
};

TEST(json, test_const_field_with_default_if_missing) {
  const auto shelbyville =
      rfl::json::read<Town, rfl::DefaultIfMissing>(
          R"({"name":"Shelbyville","population":200})")
          .value();
  EXPECT_EQ(shelbyville.name, "Shelbyville");
  EXPECT_EQ(shelbyville.population, 200);
  EXPECT_EQ(shelbyville.state, "");

  const auto springfield =
      rfl::json::read<Town, rfl::DefaultIfMissing>(R"({})").value();
  EXPECT_EQ(springfield.name, "Springfield");
  EXPECT_EQ(springfield.population, 100);
}

TEST(json, test_const_field_with_default_val_no_field_names) {
  const auto bart =
      rfl::json::read<Person, rfl::NoFieldNames>(R"(["Bart","S",10])")
          .value();
  EXPECT_EQ(bart.first_name, "Bart");
  EXPECT_EQ(bart.last_name.value(), "S");
  EXPECT_EQ(bart.age, 10);

  const auto shelbyville =
      rfl::json::read<Town, rfl::DefaultIfMissing, rfl::NoFieldNames>(
          R"(["Shelbyville",200])")
          .value();
  EXPECT_EQ(shelbyville.name, "Shelbyville");
  EXPECT_EQ(shelbyville.population, 200);
  EXPECT_EQ(shelbyville.state, "");
}

// Point has no assignment operator, so a const Point field can only be
// filled in by re-creating it.
struct Point {
  const int x = 0;
  const int y = 0;
};

struct Shape {
  const Point origin = {};
  rfl::DefaultVal<std::string> name = "unnamed";
};

TEST(json, test_const_field_without_assignment) {
  const auto shape =
      rfl::json::read<Shape>(R"({"origin":{"x":1,"y":2}})").value();
  EXPECT_EQ(shape.origin.x, 1);
  EXPECT_EQ(shape.origin.y, 2);
  EXPECT_EQ(shape.name.value(), "unnamed");
}

}  // namespace test_const_field_with_default
