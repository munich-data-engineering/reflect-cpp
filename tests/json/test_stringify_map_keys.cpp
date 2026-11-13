#include "../test_stringify_map_keys.hpp"

#include <rfl/json.hpp>

namespace test_stringify_map_keys {
using P = rfl::StringifyMapKeys;

TEST(json, stringify_map_keys_roundtrip) {
  check([](const auto& value) { return rfl::json::write<P>(value); },
        []<class T>(const auto& json) { return rfl::json::read<T, P>(json); });
}

TEST(json, stringify_map_keys_representation) {
  const std::map<Colour, int> map{{Colour::Red, 1}, {Colour::Blue, 2}};
  EXPECT_EQ(rfl::json::write<P>(map), R"({"Red":1,"Blue":2})");
  EXPECT_EQ(rfl::json::write(map), R"([["Red",1],["Blue",2]])");
  EXPECT_EQ(rfl::json::write<P>(std::map<Colour, int>{}), "{}");
  const auto legacy = rfl::json::read<std::remove_cvref_t<decltype(map)>>(
      rfl::json::write(map));
  ASSERT_TRUE(legacy);
  EXPECT_EQ(*legacy, map);
  EXPECT_FALSE(
      (rfl::json::read<std::remove_cvref_t<decltype(map)>>(R"({"Red":1})")));
  EXPECT_FALSE((rfl::json::read<std::remove_cvref_t<decltype(map)>, P>(
      R"([["Red",1]])")));
}

TEST(json, stringify_map_keys_reflector_precedence) {
  const std::map<CustomColour, int> map{{CustomColour::Red, 3}};
  EXPECT_EQ(rfl::json::write<P>(map), R"({"rouge":3})");
  EXPECT_EQ((rfl::json::write<P, rfl::UnderlyingEnums>(map)), R"({"rouge":3})");
  EXPECT_EQ(rfl::json::write(map), R"([["rouge",3]])");
  const auto result = rfl::json::read<std::remove_cvref_t<decltype(map)>, P,
                                      rfl::UnderlyingEnums>(R"({"rouge":3})");
  ASSERT_TRUE(result);
  EXPECT_EQ(*result, map);
  EXPECT_EQ(rfl::json::write<P>(std::map<Key, int>{{Key{1}, 9}}),
            R"({"key-1":9})");
}

TEST(json, stringify_map_keys_errors) {
  const auto unknown =
      rfl::json::read<std::map<Colour, int>, P>(R"({"Purple":1})");
  ASSERT_FALSE(unknown);
  EXPECT_NE(unknown.error().what().find("Purple"), std::string::npos);
  const auto thrown =
      rfl::json::read<std::map<CustomColour, int>, P>(R"({"Red":1})");
  ASSERT_FALSE(thrown);
  EXPECT_NE(thrown.error().what().find("Unknown colour: Red"),
            std::string::npos);
  const auto error = rfl::json::read<std::map<Key, int>, P>(R"({"bad":1})");
  ASSERT_FALSE(error);
  EXPECT_NE(error.error().what().find("Invalid custom key: bad"),
            std::string::npos);
  EXPECT_FALSE((rfl::json::read<std::map<Colour, int>, P>(R"({"Red":"bad"})")));
}

TEST(json, stringify_map_keys_enum_processors) {
  using Map = std::map<Colour, int>;
  const auto permissive = rfl::json::read<Map, P>(R"({"1":7})");
  ASSERT_TRUE(permissive);
  EXPECT_EQ(permissive->at(Colour::Green), 7);
  EXPECT_FALSE((rfl::json::read<Map, P, rfl::EnumNamesOnly>(R"({"1":7})")));
  EXPECT_TRUE((rfl::json::read<Map, P, rfl::EnumNamesOnly>(R"({"Green":7})")));
  const Map value{{Colour::Red, 1}};
  EXPECT_EQ((rfl::json::write<P, rfl::UnderlyingEnums>(value)), "[[0,1]]");
  const auto numeric = rfl::json::read<Map, P, rfl::UnderlyingEnums>("[[0,1]]");
  ASSERT_TRUE(numeric);
  EXPECT_EQ(*numeric, value);
}

TEST(json, stringify_map_keys_preserves_other_maps) {
  EXPECT_EQ(rfl::json::write<P>(std::map<Numeric, int>{{Numeric::One, 2}}),
            "[[1,2]]");
  EXPECT_EQ(rfl::json::write<P>(
                std::multimap<Colour, int>{{Colour::Red, 1}, {Colour::Red, 2}}),
            R"([["Red",1],["Red",2]])");
  EXPECT_EQ(rfl::json::write<P>(std::map<int, int>{{1, 2}}), R"({"1":2})");
  EXPECT_EQ(rfl::json::write<P>(std::map<std::string, int>{{"x", 2}}),
            R"({"x":2})");
}

TEST(json, stringify_map_keys_schema) {
  const auto expected = rfl::json::to_schema<std::map<std::string, int>>();
  EXPECT_EQ((rfl::json::to_schema<std::map<Colour, int>, P>()), expected);
  EXPECT_EQ((rfl::json::to_schema<std::map<CustomColour, int>, P>()), expected);
  EXPECT_EQ((rfl::json::to_schema<std::map<Key, int>, P>()), expected);
  EXPECT_NE((rfl::json::to_schema<std::map<Colour, int>>()), expected);
}
}  // namespace test_stringify_map_keys
