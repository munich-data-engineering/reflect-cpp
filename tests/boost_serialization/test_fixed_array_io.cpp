#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <rfl/NoFieldNames.hpp>
#include <rfl/boost_serialization.hpp>
#include <sstream>

#include "../check_fixed_array_io.hpp"

namespace test_fixed_array_io {

TEST(boost_serialization, fixed_array_size_and_roundtrip) {
  check_sizes(
      [](const auto& input) { return rfl::boost_serialization::write(input); },
      []<class T>(const auto& bytes) {
        return rfl::boost_serialization::read<T>(bytes);
      },
      sizeof(std::uint64_t));
}

TEST(boost_serialization, fixed_array_parent_metadata) {
  check_parents(
      [](const auto& input) { return rfl::boost_serialization::write(input); },
      []<class T>(const auto& bytes) {
        return rfl::boost_serialization::read<T>(bytes);
      });
  check_parents(
      [](const auto& input) {
        return rfl::boost_serialization::write<rfl::NoFieldNames>(input);
      },
      []<class T>(const auto& bytes) {
        return rfl::boost_serialization::read<T, rfl::NoFieldNames>(bytes);
      });
}

}  // namespace test_fixed_array_io

namespace test_fixed_array_io {

TEST(boost_serialization, fixed_array_text_archive) {
  using IArchive = boost::archive::text_iarchive;
  using OArchive = boost::archive::text_oarchive;
  const std::array<std::string, 2> input{"hello", "world"};
  std::ostringstream actual;
  {
    OArchive archive(actual);
    rfl::boost_serialization::write<IArchive>(archive, input);
  }
  std::ostringstream expected;
  {
    OArchive archive(expected);
    for (const auto& value : input) {
      archive << value;
    }
  }
  EXPECT_EQ(actual.str(), expected.str());
  std::istringstream stream(expected.str());
  IArchive archive(stream);
  const auto decoded =
      rfl::boost_serialization::read_from_archive<std::array<std::string, 2>,
                                                  IArchive, OArchive>(archive);
  ASSERT_TRUE(decoded) << decoded.error().what();
  EXPECT_EQ(*decoded, input);
}

}  // namespace test_fixed_array_io
