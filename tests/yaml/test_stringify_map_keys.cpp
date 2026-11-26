#include "../test_stringify_map_keys.hpp"

#include <rfl/yaml.hpp>

namespace test_stringify_map_keys {
TEST(yaml, stringify_map_keys_roundtrip) {
  check(
      [](const auto& value) {
        return rfl::yaml::write<rfl::StringifyMapKeys>(value);
      },
      []<class T>(const auto& value) {
        return rfl::yaml::read<T, rfl::StringifyMapKeys>(value);
      });
}
TEST(yaml, stringify_map_keys_issue_661) {
  struct Test {
    std::map<Colour, std::string> dictionary;
  };
  const auto result = rfl::yaml::read<Test, rfl::StringifyMapKeys>(
      std::string("dictionary:\n  Red: Rojo\n  Green: Verde\n  Blue: Azul\n"));
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(result->dictionary.at(Colour::Red), "Rojo");
  EXPECT_EQ(result->dictionary.at(Colour::Green), "Verde");
  EXPECT_EQ(result->dictionary.at(Colour::Blue), "Azul");
}
}  // namespace test_stringify_map_keys
