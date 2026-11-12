#include <rfl.hpp>
#include <rfl/json.hpp>
#include <string>

#include "write_and_read.hpp"

namespace test_code_points {

// UTF-8 escapes are used so the test does not depend on the source or
// execution character set.
// "héllo": 5 code points, 6 bytes.
constexpr const char* kHello = "h\xC3\xA9llo";
// "日本語": 3 code points, 9 bytes.
constexpr const char* kNihongo = "\xE6\x97\xA5\xE6\x9C\xAC\xE8\xAA\x9E";
// U+1F600 (grinning face): 1 code point, 4 bytes.
constexpr const char* kEmoji = "\xF0\x9F\x98\x80";

struct MaxFiveCodePoints {
  rfl::Validator<std::string, rfl::CodePoints<rfl::Maximum<5>>> name;
};

struct MaxFiveBytes {
  rfl::Validator<std::string, rfl::Size<rfl::Maximum<5>>> name;
};

struct ExactlyThreeCodePoints {
  rfl::Validator<std::string, rfl::CodePoints<rfl::EqualTo<3>>> name;
};

struct ExactlyThreeBytes {
  rfl::Validator<std::string, rfl::Size<rfl::EqualTo<3>>> name;
};

struct OneCodePoint {
  rfl::Validator<std::string, rfl::CodePoints<rfl::EqualTo<1>>> name;
};

std::string to_json(const std::string& _name) {
  return R"({"name":")" + _name + R"("})";
}

TEST(json, test_code_points_ascii_within_limit) {
  write_and_read(MaxFiveCodePoints{.name = "hello"}, R"({"name":"hello"})");
}

TEST(json, test_code_points_ascii_over_limit) {
  const auto res = rfl::json::read<MaxFiveCodePoints>(R"({"name":"hello!"})");
  ASSERT_FALSE(res);
  EXPECT_EQ(res.error().what(),
            "Failed to parse field 'name': Code point count validation "
            "failed: Value expected to be less than or equal to 5, but got "
            "6.");
}

TEST(json, test_code_points_multibyte_latin) {
  const auto json = to_json(kHello);

  const auto res = rfl::json::read<MaxFiveCodePoints>(json);
  ASSERT_TRUE(res) << res.error().what();
  EXPECT_EQ(res.value().name.value(), kHello);
  EXPECT_EQ(rfl::json::write(res.value()), json);

  // The same limit expressed in bytes rejects the string.
  const auto res_bytes = rfl::json::read<MaxFiveBytes>(json);
  ASSERT_FALSE(res_bytes);
  EXPECT_EQ(res_bytes.error().what(),
            "Failed to parse field 'name': Size validation failed: Value "
            "expected to be less than or equal to 5, but got 6.");
}

TEST(json, test_code_points_cjk) {
  const auto json = to_json(kNihongo);

  const auto res = rfl::json::read<ExactlyThreeCodePoints>(json);
  ASSERT_TRUE(res) << res.error().what();
  EXPECT_EQ(res.value().name.value(), kNihongo);

  const auto res_bytes = rfl::json::read<ExactlyThreeBytes>(json);
  ASSERT_FALSE(res_bytes);
  EXPECT_EQ(res_bytes.error().what(),
            "Failed to parse field 'name': Size validation failed: Value "
            "expected to be equal to 3, but got 9.");
}

TEST(json, test_code_points_emoji) {
  const auto res = rfl::json::read<OneCodePoint>(to_json(kEmoji));
  ASSERT_TRUE(res) << res.error().what();
  EXPECT_EQ(res.value().name.value(), kEmoji);

  const auto res_two =
      rfl::json::read<OneCodePoint>(to_json(std::string(kEmoji) + kEmoji));
  ASSERT_FALSE(res_two);
  EXPECT_EQ(res_two.error().what(),
            "Failed to parse field 'name': Code point count validation "
            "failed: Value expected to be equal to 1, but got 2.");
}

TEST(json, test_code_points_minimum) {
  using AtLeastThree =
      rfl::Validator<std::string, rfl::CodePoints<rfl::Minimum<3>>>;

  // "日本" is 6 bytes but only 2 code points.
  const auto res = AtLeastThree::from_value("\xE6\x97\xA5\xE6\x9C\xAC");
  ASSERT_FALSE(res);
  EXPECT_EQ(res.error().what(),
            "Code point count validation failed: Value expected to be greater "
            "than or equal to 3, but got 2.");

  EXPECT_TRUE(AtLeastThree::from_value(kNihongo));
}

TEST(json, test_code_points_schema) {
  constexpr auto expected =
      R"({"$schema":"https://json-schema.org/draft/2020-12/schema","allOf":[{"type":"string","minLength":4},{"type":"string","maxLength":6}],"$defs":{}})";

  using MinMax = rfl::Validator<std::string, rfl::CodePoints<rfl::Minimum<4>>,
                                rfl::CodePoints<rfl::Maximum<6>>>;
  EXPECT_EQ(rfl::json::to_schema<MinMax>(), expected);

  using AllOf = rfl::Validator<
      std::string,
      rfl::CodePoints<rfl::AllOf<rfl::Minimum<4>, rfl::Maximum<6>>>>;
  EXPECT_EQ(rfl::json::to_schema<AllOf>(), expected);

  // Schema output is identical to the byte-based rfl::Size.
  using SizeMinMax = rfl::Validator<std::string, rfl::Size<rfl::Minimum<4>>,
                                    rfl::Size<rfl::Maximum<6>>>;
  EXPECT_EQ(rfl::json::to_schema<MinMax>(),
            rfl::json::to_schema<SizeMinMax>());
}

TEST(json, test_code_points_schema_exact) {
  constexpr auto expected =
      R"({"$schema":"https://json-schema.org/draft/2020-12/schema","type":"string","minLength":3,"maxLength":3,"$defs":{}})";

  using Exact = rfl::Validator<std::string, rfl::CodePoints<rfl::EqualTo<3>>>;
  EXPECT_EQ(rfl::json::to_schema<Exact>(), expected);
}

}  // namespace test_code_points
