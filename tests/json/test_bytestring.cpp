#include <gtest/gtest.h>

#include <cstddef>
#include <map>
#include <optional>
#include <rfl.hpp>
#include <rfl/json.hpp>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace test_bytestring {

rfl::Bytestring bytes(const std::string_view _str) {
  rfl::Bytestring result;
  for (const unsigned char c : _str) {
    result.push_back(static_cast<std::byte>(c));
  }
  return result;
}

TEST(json, test_bytestring_rfc4648_vectors) {
  const std::vector<std::pair<std::string, std::string>> vectors = {
      {"", ""},
      {"f", "Zg=="},
      {"fo", "Zm8="},
      {"foo", "Zm9v"},
      {"foob", "Zm9vYg=="},
      {"fooba", "Zm9vYmE="},
      {"foobar", "Zm9vYmFy"}};
  for (const auto& [plain, encoded] : vectors) {
    SCOPED_TRACE(plain);
    EXPECT_EQ(rfl::json::write(bytes(plain)), '"' + encoded + '"');
    const auto result = rfl::json::read<rfl::Bytestring>('"' + encoded + '"');
    ASSERT_TRUE(result);
    EXPECT_EQ(*result, bytes(plain));
  }
}

TEST(json, test_bytestring_protojson_inputs) {
  const rfl::Bytestring expected{std::byte{0xfb}, std::byte{0xff}};
  for (const auto* encoded : {"+/8=", "+/8", "-_8=", "-_8"}) {
    SCOPED_TRACE(encoded);
    const auto result = rfl::json::read<rfl::Bytestring>(
        rfl::json::write(std::string(encoded)));
    ASSERT_TRUE(result);
    EXPECT_EQ(*result, expected);
    EXPECT_EQ(rfl::json::write(*result), R"("+/8=")");
  }
  for (const auto* encoded : {"Zg", "Zg=="}) {
    const auto result = rfl::json::read<rfl::Bytestring>(
        rfl::json::write(std::string(encoded)));
    ASSERT_TRUE(result);
    EXPECT_EQ(*result, bytes("f"));
  }
}

TEST(json, test_bytestring_binary_data) {
  rfl::Bytestring data;
  // Every byte value, across all three possible input-length remainders.
  for (unsigned int i = 0; i < 258; ++i) {
    data.push_back(static_cast<std::byte>(i & 0xff));
    const auto result =
        rfl::json::read<rfl::Bytestring>(rfl::json::write(data));
    ASSERT_TRUE(result);
    EXPECT_EQ(*result, data);
  }
  EXPECT_EQ(rfl::json::write(rfl::Bytestring{std::byte{0}, std::byte{0x80},
                                             std::byte{0xff}}),
            R"("AID/")");
}

struct Payload {
  rfl::Bytestring data;
  std::vector<rfl::Bytestring> chunks;
  std::optional<rfl::Bytestring> optional;
  std::map<std::string, rfl::Bytestring> named;
};

TEST(json, test_bytestring_nested) {
  const Payload payload{
      bytes("foo"), {bytes("f"), {}}, bytes(""), {{"a", bytes("fo")}}};
  const auto encoded = rfl::json::write(payload);
  EXPECT_EQ(
      encoded,
      R"({"data":"Zm9v","chunks":["Zg==",""],"optional":"","named":{"a":"Zm8="}})");
  const auto result = rfl::json::read<Payload>(encoded);
  ASSERT_TRUE(result);
  EXPECT_EQ(result->data, payload.data);
  EXPECT_EQ(result->chunks, payload.chunks);
  EXPECT_EQ(result->optional, payload.optional);
  EXPECT_EQ(result->named, payload.named);

  const auto absent = rfl::json::read<std::optional<rfl::Bytestring>>("null");
  ASSERT_TRUE(absent);
  EXPECT_FALSE(absent->has_value());
  EXPECT_EQ(rfl::json::write(std::optional<rfl::Bytestring>{}), "null");
}

TEST(json, test_bytestring_invalid_base64) {
  for (const auto* encoded :
       {"A", "AAAAA", "=", "==", "====", "Zg=", "Zg===", "Zm8==", "Zm9v=",
        "=Zg=", "Z=g=", "Zg==AA==", "Z g==", "Zg==\n", "Zg$=", "é"}) {
    SCOPED_TRACE(encoded);
    const auto result = rfl::json::read<rfl::Bytestring>(
        rfl::json::write(std::string(encoded)));
    ASSERT_FALSE(result);
    EXPECT_NE(result.error().what().find("Base64"), std::string::npos);
  }
  EXPECT_FALSE(rfl::json::read<rfl::Bytestring>(R"("Zg\u0000=")"));
  EXPECT_FALSE(rfl::json::read<rfl::Bytestring>(R"("Zg==\u0000")"));
  const auto nested = rfl::json::read<Payload>(
      R"({"data":"?","chunks":[],"optional":null,"named":{}})");
  ASSERT_FALSE(nested);
  EXPECT_NE(nested.error().what().find("data"), std::string::npos);
}

TEST(json, test_bytestring_rejects_non_strings) {
  for (const auto* json :
       {"null", "true", "42", "3.14", "[]", "[0,255]", "{}"}) {
    SCOPED_TRACE(json);
    EXPECT_FALSE(rfl::json::read<rfl::Bytestring>(json));
  }
}

TEST(json, test_bytestring_schema) {
  EXPECT_EQ(
      rfl::json::to_schema<rfl::Bytestring>(),
      R"({"$schema":"https://json-schema.org/draft/2020-12/schema","type":"string","contentEncoding":"base64","$defs":{}})");
  EXPECT_EQ(
      rfl::json::to_schema<std::string>(),
      R"({"$schema":"https://json-schema.org/draft/2020-12/schema","type":"string","$defs":{}})");
  using SizedBytes =
      rfl::Validator<rfl::Bytestring, rfl::Size<rfl::EqualTo<1>>>;
  EXPECT_EQ(rfl::json::to_schema<SizedBytes>(),
            rfl::json::to_schema<rfl::Bytestring>());
  EXPECT_TRUE(rfl::json::read<SizedBytes>(R"("Zg==")"));
  EXPECT_FALSE(rfl::json::read<SizedBytes>(R"("Zm8=")"));
}

TEST(json, test_bytestring_preserves_other_json_types) {
  EXPECT_EQ(rfl::json::write(std::string("Zg==")), R"("Zg==")");
  const auto result = rfl::json::read<std::string>(R"("Zg==")");
  ASSERT_TRUE(result);
  EXPECT_EQ(*result, "Zg==");
  EXPECT_EQ(rfl::json::write(std::vector<unsigned int>{0, 128, 255}),
            "[0,128,255]");
}

}  // namespace test_bytestring
