#ifndef RFL_TEST_STRINGIFY_MAP_KEYS_HPP_
#define RFL_TEST_STRINGIFY_MAP_KEYS_HPP_

#include <gtest/gtest.h>

#include <compare>
#include <map>
#include <memory>
#include <rfl.hpp>
#include <stdexcept>
#include <string>
#include <unordered_map>

namespace test_stringify_map_keys {
enum class Colour { Red, Green, Blue };
enum class CustomColour { Red, Blue };
enum class Numeric { One = 1 };

struct Key {
  int id;
  auto operator<=>(const Key&) const = default;
};
struct Hash {
  std::size_t operator()(const Key& key) const {
    return std::hash<int>{}(key.id);
  }
};
}  // namespace test_stringify_map_keys

namespace rfl {
template <>
struct Reflector<test_stringify_map_keys::CustomColour> {
  using ReflType = std::string;
  static auto to(const std::string& name) {
    using E = test_stringify_map_keys::CustomColour;
    if (name == "rouge") return E::Red;
    if (name == "bleu") return E::Blue;
    throw std::runtime_error("Unknown colour: " + name);
  }
  static std::string from(test_stringify_map_keys::CustomColour value) {
    return value == test_stringify_map_keys::CustomColour::Red ? "rouge"
                                                               : "bleu";
  }
};

template <>
struct Reflector<test_stringify_map_keys::Key> {
  using ReflType = std::string;
  static Result<test_stringify_map_keys::Key> to(const std::string& name) {
    if (name == "key-1") return test_stringify_map_keys::Key{1};
    if (name == "key-2") return test_stringify_map_keys::Key{2};
    return error("Invalid custom key: " + name);
  }
  static std::string from(const test_stringify_map_keys::Key& key) {
    return "key-" + std::to_string(key.id);
  }
};

template <>
struct Reflector<test_stringify_map_keys::Numeric> {
  using ReflType = int;
  static auto to(int value) {
    return static_cast<test_stringify_map_keys::Numeric>(value);
  }
  static int from(test_stringify_map_keys::Numeric value) {
    return static_cast<int>(value);
  }
};
}  // namespace rfl

namespace test_stringify_map_keys {
template <class T>
void roundtrip(const auto& write, const auto& read, const T& value) {
  const auto result = read.template operator()<T>(write(value));
  ASSERT_TRUE(result) << result.error().what();
  EXPECT_EQ(*result, value);
}

inline void check(const auto& write, const auto& read) {
  const auto names = read.template operator()<std::map<std::string, int>>(
      write(std::map<Colour, int>{{Colour::Red, 1}}));
  ASSERT_TRUE(names) << names.error().what();
  EXPECT_EQ(names->at("Red"), 1);
  const auto custom_names =
      read.template operator()<std::map<std::string, int>>(
          write(std::map<Key, int>{{Key{1}, 9}}));
  ASSERT_TRUE(custom_names) << custom_names.error().what();
  EXPECT_EQ(custom_names->at("key-1"), 9);
  EXPECT_FALSE((read.template operator()<std::map<Colour, int>>(
      write(std::map<std::string, int>{{"unknown", 1}}))));
  EXPECT_FALSE((read.template operator()<std::map<Key, int>>(
      write(std::map<std::string, int>{{"invalid", 1}}))));
  EXPECT_FALSE((read.template operator()<std::map<CustomColour, int>>(
      write(std::map<std::string, int>{{"invalid", 1}}))));
  roundtrip(write, read,
            std::map<Colour, int>{{Colour::Red, 1}, {Colour::Blue, 2}});
  roundtrip(write, read, std::map<Colour, int>{});
  roundtrip(
      write, read,
      std::unordered_map<Colour, int>{{Colour::Red, 1}, {Colour::Blue, 2}});
  roundtrip(write, read, std::map<CustomColour, int>{{CustomColour::Red, 3}});
  roundtrip(write, read, std::map<Key, int>{{Key{1}, 3}, {Key{2}, 4}});
  roundtrip(write, read,
            std::unordered_map<Key, int, Hash>{{Key{1}, 3}, {Key{2}, 4}});
  roundtrip(
      write, read,
      std::map<Colour, std::map<Key, int>>{{Colour::Green, {{Key{2}, 7}}}});
  // Numeric reflectors and multimaps retain the existing array representation.
  roundtrip(write, read, std::map<Numeric, int>{{Numeric::One, 1}});
  roundtrip(write, read,
            std::multimap<Colour, int>{{Colour::Red, 1}, {Colour::Red, 2}});
  const std::map<Key, std::unique_ptr<int>> move_only = [] {
    std::map<Key, std::unique_ptr<int>> map;
    map.emplace(Key{1}, std::make_unique<int>(42));
    return map;
  }();
  const auto result =
      read.template operator()<std::remove_cvref_t<decltype(move_only)>>(
          write(move_only));
  ASSERT_TRUE(result) << result.error().what();
  ASSERT_TRUE(result->at(Key{1}));
  EXPECT_EQ(*result->at(Key{1}), 42);
}
}  // namespace test_stringify_map_keys
#endif
