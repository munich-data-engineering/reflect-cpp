#include <gtest/gtest.h>

#include <array>
#include <bitset>
#include <cereal/types/map.hpp>
#include <cstdint>
#include <map>
#include <optional>
#include <rfl/NoFieldNames.hpp>
#include <rfl/Object.hpp>
#include <rfl/cereal.hpp>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace test_bitset {

using OutputArchive = ::cereal::PortableBinaryOutputArchive;
using InputArchive = ::cereal::PortableBinaryInputArchive;

template <class T>
class CerealBitset : public testing::Test {};

using Sizes = testing::Types<std::bitset<0>, std::bitset<1>, std::bitset<7>,
                             std::bitset<8>, std::bitset<9>, std::bitset<64>,
                             std::bitset<256>>;
TYPED_TEST_SUITE(CerealBitset, Sizes);

template <class Bits>
std::vector<Bits> patterns() {
  std::vector<Bits> values{Bits{}, Bits{}.set()};
  Bits alternating;
  for (std::size_t i = 0; i < alternating.size(); i += 2) {
    alternating.set(i);
  }
  values.push_back(alternating);
  values.push_back(~alternating);
  // Single bits exercise both ends and every byte/word boundary independently.
  for (std::size_t i = 0; i < alternating.size(); ++i) {
    values.push_back(Bits{}.set(i));
  }
  return values;
}

template <class T, class... Ps>
void roundtrip(const T& input) {
  const auto decoded =
      rfl::cereal::read<T, Ps...>(rfl::cereal::write<Ps...>(input));
  ASSERT_TRUE(decoded) << decoded.error().what();
  EXPECT_EQ(*decoded, input);
}

TYPED_TEST(CerealBitset, PackedNativeInterop) {
  for (const auto options : {OutputArchive::Options::LittleEndian(),
                             OutputArchive::Options::BigEndian()}) {
    for (const auto& bits : patterns<TypeParam>()) {
      SCOPED_TRACE(bits.to_string());
      std::ostringstream native;
      {
        OutputArchive archive(native, options);
        archive(bits);
      }
      std::ostringstream reflected;
      {
        OutputArchive archive(reflected, options);
        rfl::cereal::write(bits, archive);
      }
      const auto bytes = reflected.str();
      EXPECT_EQ(bytes, native.str());
      // Portable binary's endian header + representation tag + packed payload.
      ASSERT_EQ(bytes.size(), 2 + (bits.size() + 7) / 8);
      EXPECT_EQ(static_cast<unsigned char>(bytes[1]), 3);
      for (std::size_t i = 0; i < bits.size(); ++i) {
        const auto byte = static_cast<unsigned char>(bytes[2 + i / 8]);
        EXPECT_EQ((byte & (0x80 >> (i % 8))) != 0, bits[i]);
      }
      if (bits.size() % 8 != 0) {
        const auto padding_mask = (1u << (8 - bits.size() % 8)) - 1;
        EXPECT_EQ(static_cast<unsigned char>(bytes.back()) & padding_mask, 0u);
      }
      const auto decoded = rfl::cereal::read<TypeParam>(native.str());
      ASSERT_TRUE(decoded) << decoded.error().what();
      EXPECT_EQ(*decoded, bits);
      std::istringstream stream(bytes);
      InputArchive archive(stream);
      TypeParam native_decoded;
      archive(native_decoded);
      EXPECT_EQ(native_decoded, bits);
      roundtrip(bits);
    }
  }
}

template <class Bits>
struct Nested {
  int head;
  Bits bits;
  std::array<Bits, 2> fixed;
  std::vector<Bits> sequence;
  std::map<int, Bits> typed_map;
  std::map<std::string, Bits> string_map;
  std::optional<Bits> optional;
  std::variant<int, Bits> choice;
  int tail;

  bool operator==(const Nested&) const = default;
};

