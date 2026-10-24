#include <rfl/NoFieldNames.hpp>
#include <rfl/cereal.hpp>
#include <sstream>

#include "../check_fixed_array_io.hpp"

namespace test_fixed_array_io {

static_assert(sizeof(rfl::cereal::Reader::InputArrayType) ==
              sizeof(rfl::cereal::Reader::CerealArchive*));

TEST(cereal, fixed_array_size_and_roundtrip) {
  check_sizes(
      [](const auto& input) { return rfl::cereal::write(input); },
      []<class T>(const auto& bytes) { return rfl::cereal::read<T>(bytes); },
      sizeof(::cereal::size_type));
}

TEST(cereal, fixed_array_parent_metadata) {
  check_parents(
      [](const auto& input) { return rfl::cereal::write(input); },
      []<class T>(const auto& bytes) { return rfl::cereal::read<T>(bytes); });
  check_parents(
      [](const auto& input) {
        return rfl::cereal::write<rfl::NoFieldNames>(input);
      },
      []<class T>(const auto& bytes) {
        return rfl::cereal::read<T, rfl::NoFieldNames>(bytes);
      });
}

}  // namespace test_fixed_array_io

namespace test_fixed_array_io {

TEST(cereal, fixed_array_endianness_and_payload) {
  using Archive = ::cereal::PortableBinaryOutputArchive;
  const std::array<std::int32_t, 3> input{12, -34, 56};
  for (const auto options :
       {Archive::Options::LittleEndian(), Archive::Options::BigEndian()}) {
    std::ostringstream actual;
    {
      Archive archive(actual, options);
      rfl::cereal::write(input, archive);
    }
    std::ostringstream expected;
    {
      Archive archive(expected, options);
      for (const auto value : input) {
        archive(value);
      }
    }
    EXPECT_EQ(actual.str(), expected.str());
    const auto decoded =
        rfl::cereal::read<std::array<std::int32_t, 3>>(expected.str());
    ASSERT_TRUE(decoded) << decoded.error().what();
    EXPECT_EQ(*decoded, input);
  }
}

}  // namespace test_fixed_array_io
