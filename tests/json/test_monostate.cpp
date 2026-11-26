#include <rfl.hpp>
#include <rfl/json.hpp>
#include <string>
#include <variant>

#include "write_and_read.hpp"

namespace test_monostate {

struct Order {
  std::variant<std::monostate, std::string, int> price;
};

struct RflOrder {
  rfl::Variant<std::monostate, std::string, int> price;
};

TEST(json, test_monostate) {
  write_and_read(Order{.price = std::monostate{}}, R"({})");
  write_and_read(Order{.price = "1.25"}, R"({"price":"1.25"})");
  write_and_read(Order{.price = 2}, R"({"price":2})");

  for (const auto json : {R"({})", R"({"price":null})"}) {
    const auto res = rfl::json::read<Order>(json);
    ASSERT_TRUE(res) << res.error().what();
    EXPECT_TRUE(std::holds_alternative<std::monostate>(res->price));
  }
}

TEST(json, test_monostate_rfl_variant) {
  write_and_read(RflOrder{.price = std::monostate{}}, R"({})");
  write_and_read(RflOrder{.price = std::string("1.25")}, R"({"price":"1.25"})");
  write_and_read(RflOrder{.price = 2}, R"({"price":2})");

  for (const auto json : {R"({})", R"({"price":null})"}) {
    const auto res = rfl::json::read<RflOrder>(json);
    ASSERT_TRUE(res) << res.error().what();
    EXPECT_TRUE(rfl::holds_alternative<std::monostate>(res->price));
  }
}

TEST(json, test_monostate_no_optionals) {
  write_and_read<rfl::NoOptionals>(Order{.price = std::monostate{}},
                                   R"({"price":null})");
  write_and_read<rfl::NoOptionals>(Order{.price = 2}, R"({"price":2})");

  EXPECT_FALSE((rfl::json::read<Order, rfl::NoOptionals>(R"({})")));
}

TEST(json, test_monostate_no_optionals_rfl_variant) {
  write_and_read<rfl::NoOptionals>(RflOrder{.price = std::monostate{}},
                                   R"({"price":null})");
  write_and_read<rfl::NoOptionals>(RflOrder{.price = 2}, R"({"price":2})");

  EXPECT_FALSE((rfl::json::read<RflOrder, rfl::NoOptionals>(R"({})")));
}

TEST(json, test_monostate_top_level) {
  write_and_read(std::variant<std::monostate, int>{}, "null");
  write_and_read(rfl::Variant<std::monostate, int>{}, "null");
}

TEST(json, test_monostate_rejects_non_null) {
  EXPECT_FALSE(rfl::json::read<std::monostate>("1"));
}

TEST(json, test_monostate_schema) {
  EXPECT_EQ(
      rfl::json::to_schema<std::monostate>(),
      R"({"$schema":"https://json-schema.org/draft/2020-12/schema","type":"null","$defs":{}})");
}

TEST(json, test_monostate_field_not_required_in_schema) {
  EXPECT_EQ(
      rfl::json::to_schema<Order>(),
      R"({"$schema":"https://json-schema.org/draft/2020-12/schema","$ref":"#/$defs/test_monostate__Order","$defs":{"test_monostate__Order":{"type":"object","properties":{"price":{"anyOf":[{"type":"null"},{"type":"string"},{"type":"integer"}]}},"required":[]}}})");
}

}  // namespace test_monostate