TYPED_TEST(CerealBitset, ParentMetadata) {
  using Bits = TypeParam;
  const auto bits = Bits{}.set();
  const Nested<Bits> input{17,
                           bits,
                           {Bits{}, bits},
                           {bits, Bits{}},
                           {{7, bits}, {9, Bits{}}},
                           {{"first", bits}, {"second", Bits{}}},
                           bits,
                           bits,
                           42};
  roundtrip(input);
  roundtrip<Nested<Bits>, rfl::NoFieldNames>(input);
  roundtrip(std::variant<int, Bits>{std::in_place_index<1>, bits});
  roundtrip(std::unordered_map<Bits, int>{{bits, 42}});

  // Check the library's map-like object container as well.
  rfl::Object<Bits> object;
  object.insert(std::make_pair(std::string("descriptor"), bits));
  const auto decoded_object =
      rfl::cereal::read<rfl::Object<Bits>>(rfl::cereal::write(object));
  ASSERT_TRUE(decoded_object) << decoded_object.error().what();
  ASSERT_TRUE(decoded_object->get("descriptor"));
  EXPECT_EQ(decoded_object->get("descriptor").value(), bits);

  // Verify the typed map's count and keys against native Cereal, not just a
  // roundtrip through matching reflect-cpp readers and writers.
  std::ostringstream native;
  {
    OutputArchive archive(native);
    archive(input.typed_map);
  }
  const auto bytes = rfl::cereal::write(input.typed_map);
  EXPECT_EQ(std::string(bytes.begin(), bytes.end()), native.str());
  std::istringstream stream(native.str());
  const auto decoded_map = rfl::cereal::read<std::map<int, Bits>>(stream);
  ASSERT_TRUE(decoded_map) << decoded_map.error().what();
  EXPECT_EQ(*decoded_map, input.typed_map);

  // The variant discriminant must precede the bitset representation tag.
  std::ostringstream variant_native;
  {
    OutputArchive archive(variant_native);
    archive(std::size_t{1}, bits, std::uint32_t{123});
  }
  std::ostringstream variant_reflected;
  {
    OutputArchive archive(variant_reflected);
    rfl::cereal::write(std::variant<int, Bits>{bits}, archive);
    archive(std::uint32_t{123});
  }
  EXPECT_EQ(variant_reflected.str(), variant_native.str());
  std::istringstream variant_stream(variant_native.str());
  InputArchive archive(variant_stream);
  const auto choice = rfl::cereal::read<std::variant<int, Bits>>(archive);
  ASSERT_TRUE(choice) << choice.error().what();
  ASSERT_EQ(choice->index(), 1u);
  EXPECT_EQ(std::get<1>(*choice), bits);
  std::uint32_t tail;
  archive(tail);
  EXPECT_EQ(tail, 123u);
}

TYPED_TEST(CerealBitset, MalformedInput) {
  auto bytes = rfl::cereal::write(TypeParam{}.set());
  for (std::size_t size = 0; size < bytes.size(); ++size) {
    SCOPED_TRACE(size);
    const auto decoded = rfl::cereal::read<TypeParam>(bytes.data(), size);
    ASSERT_FALSE(decoded);
    EXPECT_NE(decoded.error().what().find("Cereal read error:"),
              std::string::npos);
  }
  bytes[1] = static_cast<char>(0xff);
  const auto invalid = rfl::cereal::read<TypeParam>(bytes);
  ASSERT_FALSE(invalid);
  EXPECT_NE(invalid.error().what().find("Invalid bitset data representation"),
            std::string::npos);
}

TEST(cereal, bitset_nested_errors) {
  struct Record {
    std::bitset<9> descriptor;
  };
  auto record_bytes = rfl::cereal::write(Record{});
  record_bytes.pop_back();
  const auto record = rfl::cereal::read<Record>(record_bytes);
  ASSERT_FALSE(record);
  EXPECT_NE(record.error().what().find("descriptor"), std::string::npos);

  using Bits = std::bitset<9>;
  using Vector = std::vector<Bits>;
  auto vector_bytes = rfl::cereal::write(Vector{Bits{}, Bits{}});
  vector_bytes.pop_back();
  const auto vector = rfl::cereal::read<Vector>(vector_bytes);
  ASSERT_FALSE(vector);
  EXPECT_NE(vector.error().what().find("Cereal read error:"),
            std::string::npos);

  using Map = std::map<int, Bits>;
  auto map_bytes = rfl::cereal::write(Map{{7, Bits{}}});
  map_bytes.pop_back();
  EXPECT_FALSE(rfl::cereal::read<Map>(map_bytes));

  using Variant = std::variant<int, Bits>;
  auto variant_bytes = rfl::cereal::write(Variant{Bits{}});
  variant_bytes.pop_back();
  EXPECT_FALSE(rfl::cereal::read<Variant>(variant_bytes));

  // An invalid tag inside a container must propagate as a Result error too.
  auto invalid_tag = rfl::cereal::write(std::array<Bits, 1>{Bits{}});
  invalid_tag[1] = static_cast<char>(0xff);
  EXPECT_FALSE((rfl::cereal::read<std::array<Bits, 1>>(invalid_tag)));
}

}  // namespace test_bitset
